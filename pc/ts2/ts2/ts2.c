/*
(C) Anirban Banerjee 2025
License: GNU GPL v3
*/
#ifdef _UNIX
#include <termios.h>
#include <unistd.h>
#include "../microrl/unix.h"
#endif
#include "../microrl/microrl.h"
#include "TS-core-miscel.h"
#include "TS-core-ledger.h"
#include "TS-core-stack.h"
#include "TS-core-numString.h"
#include "TS-core-math.h"
#include "ts2.h"

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

void printRegisters(Machine* vm) {
	printf("\tError: %s\n", (vm->error[0] != '\0')? vm->error: "\"\"");
}

void interpret(Machine* vm, char* sourceCode) {
	char* input;
	char output[STRING_SIZE];
	bool success;
	input = sourceCode;
	do {
		strcpy(vm->error, "No error.");
		input = tokenize(input, output);
		if (output[0] == '\0') break;
		success = process(vm, output);
		if ((strcmp(vm->error,"No error.") != 0) || !success) break;
	} while (input[0] != '\0');
}

void showScreen(Machine* vm) {
	printf("===================================\n");
	printf("\tMode: %s %s\n", (vm->modeDegrees)? "Degrees": "Radian", (vm->modePolar)? "Polar": "Cartesian");
	printStack(&vm->userStack, 0, true);
	printRegisters(vm);
	printf("===================================\n");
}

static Machine vm;
int ts2Execute (char* line) {
	char command[STRING_SIZE];
	int l = strlen(line);
	if (l > 128) l = 128;
	zstrncpy(&command[0], line, l);
	command[l] = '\0';
	interpret(&vm, command);
	showScreen(&vm);
	return 0;
}

microrl_t rl;
microrl_t * prl = &rl;

int main(int argc, char **argv) {

	platform_init ();
	initMachine(&vm);
	showScreen(&vm);

	microrl_init (prl, print);
	// set callback for execute
	microrl_set_execute_callback (prl, execute);

	#ifdef _USE_COMPLETE
	// set callback for completion
	microrl_set_complete_callback (prl, complet);
	#endif
	// set callback for Ctrl+C
	microrl_set_sigint_callback (prl, sigint);
	struct termios oldt, newt;
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );

	while (1) {
		// put received char from stdin to microrl lib
		microrl_insert_char (prl, get_char());
	}

	printf("Goodbye!\n");

	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
	return 0;
}

