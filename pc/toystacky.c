/*
(C) Anirban Banerjee 2023
License: GNU GPL v3
*/
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <errno.h>
#include "linenoise.h"
#include "../arduino-pico/ToyStacky/bigint.h"

#define DESKTOP_PC
#define MAX_MATVECSTR_LEN 4900 //enough for one 12x12 matrix of long double complex
#define MAX_MATLEN 12
#define MAX_CMD_PAGES 4
#define MAX_TOKEN_LEN 129
#define MAX_VARNAME_LEN 33
#define MAX_VARIABLES 1000
#define MEMORY_SIZE 56000 //in bytes
#define CAPACITY MEMORY_SIZE
#define PROGMEM_SIZE 34000 //in bytes
#define STACK_SIZE 10000 //in bytes
#define EXEC_STACK_SIZE 200 //in Uint
#define STACK_NUM_ENTRIES 1000 //max stack entries
#define STRING_SIZE 129
#define BIGINT_SIZE 30 //holds a number as big as described in a decimal-coded 128 char long string
#define SHORT_STRING_SIZE 51 //%.15g gives 24 * 2 + 3
#define VSHORT_STRING_SIZE 25
//#define DOUBLE_EPS __LDBL_MIN__
//#define DOUBLEFN_EPS __LDBL_MIN__
//#define DOUBLE_EPS __LDBL_EPSILON__
//#define DOUBLEFN_EPS __LDBL_EPSILON__
#define DOUBLE_EPS 1e-15
#define DOUBLEFN_EPS 1e-15 //for return values of functions
#define NRPOLYSOLV_EPS 1e-6

#define COMSTARTTOKENC '('
#define VECSTARTTOKENC '['
#define VECLASTTOKENC ']'
#define MATSTARTTOKENC '{'
#define MATLASTTOKENC '}'
#define PRINTTOKENC '?'
#define POPTOKENC '@'

#define MATSTARTTOKEN "{"
#define MATENDTOKEN "}"
#define VECSTARTTOKEN "["
#define VECENDTOKEN "]"
#define IFTOKEN "if"
#define ELSETOKEN "el"
#define ENDIFTOKEN "fi"
#define JMPTOKEN "jmp"
#define JPZTOKEN "jpz"
#define JNZTOKEN "jnz"
#define ADDTOKEN "+"
#define SUBTOKEN "-"
#define MULTOKEN "*"
#define DIVTOKEN "/"
#define MODTOKEN "%"
#define GTTOKEN ">"
#define LTTOKEN "<"
#define GTETOKEN ">="
#define LTETOKEN "<="
#define EQTOKEN "="
#define NEQTOKEN "!="
#define PARTOKEN "//"
#define DPOPTOKEN "@@"

void (*NULLFN)(void) = NULL;
const long double __TS_PI__ = 3.14159265358979323846L;
#define MKCPLX(a,...) makeComplex(a, (0, ##__VA_ARGS__))

#define FAILANDRETURN(failcondition,dest,src,fnptr)	\
	if (failcondition) {							\
		sprintf(dest, src);							\
		if (fnptr != NULL) fnptr();					\
		return false;								\
	}

#define FAILANDRETURNVAR(failcondition,dest,src,var)	\
	if (failcondition) {								\
		sprintf(dest, src, var);						\
		return false;									\
	}

#define GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)	\
	conditional = ((execData >> 2) & 0x1);								\
	if (conditional) {													\
		ifCondition = (execData >> 1) & 0x1;							\
		doingIf = execData & 0x1;										\
	}

typedef enum {
	METASCALAR,
	METAVECTOR,
	METAMATRIX,
	METAVECTORPARTIAL,
	METAVECTORMATRIXPARTIAL,
	METAMATRIXPARTIAL
} StrackMeta;

const char* DEBUGMETA[6] = {
	"METASCALAR",
	"METAVECTOR",
	"METAMATRIX",
	"METAVECTORPARTIAL",
	"METAVECTORMATRIXPARTIAL",
	"METAMATRIXPARTIAL"};

#define METABARRIER 0x8

typedef struct {
	long double real;
	long double imag;
} ComplexDouble;

ComplexDouble makeComplex(double re, long double im) {
	ComplexDouble ret;
	ret.real = re;
	ret.imag = im;
	return ret;
}

typedef struct Matrix{
	int rows;
	int columns;
	ComplexDouble numbers[MAX_MATLEN][MAX_MATLEN];
} Matrix;

typedef struct Matrixd{
	int rows;
	int columns;
	long double numbers[MAX_MATLEN][MAX_MATLEN];
} Matrixd;

#define __DEBUG_TOYSTACKY__
#ifdef __DEBUG_TOYSTACKY__
void SerialPrint(const int count, ...) {
	char *str;
    va_list args;
	va_start(args, count);
	for (int i = 0; i < count; i++) {
		str = va_arg(args, char*);
		printf("%s\n", str);
	}
    va_end(args);
}
#else
void SerialPrint(const int count, ...) {}
#endif

//used for variables
typedef enum {
	VARIABLE_TYPE_COMPLEX,
	VARIABLE_TYPE_VECMAT,	
	VARIABLE_TYPE_STRING
} VariableType;

typedef struct {
	char name[MAX_VARNAME_LEN];
	VariableType type;
	union {
		ComplexDouble doubleValue;
		size_t stringValueIndex;
	} value;
} Variable;

//for garbage collector
typedef struct {
	Variable variables[MAX_VARIABLES];
	char memory[MEMORY_SIZE];
	size_t memoryOffset;
	size_t varCount;
} Ledger;

//used for main automatic stack
typedef struct {
	char stackStr[STACK_SIZE];
	size_t stackLen[STACK_NUM_ENTRIES]; //bits 31:24 are meta data - 
										//used to indicate start/end of vectors/matrices
	int topStr;
	int topLen;
	int itemCount;
} Strack;

//used for branch and condition stack
typedef struct {
	size_t stack[EXEC_STACK_SIZE];
	int top;
} UintStack;

typedef struct {
	char PROGMEM[PROGMEM_SIZE];
	Ledger ledger;
	Strack userStack; //the user stack
	UintStack execStack; //the execution stack to keep track of conditionals, loops and calls
	char bak[STRING_SIZE];//bakilliary register
	char acc[STRING_SIZE];//the accumulator
	char error[SHORT_STRING_SIZE];//error notification
	char coadiutor[STRING_SIZE]; //coadiutor = helper
	char dummy[STRING_SIZE];
	char matvecStrA[MAX_MATVECSTR_LEN]; 
	char matvecStrB[MAX_MATVECSTR_LEN];
	char matvecStrC[MAX_MATVECSTR_LEN];
	char lastY[MAX_MATVECSTR_LEN];
	char lastX[MAX_MATVECSTR_LEN];
	int8_t lastXMeta;
	int8_t lastYMeta;

	bigint_t bigA;
	bigint_t bigB;
	bigint_t bigC;
	Matrix matrixA;
	Matrix matrixB;
	Matrix matrixC;

	bigint_t bigMod;
	uint8_t width;

	int cmdPage;	
	int altState;	
	long double frequency;
	bool modeDegrees;
	bool modePolar;
	bool partialVector;
	bool partialMatrix;
	bool partialComplex;

	long double locationLat;
	long double locationLong;
	long double locationTimeZone;

	int month;
	int year;

	uint8_t precision;
	char notationStr[3];

} Machine;
static Machine vm;

#include "../arduino-pico/ToyStacky/TS-core-tokenize.h"
#include "../arduino-pico/ToyStacky/TS-core-stack.h"
#include "../arduino-pico/ToyStacky/TS-core-math.h"
#include "../arduino-pico/ToyStacky/TS-core-numString.h"
#include "../arduino-pico/ToyStacky/TS-core-llist.h"

typedef long double (*RealFunctionPtr)(ComplexDouble);
typedef void (*BigIntVoid1ParamFunctionPtr)(const bigint_t*, char*);

typedef void (*BigIntVoid2ParamFunctionPtr)(const bigint_t*, const bigint_t*, bigint_t*);

typedef int (*BigIntIntFunctionPtr)(const bigint_t*, const bigint_t*);
typedef ComplexDouble (*ComplexFunctionPtr1Param)(ComplexDouble);
typedef ComplexDouble (*ComplexFunctionPtr2Param)(ComplexDouble, ComplexDouble);
typedef ComplexDouble (*ComplexFunctionPtrVector)(ComplexDouble, ComplexDouble, ComplexDouble, int);

const char* matrixfnname[] = {
						"det", "inv",
						"idn", "trc",
						"eival", "eivec",
						"tpose"
						};
const int NUMMATRIXPARAMFN = 7;
const char* mathfn1paramname[] = {
						"sin", "cos", "tan", "cot", 
						"rect",
						"asin", "acos", "atan", "acot", 
						"polar", 
						"sinh", "cosh", "tanh", "coth", 
						"asinh", "acosh", "atanh", "acoth", 
						"exp", "log10", "log", "log2", "sqrt", "cbrt", "conj",
						"torad", "todeg", "recip", "neg",						
						"abs", "arg", "re", "im"}; //the 1 param functions
const int EXPFNINDEX = 18;
const int NUMMATH1PARAMFN = 29;
const int NUMREAL1PARAMFN = 4;
const int NUMMATH1PARAMTRIGFN = 5;
const int NUMMATH1PARAMTRIGANTIFN = 10;

//these 23 return ComplexDouble
ComplexFunctionPtr1Param mathfn1param[] = {	csine,
											ccosine,
											ctangent,
											ccotangent,
											rect,

											carcsine,
											carccosine,
											carctangent,
											carccotangent,
											polar,

											csinehyp,
											ccosinehyp,
											ctangenthyp,
											ccotangenthyp,

											carcsinehyp, 
											carccosinehyp, 
											carctangenthyp,
											carccotangenthyp,

											cexpo, clogarithm10, 
											cln, clog2, 
											csqroot, cbroot, 
											conjugate,

											crad,
											cdeg,
											crecip,
											cneg};

//the 1 param functions that have real result
RealFunctionPtr realfn1param[] = {abso, cargu, crealpart, cimagpart};

BigIntVoid1ParamFunctionPtr bigfnvoid1param[] = {bigint_hex, bigint_dec, bigint_bin, bigint_oct, bigint_neg};
BigIntVoid2ParamFunctionPtr bigfnvoid2param[] = {bigint_pow, bigint_max, bigint_min, bigint_add, bigint_sub, bigint_mul, bigint_div, bigint_rem};

BigIntIntFunctionPtr bigfnint2param[] = {bigint_gt, bigint_lt, bigint_gte, bigint_lte, bigint_eq, bigint_neq};

const char* bigfnvoid1paramname[] = {"hex", "dec", "bin", "oct", "neg"};
const int NUMBIGINT1FNS = 5;
const char* vecfn1paramname[] = {"sum", "sqsum", "var", "sd", "mean", "rsum"};
const char* vecfn2paramname[] = {"dot"};
const int NUMVECFNS = 6;
const int NUMVEC2FNS = 1;

const char* mathfnop2paramname[] = {
						"logxy",
						"atan2", "pow",
						"max", "min",
						ADDTOKEN, SUBTOKEN,
						MULTOKEN, DIVTOKEN,
						MODTOKEN, GTTOKEN,
						LTTOKEN, GTETOKEN,
						LTETOKEN, EQTOKEN,
						NEQTOKEN, PARTOKEN,
						"rem"
						}; //the 2 params functions
const int POWFNINDEX = 2; 
const int  NUMMATH2PARAMFNOP = 18; 
const int NUMVOID2PARAMBIGINTFNMIN = 2; //bigint fns start from pow
const int NUMVOID2PARAMBIGINTFNMAX = 9;

const int ATAN2FNINDEX = 1;

//ADDTOKEN, SUBTOKEN,
//MULTOKEN, DIVTOKEN,
//MODTOKEN, GTTOKEN,
//LTTOKEN, GTETOKEN,
//LTETOKEN, EQTOKEN,
//NEQTOKEN

ComplexFunctionPtr2Param mathfn2param[] = {clogx, carctangent2, cpower, cmax, cmin, cadd, csub,
									cmul, cdiv, cmod, cgt, clt, cgte, clte, ceq, cneq, cpar, crem};

ComplexDouble suminternal(ComplexDouble running, ComplexDouble next) {
	return cadd(running, next);
}
ComplexDouble sum(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n) { 
	return summed;
}
ComplexDouble sqsum(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n) { 
	return sqsummed;
}
ComplexDouble var(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n) {
	if (n <= 1) return makeComplex(0.0, 0.0);
	ComplexDouble u = cdivd(summed, (double)(n)); //mean
	return csub(cdivd(sqsummed, (double)(n)), cmul(u, u)); //expansion of sum((x - u)**2)
}
ComplexDouble sdv(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n) {
	if (n <= 1) return makeComplex(0.0, 0.0);
	ComplexDouble u = cdivd(summed, (double)(n)); //mean
	ComplexDouble v = csub(cdivd(sqsummed, (double)(n - 1)), cdivd(cmul(cpowerd(u, 2), u), (double)(n - 1))); //use 1/(n - 1)
	return csqroot(v);
}
ComplexDouble mean(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n) {
	return cdivd(summed, (double)(n));
}
ComplexDouble rsum(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n) {
	return rsummed;
}

ComplexFunctionPtrVector mathfnvec1param[] = {sum, sqsum, var, sdv, mean, rsum};

ComplexDouble callVectorMath1ParamFunction(int fnindex, ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n) {
	if (fnindex < 0) return makeComplex(0.0, 0.0);
	return mathfnvec1param[fnindex](summed, sqsummed, rsummed, n);
}

ComplexDouble call1ParamMathFunction(int fnindex, ComplexDouble input) {
	return mathfn1param[fnindex](input);
}

double call1ParamRealFunction(int fnindex, ComplexDouble input) {
	return realfn1param[fnindex](input);
}

ComplexDouble call2ParamMathFunction(int fnindex, ComplexDouble input, ComplexDouble second) {
	return mathfn2param[fnindex](input, second);
}

void call1ParamBigIntVoidFunction(int fnindex, bigint_t *x, char* res) {
	bigfnvoid1param[fnindex](x, res);
}

void call2ParamBigIntVoidFunction(int fnindex, bigint_t *x, bigint_t *y, bigint_t *res) {
	bigfnvoid2param[fnindex](x, y, res);
}

int call2ParamBigIntIntFunction(int fnindex, bigint_t *x, bigint_t *y) {
	return bigfnint2param[fnindex](x, y);
}

void zstrncpy (char*dest, const char* src, int len) {
	//copy n chars -- don't use strncpy
	if (len) {
		*dest = '\0'; 
		strncat(dest, src, len);
	}
}

//dest must be previously sized to len + 1
char* fitstr(char*dest, const char* src, size_t len) {
	if (strlen(src) > len) {
		dest[len] = '\0';
		dest[len - 1] = dest[len - 2] = dest[len - 3] = '.';
		zstrncpy(dest, src, len - 3);
	} else zstrncpy(dest, src, len);
	return dest;
}

bool fn1ParamScalar(Machine* vm, const char* fnname, int fnindex, int isTrig, int bigInt1Param);

#include "../arduino-pico/ToyStacky/TS-core-ledger.h"

void printLedger(Ledger* ledger) {
	if (ledger->varCount > 0) printf("printLedger:\n");
	else return;
	uint32_t varCount = 0;
	while (varCount < MAX_VARIABLES) {
		Variable* variable = &ledger->variables[varCount];
		//printf("printLedger ===================== varCount = %d\n", varCount);
		if (varCount >= ledger->varCount) break;
		if (variable->name[0] == '\0') {
			varCount++;
			continue; //skip the empty variable pods
		}
		if (variable->type == VARIABLE_TYPE_STRING) {
			printf("Variable %s at vartable index %d: %s\n", variable->name, varCount, ledger->memory + variable->value.stringValueIndex);
		} else {
			printf("Variable %s at vartable index %d: %.15Lf\n", variable->name, varCount, variable->value.doubleValue.real);
		}
		varCount++;
	}
}

void printMemory(Ledger* ledger) {
	for (int i = 0; i < ledger->memoryOffset; i += 16) {
		printf("%04X: ", i);

		for (int j = 0; j < 16; j++) {
			if (i + j < ledger->memoryOffset) {
				printf("%02X ", (unsigned char)ledger->memory[i + j]);
			} else {
				printf("   ");
			}
		}

		printf(": ");
		for (int j = 0; j < 16; j++) {
			if (i + j < ledger->memoryOffset) {
				char ch = ledger->memory[i + j];
				if (ch >= 32 && ch <= 126) {
					putchar(ch);
				} else {
					putchar('.');
				}
			} else {
				putchar(' ');
			}
		}
		putchar('\n');
	}
}

void printStack(Strack* s, int count, bool firstLast) {
	if (stackIsEmpty(s)) return;
	//ensure the count is within the range of the stack elements
	if (count == 0 || (count > s->topLen + 1)) 
		count = s->topLen + 1;

	//printf("printStack: s->topStr = %d\n", s->topStr);
	//printf("printStack: s->topLen = %d\n", s->topLen);
	//printf("Printing %d entries from the top of the stack:\n", count);
	char* stringPtr;
	size_t stringLen = -1;
	int8_t meta;
	int8_t barrier;
	//int rightOflowIndicatorPos = -1;
	//char display[STRING_SIZE]; //coadiutor = helper
	if (firstLast) {
		for (int i = 0; i < count; i++) {
			//printf("printStack: s->stackLen[%d] = %lu\n", i, s->stackLen[s->topLen - i]);
			stringLen += s->stackLen[s->topLen - i] & 0x0fffffff;
		}
		//now s->topStr - stringLen will point to the first string
		//printf("printStack - doing firstLast -- before FOR loop -- topLen = %d, stringLen = %lu\n", s->topLen, stringLen);
		for (int i = 0; i < count; i++) {
			//printf("printStack - doing firstLast -- s->topLen = %d and stringLen = %lu\n", s->topLen, stringLen);
			stringPtr = &s->stackStr[s->topStr - stringLen];
			meta = (s->stackLen[s->topLen - count + i + 1] >> 28) & 0xf;
			//printf("printStack: at first barrier = %d\n", barrier);
			barrier = meta & 0x8;
			meta = meta & 0x7;
			//printf("printStack: barrier = %d meta = %s\n", barrier, DEBUGMETA[meta]);
			//printf("printStack - doing firstLast -- s->topLen = %d and stringLen = %lu and meta = %d\n", s->topLen, stringLen, meta);
			printf("\t%s meta = %s %s\n", 
					stringPtr, 
					DEBUGMETA[meta], (barrier)? "BARRIER":"");

			stringLen -= strlen(stringPtr) + 1;
		}
	} else {
		for (int i = s->topLen; i > s->topLen - count; i--) {		
			stringLen += s->stackLen[i] & 0xfffffff;
			stringPtr = &s->stackStr[s->topStr - stringLen];
			printf("\t%s\n", stringPtr);
		}
	}
}

#define UTF8
#ifdef UTF8
#include "utf8.h"
#endif
void completion(const char *buf, linenoiseCompletions *lc) {
	if (buf[0] == 'h') {
#ifdef UTF8
		linenoiseAddCompletion(lc,"hello こんにちは");
		linenoiseAddCompletion(lc,"hello こんにちは there");
#else
		linenoiseAddCompletion(lc,"hello");
		linenoiseAddCompletion(lc,"hello there");
#endif
	}
}

char *hints(const char *buf, int *color, int *bold) {
	if (strcmp(buf,"No error.") != 0) {
		*color = 35;
		*bold = 0;
	/* The hints callback returns non-const, because it is possible to
	 * dynamically allocate the hints we return, so long as we provide a
	 * cleanup callback to linenoise that it can call later to deallocate
	 * them. Here, we do not provide such a cleanup callback and we return a
	 * static const - that's why we can cast this const away. */
		//return (char *)" World";
		return (char *)"";
		//return vm.error;
	}
	return NULL;
}

#include "../arduino-pico/ToyStacky/yasML.h"
#include "../arduino-pico/ToyStacky/day.h"

uint8_t conditionalData(size_t execData) {
	//data being currently entered is a conditional if/else
	return (execData & 0x7);
}

int is1ParamBigIntFunction(const char* token) {
	for (int i = 0; i < NUMBIGINT1FNS; i++) {
		if (strcmp(bigfnvoid1paramname[i], token) == 0) return i;
	}
	return -1;
}

bool fnOrOpVec2Param(Machine* vm, const char* token, int fnindex, int8_t cmeta, int8_t meta, bool returnsVector);
#include "../arduino-pico/ToyStacky/TS-core-fnOrOp2Param.h"
#include "../arduino-pico/ToyStacky/TS-core-fnOrOpMat2Param.h"
#include "../arduino-pico/ToyStacky/TS-core-fnOrOpVec2Param.h"
#include "../arduino-pico/ToyStacky/TS-core-fnOrOpVec1Param.h"
#include "../arduino-pico/ToyStacky/TS-core-fnOrOpMat1Param.h"
#include "../arduino-pico/ToyStacky/TS-core-fn1Param.h"

int isMatFunction(const char* token) {
	for (int i = 0; i < NUMMATRIXPARAMFN; i++) {
		if (strcmp(matrixfnname[i], token) == 0) return i;
	}
	return -1;
}

int is2ParamFunction(char* token) {
	//lcase(token);
	for (int i = 0; i < NUMMATH2PARAMFNOP; i++) {
		if (strcmp(mathfnop2paramname[i], token) == 0) return i;
	}
	return -1;
}

int is1ParamFunction(const char* token) {
	//lcase(token);
	for (int i = 0; i < NUMMATH1PARAMFN + NUMREAL1PARAMFN; i++) {
		if (strcmp(mathfn1paramname[i], token) == 0) return i;
	}
	return -1;
}

int is1ParamTrigFunction(const char* token) {
	for (int i = 0; i < NUMMATH1PARAMTRIGFN; i++) {
		if (strcmp(mathfn1paramname[i], token) == 0) return 1;
	}
	for (int i = NUMMATH1PARAMTRIGFN; i < NUMMATH1PARAMTRIGANTIFN; i++) {
		if (strcmp(mathfn1paramname[i], token) == 0) return 2;
	}
	return 0;
}

int isVec1ParamFunction(const char* token) {
	for (int i = 0; i < NUMVECFNS; i++) {
		if (strcmp(vecfn1paramname[i], token) == 0) return i;
	}
	return -1;
}

bool processPrint(Machine* vm, char* token) {
	//printf("------------------- PRINT - called with token = %s\n", token);
	
	//FAILANDRETURN((token[0] == '\0'), vm->error, "Error: illegal PRINT", NULLFN)
	if (varNameIsLegal(token)) {
		strcpy(vm->bak, token);
		int vt = findVariable(&vm->ledger, vm->bak);
		if (vt == VARIABLE_TYPE_COMPLEX) {
			ComplexDouble c = fetchVariableComplexValue(&vm->ledger, vm->bak);
			if (complexToString(c, vm->coadiutor, vm->precision, vm->notationStr)) { 
				printf("\t%s = %s\n", vm->bak, vm->coadiutor);
			}
		} else if (vt == VARIABLE_TYPE_STRING) {
			if (getVariableStringVecMatValue(&vm->ledger, token, vm->coadiutor)) { //returns true
				printf("\t%s = %s\n", vm->bak, vm->coadiutor);
			}
		} else {
			FAILANDRETURNVAR(true, vm->error, "Error: no variable '%s'.", fitstr(vm->coadiutor, vm->bak, 10))
		}
	} else {
		FAILANDRETURNVAR(true, vm->error, "Error: no variable '%s'.", fitstr(vm->coadiutor, token, 10))
	}
	return true;
}

void initStacks(Machine* vm) {
	stackInit(&vm->userStack);
	UintStackInit(&vm->execStack);
}

void initMachine(Machine* vm) {
	initializeLedger(&vm->ledger);
	initStacks(vm);
	//after a push, last will have ToS-1, 
	//after a pop, last will have the previous ToS
	strcpy(vm->acc, "0");
	strcpy(vm->bak, "0");
	strcpy(vm->error, "No error.");
	strcpy(vm->lastX, "0");
	strcpy(vm->lastY, "0");

	vm->frequency = 1;
	vm->modeDegrees = false;
	vm->partialVector = false;
	vm->partialMatrix = false;
	vm->partialComplex = false;
	vm->cmdPage = 0;
	vm->altState = 0;
	vm->precision = 15;
	strcpy(vm->notationStr, "Lg");
	vm->bigMod.length = -1;
	vm->width = 64; //max = 18446744073709551616	

	vm->locationLat = 12.9716;
	vm->locationLong = 77.5946;
	//vm->locationLat = 22.5726;
	//vm->locationLong = 88.3639;
	vm->locationTimeZone = 5.5;
	//zhr=5.5, latt=22.5726, longt=88.3639): Kolkata
	//zhr=5.5, latt=12.9716, longt=77.5946): Bangalore


}

#include "../arduino-pico/ToyStacky/TS-core-processPop.h"
#include "../arduino-pico/ToyStacky/TS-core-processDefaultPush.h"
#include "../arduino-pico/ToyStacky/TS-core-process.h"

void printRegisters(Machine* vm) {
	printf("\tError: %s\n", (vm->error[0] != '\0')? vm->error: "\"\"");
}

void interpret(Machine* vm, char* sourceCode) {
	char* input;
	char output[STRING_SIZE];
	bool success;
	//printf("-------------------START: Source Code = %s\n", sourceCode);
	input = sourceCode;
	do {
		strcpy(vm->error, "No error.");
		//printf ("main: do-while before tokenize input = %p\n", input);
		//printf ("main: do-while before tokenize input = %s\n", input);
		input = tokenize(input, output);
		//printf ("main: do-while after tokenize input = %p\n", input);
		//printf ("main: do-while after tokenize output = %s\n", output);
		if (output[0] == '\0') break;
		success = process(vm, output);
		//printf("-------------------while: output = %s next input = %s input[0] = %d\n", output, input, input[0]);
		if ((strcmp(vm->error,"No error.") != 0) || !success) break;
	} while (input[0] != '\0');
}

void showScreen(Machine* vm, char* line) {
	printf("===================================\n");
	printf("\tMode: %s %s\n", (vm->modeDegrees)? "Degrees": "Radian", (vm->modePolar)? "Polar": "Cartesian");
	printStack(&vm->userStack, 0, true);
	//printf("-----------------------------------\n");
	printRegisters(vm);
	//printf("\texecStack: matrix = %d, vector = %d IFstate = %x\n", (int) (execData >> 4) & 0x1, (int) (execData >> 3) & 0x1, (int) execData & 0x7);
	printf("===================================\n");
	linenoiseHistoryAdd(line); /* Add to the history. */
	linenoiseHistorySave(".ts.history"); /* Save the history on disk. */
}

int main(int argc, char **argv) {
	initMachine(&vm);
	//char sourceCode[] = "\"Hello, World!\" DUP ? -42.5 3.14 IF 1.0 ? fi";
	//char sourceCode[] = "\"Hello, World!\" ? -42.5 IF 3.14 ? FI";
	//char sourceCode[] = "\"Hello, World!\" if 3.14 ? fi";
	//char sourceCode[] = "\"Hello, World!\" 1 if 10 ? fi ?";
	//char sourceCode[] = "3.14 if \"Hello, World!\" ? fi";
	//char sourceCode[] = "1 2 + 4 / 1 if \"Hello, World!\" ? fi ? do 10 20";
	//char sourceCode[] = "0 if 3 ? else 0 if 999 ? else 0 ? fi fi last";
	//char sourceCode[] = "999 123 100.0 555";
	//char sourceCode[] = "3.14159265358979323846 tan 2.78 + tan \"Hello, World!\" >acc 999 111 last > 123";
	//strcpy(&vm.PROGMEM[0], sourceCode);
	//interpret(&vm, &vm.PROGMEM[0]);

	char command[STRING_SIZE];
	char* line;

	printf("===================================\n");
	printf("\tMode: %s %s\n", (vm.modeDegrees)? "Degrees": "Radian", (vm.modePolar)? "Polar": "Cartesian");
	printStack(&vm.userStack, 0, true);
	printRegisters(&vm);
	printf("===================================\n");
	/* Parse options, with --multiline we enable multi line editing. */
	while(argc > 1) {
		argc--;
		argv++;
		if (strcmp(*argv,"-m") == 0) {
			linenoiseSetMultiLine(1);
			printf("Multi-line mode enabled.\n");
		} else if (strcmp(*argv,"-k") == 0) {
			linenoisePrintKeyCodes();
			exit(0);
		} else {
			fprintf(stderr, "Usage: %s [-m] [-k]\n", argv[0]);
			exit(1);
		}
	}

#ifdef UTF8
	linenoiseSetEncodingFunctions(
		linenoiseUtf8PrevCharLen,
		linenoiseUtf8NextCharLen,
		linenoiseUtf8ReadCode);
#endif

	/* Set the completion callback. This will be called every time the
	 * user uses the <tab> key. */
	linenoiseSetCompletionCallback(completion);
	linenoiseSetHintsCallback(hints);

	/* Load history from file. The history file is just a plain text file
	 * where entries are separated by newlines. */
	linenoiseHistoryLoad(".ts.history"); /* Load the history at startup */

	/* Now this is the main loop of the typical linenoise-based application.
	 * The call to linenoise() will block as long as the user types something
	 * and presses enter.
	 *
	 * The typed string is returned as a malloc() allocated string by
	 * linenoise, so the user needs to free() it. */
#ifdef UTF8
	while((line = linenoise("\033[32mts\x1b[0m> ")) != NULL) {
#else
	while((line = linenoise("\033[32mts\x1b[0m> ")) != NULL) {
#endif
		/* Do something with the string. */
		if (!strncmp(line,":hl", 3)) {
			/* The ":hl" command will change the history len. */
			int len = atoi(line + 3);
			linenoiseHistorySetMaxLen(len);
		} else if (!strncmp(ltrim(line),":q", 2)) {
			//quit
			free(line); break;
		} else if (line[0] == ':') {
			printf("Unrecognized command: %s\n", line);
		} else if (line[0] != '\0') {
			//printf("echo: '%s'\n", line);
			int l = strlen(line);
			if (l > 128) l = 128;
			zstrncpy(&command[0], line, l);
			command[l] = '\0';
			interpret(&vm, command);
			showScreen(&vm, line);
		}
		free(line);
	}
	printf("Goodbye!\n");
	return 0;
}

