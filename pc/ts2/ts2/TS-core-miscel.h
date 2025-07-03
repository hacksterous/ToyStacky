#ifndef __TS_CORE_MISCEL__
#define __TS_CORE_MISCEL__
#include <stdint.h>
#include <stdio.h>
#include "bigint.h"
#include "TS-core-tokenize.h"

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

static void (*NULLFN)(void) = NULL;

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

#ifndef __COMPLEX_DOBULE_TYPE__
#define __COMPLEX_DOBULE_TYPE__
typedef struct {
	long double real;
	long double imag;
} ComplexDouble;
#endif

//used for variables
typedef enum {
	VARIABLE_TYPE_COMPLEX,
	VARIABLE_TYPE_VECMAT,	
	VARIABLE_TYPE_STRING,
	VARIABLE_TYPE_CODEBLOCK	
} VariableType;

typedef struct {
	char name[MAX_VARNAME_LEN];
	VariableType type;
	union {
		ComplexDouble doubleValue;
		size_t stringValueIndex;
	} value;
} Variable;

typedef enum {
	METASCALAR,
	METAVECTOR,
	METAMATRIX,
	METAVECTORPARTIAL,
	METAVECTORMATRIXPARTIAL,
	METAMATRIXPARTIAL
} StrackMeta;

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

#ifndef __MATRIX_TYPEDEF__
#define __MATRIX_TYPEDEF__
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
#endif

//for garbage collector
typedef struct {
	Variable variables[MAX_VARIABLES];
	char memory[MEMORY_SIZE];
	size_t memoryOffset;
	size_t varCount;
} Ledger;

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

ComplexDouble call2ParamMathFunction(int fnindex, ComplexDouble input, ComplexDouble second);
void call2ParamBigIntVoidFunction(int fnindex, bigint_t *x, bigint_t *y, bigint_t *res);
int call2ParamBigIntIntFunction(int fnindex, bigint_t *x, bigint_t *y);
void call1ParamBigIntVoidFunction(int fnindex, bigint_t *x, char* res);
ComplexDouble call1ParamMathFunction(int fnindex, ComplexDouble input);
double call1ParamRealFunction(int fnindex, ComplexDouble input);
ComplexDouble callVectorMath1ParamFunction(int fnindex, ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n);

uint8_t conditionalData(size_t execData);
int is1ParamBigIntFunction(const char* token);
int isMatFunction(const char* token);
int is2ParamFunction(char* token);
int is1ParamFunction(const char* token);
int is1ParamTrigFunction(const char* token);
int isVec1ParamFunction(const char* token);

bool processPop(Machine* vm, char* token);
bool popIntoVariable (Machine* vm, char* var);
bool processDefaultPush(Machine* vm, char* token, char* retstr, StrackMeta* metap);
bool process(Machine* vm, char* token);
bool fn1ParamScalar(Machine* vm, const char* fnname, int fnindex, int isTrig, int bigInt1Param);
bool fnOrOpVec2Param(Machine* vm, const char* token, int fnindex, int8_t cmeta, int8_t meta, bool returnsVector);
bool fn1Param(Machine* vm, const char* token, int fnindex, int isTrig, int bigInt1Param);
bool fnOrOp2Param(Machine* vm, const char* token, int fnindex);
bool fnOrOpVec1Param(Machine* vm, const char* token, int fnindex, bool isTrig, bool returnsVector);
bool fnOrOpMat1Param(Machine* vm, const char* token, int fnindex, bool isTrig);

bool processPrint(Machine* vm, char* token);
void initStacks(Machine* vm);
#endif
