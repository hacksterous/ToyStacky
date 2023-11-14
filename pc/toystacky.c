#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include "linenoise.h"
#include "bigint.h"

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
#define SHORT_STRING_SIZE 33
#define DOUBLE_EPS 2.2250738585072014e-308
#define DOUBLEFN_EPS 1.5e-16 //for return values of functions
#define POSTFIX_BEHAVE 0

#define VECSTARTTOKENC '['
#define VECLASTTOKENC ']'
#define PRINTTOKENC '?'
#define POPTOKENC '@'
#define VECSTARTINDICATOR '\22'

#define VECSTARTTOKEN "["
#define MATSTARTTOKEN "[["
#define VECLASTTOKEN "]"
#define MATLASTTOKEN "]]"
#define IFTOKEN "if"
#define ELSETOKEN "el"
#define ENDIFTOKEN "fi"
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

void (*NULLFN)(void) = NULL;

#define FAILANDRETURN(failcondition,dest,src,fnptr)	\
	if (failcondition) {							\
		sprintf(dest, src);							\
		if (fnptr != NULL) fnptr();					\
		return false;								\
	}												\

#define FAILANDRETURNVAR(failcondition,dest,src,var)	\
	if (failcondition) {								\
		sprintf(dest, src, var);						\
		return false;									\
	}													\

#define GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)	\
	conditional = ((execData >> 2) & 0x1);							\
	if (conditional) {												\
		ifCondition = (execData >> 1) & 0x1;							\
		doingIf = execData & 0x1;									\
	}																\

typedef enum {
	METANONE,
	METASTARTVECTOR,
	METAMIDVECTOR,
	METAENDVECTOR,
	METASTARTMATRIX,
	METAMIDMATRIX,
	METAENDMATRIX
} StrackMeta;

const char* VACC = "#vacc";
const char* DEBUGMETA[7] = {"METANONE",
	"METASTARTVECTOR",
	"METAMIDVECTOR",
	"METAENDVECTOR",
	"METASTARTMATRIX",
	"METAMIDMATRIX",
	"METAENDMATRIX"};

typedef struct {
	double real;
	double imag;
} ComplexDouble;

ComplexDouble makeComplex(double re, double im) {
	ComplexDouble ret;
	ret.real = re;
	ret.imag = im;
	return ret;
}

#define MKCPLX(a,...) makeComplex(a, (0, ##__VA_ARGS__))

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
} Strack;

//used for branch and condition stack
typedef struct {
	size_t stack[EXEC_STACK_SIZE];
	int top;
} UintStack;

//dest must be previously sized to len + 1
char* fitstr(char*dest, const char* src, size_t len) {
	if (strlen(src) > len) {
		dest[len] = '\0';
		dest[len - 1] = dest[len - 2] = dest[len - 3] = '.';
		strncpy(dest, src, len - 3);
	} else strncpy(dest, src, len);
	return dest;
}

//https://www.cs.hmc.edu/~geoff/classes/hmc.cs070.200101/homework10/hashfuncs.html
//	highorder = h & 0xf8000000	// extract high-order 5 bits from h
//								  // 0xf8000000 is the hexadecimal representation
//								  //   for the 32-bit number with the first five 
//								  //   bits = 1 and the other bits = 0   
//	h = h << 5					// shift h left by 5 bits
//	h = h ^ (highorder >> 27)	 // move the highorder 5 bits to the low-order
//								  //   end and XOR into h
//	h = h ^ ki					// XOR h and ki
//
//	32-bit CRC

uint32_t hash(const char* key, uint32_t capacity) {
	uint32_t h = 0;
	uint32_t highorder = 0;
	for (int i = 0;  i < strlen(key); i++) {
		highorder = h & 0xf8000000;
		h = h << 5;
		h = h ^ (highorder >> 27);
		h = h ^ (uint32_t) key[i];
	}
	return h % capacity;
}

void initializeLedger(Ledger* ledger) {
	memset(ledger->variables, 0, sizeof(Variable) * MAX_VARIABLES);
	memset(ledger->memory, 0, MEMORY_SIZE);
	ledger->memoryOffset = 0;
	ledger->varCount = 0;
}

Variable* createVariable(Ledger* ledger, const char* name, 
						VariableType type, ComplexDouble doubleValue, const char* stringValue) {
	// if a variable with the same name already exists, overwrite it!

	//printf ("createVariable: just entered ledger->memoryOffset is %lu; creating variable %s\n", ledger->memoryOffset, name);
	if (ledger->varCount >= MAX_VARIABLES) {
		//printf ("createVariable: just entered FAIL ledger->varCount >= MAX_VARIABLES\n");
		return NULL;
	}
	size_t stringLength = strlen(stringValue);
	
	if ((stringLength == 0) && (type == VARIABLE_TYPE_STRING)) return NULL;
	if (stringLength > 0) stringLength++;

	if (ledger->memoryOffset + stringLength > MEMORY_SIZE) {
		//printf ("createVariable: just entered FAIL ledger->memoryOffset + stringLength > MEMORY_SIZE\n");
		return NULL;
	}
	uint32_t index = hash(name, MAX_VARIABLES);
	//printf ("createVariable: just entered ledger->varCount is %lu; creating variable %s with type %d\n", ledger->varCount, name, (int) type);
	Variable* variable = &ledger->variables[index];

	strncpy(variable->name, name, sizeof(variable->name) - 1);
	variable->name[sizeof(variable->name) - 1] = '\0';
	variable->type = type;

	if (type == VARIABLE_TYPE_STRING) {
		variable->value.stringValueIndex = ledger->memoryOffset;
		//printf ("createVariable: stringValueIndex is set to %X for variable %s\n", (unsigned int)variable->value.stringValueIndex, name);
		strncpy(ledger->memory + ledger->memoryOffset, stringValue, stringLength);
		ledger->memoryOffset += stringLength;
	} else if (type == VARIABLE_TYPE_COMPLEX || type == VARIABLE_TYPE_VECMAT) {
		//printf ("createVariable: doubleValue.real is set to %g for variable %s\n", doubleValue.real, name);
		variable->value.doubleValue = doubleValue;
	}
	ledger->varCount++;
	return variable;
}

bool getVariableComplexValue(Ledger* ledger, const char* name, ComplexDouble* value) {
	uint32_t index = hash(name, MAX_VARIABLES);
	Variable* variable = &ledger->variables[index];
	if (variable->name[0] == '\0') return false;
	if (strcmp(variable->name, name) == 0 && (variable->type == VARIABLE_TYPE_COMPLEX
			|| variable->type == VARIABLE_TYPE_VECMAT)) {
		value->real = variable->value.doubleValue.real;
		value->imag = variable->value.doubleValue.imag;
		return true;
	} else return false;
}

bool getVariableStringValue(Ledger* ledger, const char* name, char* value) {
	uint32_t index = hash(name, MAX_VARIABLES);
	Variable* variable = &ledger->variables[index];
	if (variable->name[0] == '\0') return NULL;
	if (strcmp(variable->name, name) == 0 && variable->type == VARIABLE_TYPE_STRING) {
		strcpy(value, ledger->memory + variable->value.stringValueIndex);
		return true;
	} else return false;
}

ComplexDouble fetchVariableComplexValue(Ledger* ledger, const char* name) {
	uint32_t index = hash(name, MAX_VARIABLES);
	Variable* variable = &ledger->variables[index];
	if (variable->name[0] == '\0') return MKCPLX(0);
	if (strcmp(variable->name, name) == 0 && (variable->type == VARIABLE_TYPE_COMPLEX
			|| variable->type == VARIABLE_TYPE_VECMAT)) {
		//printf("fetchVariableComplexValue: returning with value.real = %g for var '%s'\n", variable->value.doubleValue.real, name);
		return variable->value.doubleValue;
	} else return MKCPLX(0);
}

char* fetchVariableStringValue(Ledger* ledger, const char* name) {
	uint32_t index = hash(name, MAX_VARIABLES);
	Variable* variable = &ledger->variables[index];
	if (variable->name[0] == '\0') return NULL;
	if (strcmp(variable->name, name) == 0 && variable->type == VARIABLE_TYPE_STRING) {
		return ledger->memory + variable->value.stringValueIndex;
	} else return NULL;
}

void updateComplexVariable(Ledger* ledger, const char* name, ComplexDouble doubleValue) {
	for (int i = 0; i < ledger->varCount; i++) {
		Variable* variable = &ledger->variables[i];
		if (strcmp(variable->name, name) == 0 && (variable->type == VARIABLE_TYPE_COMPLEX
				|| variable->type == VARIABLE_TYPE_VECMAT)) {
			variable->value.doubleValue = doubleValue;
			return;
		}
	}
}

void deleteVariable(Ledger* ledger, const char* name) {
	uint32_t index = hash(name, MAX_VARIABLES);
	Variable* variable = &ledger->variables[index];
	if (strcmp(variable->name, name) == 0) {
		memset(variable->name, 0, sizeof(variable->name));
		if (variable->type == VARIABLE_TYPE_STRING) {
			int stringLength = strlen(ledger->memory + variable->value.stringValueIndex) + 1;
			memset(ledger->memory + variable->value.stringValueIndex, 0, stringLength);
		}
		ledger->varCount--;
		return;
	}
}

int findVariable(Ledger* ledger, const char* name) {
	//printf("findVariable: -- looking for variable %s\n", name);
	if (strlen(name) > MAX_VARNAME_LEN - 1) 
		return -1;
	uint32_t index = hash(name, MAX_VARIABLES);
	Variable* variable = &ledger->variables[index];
	//if (variable->name[0] == '\0') return -1;
	if (strcmp(variable->name, name) == 0) {
		//printf("findVariable: -- variable %s returning with %d\n", name, (int) variable->type);
		return (int) variable->type;
	}
	//printf("findVariable: -- did NOT find variable %s\n", name);
	return -1;
}

bool updateLedger(Ledger* ledger, size_t memIndexThreshold, int memIndexIncr) {
	if (ledger->varCount == 0) return true; //nothing to update
	int varCount = 0;
	int i = 0;
	while (i < MAX_VARIABLES) {
		Variable* variable = &ledger->variables[i++];
		//printf("updateLedger: ===================== varCount = %d\n", varCount);
		if (varCount >= ledger->varCount) break;
		if (variable->name[0] == '\0') continue; //skip the empty variable pods
		if (variable->type == VARIABLE_TYPE_STRING) {
			if (variable->value.stringValueIndex > memIndexThreshold) {
				if (variable->value.stringValueIndex + memIndexIncr > MEMORY_SIZE) return false;
				if (variable->value.stringValueIndex + memIndexIncr < 0) return false;
				variable->value.stringValueIndex += memIndexIncr;
				//printf("updateLedger: Variable %s at vartable index %d: updated memoryIndex to %s\n", variable->name, i, ledger->memory + variable->value.stringValueIndex);
			}
		}
		varCount++;
	}
	return true;
}

bool updateStringVariable(Ledger* ledger, const char* name, const char* newString) {
	int variableIndex = findVariable(ledger, name);
	if (variableIndex == -1) {
		return false;  // variable not found
	}

	size_t newSize = strlen(newString);

	Variable* variable = &ledger->variables[variableIndex];
	if (variable->type != VARIABLE_TYPE_STRING) {
		return false;  // variable is not of type VARIABLE_TYPE_STRING
	}

	char* stringValue = ledger->memory + variable->value.stringValueIndex;
	size_t currentSize = strlen(stringValue);

	if (newSize > currentSize) {
		int additionalBytes = newSize - currentSize;
		//printf("updateStringVariable: additionalBytes required = %lu\n", additionalBytes);
		if (ledger->memoryOffset + additionalBytes + 1 > MEMORY_SIZE) {
			return false;  // Not enough memory available to resize
		}
		//printf("updateStringVariable: will move string %s\n", stringValue + currentSize + 1);
		memmove(stringValue + newSize + 1, stringValue + currentSize + 1, ledger->memoryOffset - currentSize);
		ledger->memoryOffset += additionalBytes;
		updateLedger(ledger, variable->value.stringValueIndex, additionalBytes);
	} else if (newSize < currentSize) {
		size_t bytesToRemove = currentSize - newSize;
		//printf("updateStringVariable: bytesToRemove = %lu\n", bytesToRemove);
		memmove(stringValue + newSize, stringValue + newSize + bytesToRemove, currentSize - newSize);
		ledger->memoryOffset -= bytesToRemove;
		updateLedger(ledger, variable->value.stringValueIndex, -bytesToRemove);
	} //else they are equal

	strncpy(stringValue, newString, newSize);
	return true;  // No need to resize
}

void compactMemory(Ledger* ledger) {
	int compactOffset = 0;

	int varCount = 0;
	int i = 0;
	while (i < MAX_VARIABLES) {
		Variable* variable = &ledger->variables[i++];
		if (varCount >= ledger->varCount) break;
		//printf("compactMemory ===================== varCount = %d\n", varCount);
		if (variable->name[0] == '\0') continue; //skip the empty variable pods
		if (variable->type == VARIABLE_TYPE_STRING) {
			//printf ("compactMemory: variable index is %d; variable variable %s compactOffset = %d\n", i, variable->name, compactOffset);
			int stringLength = strlen(ledger->memory + variable->value.stringValueIndex);
			if (stringLength > 0)
				stringLength++; //add 1 for the \0, not required if string is already null
			memmove(ledger->memory + compactOffset, ledger->memory + variable->value.stringValueIndex, stringLength);
			variable->value.stringValueIndex = compactOffset;
			compactOffset += stringLength;
			//printf ("compactMemory: Updated compactOffset to %d\n", compactOffset);
		}
		varCount++;
	}
	ledger->memoryOffset = compactOffset;
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
			printf("Variable %s at vartable index %d: %.15f\n", variable->name, varCount, variable->value.doubleValue.real);
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

void stackInit(Strack *s) {
	s->topStr = -1;
	s->topLen = -1;
}

bool stackIsEmpty(Strack *s) {
	if (s->topLen == -1 && s->topStr != -1)
		printf("Stack warning: Strack top indices out of sync!\n");
	return (s->topLen == -1 || s->topStr == -1);
}

int stackIsFull(Strack *s) {
	return (s->topStr >= STACK_SIZE - 1 || s->topLen >= STACK_NUM_ENTRIES - 1);
}

bool push(Strack *s, char* value, int8_t meta) {
	//printf("push: entering-------- with %s of len = %lu (including '\\0')\n", value, strlen(value)+1);
	size_t len = strlen(value);
	if (len > 0) len++; //include the '\0'
	if (s->topStr + len >= STACK_SIZE - 1 || s->topLen == STACK_SIZE - 1) {
		return false;
	}
	//printf("push: s->topStr = %d\n", s->topStr);
	//printf("push: s->topLen = %d\n", s->topLen);
	s->stackLen[++s->topLen] = len + (((int8_t)meta & 0xff) << 24); //meta
	//printf("push: len + meta = %lu\n", len + (((int8_t)meta & 0xff) << 24));
	strcpy(&s->stackStr[s->topStr + 1], value);
	//printf("push: string value = %s\n", &s->stackStr[s->topStr + 1]);
	s->topStr += len;

	//printf("push: NOW s->topStr = %d\n", s->topStr);
	//printf("push: NOW s->topLen = %d\n", s->topLen);
	//printf("push: returning--------\n");
	return true;
}

int8_t pop(Strack *s, char* output) {
	//printf("pop: entering--------\n");
	if (stackIsEmpty(s)) {
		output = NULL;
		return -1;
	}
	//printf("pop: s->topStr = %d\n", s->topStr);
	//printf("pop: s->topLen = %d\n", s->topLen);
	int8_t meta = (s->stackLen[s->topLen] >> 24) & 0xff;
	size_t len = s->stackLen[s->topLen--] & 0xffffff;
	//printf("pop: length retrieved = %lu\n", len);
	if (output)
		strcpy(output, &s->stackStr[s->topStr - len + 1]);
	//printf("pop: string output = %s\n", output);
	s->topStr -= len;
	//printf("pop: NOW s->topStr = %d\n", s->topStr);
	//printf("pop: NOW s->topLen = %d\n", s->topLen);
	//printf("pop: returning--------\n");
	return meta;
}

int8_t peek(Strack *s, char output[STRING_SIZE]) {
	//printf("Peek: entered Strack peek\n");
	if (stackIsEmpty(s)) {
		output = NULL;
		//printf("Peek: Strack is empty\n");
		return -1;
	}
	//printf("peek: s->topStr = %d\n", s->topStr);
	//printf("peek: s->topLen = %d\n", s->topLen);
	int8_t meta = (s->stackLen[s->topLen] >> 24) & 0xff;
	size_t len = s->stackLen[s->topLen] & 0xffffff; 
	if (output)
		strcpy(output, &s->stackStr[s->topStr - len + 1]);
	//printf("peek: string output = %s\n", output);
	//printf("peek: NOW s->topStr = %d\n", s->topStr);
	//printf("peek: NOW s->topLen = %d\n", s->topLen);
	return meta;
}

int8_t peekn(Strack* s, char output[STRING_SIZE], int n) {
	//n = 0: return T
	//n = 1: return T - 1
	if (n == 0) return peek(s, output);
	if (stackIsEmpty(s) || (n > s->topLen)) {
		//printf("peekn: n is > s->topLen %d\n", s->topLen);
		output = NULL;
		return -1;
	}
	n++;

	//printf("peekn: s->topStr = %s\n", s->topStr);
	//printf("peekn: s->topLen = %d\n", s->topLen);
	char* stringPtr;
	size_t stringLen = -1;
	int8_t meta;
	for (int i = 0; i < n; i++) {
		//printf("peekn: s->stackLen[%d] = %lu\n", i, s->stackLen[s->topLen - i]);
		stringLen += s->stackLen[s->topLen - i] & 0xffffff;
	}
	//now s->topStr - stringLen will point to the first string
	stringPtr = &s->stackStr[s->topStr - stringLen];
	meta = (s->stackLen[s->topLen - n + 1] >> 24) & 0xff;
	if (output)
		strcpy(output, stringPtr);
	return meta;
}

int makeString(char*dest, const char* src, int meta, size_t len) {
	//printf("makeString - src = %s\n", src);
	char pre[3];
	char post[3];

	int slen = strlen(src);
	strcpy(pre, (meta == 1)? "[": ((meta == 4)? "[[": ""));
	int prelen = strlen(pre);
	strcpy(post, (meta == 3)? "]": ((meta == 6)? "]]": ""));
	int postlen = strlen(post);
	if (prelen)
		memcpy(dest, pre, prelen); //the '[' or '[[' if present

	int availLen = len - prelen - postlen;
	
	if (slen <= availLen) {
		strcpy(dest + prelen, src);
		return -1;
	}

	int expoIndex = 0;
	for (expoIndex = 0; expoIndex < slen; expoIndex++) {
		if ((src[expoIndex] == 'e') || (src[expoIndex] == 'E')) {
			break;
		}
	}

	int partMantissaLen = availLen - (slen - expoIndex) - 1; //1 for the indicator '>'
	//printf("makeString - partMantissaLen = %d and expoIndex = %d\n", partMantissaLen, expoIndex);
	//printf("makeString - len = %lu and slen = %d\n", len, slen);
	//printf("makeString - availLen = %d\n", availLen);
	if (expoIndex == slen) {
		//does not have the exponent format
		if (slen > availLen) {
			//printf("makeString - C dest = %s len of dest = %lu\n", dest, strlen(dest));
			//bigint --  show right overflow
			strncpy(dest + prelen, src, availLen - 1);
			dest[prelen + availLen - 1] = '>';
			dest[prelen + availLen] = '\0';
			if (postlen)
				strcpy(dest + prelen + availLen, post);
			return prelen + availLen - 1;
		} else {
			//printf("makeString - D dest = %s len of dest = %lu\n", dest, strlen(dest));
			strncpy(prelen + dest, src, slen);
			if (postlen)
				strcpy(prelen + dest + slen, post);
			return -1;
		}
	}
	//printf("makeString ------------- Z\n");

	//we want to remove some digits from the mantissa
	//part of the real number so that it fits within len
	//availLen = len - prelen - postlen
	//partmantissalen = len - prelen - postlen - (slen - expoIndex) - 1
	//this has to be less than if we have to truncate
	//the mantissa
	//availLen = len - prelen - postlen --> is available for the number
	//slen - expoIndex --> exponent part
	if (availLen < slen) {
		//printf("makeString - A dest = %s len of dest = %lu\n", dest, strlen(dest));
		strncpy(dest + prelen, src, partMantissaLen);
		dest[prelen + partMantissaLen] = '>'; //overflow indicator
		strncpy(dest + prelen + partMantissaLen + 1, &src[expoIndex], slen - expoIndex);
		if (postlen)
			strcpy(dest + len - postlen + 1, post);
		return (prelen + partMantissaLen);
	} else {
		//printf("makeString - B dest = %s len of dest = %lu\n", dest, strlen(dest));
		strcpy(dest, src);
		if (postlen)
			strcpy(dest + len - postlen + 1 , post);
		return -1;
	}
}

bool isRealNumber(const char *token) {
	int i = 0;
	int len = strlen(token);
	int hasDecimal = 0;
	int hasExponent = 0;
	int hasSign = 0;

	//printf("isRealNumber: - token = %s, len = %d\n", token, len);
	//printf("isRealNumber: - entered token[i] = %d (%c), i = %d\n", token[i], token[i], i);
	// Check for negative sign
	if (token[i] == '-' || token[i] == '+') {
		hasSign = 1;
		i++;
		if (i == len) {
			//printf("isRealNumber: - ret false A\n");
			return false;  // '-' alone is not a valid number
		}
	}

	// Check for digits or decimal point
	while (i < len) {
		//printf("isRealNumber: while loop -- i = %d -- token[i] = %d (%c) isdigit = %d\n", i, token[i], token[i], isdigit(token[i]));
		if (isdigit(token[i])) {
			//printf("isRealNumber: while loop -- is a digit is true\n");
			i++;
		} else if (token[i] == '.') {
			if (hasDecimal || hasExponent) {
				//printf("isRealNumber: - ret false B\n");
				return false;  // Multiple decimal points or decimal point after exponent is not valid
			}
			hasDecimal = 1;
			i++;
		} else if (token[i] == 'e' || token[i] == 'E') {
			if (hasExponent) {
				//printf("isRealNumber: - ret false C\n");
				return false;  // Multiple exponents are not valid
			}
			hasExponent = 1;
			i++;
			if (i < len && (token[i] == '+' || token[i] == '-')) {
				i++;  // Allow optional sign after exponent
			}
		} else {
			//printf("isRealNumber: - token[i] = %d (%c), i = %d\n", token[i], token[i], i);
			if ((token[i] == 'r' || token[i] == 'l' || token[i] == 'c') &&
				(i == (len - 1))) {
				//printf("isRealNumber: - ret true D\n");
				return true;
			} else {
				//printf("isRealNumber: - ret false D\n");
				return false;  // Invalid character
			}
		}
	}

	//make sure the token is fully parsed
	if (hasSign && i == 1) {
		//printf("isRealNumber: - ret false E\n");
		return false;  // Only a sign is not a valid number
	}
	//printf("isRealNumber: - ret (i == len) = %d E\n", (i == len));
	//printf("isRealNumber: - token[i] = %d (%c), i = %d\n", token[i], token[i], i);
	return (bool)(i == len);
}

void printStack(Strack* s, int count, bool firstLast) {
	if (stackIsEmpty(s) || count == 0) return;
	int origCount = count;
	//ensure the count is within the range of the stack elements
	count = (count > s->topLen + 1) ? s->topLen + 1 : count;

	for (int i = 0; i < origCount - count; i++) printf("\t..................\n"); 
	//printf("printStack: s->topStr = %d\n", s->topStr);
	//printf("printStack: s->topLen = %d\n", s->topLen);
	//printf("Printing %d entries from the top of the stack:\n", count);
	char* stringPtr;
	size_t stringLen = -1;
	int8_t meta;
	//int rightOflowIndicatorPos = -1;
	//char display[STRING_SIZE]; //coadiutor = helper
	if (firstLast) {
		for (int i = 0; i < count; i++) {
			//printf("printStack: s->stackLen[%d] = %lu\n", i, s->stackLen[s->topLen - i]);
			stringLen += s->stackLen[s->topLen - i] & 0xffffff;
		}
		//now s->topStr - stringLen will point to the first string
		//printf("printStack - doing firstLast -- before FOR loop -- topLen = %d, stringLen = %lu\n", s->topLen, stringLen);
		for (int i = 0; i < count; i++) {
			//printf("printStack - doing firstLast -- s->topLen = %d and stringLen = %lu\n", s->topLen, stringLen);
			stringPtr = &s->stackStr[s->topStr - stringLen];
			meta = (s->stackLen[s->topLen - count + i + 1] >> 24) & 0xff;
			//if (isRealNumber(stringPtr))
			//	rightOflowIndicatorPos = makeString(display, stringPtr, meta, 15);
			//printf("printStack: rightOflowIndicatorPos == %d, display string = %s\n", 
			//	rightOflowIndicatorPos, display);
			printf("\t%s%s%s meta = %s\n", 
					(meta == 1)? "[": ((meta == 4)? "[[": ""), 
					stringPtr, 
					(meta == 3)? "]": ((meta == 6)? "]]": ""), 
					DEBUGMETA[meta]);
			stringLen -= strlen(stringPtr) + 1;
		}
	} else {
		for (int i = s->topLen; i > s->topLen - count; i--) {		
			stringLen += s->stackLen[i] & 0xffffff;
			stringPtr = &s->stackStr[s->topStr - stringLen];
			printf("\t%s\n", stringPtr);
		}
	}
}

void UintStackInit(UintStack *s) {
	s->top = -1;
}

bool UintStackIsEmpty(UintStack *s) {
	return (s->top == -1);
}

bool UintStackIsFull(UintStack *s) {
	return (s->top == EXEC_STACK_SIZE - 1);
}

bool UintStackPush(UintStack *s, char value) {
	if (UintStackIsFull(s)) {
		return false;
	}
	s->stack[++s->top] = value;
	if (s->top > EXEC_STACK_SIZE - 1) s->top = EXEC_STACK_SIZE - 1;
	return true;
}

size_t UintStackPop(UintStack *s) {
	//printf("Popping from UintStack\n");
	if (UintStackIsEmpty(s)) {
		//printf("Pop warning: UintStack is empty\n");
		return 0;
	}
	return s->stack[s->top--];
}

size_t UintStackPeek(UintStack *s) {
	if (UintStackIsEmpty(s)) {
		//printf("Peek: Stack is empty\n");
		return 0;
	}
	return s->stack[s->top];
}

bool isComplexNumber(const char *input) {
	int i = 0;
	if (strlen(input) < 2) return false;
	bool success = false;
	//printf("isComplexNumber:------------------- input = %s\n", input);
	while (isspace(input[i])) {
		i++; //skip leading spaces
	}
	if (input[i++] == '(') success = true;
	else return false;
	//printf("isComplexNumber:------------------- len input = %lu, i = %d success = %d\n", strlen(input), i, success);
	while (input[i] != ')') {
		i++; //
	}
	if (input[i] == ')' && success) return true;
	return false;
}

int parseComplex(const char* input, char* substring1, char* substring2) {
	//a      --> real number
	//(b)    --> imaginary number
	//(a b)  --> complex number
	//printf("parseComplex: 0. input: '%s'\n", input);
	int i = 0;
	while (isspace(input[i])) {
		i++; //Skip leading spaces
	}
	//printf("parseComplex: 0. i = %d\n", i);
	//printf("parseComplex: 0. input[%d]: '%c'\n", i, input[i]);
	if (input[i] == '(') {
		i++;
		while (isspace(input[i])) {
			i++; //skip spaces
		}
	}
	//printf("parseComplex: 1. i = %d\n", i);
	//first string
	int j = 0;
	while (isalpha(input[i]) || isdigit(input[i]) || input[i] == '+' || input[i] == '-' || input[i] == '.') {
		//printf("parseComplex: i = %d, input[%d] = '%c'\n", i, i,input[i]);
		substring1[j++] = input[i++];
	}
	substring1[j] = '\0'; //null-terminate the first substring
	//printf("parseComplex: Substring 1: '%s'\n", substring1);
	while (isspace(input[i])) {
		i++; //skip spaces after first string
	}

	if (isalpha(input[i]) || isdigit(input[i]) || input[i] == '+' || input[i] == '-' || input[i] == '.') {
		int k = 0;
		while (isalpha(input[i]) || isdigit(input[i]) || input[i] == '+' || input[i] == '-' || input[i] == '.') {
			substring2[k++] = input[i++];
		}
		substring2[k] = '\0'; // null-terminate the second substring

		while (isspace(input[i])) {
			i++; //skip trailing spaces after second string
		}
		if (input[i] == ')' && j > 0 && k > 0) {
			//successfully parsed the second string
			//printf("parseComplex: Done Substring 2: '%s'\n", substring1);
			return 0;
		} else {
			return i;
		}
	} else if (input[i] == ')') {
		substring2[0] = '\0'; //no second part
		return 0;
	}
	return i; //invalid expression format - return bad character position
}

char strIsRLC(char* str) {
	size_t len = strlen(str);
	if (str[len-1] == 'r' || str[len-1] == 'l' || str[len-1] == 'c') return str[len-1];
	else return 0;
}

bool stringToDouble(char* str, double* dbl) {
	//printf("stringToDouble: entered ------------------- str = %s\n", str);
	char* endPtr;
	size_t len = strlen(str);
	char rlc = strIsRLC(str);
	if (rlc) {
		str[len-1] = '\0';
	}
	*dbl = strtod(str, &endPtr);
	//if (endPtr == str) {
	if (*endPtr != '\0') {
		//printf("stringToDouble: done with false ------------------- dbl = %.15g\n", *dbl);
		return false;
	} else {
		//printf("stringToDouble: done with true ------------------- dbl = %.15g\n", *dbl);
		if (rlc) str[len-1] = rlc;
		return true;
	}
}

bool stringToComplex(const char *input, ComplexDouble* c) {
	//the input is guaranteed not to have to leading or trailing spaces
	//(a, b) -> a + ib
	//(b) -> ib
	//printf("stringToComplex: just entered input = %s\n", input);
	if (input[0] != '(' ||  input[strlen(input)-1] != ')') return false;
	char str1[SHORT_STRING_SIZE];
	char str2[SHORT_STRING_SIZE];
	int failpoint = parseComplex(input, str1, str2);
	//printf("stringToComplex: parseComplex returned %d str1 = %s --- str2 = %s\n", failpoint, str1, str2);
	double r;
	double i;
	bool success;
	if (failpoint != 0) return false;
	if (str1[0] != '\0' && str2[0] != '\0') {
		success = stringToDouble(str1, &r);
		success = success && stringToDouble(str2, &i);
		if (!success) {
			return false;
		} else {
			c->real = r;
			c->imag = i;
			return true;
		}
	} else if (str1[0] != '\0' && str2[0] == '\0') {
		success = stringToDouble(str1, &i);
		if (!success) {
			return false;
		} else {
			c->real = 0;
			c->imag = i;
			return true;
		}
	}
	return false;
}

bool alm0(double value) {
	//printf("alm0: ------ A. At start value string = %.16g\n", cabs(value));
	//if (cabs(value) < DOUBLE_EPS) return true;
	if (value == 0) return true;
	return false;
}

double complex add(double complex value, double complex second) {
	return value + second;
}

double complex sub(double complex value, double complex second) {
	return value - second;
}

double complex cmul(double complex value, double complex second) {
	return value * second;
}

double complex cdiv(double complex value, double complex second) {
	//printf("cdiv: value = %g second = %g\n", creal(value), creal(second));
	//printf("cdiv: value / second = %g\n", creal(value / second));
	return value / second;
}

double complex mod(double complex value, double complex second) {
	return 0.0 + 0.0*I;
}

double complex eq(double complex value, double complex second) {
	return (alm0(cabs(value - second))? 1.0 + 0.0*I: 0.0 + 0.0*I);
}

double complex neq(double complex value, double complex second) {
	return (!alm0(cabs(value - second))? 1.0 + 0.0*I: 0.0 + 0.0*I);
}

double complex par(double complex value, double complex second) {
	return (value*second)/(value + second);
}

double complex lt(double complex value, double complex second) {
	double complex diff = second - value;
	if (abs(cimag(diff)) < DOUBLE_EPS)
		//difference is real
		//LT is true if difference is positive
		return (creal(diff) > DOUBLE_EPS)? 1.0 + 0.0*I: 0.0 + 0.0*I;
	else
		//difference is complex
		//LT is true if magnitude of the diff is positive
		return (cabs(diff) > DOUBLE_EPS)? 1.0 + 0.0*I: 0.0 + 0.0*I;
}

double complex gt(double complex value, double complex second) {
	double complex diff = value - second;
	if (abs(cimag(diff)) < DOUBLE_EPS)
		//difference is real
		//GT is true if difference is positive
		return (creal(diff) > DOUBLE_EPS)? 1.0 + 0.0*I: 0.0 + 0.0*I;
	else
		//difference is complex
		//GT is true if magnitude of the diff is positive
		return (cabs(diff) > DOUBLE_EPS)? 1.0 + 0.0*I: 0.0 + 0.0*I;
}

double complex gte(double complex value, double complex second) {
	double complex l = lt(value, second);
	if (creal(l) == 1.0) return 0.0 + 0.0*I;
	else return 1.0 + 0.0*I;
}

double complex lte(double complex value, double complex second) {
	double complex g = gt(value, second);
	if (creal(g) == 1.0) return 0.0 + 0.0*I;
	else return 1.0 + 0.0*I;
}

/* reverse:  reverse string s in place */
void reverse(char s[]) {
	int i, j;
	char c;

	for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

/* itoa:  convert n to characters in s */
void itoa(int n, char s[]) {
	int i, sign;

	if ((sign = n) < 0)  /* record sign */
		n = -n;		  /* make n positive */
	i = 0;
	do {	   /* generate digits in reverse order */
		s[i++] = n % 10 + '0';   /* get next digit */
	} while ((n /= 10) > 0);	 /* delete it */
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}

char* lcase(char* token) {
	for (int i = 0; token[i] != '\0'; i++) {
		token[i] = tolower(token[i]);
	}
	return token;
}

bool doubleToString(double value, char* buf) {
	if (value == INFINITY || value == -INFINITY) return false;
	if (alm0(value)) {
		strcpy(buf, "0.0");		
		return true;
	}
	char fmt[7];
	//for small numbers (with exponent -10 etc), 16 decimal places
	//will give wrong digits - since these are beyond the precision
	//of double floats
	sprintf(buf, "%.16g", value);
	//printf("doubleToString: ------ A. At start value string = %s\n", buf);
	if (strcmp(lcase(buf), "inf") == 0 || strcmp(lcase(buf), "-inf") == 0) return false;
	if (strcmp(lcase(buf), "nan") == 0 || strcmp(lcase(buf), "-nan") == 0) return false;
	//printf("doubleToString: ------ B. At start value string = %s\n", buf);
	int i = 0;
	if (buf[0] == '-') i++; //skip any opening '-'
	bool expoIsNeg = false;
	for (i = i; buf[i] != '\0'; i++) {
		if (buf[i] == '-') {
			i++;
			expoIsNeg = true;
			break;
		}
	}
	if (expoIsNeg == false) return true; 
	//printf("doubleToString: ------ Minus found at i = %d\n", i);
	char* endPtr;
	int expo = strtod(buf + i, &endPtr);
	//printf("doubleToString: ------ Expo = %d\n", expo);
	if (expoIsNeg == true) {
		//printf("doubleToString: ------ expoIsNeg = %d\n", expoIsNeg);
		if (expo > 13) {
			strcpy(fmt, "%.3g");
		} else {
			int numDecimals = 15 - expo; //double precision
			if (numDecimals < 6) numDecimals = 6;
			//printf("doubleToString: ------ numDecimals = %d\n", numDecimals);
			strcpy(fmt, "%.");
			itoa(numDecimals, &fmt[2]);
			strcat(fmt, "g");
		}
		sprintf(buf, fmt, value);
	}

	//printf("doubleToString ------ buf = %s fmt = %s\n", buf, fmt);
	if (buf == NULL) return false;
	return true;
}

bool complexToString(ComplexDouble c, char* value) {
	char r[SHORT_STRING_SIZE];
	char i[SHORT_STRING_SIZE];
	bool goodnum = doubleToString(c.real, r);
	if (!goodnum) return false;
	goodnum = doubleToString(c.imag, i);
	if (!goodnum) return false;
	if (alm0(c.imag)) {
		//imag value is 0 - return r
		strcpy(value, r);
		return true;
	}
	//imag value is not zero, return one of
	//(1) (r i)
	//(2) (i)
	strcpy(value, "(");
	if (!(alm0(c.real))) {
		strcat(value, r);
		strcat(value, " ");
	}
	strcat(value, i);
	strcat(value, ")");
	return true;
}

int makeExecStackData(bool conditional, bool ifCondition, bool doingIf, bool matStarting, bool vecStarting) {
	//exactly one of the last two parameters can be true
	return  (matStarting << 4) + (vecStarting << 3) + 
			(conditional << 2) + (ifCondition << 1) + doingIf;
}

typedef struct {
	char PROGMEM[PROGMEM_SIZE];
	Ledger ledger;
	Strack userStack; //the user stack
	UintStack execStack; //the execution stack to keep track of conditionals, loops and calls
	char bak[STRING_SIZE];//bakilliary register
	char acc[STRING_SIZE];//the accumulator
	char error[SHORT_STRING_SIZE];//error notification
	char coadiutor[STRING_SIZE]; //coadiutor = helper
	bigint_t bigA;
	bigint_t bigB;
	bigint_t bigC;
} Machine;
static Machine vm;

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

double dabs(double d) {
	if (d < 0) return -d;
	return d;
}

double complex conj(double complex c) {
	double complex r = creal(c) - cimag(c)*I;
	return r;
}

double complex ccbrt(double complex x){
	double complex r = cpow(x, (1.0/3.0));
	//printf("ccbrt: done ------------------- r = %g + %gi dabs(r.imag) = %g TRUE = %d\n", creal(r), cimag(r), dabs(cimag(r)), (dabs(cimag(r)) > 0.0));
	if (dabs(cimag(r)) > 0.0) {
		//has imag part
		r = cdiv(x, cmul(r, conj(r)));
		//printf("ccbrt: recompute done ------------------- r = %g + %gi\n", creal(r), cimag(r));
	}
	return (r);
}

double complex clog2(double complex x){
	return clog(x)/clog(2);
}

double complex clog10(double complex x){
	return clog(x)/clog(10);
}

double complex cmax(complex double a, complex double b) {
	return cabs(a) > cabs(b) ? a : b;
}

double complex cmin(complex double a, complex double b) {
	return cabs(a) < cabs(b) ? a : b;
}

double complex catan2(complex double a, complex double b) {
	return cabs(a) < cabs(b) ? a : b;
}

typedef double (*RealFunctionPtr)();
typedef void (*BigIntVoidFunctionPtr)();
typedef int (*BigIntIntFunctionPtr)();
typedef double complex (*ComplexFunctionPtr)(double complex);
typedef double complex (*ComplexFunction2ParamPtr)(double complex, double complex);
typedef double complex (*ComplexFunctionPtrVector)(double complex, double complex, double complex, int);

//the 1 param functions that have complex or real result
const char* mathfn1paramname[] = {"sin", "cos",
								  "tan", "asin",
								  "acos", "atan", 
								  "exp", "lg10", 
								  "ln", "log2", 
								  "sqrt", "cbrt",
								  "proj", "conj",
								  "abs", "arg", 
								  "re", "im"}; 
const int NUMMATH1PARAMFN = 14;
const int NUMREAL1PARAMFN = 4;
const char* miscelfn1paramname[] = {"dup"};

const char* vecprefnname[] = {"var", "stdev", "ssdev"}; //these functions require a pre subtraction operation
const char* vecfn1paramname[] = {"sum", "sqsum", "var", "stdev", "mean", "rsum"};
const int NUMVECFNS = 6;

const char* mathfnop2paramname[] = {"swp", 
						"pow", "atn2", 
						"max", "min", 
						ADDTOKEN, SUBTOKEN,
						MULTOKEN, DIVTOKEN,
						MODTOKEN, GTTOKEN,
						LTTOKEN, GTETOKEN,
						LTETOKEN, EQTOKEN,
						NEQTOKEN, PARTOKEN
						}; //the 2 params functions
const int  NUMMATH2PARAMFNOP = 17;

//ADDTOKEN, SUBTOKEN,
//MULTOKEN, DIVTOKEN,
//MODTOKEN, GTTOKEN,
//LTTOKEN, GTETOKEN,
//LTETOKEN, EQTOKEN,
//NEQTOKEN
const int NUMMATHOPS = 11;

const char* mathfn3param[] = {"minv", "mexp"}; //the 3 params functions

double complex proj(double complex c) {
	return c;
}

//the 1 param functions that have complex result
ComplexFunctionPtr mathfn1param[] = {csin, ccos, 
									ctan, casin, 
									cacos, catan, 
									cexp, clog10, 
									clog, clog2, 
									csqrt, ccbrt, 
									proj, conj};

//the 1 param functions that have real result
RealFunctionPtr realfn1param[] = {cabs, carg, creal, cimag};

BigIntVoidFunctionPtr bigfnvoid2param[] = {bigint_max, bigint_min, bigint_add, bigint_sub, bigint_mul, bigint_div, bigint_rem};

BigIntIntFunctionPtr bigfnint2param[] = {bigint_gt, bigint_lt, bigint_gte, bigint_lte, bigint_eq,
									bigint_neq};

ComplexFunction2ParamPtr mathfn2param[] = {cpow, catan2, cmax, cmin, add, sub,
									cmul, cdiv, mod, gt, lt, gte, lte, eq, neq, par};

double complex suminternal(double complex running, double complex next) { 
	//printf("suminternal: received running = %g, received next = %g\n", creal(running), creal(next));
	return running + next;
}
double complex sum(double complex summed, double complex sqsummed, double complex rsummed, int n) { 
	return summed;
}
double complex sqsum(double complex summed, double complex sqsummed, double complex rsummed, int n) { 
	return sqsummed;
}
double complex var(double complex summed, double complex sqsummed, double complex rsummed, int n) {
	if (n <= 1) return (double complex) 0.0;
	double complex u = summed/n; //mean
	return (sqsummed/n) - (u*u); //expansion of sum((x - u)**2)
}
double complex sdv(double complex summed, double complex sqsummed, double complex rsummed, int n) {
	if (n <= 1) return (double complex) 0.0;
	double complex u = summed/n; //mean
	double complex v = (sqsummed/(n - 1)) - ((u*u*n)/(n - 1)); //use 1/(n - 1)
	return sqrt(v);
}
double complex mean(double complex summed, double complex sqsummed, double complex rsummed, int n) {
	double complex r = summed/n;
	//printf("mean: entered ------------------- input = %g + %gi, n = %d real r = %g\n", creal(summed), cimag(summed), n, creal(r));
	return r;
}
double complex rsum(double complex summed, double complex sqsummed, double complex rsummed, int n) {
	return rsummed;
}

ComplexFunctionPtrVector mathfnvec1param[] = {sum, sqsum, var, sdv, mean, rsum};
double complex callVectorMath1ParamFunction(int fnindex, double complex summed, double complex sqsummed, double complex rsummed, int n) {
	if (fnindex < 0) return (double complex) 0.0;
	//printf("-----callVectorMath1ParamFunction: called with fnindex = %d summed = (%g, %g) n = %d\n", fnindex, creal(summed), cimag(summed), n);
	double complex r = mathfnvec1param[fnindex](summed, sqsummed, rsummed, n);
	//printf("-----callVectorMath1ParamFunction: called with fnindex = %d summed = (%g, %g) n = %d real return = %g\n", fnindex, creal(summed), cimag(summed), n, creal(r));
	return r;
}

double complex call1ParamMathFunction(int fnindex, double complex input) {
	//printf("call1ParamMathFunction: entered ------------------- input = %g + %gi, fnindex = %d\n", creal(input), cimag(input), fnindex);
	return mathfn1param[fnindex](input);
}

double call1ParamRealFunction(int fnindex, double complex input) {
	return realfn1param[fnindex](input);
}

double complex call2ParamMathFunction(int fnindex, double complex input, double complex second) {
	return mathfn2param[fnindex](input, second);
}

void call2ParamBigIntVoidFunction(int fnindex, bigint_t *x, bigint_t *y, bigint_t *res) {
	bigfnvoid2param[fnindex](x, y, res);
}

int call2ParamBigIntIntFunction(int fnindex, bigint_t *x, bigint_t *y) {
	return bigfnint2param[fnindex](x, y);
}

bool variableVetted(char* var) {
	if (isalpha(var[0]) || var[0] == '_') return true;
	else return false;
}

char* tokenize(char* input, char* output) {
	//printf ("tokenize: START input = %s\n", input);
	//skip leading spaces
	input += strspn(input, " \t\r\n");
	int quoteOrParens = 0;
	//check if the current token is a double-quoted string or a complex or a complex or a print command
	if (*input == '"' || (*input == '?' && input[1] == '"') || (*input == '@' && input[1] == '"'))
		quoteOrParens = 1;
	else if (*input == '(') 
		//a complex number
		quoteOrParens = 2;
	else if ((*input == '[') && (input[1] == '('))
		//a vector with a complex number
		quoteOrParens = 3;
	//printf ("tokenize: quoteOrParens = %d\n", quoteOrParens);

	if (quoteOrParens > 0) {
		int i = 0;
		if (*input == '?') {
			output[i++] = '?';
			input++;
		} else if (*input == '@') {
			output[i++] = '@';
			input++;
		}
		input++; // skip the opening delim
		if (quoteOrParens == 3) {
			input++; // skip 2 chars if [( is found
			output[i++] = '[';
		}
		output[i++] = (quoteOrParens >= 2)? '(':'"';
		//printf ("tokenize: NOW 1 output = %s\n", output);

		//copy characters until the closing double-quote or closing parenthesis or 
		//the end of the input string is found
		while (*input && *input != ((quoteOrParens >= 2)? ')':'"')) {
			output[i++] = *input++;
		}
		//printf ("tokenize: NOW 2 output = %s\n", output);

		//add null-terminator to the output string
		output[i++] = (quoteOrParens >= 2)? ')':'"';
		output[i] = '\0';

		//if a closing double-quote  or closing parenthesis was found, advance to the next character after it
		if (*input == ((quoteOrParens >= 2)? ')':'"')) {
			input++;
		}

		//skip trailing spaces after the double-quoted string
		input += strspn(input, " \t\r\n");
	} else {
		//if the current token is not a double-quoted string, it is a regular word

		//find the length of the word (until the next space or end of the input)
		size_t wordLength = strcspn(input, " \t\r\n");
		//printf ("tokenize: NOT string. wordLength = %lu\n", wordLength);

		//copy the word to the output buffer
		strncpy(output, input, wordLength);
		output[wordLength] = '\0';
		input += wordLength;
		//printf ("tokenize: NOT string. input = %s\n", input);
	}
	//printf ("tokenize: END output = %s\n", output);
	return input;
}

uint8_t conditionalData(size_t execData) {
	//data being currently entered is a conditional if/else
	return (execData & 0x7);
}

uint8_t vectorMatrixData(size_t execData) {
	//data being currently entered is vector or matrix
	return ((execData >> 3) & 0xff);
}

bool makeStrNum(char* input, size_t maxstrlen, uint8_t num){
	//adds a number to the end of the string
	//input has already been sized to input[maxstrlen]
	size_t len = strlen(input);
	if (len == 0) return false;
	if (num > 99) return false;
	if (len > maxstrlen - 3) return false;
	snprintf(&input[len], 3, "%u", num);
	return true;
}

bool fnOrOpVec2Param(Machine* vm, const char* token, int fnindex) {
	//printf ("fnOrOpVec2Param: entered vec 2 param ------------------- last = %s bak = %s\n", vm->coadiutor, vm->bak);
	bool success;
	bool ifCondition, doingIf, conditional;
	ifCondition = doingIf = false;
	size_t execData = UintStackPeek(&vm->execStack);
	//last instruction on exec stack was not an IF or ELSE
	GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)

	if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
		int8_t meta;
		char varName[MAX_VARNAME_LEN];
		ComplexDouble c;
		ComplexDouble numXCount;
		numXCount.real = 0.0;
		uint8_t numTCount = 0;
		meta = peek(&vm->userStack, NULL);
		//T vector
		do {
			//from stack, we can do a vector operation or a pop into a vector
			if (stackIsEmpty(&vm->userStack)) break;
			meta = pop(&vm->userStack, vm->coadiutor);
			if (meta == METANONE) break; //safe
			//printf("-----fnOrOpVec2Param: do-while loop: meta = %d -- %s -- popped data = %s\n", meta, DEBUGMETA[meta], vm->coadiutor);
			success = false;
			if (isComplexNumber(vm->coadiutor)) { //complex
				success = stringToComplex(vm->coadiutor, &c);
			} else if (isRealNumber(vm->coadiutor)) { //real number
				success = stringToDouble(vm->coadiutor, &c.real);
				c.imag = 0.0;
			} else {
				FAILANDRETURN(true, vm->error, "Error: Unexpected string.", NULLFN)
			}
			FAILANDRETURNVAR(!success, vm->error, "Error: Math fn '%s' failed.", fitstr(vm->coadiutor, token, 9))
			numTCount++;
		} while (meta != METASTARTVECTOR); //start of the vector or empty stack

		//X vector from stack or variable memory
		bool vectorFromStack = true;//get vector from stack, not variables
		uint8_t count = 0;
		meta = peek(&vm->userStack, NULL);
		if (meta != METAENDVECTOR) {
			//there's no vector on the stack
			//find out if a vector was recently popped
			//and placed in the acc
			//does a vacc variable exist?
			int variableIndex = findVariable(&vm->ledger, VACC);
			//if no vector variable VACC, return error
			FAILANDRETURN((variableIndex == -1), vm->error, "Error: Expected vector.", NULLFN)
			if (variableIndex >= 0) {
				numXCount = fetchVariableComplexValue(&vm->ledger, VACC);
				FAILANDRETURN((numXCount.real == 0.0), vm->error, "Error: Expected vector. A", NULLFN)
				FAILANDRETURN((numXCount.imag != 0.0), vm->error, "Error: Expected vector. B", NULLFN)
				//if not failed - we get vector from variables
				vectorFromStack = false;
			}
		}
		//X vector -- if it is from the stack, copy it to a variable vector
		do {
			if (vectorFromStack) { //from stack
				//since it's from stack, we can do a vector operation or a pop into a vector
				if (stackIsEmpty(&vm->userStack)) return false;
				meta = pop(&vm->userStack, vm->coadiutor);
				if (meta == METANONE) return false; //safe
				//printf("-----fnOrOpVec2Param: do-while loop: meta = %d -- %s -- popped data = %s\n", meta, DEBUGMETA[meta], vm->coadiutor);
				success = false;
				if (isComplexNumber(vm->coadiutor)) { //complex
					success = stringToComplex(vm->coadiutor, &c);
				} else if (isRealNumber(vm->coadiutor)) { //real number
					success = stringToDouble(vm->coadiutor, &c.real);
					c.imag = 0.0;
				} else {
					FAILANDRETURN(true, vm->error, "Error: Unexpected string.", NULLFN)
				}
				FAILANDRETURNVAR(!success, vm->error, "Error: Math fn '%s' failed.", fitstr(vm->coadiutor, token, 9))
				strcpy(varName, VACC); //vector operation
				makeStrNum(varName, MAX_VARNAME_LEN, count);
				//printf("-----fnOrOpVec2Param: do-while loop: from stack: variable name is %s of value (%g, %g)\n", varName, c.real, c.imag);
				createVariable(&vm->ledger, varName, VARIABLE_TYPE_COMPLEX, c, "");
			} else { //from variable
				//since it's from a variable, we can do a vector operation but not pop into a vector
				if (count == (int)numXCount.real) break;
				strcpy(varName, VACC); //vector operation -- can't have pop
				makeStrNum(varName, MAX_VARNAME_LEN, count);
				c = fetchVariableComplexValue(&vm->ledger, varName);
				//printf("-----fnOrOpVec2Param: do-while loop: from memory: variable name is %s of value (%g, %g)\n", varName, c.real, c.imag);
			}
			count++;
		} while (meta != METASTARTVECTOR); //start of the vector or empty stack
	}

	return true;
}

bool hasDblQuotes(char* input) {
    if (input == NULL)
        return NULL; //bad input 
	//must be trimmed of spaces
    size_t len = strlen(input);
	return (len > 2 && input[0] == '"' && input[len - 1] == '"');
}

char* removeDblQuotes(char* input) {
	//printf ("removeDblQuotes: input is: %s\n", input);
	if (input[0] != '"') return input;
    if (input == NULL)
        return NULL; //bad input 
    size_t len = strlen(input);
    if (len == 2 && input[0] == '"' && input[1] == '"') {
		input[0] = '\0';
    } else if (len > 2 && input[0] == '"' && input[len - 1] == '"') {
        input[len - 1] = '\0'; 
		input++;
    }
	//printf ("removeDblQuotes: at end input is: %s\n", input);
	return input;//single character -- can't reduce
}

bool addDblQuotes(char *input) {
    if (input == NULL) {
        return false;
    }
    size_t len = strlen(input);
    if (len >= 2 && input[0] == '"' && input[len - 1] == '"') {
        return true; 
    }
    memmove(input + 1, input, len + 1);
    input[0] = '"';
    input[len + 1] = '"';
    input[len + 2] = '\0';
	return true;
}

bool fnOrOp2Param(Machine* vm, const char* token, int fnindex) {
	bool ifCondition, doingIf, conditional;
	size_t execData = UintStackPeek(&vm->execStack);
	bool success;
	FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "Error: Unexpected scalar op. H", NULLFN)
	ifCondition = doingIf = false;
	GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)
	//printf ("fnOrOp2Param: 0 ------------------- last = %s bak = %s\n", vm->coadiutor, vm->bak);
	//printf("fnOrOp2Param: ------------------- token = %s\n", token);
	int8_t cmeta = peek(&vm->userStack, vm->coadiutor); //c
	//if current stack has end of vector, call the 2-param vector function
	if (cmeta == METAENDVECTOR) return fnOrOpVec2Param(vm, token, fnindex);
	int8_t meta = peekn(&vm->userStack, vm->bak, 1);  //b
	//printf("fnOrOp2Param --- meta for var a = %d\n", meta);
	if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
		FAILANDRETURN((meta == -1), vm->error, "Error: Stack empty", NULLFN)		
		//printf("fnOrOp2Param --- ifcondition is true or is unconditional. meta for var a = %d\n", meta);
		FAILANDRETURN((meta != METANONE), vm->error, "Error: Unexpected scalar op L.", NULLFN)
		if (fnindex == 0) { //swap
			FAILANDRETURN((meta == -1), vm->error, "Error: stack empty.B", NULLFN)			
			FAILANDRETURN((meta != METANONE), vm->error, "Error: Unexpected scalar op. E2", NULLFN)
			pop(&vm->userStack, NULL);
			pop(&vm->userStack, NULL);
			push(&vm->userStack, vm->coadiutor, cmeta);
			push(&vm->userStack, vm->bak, meta);
			return true;
		}
		//printf("fnOrOp2Param --- meta for var b = %d\n", meta);
		FAILANDRETURN((meta == -1), vm->error, "Error: stack empty.C", NULLFN)		
		FAILANDRETURN((meta != METANONE), vm->error, "Error: Unexpected scalar op A.", NULLFN)
		//printf ("fnOrOp2Param: 1 ------------------- coadiutor = %s bak = %s\n", vm->coadiutor, vm->bak);
		//printf ("fnOrOp2Param: 0 ------------------- got ADD\n");
		double complex vala, valb;
		ComplexDouble c, d;
		success = false;
		if (hasDblQuotes(vm->coadiutor) || hasDblQuotes(vm->bak)) {
			char* coadiutor = removeDblQuotes(vm->coadiutor);
			char* bak = removeDblQuotes(vm->bak);
			if ((isBigIntDecString(coadiutor)) && 
					(isBigIntDecString(bak))) { 
				success = bigint_create_str(coadiutor, &(vm->bigC));
				success = bigint_create_str(bak, &vm->bigB) && success;
				FAILANDRETURN(!success, vm->error, "Error: Bad operand. X2", NULLFN)
				success = (fnindex > 2) && (fnindex < 16);
				FAILANDRETURN(!success, vm->error, "Error: Unsupported bigint fn.", NULLFN)
				if (fnindex > 2) {
					if ((fnindex - 3) < 7) { //void functions -- max, min through rem
						//printf("fnOrOp2Param: X ------------------- bigint - doing DIV\n");
						call2ParamBigIntVoidFunction(fnindex - 3, &vm->bigB, &(vm->bigC), &vm->bigA);
						//if ((&vm->bigA)->length == -1) {
							//printf("fnOrOp2Param: Y ------------------- bigint - DIV by 0!\n");
						//}
						FAILANDRETURN(((&vm->bigA)->length == -1), vm->error, "Error: bigint div by 0.", NULLFN)
						char* acc = &vm->acc[0];
						success = bigint_tostring (&vm->bigA, acc);
						success = addDblQuotes(acc) && success;
						FAILANDRETURN(!success, vm->error, "Error: Unsupported bigint fn.", NULLFN)
					} else { //compare functions, return int
						vm->acc[0] = '0' +  call2ParamBigIntIntFunction(fnindex - 5, &vm->bigC, &vm->bigB);
						vm->acc[1] = '\0';
					}
				}
				pop(&vm->userStack, NULL);
				pop(&vm->userStack, NULL);
				push(&vm->userStack, vm->acc, METANONE);
				return true;
			}
		}
		c.imag = 0;
		if (isComplexNumber(vm->coadiutor)) { //complex
			success = stringToComplex(vm->coadiutor, &c);
			//printf("fnOrOp2Param: returned from stringToComplex ------------------- c = %g + %gi success = %d\n", c.real, c.imag, success);
		} else if (isRealNumber(vm->coadiutor)) { //real number
			char rlc = strIsRLC(vm->coadiutor);
			success = stringToDouble(vm->coadiutor, &c.real);
			if (rlc) {
				//does a frequency variable exist?
				ComplexDouble f = fetchVariableComplexValue(&vm->ledger, "f");
				if (f.real <= 0) f.real = 1;
				if (rlc == 'c'){
					c.imag = -1/(2 * 3.141592653589793 * f.real * c.real);
					c.real = 0;
				} else if (rlc == 'l') {
					c.imag = 2 * 3.141592653589793 * f.real * c.real;
					c.real = 0;
				} //else keep value of c.real
			}
			//printf("fnOrOp2Param: d = %s returned %d\n", vm->coadiutor, success);
		} else {
			FAILANDRETURN(true, vm->error, "Error: Bad operand. A", NULLFN)
		}
		FAILANDRETURN(!success, vm->error, "Error: Bad operand. A2", NULLFN)
		d.imag = 0;
		if (isComplexNumber(vm->bak)) { //looks like complex number '(## ##)'
			success = stringToComplex(vm->bak, &d);
		} else if (isRealNumber(vm->bak)) { //real number
			char rlc = strIsRLC(vm->bak);
			success = stringToDouble(vm->bak, &d.real);
			if (rlc) {
				//does a frequency variable exist?
				ComplexDouble f = fetchVariableComplexValue(&vm->ledger, "f");
				if (f.real <= 0) f.real = 1;
				if (rlc == 'c'){
					d.imag = -1/(2 * 3.141592653589793 * f.real * d.real);
					d.real = 0;
				} else if (rlc == 'l') {
					d.imag = 2 * 3.141592653589793 * f.real * d.real;
					d.real = 0;
				} //else keep value of d.real
			}
			//printf("fnOrOp2Param: d = %s returned %d ", vm->bak, success);
		}  else {
			FAILANDRETURN(true, vm->error, "Error: Bad operand. B", NULLFN)
		}
		FAILANDRETURN(!success, vm->error, "Error: Bad operand. B2", NULLFN)
		vala = c.real + c.imag * I;
		//printf("fnOrOp2Param:------------------- d = %g + %gi\n", d.real, d.imag);
		valb = d.real + d.imag * I;
		//call 2-parameter function
		vala = call2ParamMathFunction(fnindex - 1, valb, vala);
		//printf ("fnOrOp2Param: 2 ------------------- got %s, vala = %g + %gi\n", token, creal(vala), cimag(vala));
		c.real = creal(vala);
		c.imag = cimag(vala);
		//printf("fnOrOp2Param: function returned c.real = %g c.imag = %g abs(c.real) = %g\n", c.real, c.imag, dabs(c.real));
		if (dabs(c.real) < DOUBLE_EPS) c.real = 0.0;
		if (dabs(c.imag) < DOUBLE_EPS) c.imag = 0.0;
		//printf("fnOrOp2Param: rounded c.real = %g c.imag = %g\n", c.real, c.imag);
		success = doubleToString(c.real, vm->coadiutor) && doubleToString(c.imag, vm->coadiutor);
		success = complexToString(c, vm->coadiutor) && success; //result in coadiutor
		//printf ("-------------------success = %d, acc = %s\n", success, vm->coadiutor);
		FAILANDRETURNVAR(!success, vm->error, "Error: Op '%s' had error.", fitstr(vm->coadiutor, token, 8))
		strcpy(vm->bak, vm->acc); //backup
		strcpy(vm->acc, vm->coadiutor);
		//when no errors are present, actually pop the other var
		pop(&vm->userStack, NULL);
		pop(&vm->userStack, NULL);
		push(&vm->userStack, vm->acc, METANONE);
	}
	return true;
}

bool fn1Param(Machine* vm, const char* token, int fnindex) {
	//printf("fn1Param: entered ------------------- token = %s fnindex = %d\n", token, fnindex);
	bool ifCondition, doingIf, conditional;
	size_t execData = UintStackPeek(&vm->execStack);
	FAILANDRETURN((vectorMatrixData(execData) != 0), vm->error, "Error: Unexpected scalar op. A", NULLFN)
	ifCondition = doingIf = false;
	GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)
	//printf("fn1Param:------------------- execData = %lu\n", execData);
	if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
		int8_t meta;
		meta = peek(&vm->userStack, vm->coadiutor);
		FAILANDRETURN((meta != METANONE), vm->error, "Error: Unexpected scalar op. B", NULLFN)
		peek(&vm->userStack, vm->acc);
		if (vm->acc[0] != '\0') {
			ComplexDouble c;
			bool success = false;
			if (isComplexNumber(vm->acc)) { //complex
				success = stringToComplex(vm->acc, &c);
			} else if (isRealNumber(vm->acc)) { //real number
				success = stringToDouble(vm->acc, &c.real);
				c.imag = 0.0;
			}
			FAILANDRETURNVAR(!success, vm->error, "Error: Math fn '%s' failed.", fitstr(vm->coadiutor, token, 8))
			//printf("fn1Param: ---  success = %d\n", success);
			double complex ctemp = c.real + c.imag * I;
			if (fnindex < NUMMATH1PARAMFN) {
				ctemp = call1ParamMathFunction(fnindex, ctemp);
				c.real = creal(ctemp);
				c.imag = cimag(ctemp);
			} else if (fnindex < NUMMATH1PARAMFN + NUMREAL1PARAMFN) {
				c.imag = 0;
				c.real = call1ParamRealFunction(fnindex - NUMMATH1PARAMFN, ctemp);
			}
			//printf("fn1Param: function returned c.real = %g c.imag = %g\n", c.real, c.imag);
			FAILANDRETURNVAR((c.real == INFINITY || c.imag == INFINITY || c.real == -INFINITY || c.imag == -INFINITY), 
				vm->error, "Error: Fn '%s' out of range. A", fitstr(vm->coadiutor, token, 8))
			if (dabs(c.real) < DOUBLEFN_EPS) c.real = 0.0;
			if (dabs(c.imag) < DOUBLEFN_EPS) c.imag = 0.0;
			success = complexToString(c, vm->acc);
			FAILANDRETURNVAR(!success, vm->error, "Error: Math fn '%s' failed. X", fitstr(vm->coadiutor, token, 8))
			pop(&vm->userStack, NULL);
			push(&vm->userStack, vm->acc, METANONE);
		}
	}
	return true;
}

void cleanupVec(Machine* vm, int vecOrMat){
	//vecOrMat = 0 - cleanup till start of previous vector
	//vecOrMat = 1 - cleanup till start of previous matrix
	int8_t meta;
	do {
		if (stackIsEmpty(&vm->userStack)) break;
		meta = pop(&vm->userStack, vm->coadiutor);
		//printf("-----cleanupVec: do-while loop: meta = %d -- %s -- popped data = %s\n", meta, DEBUGMETA[meta], vm->coadiutor);
	} while (meta != ((vecOrMat == 0)? METASTARTVECTOR: METASTARTMATRIX)); //start of the vector or empty stack
	if (vectorMatrixData(UintStackPeek(&vm->execStack)) == ((vecOrMat == 0)? 1: 2)) //use was in the middle of entering a vector
		UintStackPop(&vm->execStack);
}

bool fnOrOpVec1Param(Machine* vm, const char* token, int fnindex, bool isVoidFunction) {
	bool success;
	bool ifCondition, doingIf, conditional;
	ifCondition = doingIf = false;
	size_t execData = UintStackPeek(&vm->execStack);
	//last instruction on exec stack was not an IF or ELSE
	GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)
	//printf("fnOrOpVec1Param:------------------- execData = %lu\n", execData);
	//lcase(token);
	if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
		int8_t meta;
		char varName[MAX_VARNAME_LEN];
		double complex crunningsum = 0;
		double complex crunningsqsum = 0;
		double complex crunningrsum = 0;
		double complex crunning = 0;
		ComplexDouble c;
		ComplexDouble numTCount;
		numTCount.real = 0.0;
		uint8_t count = 0;
		bool vectorFromStack = true;//get vector from stack, not variables
		//printf("fnOrOpVec1Param:------------------- called with token = %s fnindex = %d\n", token, fnindex);
		meta = peek(&vm->userStack, NULL);
		FAILANDRETURN((meta == -1), vm->error, "Error: Stack empty.", NULLFN)
		if (meta == METASTARTVECTOR) {
			//only start of vector, return it as scalar
			pop(&vm->userStack, vm->coadiutor);
			UintStackPop(&vm->execStack); 
			push(&vm->userStack, vm->coadiutor, METANONE);
			return true;
		} else if (meta == METAMIDVECTOR) {
			pop(&vm->userStack, vm->coadiutor);
			push(&vm->userStack, vm->coadiutor, METAENDVECTOR);
			UintStackPop(&vm->execStack); 
		} else if (meta == METANONE) {
			//there's no vector on the stack, just scalar
			//find out if a vector was recently popped
			//and placed in the acc
			//does a vacc variable exist?
			int variableIndex = findVariable(&vm->ledger, VACC);
			//if no vector variable VACC, return error
			FAILANDRETURN((variableIndex == -1), vm->error, "Error: Expected vector.", NULLFN)
			if (variableIndex >= 0) {
				numTCount = fetchVariableComplexValue(&vm->ledger, VACC);
				FAILANDRETURN((numTCount.real == 0.0), vm->error, "Error: Expected vector. A", NULLFN)
				FAILANDRETURN((numTCount.imag != 0.0), vm->error, "Error: Expected vector. B", NULLFN)
				//if not failed - we get vector from variables
				vectorFromStack = false;
			}
		}
		do {
			if (vectorFromStack) { //from stack
				//since it's from stack, we can do a vector operation or a pop into a vector
				if (stackIsEmpty(&vm->userStack)) return false;
				meta = pop(&vm->userStack, vm->coadiutor);
				if (meta == METANONE) return false; //safe
				//printf("-----fnOrOpVec1Param: do-while loop: meta = %d -- %s -- popped data = %s\n", meta, DEBUGMETA[meta], vm->coadiutor);
				success = false;
				if (isComplexNumber(vm->coadiutor)) { //complex
					success = stringToComplex(vm->coadiutor, &c);
				} else if (isRealNumber(vm->coadiutor)) { //real number
					success = stringToDouble(vm->coadiutor, &c.real);
					c.imag = 0.0;
				} else {
					FAILANDRETURN(true, vm->error, "Error: Unexpected string.", NULLFN)
				}
				FAILANDRETURNVAR(!success, vm->error, "Error: Math fn '%s' failed.", fitstr(vm->coadiutor, token, 9))
				if (fnindex != -1 || token == NULL) strcpy(varName, VACC); //vector operation
				else strncpy(varName, token, MAX_VARNAME_LEN); //vector pop
				makeStrNum(varName, MAX_VARNAME_LEN, count);
				//printf("-----fnOrOpVec1Param: do-while loop: from stack: variable name is %s of value (%g, %g)\n", varName, c.real, c.imag);
				createVariable(&vm->ledger, varName, VARIABLE_TYPE_COMPLEX, c, "");
			} else { //from variable
				//since it's from a variable, we can do a vector operation but not pop into a vector
				if (count == (int)numTCount.real) break;
				strcpy(varName, VACC); //vector operation -- can't have pop
				makeStrNum(varName, MAX_VARNAME_LEN, count);
				c = fetchVariableComplexValue(&vm->ledger, varName);
				//printf("-----fnOrOpVec1Param: do-while loop: from memory: variable name is %s of value (%g, %g)\n", varName, c.real, c.imag);
			}
			crunningsum = suminternal(crunningsum, c.real + c.imag*I);
			crunningsqsum = suminternal(crunningsqsum, cmul(c.real + c.imag*I, c.real + c.imag*I));
			crunningrsum = suminternal(crunningrsum, cdiv(1.0 + 0.0*I, c.real + c.imag*I));
			count++;
		} while (meta != METASTARTVECTOR); //start of the vector or empty stack

		if (fnindex != -1 || token == NULL) strcpy(varName, VACC); //vector operation
		else strncpy(varName, token, MAX_VARNAME_LEN); //vector pop
		if (vectorFromStack)
			createVariable(&vm->ledger, varName, VARIABLE_TYPE_VECMAT, MKCPLX(count), ""); //the meta vector var - num of elements
		if (!isVoidFunction) {
			//not vector pop
			crunning = callVectorMath1ParamFunction(fnindex, crunningsum, crunningsqsum, crunningrsum, count);
			c.real = creal(crunning);
			c.imag = cimag(crunning);
			success = complexToString(c, vm->coadiutor);
			FAILANDRETURNVAR(!success, vm->error, "Error: Math fn '%s' failed.", fitstr(vm->coadiutor, token, 9))
			strcpy(vm->bak, vm->acc); //backup
			strcpy(vm->acc, vm->coadiutor);
			push(&vm->userStack, vm->acc, METANONE);
		}
	}
	//printf("fnOrOpVec1Param:------------------- returning\n");
	return true;
}

bool isOpTerminalChar(char c) {
	const char* allTerminalOpChars = "+-/*><=.";
	for (int i = 0; i < 8; i++)
		if (c == allTerminalOpChars[i]) return true;
	return false;
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

int isVec1ParamFunction(const char* token) {
	for (int i = 0; i < NUMVECFNS; i++) {
		if (strcmp(vecfn1paramname[i], token) == 0) return i;
	}
	return -1;
}

bool processPrint(Machine* vm, char* token) {
	//printf("------------------- PRINT - called with token = %s\n", token);
	
	//FAILANDRETURN((token[0] == '\0'), vm->error, "Error: illegal PRINT", NULLFN)
	if (strcmp(token, "va") == 0) {
		//FIXME
	} else if (strcmp(token, "b") == 0) {
		//printf("------------------- PRINT - found ?bak\n");
		printf("\tbak = %s\n", vm->bak);
	} else if (strcmp(token, "a") == 0) {
		//printf("------------------- PRINT - found ?acc\n");
		printf("\tacc = %s\n", vm->acc);
	} else if (token[0] == '"' && token[strlen(token)-1] == '"') {
		//immediate string
		token = removeDblQuotes(token);
		printf("\t%s\n", token);
	} else if (token[0] == '\0' || variableVetted(token)) {
		if (token[0] == '\0') {
			int8_t meta;
			//POP var -- pop the top of stack into variable 'var'
			meta = pop(&vm->userStack, vm->bak);
			FAILANDRETURN((meta == -1), vm->error, "Error: Stack empty", NULLFN)		
		} else  {
			strcpy(vm->bak, token);
		}
		int vt = findVariable(&vm->ledger, vm->bak);
		if (vt == VARIABLE_TYPE_COMPLEX || vt == VARIABLE_TYPE_VECMAT) {
			ComplexDouble c = fetchVariableComplexValue(&vm->ledger, vm->bak);
			if (complexToString(c, vm->coadiutor)) { 
				printf("\t%s = %s\n", vm->bak, vm->coadiutor);
			}
		} else if (vt == VARIABLE_TYPE_STRING) {
			if (getVariableStringValue(&vm->ledger, token, vm->coadiutor)) { //returns true
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

bool processVectorPop(Machine* vm) {
	//printf("processVectorPop:------------------- got no token calling fnOrOpVec1Param\n");
	fnOrOpVec1Param(vm, NULL, -1, true);
	return true;
}

bool processMatrixPop(Machine* vm) {
	return true;
}

bool pop1VecMat(Machine* vm, int8_t meta) {
	//printf("pop1VecMat: just entered\n");
	bool success = true;
	//make a proper vector/matrix end and pop
	//the entire vector/matrix
	if (meta == METASTARTVECTOR || meta == METASTARTMATRIX) {
		//printf("meta == METASTARTVECTOR\n");
		//treat as a regular (scalar) pop
		pop(&vm->userStack, vm->coadiutor);
		//if the popped out data is start of vector or matrix pop out execStack
		//so that we leave no trace of the vector or matrix
		if (vectorMatrixData(UintStackPeek(&vm->execStack)) != 0) 
			UintStackPop(&vm->execStack); 
	} else if (meta == METAMIDMATRIX) {
		pop(&vm->userStack, vm->coadiutor); //pop out 
		push(&vm->userStack, vm->coadiutor, METAENDMATRIX);//mark end of matrix
		success = processMatrixPop(vm);
		if (vectorMatrixData(UintStackPeek(&vm->execStack)) != 0) 
			UintStackPop(&vm->execStack); 
	} else if (meta == METAMIDVECTOR) {
		pop(&vm->userStack, vm->coadiutor); //pop out 
		push(&vm->userStack, vm->coadiutor, METAENDVECTOR);//mark end of vector
		success = processVectorPop(vm);
		if (vectorMatrixData(UintStackPeek(&vm->execStack)) != 0) 
			UintStackPop(&vm->execStack); 
	} else if (meta == METAENDMATRIX) { //current data is end of a matrix
		success = processMatrixPop(vm);
	} else if (meta == METAENDVECTOR) { //current data is end of a vector
		success = processVectorPop(vm);
	}
	return success;
}

bool popNItems(Machine* vm, int maxCount) {
	int8_t meta;
	int count = 0;
	if (maxCount == 0) {
		if (vectorMatrixData(UintStackPeek(&vm->execStack)) != 0) 
			UintStackPop(&vm->execStack); 
		//execStack	does not have any more unfinished vector
		//or matrix elements, but it might not be empty
		//should execStack be completely emptied? 
		//NOTE:
		stackInit(&vm->userStack);
		return true;
	}
	while (!stackIsEmpty(&vm->userStack) && count < maxCount) {
		//if count is 0, clean up the stack
		meta = peek(&vm->userStack, NULL);
		if (meta != METANONE) pop1VecMat(vm, meta);
		else pop(&vm->userStack, NULL);
		count++;
	}
	return true;
}

bool processPop(Machine* vm, char* token) {
	int8_t meta;
	//printf("processPop: entered with token %s\n", token);
	//check if the user was in the middle of entering a vector
	//or matrix
	meta = peek(&vm->userStack, NULL);
	FAILANDRETURN((meta == -1), vm->error, "Error: Stack empty", NULLFN)		
	double dbl;
	if (token[0] == '\0') {
		//@ will pop 1 element of a vector/matrix
		//@1 will pop 1 entire vector/matrix
		strcpy(vm->bak, vm->acc); //save acc
		pop(&vm->userStack, vm->acc);
		//if (meta == METANONE) {
		//	//just pop out a scalar in acc
		//	strcpy(vm->bak, vm->acc); //save acc
		//	pop(&vm->userStack, vm->acc);
		//} else {
		//	//printf("processPop: calling pop1VecMat\n");
		//	pop1VecMat(vm, meta);
		//}
	} else if (strcmp(token, "b") == 0) {
		strcpy(vm->coadiutor, vm->bak); //save bak
		pop(&vm->userStack, vm->bak);
	} else if (variableVetted(token)) {
		//printf("processPop: variableVetted is true for token %s\n", token);
		ComplexDouble c;
		//POP var -- pop the top of stack into variable 'var'
		meta = pop(&vm->userStack, vm->bak);
		//createVariable will overwrite the variable if  it already exists
		if (stringToComplex(vm->bak, &c)) {
			//printf("processPop:------ calling createVar complex with varname = %s and value = %s\n", token, vm->bak);
			createVariable(&vm->ledger, token, VARIABLE_TYPE_COMPLEX, c, "");
		} else if (stringToDouble(vm->bak, &dbl)) {
			//printf("processPop:------ calling createVar real with varname = %s and value = %s\n", token, vm->bak);
			createVariable(&vm->ledger, token, VARIABLE_TYPE_COMPLEX, MKCPLX(dbl), "");
		} else {
			//printf("processPop:------ calling createVar string with varname = %s and value = %s\n", token, vm->bak);
			createVariable(&vm->ledger, token, VARIABLE_TYPE_STRING, MKCPLX(0), vm->bak);
		}
	} else { 
		//a pop but the var does not have a variable name
		//or is a number -- number means pop n entities out of stack
		int howMany = -1;
		if (POSTFIX_BEHAVE) { //@<number> is the postfix behavior -- we want <number> @@
			if (stringToDouble(token, &dbl)) howMany = (int) dbl;
		} else if (strcmp(token, "@") == 0) {
			pop(&vm->userStack, vm->bak);
			if (stringToDouble(vm->bak, &dbl)) howMany = (int) dbl;
		}
		if (howMany >= 0) popNItems(vm, howMany);
		else FAILANDRETURN(true, vm->error, "Error: illegal POP B", NULLFN)
	}
	return true;
}

bool processDefaultPush(Machine* vm, char* token, int8_t meta) {
	//printf("processDefaultPush: entered -------------------Default condition with token = %s and meta = %s\n", token, DEBUGMETA[meta]);
	//handle shorthands
	ComplexDouble c;
	char varName[MAX_VARNAME_LEN];
	if (strlen(token) == 1) {
		switch (token[0]) {
			case 'T': strcpy(token, "T0"); break;
			case 'X': strcpy(token, "T1"); break;
			case 'Y': strcpy(token, "T2"); break;
			case 'Z': strcpy(token, "T3"); break;
			case 'W': strcpy(token, "T4"); break;
			case 'V': strcpy(token, "T5"); break;
		}
	} 
	if (strlen(token) == 2) {
		//handle T0 - T9
		if (token[0] == 'T' && token[1] <= '9' && token[1] >= '0') {
			int n = (int)(token[1] - '0');
			int8_t localmeta = peekn(&vm->userStack, vm->coadiutor, n);
			//printf("processDefaultPush:------------------- token = %s 'token[1]' = %d '0' = %d found T for %d = %s\n", token, token[1], '0', n, vm->coadiutor);
			FAILANDRETURN((localmeta == -1), vm->error, "Error: Too deep.", NULLFN)
			push(&vm->userStack, vm->coadiutor, METANONE);
			return true;
		}
	} else if (strlen(token) == 3) {
		//handle T10 - T99
		if (token[0] == 'T' && token[1] <= '9' && token[1] > '0' && 
				token[2] <= '9' && token[2] >= '0') {
			int n = (int)((token[1] - '0') * 10 + (token[2] - '0'));
			int8_t localmeta = peekn(&vm->userStack, vm->coadiutor, n);
			FAILANDRETURN((localmeta == -1), vm->error, "Error: Too deep.", NULLFN)
			push(&vm->userStack, vm->coadiutor, METANONE);
			return true;
		}
	}
	if (strcmp(token, "b") == 0) {
		push(&vm->userStack, vm->bak, meta);
	} else if (strcmp(token, "a") == 0) {
		push(&vm->userStack, vm->acc, meta);
		//printf("-------------------Default condition - found acc\n");
	} else if (variableVetted(token) || (strcmp(token, "va") == 0)) {
		if (strcmp(token, "va") == 0) strcpy(token, VACC);;
		if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_VECMAT) {
			c = fetchVariableComplexValue(&vm->ledger, token);
			//printf("-------------------Default condition - found token got c.real = %g\n", c.real);
			int count = (int) c.real;
			for (int i = (int) count - 1; i >= 0; i--) {
				int8_t localmeta;
				strcpy(varName, token);
				makeStrNum(varName, MAX_VARNAME_LEN, i);
				c = fetchVariableComplexValue(&vm->ledger, varName);
				if (i == (int) count - 1) localmeta = METASTARTVECTOR;
				else if (i == 0) localmeta = METAENDVECTOR;
				else localmeta = METAMIDVECTOR;
				//printf("-------------------Default condition - found vacc i = %d got c.real = %g\n", i, c.real);
				if (complexToString(c, vm->coadiutor)) {
					push(&vm->userStack, vm->coadiutor, localmeta);
				}
			}
		} else if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_COMPLEX) {
			c = fetchVariableComplexValue(&vm->ledger, token);
			if (complexToString(c, vm->coadiutor)) {
				push(&vm->userStack, vm->coadiutor, meta);
			}
		} else if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_STRING) {
			if (getVariableStringValue(&vm->ledger, token, vm->coadiutor)) {
				push(&vm->userStack, vm->coadiutor, meta);
			}
		} else {
			FAILANDRETURNVAR(true, vm->error, "Error: no variable '%s'.", fitstr(vm->coadiutor, token, 10))
		}
	} else {
		//printf("processDefaultPush:------------------- 0. ELSE Default condition with token = %s\n", token);
		double dbl;
		char rlc = strIsRLC(token);
		bool s2d = stringToDouble(token, &dbl);
		if (stringToComplex(token, &c) || s2d) {
			//printf("processDefaultPush:------------------- 1. ELSE Default condition with token = %s, no-has-dbl-quotes = %d, dbl = %g\n", token, !hasDblQuotes(token), dbl);
			//printf("processDefaultPush:------------------- 1. ELSE Default condition with len token = %lu\n", strlen(token));
			if (s2d) {
				//token is an RLC type double -- 1.234e3l
				doubleToString(dbl, vm->coadiutor);
				char RLCstr[2];
				RLCstr[1] = '\0';
				if (rlc) {
					RLCstr[0] = rlc;
					//printf("processDefaultPush: RLCstr = %s\n", RLCstr);
					strcat(vm->coadiutor, RLCstr);
				}
				//printf("processDefaultPush: len coad = %lu\n", strlen(vm->coadiutor));
				push(&vm->userStack, vm->coadiutor, meta);
			} else {
				push(&vm->userStack, token, meta);
			}
		} else if (!hasDblQuotes(token) && meta != METASTARTVECTOR) {
			//don't want to add double quotes to '['
			//printf("processDefaultPush:------------------- 2. ELSE Default condition with token = %s, no-has-dbl-quotes = %d\n", token, !hasDblQuotes(token));
			addDblQuotes(token);
			push(&vm->userStack, token, meta);
		} else {
			push(&vm->userStack, token, meta);
		}
	}
	return true;
}

bool process(Machine* vm, char* token, int branchIndex) {
	bool success = true;
	bool ifCondition, doingIf, conditional;
	ifCondition = doingIf = conditional = false;
	size_t execData = UintStackPeek(&vm->execStack);

	GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)
	//printf("process:------------------- token = %s\n", token);
	//printf("process:------------------- UintStackPeek = %lu\n", execData);
	int is1pfn = is1ParamFunction(token);
	int is2pfn = is2ParamFunction(token);
	int isvecfn = isVec1ParamFunction(token);
	if (is1pfn != -1) {
		//printf("process:------------------- is1pfn = %d\n", is1pfn);
		//lcase(token);
		success = fn1Param(vm, token, is1pfn);
	} else if (is2pfn != -1) {
		//printf("process:------------------- is2pfn|op = %d\n", is2pfn);
		//lcase(token);
		success = fnOrOp2Param(vm, token, is2pfn); //don't pass is2pop
	} else if (isvecfn != -1) {
		//lcase(token);
		success = fnOrOpVec1Param(vm, token, isvecfn, false);
	} else if (strcmp(token, "dup") == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "Error: No vector/matrix here. C", NULLFN)
		int8_t meta = peek(&vm->userStack, vm->coadiutor);
		FAILANDRETURN((meta != 0), vm->error, "Error: Unexpected scalar op. D", NULLFN)
		if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
			push(&vm->userStack, vm->coadiutor, METANONE);
			return true;
		}
	} else if (strcmp(token, "do") == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "Error: No vector/matrix here. F", NULLFN)
		//conditional loop, check for previous if/else
		UintStackPush(&vm->execStack, (1<<(sizeof(size_t)-1)));
		return true;
	} else if (strcmp(token, IFTOKEN) == 0) {
		//printf ("-------------------Found if -- now popping vm->userStack\n");
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "Error: No vector/matrix here. G", NULLFN)
		strcpy(vm->bak, vm->acc); //backup
		pop(&vm->userStack, vm->acc);
		ComplexDouble conditionComplex;
		success = false;
		if (isComplexNumber(vm->acc)) { //complex
			success = stringToComplex(vm->acc, &conditionComplex);
			ifCondition = !alm0(conditionComplex.real) || !alm0(conditionComplex.imag);
		} else if (isRealNumber(vm->acc)) { //real number
			success = stringToDouble(vm->acc, &conditionComplex.real);
			ifCondition = !alm0(conditionComplex.real);
			//printf("-------------------Found if: conditionComplex.real = %.15g\n", conditionComplex.real);
		} else { //string
			ifCondition = false;
		}
		//printf("-------------------Found if: ifCondition = %d\n", ifCondition);
		execData = makeExecStackData(true, ifCondition, true, false, false); //doingIf = true
		//printf("-------------------Found if: execData calculated = %lu\n", execData);
		UintStackPush(&vm->execStack, execData);
		return true;
	} else if (strcmp(token, ELSETOKEN) == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "Error: No vector/matrix here. H", NULLFN)
		//printf ("-------------------Found else -- execData = %lu\n", execData);
		FAILANDRETURN((conditionalData(execData) == 0x0), vm->error, "Error: else without if. A", NULLFN)
		FAILANDRETURN(!doingIf, vm->error, "Error: else without if.", NULLFN)
		FAILANDRETURN(UintStackIsEmpty(&vm->execStack), vm->error, "Error: else without if. B", NULLFN)
		UintStackPop(&vm->execStack); //discard the if
		execData = makeExecStackData(true, ifCondition, false, false, false); //else
		//printf ("-------------------Found else -- set execData to = %lu\n", execData);
		UintStackPush(&vm->execStack, execData);
		return true;
	} else if (strcmp(token, ENDIFTOKEN) == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "Error: No vector/matrix here. I", NULLFN)
		FAILANDRETURN((conditionalData(execData) == 0x0), vm->error, "Error: fi without if. A", NULLFN)
		FAILANDRETURN(UintStackIsEmpty(&vm->execStack), vm->error, "Error: fi without if. B", NULLFN)
		UintStackPop(&vm->execStack); //discard the if/else
		return true;
	} else {
		uint8_t meta = peek(&vm->userStack, vm->coadiutor);
		FAILANDRETURN((strcmp(token, MATSTARTTOKEN) == 0 || 
				strcmp(token, MATLASTTOKEN) == 0), vm->error, 
				"Error: Dangling '['s.", NULLFN)
		//printf("-------------------Default condition with token = %s\n", token);
		bool doingMat = vectorMatrixData(execData) == 0x2;
		bool doingVec = vectorMatrixData(execData) == 0x1;
		//printf("-------------------Default -- execData >> 3 = %lu\n", execData >> 3);
		//last instruction on exec stack was not an IF or ELSE

		//printf("-------------------Default: conditional = %d \n", conditional);
		if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
			int len_m_1 = strlen(token) - 1;
			bool token_0_eq_VECSTARTTOKENC = (token[0] == VECSTARTTOKENC);
			bool token_1_eq_VECSTARTTOKENC = (token[1] == VECSTARTTOKENC);
			bool token_l_eq_VECLASTTOKENC = (token[len_m_1] == VECLASTTOKENC);
			bool token_l_m_1_eq_VECLASTTOKENC = (token[len_m_1 - 1] == VECLASTTOKENC);
			if (token_l_eq_VECLASTTOKENC) { //vector or matrix ending 
				if (token_l_m_1_eq_VECLASTTOKENC) { //matrix ending
					if (len_m_1 < 2) return false; //too short
					token[len_m_1 - 1] = '\0';
					if (token_0_eq_VECSTARTTOKENC && token_1_eq_VECSTARTTOKENC){ //both matrix starting and ending
						//[[aa]]
						success = processDefaultPush(vm, &token[2], METANONE);
					} else if (token_0_eq_VECSTARTTOKENC) {
						//[aa]]
						FAILANDRETURN(!doingMat, vm->error, "Error: Expecting MAT end. A", NULLFN)
						success = processDefaultPush(vm, &token[1], METAENDMATRIX);
					} else {
						//aa]]
						//printf ("-------------------Default -- aa]] Current execData >> 3 = %lu\n", execData >> 3);
						FAILANDRETURN(!doingMat, vm->error, "Error: Expecting MAT end. B", NULLFN)
						//matrix is done
						execData = UintStackPop(&vm->execStack);
						success = processDefaultPush(vm, token, METAENDMATRIX);
					}
				} else if (token_0_eq_VECSTARTTOKENC){ //vector starting and ending
					//[aa]
					if (len_m_1 < 2) return false; //too short
					FAILANDRETURN((len_m_1 < 2), vm->error, "Error: Syntax error. A", NULLFN)
					token[len_m_1] = '\0';
					success = processDefaultPush(vm, &token[1], METANONE);
				} else { //vector ending
					//a] or ]
					if (len_m_1 == 0) { //just a ']'
						FAILANDRETURN(!doingMat && !doingVec, vm->error, "Error: Unexpected VEC end.", NULLFN)
						pop(&vm->userStack, vm->coadiutor);
						if (meta == METAMIDVECTOR) {
							push(&vm->userStack, vm->coadiutor, METAENDVECTOR);
						} else if (meta == METASTARTVECTOR) { //[2] becomes 2
							if (vm->coadiutor[0] != VECSTARTINDICATOR)
								push(&vm->userStack, vm->coadiutor, METANONE);
						} else if (meta == METAMIDMATRIX) {
							push(&vm->userStack, vm->coadiutor, METAENDMATRIX);
						}
					} else { //a]
						//if the last was a vector ending, we append the new ending
						//by making the previous one a mid-vector
						if (meta == METAENDVECTOR) {
							pop(&vm->userStack, vm->coadiutor);
							push(&vm->userStack, vm->coadiutor, METAMIDVECTOR);
						}
						token[len_m_1] = '\0'; //remove the ']' character
						success = processDefaultPush(vm, token, METAENDVECTOR);
					}
					if (doingVec) { //vector was going on, not matrix
						//vector is done
						execData = UintStackPop(&vm->execStack);
					}
				}
			} else if (token_0_eq_VECSTARTTOKENC) { //vector or matrix starting 
				if (len_m_1 == 0) {
					//[
					FAILANDRETURN(doingMat, vm->error, "Error: No nested matrices! A", NULLFN)
					if (!doingVec) { //already vector entry is NOT going on; another '[' starts a matrix entry
						execData = makeExecStackData(false, false, false, false, true); //vector
						UintStackPush(&vm->execStack, execData);
						token[0] = VECSTARTINDICATOR; //ASCII SYNC
						token[1] = '\0';
						success = processDefaultPush(vm, token, METASTARTVECTOR);
					} else {
						//already a vector going on, so this will be start of matrix entry
						//if TOS of userStack has VECSTARTINDICATOR, then ok, else error
						printf("#### UNSUPPORTED #### UNSUPPORTED ####\n");
					}
				} else if (token_1_eq_VECSTARTTOKENC) {//matrix starting
					//[[aa
					FAILANDRETURN(doingMat, vm->error, "Error: No nested matrices! B", NULLFN)
					execData = makeExecStackData(false, false, false, true, false); //matrix
					UintStackPush(&vm->execStack, execData);
					success = processDefaultPush(vm, &token[2], METASTARTMATRIX);
				} else { //vector starting
					//[a
					//printf ("-------------------Default -- [a Current execData >> 3 = %lu\n", execData >> 3);
					FAILANDRETURN(doingVec, vm->error, "Error: No nested vectors! C", NULLFN)
					//in above check for previous vector being the ASCII SYNC char
					FAILANDRETURN(doingMat, vm->error, "Error: No nested matrices! C", NULLFN)
					if (!doingVec) { //last '[' entry means another '[' starts a matrix entry
						execData = makeExecStackData(false, false, false, false, true); //vector
						UintStackPush(&vm->execStack, execData);
					}
					success = processDefaultPush(vm, &token[1], METASTARTVECTOR);
				}
			} else if (token[0] == POPTOKENC) { //pop to register or variable
				//printf("------------------- Got POPTOKEN calling processPop with %s\n", &token[1]);
				success = processPop(vm, &token[1]);
			} else if (token[0] == PRINTTOKENC) { //print register or variable
				success = processPrint(vm, &token[1]);
			} else {
				uint32_t tlen = strlen(token);
				if (token[tlen - 1] == PRINTTOKENC) { //pop to register or variable
					//postfix version of pop
					strncpy(vm->coadiutor, token, tlen - 1);
					vm->coadiutor[tlen - 1] = '\0';
					//printf("------------------- Got PRINTTOKEN for postfix version of pop; calling processPop with %s\n", vm->coadiutor);
					success = processPrint(vm, vm->coadiutor);
				} else if (token[tlen - 1] == POPTOKENC) { //pop to register or variable
					//postfix version of pop
					strncpy(vm->coadiutor, token, tlen - 1);
					vm->coadiutor[tlen - 1] = '\0';
					//printf("------------------- Got POPTOKEN for postfix version of pop; calling processPop with %s\n", vm->coadiutor);
					success = processPop(vm, vm->coadiutor);
				} else if (doingVec) {
					//printf("-------------------Default condition - push token = %s which is char %d\n", token, (int)token[0]);
					//printf("-------------------STACKPUSH: execData  = %lu\n", execData);	
					if (meta == METASTARTVECTOR && (vm->coadiutor[0] == VECSTARTINDICATOR)) {
						pop(&vm->userStack, vm->coadiutor);
						//pop out the placeholder and push the current token, marking as vector start
						success = processDefaultPush(vm, token, METASTARTVECTOR);
					} else {
						success = processDefaultPush(vm, token, METAMIDVECTOR);
					}
				} else if (doingMat) //a matrix end is also a vector end
					success = processDefaultPush(vm, token, METAMIDMATRIX); //mid matrix is also mix vector
				else if (token[0] != '\0') {
					success = processDefaultPush(vm, token, METANONE);
				}
			}
		}
	}
	return success;
}

void printRegisters(Machine* vm) {
	printf("\tERR = %s\n", (vm->error[0] != '\0')? vm->error: "\"\"");
}

void interpret(Machine* vm, char* sourceCode) {
	char* input;
	char output[STRING_SIZE];
	size_t branchIndex = 0;
	bool success;
	//printf("-------------------START: Source Code = %s\n", sourceCode);
	input = sourceCode;
	do {
		strcpy(vm->error, "No error.");
		//printf ("main: do-while before tokenize input = %p\n", input);
		//printf ("main: do-while before tokenize input = %s\n", input);
		input = tokenize(input, output);
		//printf ("main: do-while after tokenize input = %p\n", input);
		//printf ("main: do-while after tokenize input = %s\n", input);
		if (output[0] == '\0') break;
		success = process(vm, output, branchIndex);
		branchIndex = input - vm->PROGMEM;
		//printf("-------------------while: output = %s next input = %s input[0] = %d\n", output, input, input[0]);
		if ((strcmp(vm->error,"No error.") != 0) || !success) break;
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
	strcpy(vm->error, "No error.");
}

char* ltrim(char* str) {
	if (str[0] == '\0') return str;
	return str + strspn(str, " \t\n\r\f\v");
	//while (isspace(*str)) str++;
	//return str;
}

void showScreen(Machine* vm, char* line) {
	printf("===================================\n");
	printStack(&vm->userStack, 3, true);
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
		} else if (!strncmp(ltrim(line),":cv", 3)) {
			cleanupVec(&vm, 0);
			showScreen(&vm, line);
		} else if (line[0] == ':') {
			printf("Unrecognized command: %s\n", line);
		} else if (line[0] != '\0') {
			//printf("echo: '%s'\n", line);
			int l = strlen(line);
			if (l > 128) l = 128;
			strncpy(&command[0], line, l);
			command[l] = '\0';
			interpret(&vm, command);
			showScreen(&vm, line);
		}
		free(line);
	}
	printf("Goodbye!\n");
	return 0;
}

