/*
(C) Anirban Banerjee 2023
License: GNU GPL v3
*/
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#pragma GCC diagnostic ignored "-Wformat-overflow"
#pragma GCC diagnostic ignored "-Wformat-truncation"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wrestrict"
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
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
#include <hardware/rtc.h>
#include <LiquidCrystal.h>
#include <hardware/resets.h>
#include <hardware/regs/rosc.h>
#include <Arduino.h>
#include "bigint.h"
#include "TS-core.h"

const char* __TS_GLOBAL_ERRORCODE[] = { "undef arg",
								"log undef",
								"bad fn tanh",
								"bad fn coth",

								"tan undef",
								"cot undef",
								"bad fn domain",
								"+/-inf"
								};

#define FAILANDRETURN(failcondition,dest,src,fnptr)	\
	if (failcondition) {							\
		sprintf(dest, src);							\
		if (fnptr != NULL) fnptr();					\
		SerialPrint(3, "E:", dest, "\r\n");			\
		return false;								\
	}

#define FAILANDRETURNVAR(failcondition,dest,src,var)\
	if (failcondition) {							\
		sprintf(dest, src, var);					\
		SerialPrint(3, "E:", dest, "\r\n");			\
		return false;								\
	}

#define GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)	\
	conditional = ((execData >> 2) & 0x1);								\
	if (conditional) {													\
		ifCondition = (execData >> 1) & 0x1;							\
		doingIf = execData & 0x1;										\
	}

const char* DEBUGMETA[5] = {
	"METASCALAR"
	"METAVECTOR",
	"METAVECTOR",
	"METAVECTORPARTIAL",
	"METAMATRIXPARTIAL"};

const long double __TS_PI__ = 3.14159265358979323846L;
void (*NULLFN)(void) = NULL;

ComplexDouble makeComplex(long double re, long double im) {
	ComplexDouble ret;
	ret.real = re;
	ret.imag = im;
	return ret;
}

Machine vm;

#undef __DEBUG_TOYSTACKY__
#ifdef __DEBUG_TOYSTACKY__
void SerialPrint(const int count, ...) {
	char *str;
    va_list args;
	va_start(args, count);
	for (int i = 0; i < count; i++) {
		str = va_arg(args, char*);
		Serial.print(str);
	}
    va_end(args);
}
#else
void SerialPrint(const int count, ...) {}
#endif

//Von Neumann extractor: From the input stream, his extractor took bits, 
//two at a time (first and second, then third and fourth, and so on). 
//If the two bits matched, no output was generated. 
//If the bits differed, the value of the first bit was output. 
uint32_t truerandom(void){
    int k, random=0;
    int random_bit1, random_bit2;
    volatile uint32_t *rnd_reg=(uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);
    
    for(k = 0; k < 32; k++){
        while(1) {
			sleep_us(10);
            random_bit1 = 0x00000001 & (*rnd_reg);
			sleep_us(10);
			sleep_us(10 + (0x00000007 & (*rnd_reg)));
			sleep_us(10);
            random_bit2 = 0x00000001 & (*rnd_reg);
			sleep_us(10);
			sleep_us(10 + (0x00000007 & (*rnd_reg)));
            if(random_bit1 != random_bit2) break;
        }
		random = random << 1;        
		random = random + random_bit1;
    }
    return random;
}

void clearLED() {
	rtc_disable_alarm();
	vm.LEDState = false;
	digitalWrite(LED_BUILTIN, LOW);
}

void toggleLED() {
	vm.LEDState = !vm.LEDState;
	if (vm.LEDState) 
		digitalWrite(LED_BUILTIN, HIGH);	
	else
		digitalWrite(LED_BUILTIN, LOW);

	rtc_disable_alarm();
	if (vm.repeatingAlarm) {
		datetime_t alarm;
		alarm.year  = 2023;
		alarm.month = 9;
		alarm.day   = 15;
		alarm.dotw  = 5; // Friday; 0 is Sunday
		alarm.sec = 0;
		alarm.min = 0;
		alarm.hour = 0;
		rtc_set_datetime(&alarm);
		alarm.sec = vm.alarmSec;
		alarm.min = vm.alarmMin;
		alarm.hour = vm.alarmHour;
		rtc_set_alarm(&alarm, &toggleLED);
	}
}

void zstrncpy (char*dest, const char* src, int len) {
	//copy n chars -- don't use strncpy
	if (len) {
		*dest = '\0'; 
		strncat(dest, src, len);
	}
}

//dest must be previously sized to len + 1
char* fitstr(char*dest, const char* src, int32_t len) {
	if ((int32_t)strlen(src) > len) {
		dest[len] = '\0';
		dest[len - 1] = dest[len - 2] = dest[len - 3] = '.';
		zstrncpy(dest, src, len - 3);
	} else zstrncpy(dest, src, len);
	return dest;
}

bool fn1ParamScalar(Machine* vm, const char* fnname, int fnindex, int isTrig, int bigInt1Param);
#include "TS-core-tokenize.h"
#include "TS-core-ledger.h"
#include "TS-core-stack.h"
#include "TS-core-math.h"
#include "TS-core-numString.h"
#include "TS-core-llist.h"
#include "yasML.h"
#include "day.h"

void makeLDoubleStringFit(char*dest, char* src, int len) {
	char expstr[10];
	char fmt[10];
	char* cPtr;
	long double dbl;
	long int exponent;
	int adjlen;
	int pos = 0;
	dbl = strtold(src, &cPtr);
	sprintf(dest, "%.15Le", dbl);//force into exp notation
	//printf("A: dest = %s\n", dest);
	cPtr = strchr(dest, 'e');
	if (cPtr) {
		pos = (int) (cPtr - dest);//position of 'e'
		strcpy(expstr, &dest[pos + 1]);
	} 
	else strcpy(expstr, "");
	//printf("B: pos = %d\n", pos);	
	//printf("C: expstr = %s\n", expstr);	
	exponent = strtol((const char*)&dest[pos + 1], &cPtr, 10);
	if (exponent != 0) itoa(exponent, expstr);
	else strcpy(expstr, "");
	//printf("D: expstr = '%s'\n", expstr);
	pos = strlen(expstr);
	if (pos)
		adjlen = len - pos - 3;// leave space for the 'e', '.' and integer part
	else
		adjlen = len - pos - 2;// leave space for the '.' and integer part
	if (dbl < 0) adjlen--;
	if (adjlen < 0) adjlen = 0;
	//printf("E: strlen(expstr) = %ld\n", strlen(expstr));	
	strcpy(fmt, "%.");
	itoa(adjlen, &fmt[2]);
	strcat(fmt, "Le");
	//printf("F: fmt = '%s'\n", fmt);	
	sprintf(dest, fmt, dbl); 
	pos = (int)(strchr(dest, 'e') - dest); //pos will be non-zero
	dest[pos] = '\0';
	//printf("G: dest = '%s'\n", dest);	
	if (exponent != 0) strcat(dest, "e");
	strcat(dest, expstr);
	//printf("Z: adjlen = %d expstr = '%s' dest = '%s'\n", adjlen, expstr, dest);	
}


void addOFlowIndicator(char*dest, int8_t barrier) {
	char indicator[] = {OFLOWIND, '\0'};
	char barrierind[] = {BARRIERIND, '\0'};
	strcat(dest, indicator);
	if (barrier) strcat(dest, barrierind);
}

void makeComplexStringFit(char*dest, char* src, int len, int8_t barrier) {
	//barrier is 0 or 1
	char temp0[VSHORT_STRING_SIZE];
	char temp1[VSHORT_STRING_SIZE];
	int spacePos = 0;

	int slen = strlen(src);

	if (slen <= NUMBER_LINESIZE - barrier) {
		strcpy(dest, src);
		char barrierind[] = {BARRIERIND, '\0'};
		if (barrier) strcat(dest, barrierind);
		return;
	}

	if (src[0] == '"') {
		//string
		zstrncpy(dest, &src[1], NUMBER_LINESIZE - barrier); //don't copy "s
		char barrierind[] = {BARRIERIND, '\0'};
		if (barrier) strcat(dest, barrierind);
		return;
	}
	else if (src[0] != '(') {
		//number is real
		makeLDoubleStringFit(dest, src, NUMBER_LINESIZE - 1 - barrier);
		addOFlowIndicator(dest, barrier);
		return;
	}

	//code below is only for imaginary or complex numbers
	strcpy(dest, "(");

	while (src[spacePos] != ' ' && src[spacePos++] != '\0');

	//spacePos indicates the position of the space
	if (spacePos > (slen - 2)) {
		//space was not found, so this is a pure imaginary number
		zstrncpy(temp0, &src[1], slen - 2);//copy the number into temp0
		makeLDoubleStringFit(temp1, temp0, NUMBER_LINESIZE_WO_PARENS - 1 - barrier); //keep space for '(' & ')' and overflow ind
		strcat(dest, temp1);
		strcat(dest, ")");
		if (slen > NUMBER_LINESIZE) addOFlowIndicator(dest, barrier);
	} else {
		//complex number
		//real part length = spacePos - 1
		//imag part length = slen - (spacePos - 1) - 3 = slen - spacePos - 2
		zstrncpy(temp0, &src[1], spacePos - 1);
		makeLDoubleStringFit(temp1, temp0, COMPLEX_NUM_DISP_SIZE - barrier);
		//temp1 is now the real part
		strcat(dest, temp1);
		strcat(dest, " ");
		zstrncpy(temp1, &src[spacePos + 1], slen - spacePos - 2);
		makeLDoubleStringFit(temp0, temp1, COMPLEX_NUM_DISP_SIZE - barrier);
		//temp0 is now the imag part
		strcat(dest, temp0);
		strcat(dest, ")");
		addOFlowIndicator(dest, barrier);
	}
}

void makeComplexMatVecStringFit(char*dest, char* src, int len, int8_t barrier) {
	if (src[0] == '[' || src[0] == '{') {
		if ((int)strlen(src) > len) {
			zstrncpy(dest, src, len - 1);
			addOFlowIndicator(dest, barrier);
		}
		else strcpy(dest, src);
	}
	else makeComplexStringFit(dest, src, NUMBER_LINESIZE, barrier);
}

#include "TS-core-showStack.h"

void showUserEntryLine(int bsp){ 
	//print userline status here
	memset(vm.userDisplay, 0, SHORT_STRING_SIZE);
	int len = strlen(vm.userInput);

	//keep 1 char for current input char
	if (len > DISPLAY_LINESIZE - 1) {
		lcd.setCursor(0, 3); //col, row
		//keep 1 char for the scrolled off indication
		lcd.print(LEFTARROW);
		vm.userInputLeftOflow = true;
		zstrncpy(vm.userDisplay, &vm.userInput[len - DISPLAY_LINESIZE + 2], DISPLAY_LINESIZE - 2);
	} else {
		int colPos = DISPLAY_LINESIZE - len - 1;
		if (bsp) {
			lcd.setCursor(colPos - 1, DISPLAY_LINECOUNT - 1);
			lcd.print(' ');
		}
		lcd.setCursor(colPos, DISPLAY_LINECOUNT - 1); //col, row
		zstrncpy(vm.userDisplay, vm.userInput, len);
	}
	lcd.print(vm.userDisplay);
	lcd.setCursor(vm.cursorPos, DISPLAY_LINECOUNT - 1); //col, row
}

typedef long double (*RealFunctionPtr)(ComplexDouble);
typedef void (*BigIntVoid1ParamFunctionPtr)(const bigint_t*, char*);
typedef void (*BigIntVoid2ParamFunctionPtr)(const bigint_t*, const bigint_t*, bigint_t*);
typedef int (*BigIntIntFunctionPtr)(const bigint_t*, const bigint_t*);
typedef ComplexDouble (*ComplexFunctionPtr1Param)(ComplexDouble);
typedef ComplexDouble (*ComplexFunctionPtr2Param)(ComplexDouble, ComplexDouble);
typedef ComplexDouble (*ComplexFunctionPtrVector)(ComplexDouble, ComplexDouble, ComplexDouble, int);

const char* matrixfnname[] = {
						"det", "inv",
						"iden", "trace",
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

//these return ComplexDouble
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
const int NUMMATH2PARAMFNOP = 18;
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
	ComplexDouble u = cdivd(summed, double(n)); //mean
	return csub(cdivd(sqsummed, double(n)), cmul(u, u)); //expansion of sum((x - u)**2)
}
ComplexDouble sdv(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n) {
	if (n <= 1) return makeComplex(0.0, 0.0);
	ComplexDouble u = cdivd(summed, double(n)); //mean
	ComplexDouble v = csub(cdivd(sqsummed, double(n - 1)), cdivd(cmul(cpowerd(u, 2), u), double(n - 1))); //use 1/(n - 1)
	return csqroot(v);
}
ComplexDouble mean(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n) {
	return cdivd(summed, double(n));
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

uint8_t conditionalData(int32_t execData) {
	//data being currently entered is a conditional if/else
	return (execData & 0x7);
}

int is1ParamBigIntFunction(const char* token) {
	for (int i = 0; i < NUMBIGINT1FNS; i++) {
		if (strcmp(bigfnvoid1paramname[i], token) == 0) return i;
	}
	return -1;
}

#include "TS-core-fnOrOpVec2Param.h"
#include "TS-core-fnOrOpMat2Param.h"
#include "TS-core-fnOrOp2Param.h"
#include "TS-core-fnOrOpVec1Param.h"
#include "TS-core-fnOrOpMat1Param.h"
#include "TS-core-fn1Param.h"

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
	//SerialPrint(1, "------------------- PRINT - called with token = %s", token);
	
	if (token[0] == '"' && token[strlen(token)-1] == '"') {
		//immediate string
		//token = removeDblQuotes(token);
		//SerialPrint(2, token, "\r\n");
	} else if (varNameIsLegal(token)) {
		if (token[0] == '\0') {
			int8_t meta;
			//POP var -- pop the top of stack into variable 'var'
			meta = pop(&vm->userStack, vm->bak);
			FAILANDRETURN((meta == -1), vm->error, "stack empty", NULLFN)		
		} else  {
			strcpy(vm->bak, token);
		}
		int vt = findVariable(&vm->ledger, token);
		if (vt == VARIABLE_TYPE_COMPLEX || vt == VARIABLE_TYPE_VECMAT) {
			ComplexDouble c = fetchVariableComplexValue(&vm->ledger, token);
			if (complexToString(c, vm->coadiutor, vm->precision, vm->notationStr)) { 
				//SerialPrint(4, token, " = ", vm->coadiutor, "\r\n");
			}
		} else if (vt == VARIABLE_TYPE_STRING) {
			if (getVariableStringVecMatValue(&vm->ledger, token, vm->coadiutor)) { //returns true
				//SerialPrint(4, token, " = ", vm->coadiutor, "\r\n");
			}
		} else {
			FAILANDRETURNVAR(true, vm->error, "no variable %s", fitstr(vm->coadiutor, token, 4))
		}
	} else {
		FAILANDRETURNVAR(true, vm->error, "no variable %s", fitstr(vm->coadiutor, token, 4))
	}
	return true;
}

#include "TS-core-processPop.h"
#include "TS-core-processDefaultPush.h"
#include "TS-core-process.h"

void interpret(Machine* vm, char* sourceCode) {
	char* input;
	char output[STRING_SIZE];
	bool success;
	//SerialPrint(1, "-------------------START: Source Code = %s", sourceCode);
	input = sourceCode;
	do {
		strcpy(vm->error, "");
		//SerialPrint(1, "main: do-while before tokenize input = %p", input);
		//SerialPrint(1, "main: do-while before tokenize input = %s", input);
		input = tokenize(input, output);
		//SerialPrint(1, "main: do-while after tokenize input = %p", input);
		//SerialPrint(1, "main: do-while after tokenize input = %s", input);
		if (output[0] == '\0') break;
		success = process(vm, output);
		//SerialPrint(1, "-------------------while: output = %s next input = %s input[0] = %d", output, input, input[0]);
		if ((strcmp(vm->error,"") != 0) || !success) break;
	} while (input[0] != '\0');
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
	strcpy(vm->error, "");
	strcpy(vm->lastX, "0");
	strcpy(vm->lastY, "0");
	vm->userInputPos = 0;
	vm->cmdPage = 0;
	vm->altState = 0;
	vm->viewPage = NORMAL_VIEW;
	vm->topEntryNum = DISPLAY_LINECOUNT - 1;
	vm->pointerRow = DISPLAY_LINECOUNT - 1;
	vm->modeDegrees = false;
	vm->modePolar = false;
	vm->userInputLeftOflow = false;
	vm->userInputRightOflow = false;
	vm->cursorPos = DISPLAY_LINESIZE - 1; //between 0 and 19, both inclusive
	vm->timerRunning = false;
	vm->repeatingAlarm = false;
	vm->LEDState = false;
	vm->frequency = 1;
	vm->partialVector = false;
	vm->partialMatrix = false;
	vm->partialComplex = false;
	vm->precision = 15;
	vm->quickViewPage = 0;
	strcpy(vm->notationStr, "Lg");
	vm->bigMod.length = -1;	
	vm->width = 64; //max = 18446744073709551616

	//zhr=5.5, latt=22.5726, longt=88.3639): Kolkata
	//zhr=5.5, latt=12.9716, longt=77.5946): Bangalore

	vm->locationLat = 12.9716;
	vm->locationLong = 77.5946;
	//vm->locationLat = 22.5726;
	//vm->locationLong = 88.3639;
	vm->locationTimeZone = 5.5;
}

