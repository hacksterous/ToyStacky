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

#define FAILANDRETURN(failcondition,dest,src,fnptr)	\
	if (failcondition) {							\
		sprintf(dest, src);							\
		if (fnptr != NULL) fnptr();					\
		SerialPrint(3, "Error: ", dest, "\r\n");	\
		return false;								\
	}												

#define FAILANDRETURNVAR(failcondition,dest,src,var)	\
	if (failcondition) {								\
		sprintf(dest, src, var);						\
		SerialPrint(3, "Error: ", dest, "\r\n");		\
		return false;									\
	}													


#define GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)	\
	conditional = ((execData >> 2) & 0x1);								\
	if (conditional) {													\
		ifCondition = (execData >> 1) & 0x1;							\
		doingIf = execData & 0x1;										\
	}																

const char* VACC = "#vacc";
const char* DEBUGMETA[7] = {"METANONE",
	"METASTARTVECTOR",
	"METAMIDVECTOR",
	"METAENDVECTOR",
	"METASTARTMATRIX",
	"METAMIDMATRIX",
	"METAENDMATRIX"};

const double __TS_PI__ = 3.141592653589793;
void (*NULLFN)(void) = NULL;

ComplexDouble makeComplex(double re, double im) {
	ComplexDouble ret;
	ret.real = re;
	ret.imag = im;
	return ret;
}

Machine vm;

#define __DEBUG_TOYSTACKY__
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
uint32_t rnd_whitened(void){
    int k, random=0;
    int random_bit1, random_bit2;
    volatile uint32_t *rnd_reg=(uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);
    
    for(k = 0; k < 32; k++){
        while(1) {
            random_bit1 = 0x00000001 & (*rnd_reg);
            random_bit2 = 0x00000001 & (*rnd_reg);
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
	for (uint32_t i = 0;  i < strlen(key); i++) {
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

	//SerialPrint(1, "createVariable: just entered ledger->memoryOffset is %lu; creating variable %s", ledger->memoryOffset, name);
	if (ledger->varCount >= MAX_VARIABLES) {
		//SerialPrint(1, "createVariable: just entered FAIL ledger->varCount >= MAX_VARIABLES");
		return NULL;
	}
	size_t stringLength = strlen(stringValue);
	
	if ((stringLength == 0) && (type == VARIABLE_TYPE_STRING)) return NULL;
	if (stringLength > 0) stringLength++;

	if (ledger->memoryOffset + stringLength > MEMORY_SIZE) {
		//SerialPrint(1, "createVariable: just entered FAIL ledger->memoryOffset + stringLength > MEMORY_SIZE");
		return NULL;
	}
	uint32_t index = hash(name, MAX_VARIABLES);
	//SerialPrint(1, "createVariable: just entered ledger->varCount is %lu; creating variable %s with type %d", ledger->varCount, name, (int) type);
	Variable* variable = &ledger->variables[index];

	strncpy(variable->name, name, sizeof(variable->name) - 1);
	variable->name[sizeof(variable->name) - 1] = '\0';
	variable->type = type;

	if (type == VARIABLE_TYPE_STRING) {
		variable->value.stringValueIndex = ledger->memoryOffset;
		//SerialPrint(1, "createVariable: stringValueIndex is set to %X for variable %s", (unsigned int)variable->value.stringValueIndex, name);
		strncpy(ledger->memory + ledger->memoryOffset, stringValue, stringLength);
		ledger->memoryOffset += stringLength;
	} else if (type == VARIABLE_TYPE_COMPLEX || type == VARIABLE_TYPE_VECMAT) {
		//SerialPrint(1, "createVariable: doubleValue.real is set to %g for variable %s", doubleValue.real, name);
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
		//SerialPrint(1, "fetchVariableComplexValue: returning with value.real = %g for var '%s'", variable->value.doubleValue.real, name);
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
	for (uint32_t i = 0; i < ledger->varCount; i++) {
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
	//SerialPrint(1, "findVariable: -- looking for variable %s", name);
	if (strlen(name) > MAX_VARNAME_LEN - 1) 
		return -1;
	uint32_t index = hash(name, MAX_VARIABLES);
	Variable* variable = &ledger->variables[index];
	//if (variable->name[0] == '\0') return -1;
	if (strcmp(variable->name, name) == 0) {
		//SerialPrint(1, "findVariable: -- variable %s returning with %d", name, (int) variable->type);
		return (int) variable->type;
	}
	//SerialPrint(1, "findVariable: -- did NOT find variable %s", name);
	return -1;
}

bool updateLedger(Ledger* ledger, size_t memIndexThreshold, int memIndexIncr) {
	if (ledger->varCount == 0) return true; //nothing to update
	uint32_t varCount = 0;
	uint32_t i = 0;
	while (i < MAX_VARIABLES) {
		Variable* variable = &ledger->variables[i++];
		//SerialPrint(1, "updateLedg===================== varCount = %d", varCount);
		if (varCount >= ledger->varCount) break;
		if (variable->name[0] == '\0') continue; //skip the empty variable pods
		if (variable->type == VARIABLE_TYPE_STRING) {
			if (variable->value.stringValueIndex > memIndexThreshold) {
				if (variable->value.stringValueIndex + memIndexIncr > MEMORY_SIZE) return false;
				if (int32_t(variable->value.stringValueIndex) + memIndexIncr < 0) return false;
				variable->value.stringValueIndex += memIndexIncr;
				//SerialPrint(1, "updateLedgVariable %s at vartable index %d: updated memoryIndex to %s", variable->name, i, ledger->memory + variable->value.stringValueIndex);
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
		//SerialPrint(1, "updateStringVariable: additionalBytes required = %lu", additionalBytes);
		if (ledger->memoryOffset + additionalBytes + 1 > MEMORY_SIZE) {
			return false;  // Not enough memory available to resize
		}
		//SerialPrint(1, "updateStringVariable: will move string %s", stringValue + currentSize + 1);
		memmove(stringValue + newSize + 1, stringValue + currentSize + 1, ledger->memoryOffset - currentSize);
		ledger->memoryOffset += additionalBytes;
		updateLedger(ledger, variable->value.stringValueIndex, additionalBytes);
	} else if (newSize < currentSize) {
		int bytesToRemove = currentSize - newSize;
		//SerialPrint(1, "updateStringVariable: bytesToRemove = %lu", bytesToRemove);
		memmove(stringValue + newSize, stringValue + newSize + bytesToRemove, currentSize - newSize);
		ledger->memoryOffset -= bytesToRemove;
		updateLedger(ledger, variable->value.stringValueIndex, -bytesToRemove);
	} //else they are equal

	strncpy(stringValue, newString, newSize);
	return true;  // No need to resize
}

void compactMemory(Ledger* ledger) {
	int compactOffset = 0;

	uint32_t varCount = 0;
	uint32_t i = 0;
	while (i < MAX_VARIABLES) {
		Variable* variable = &ledger->variables[i++];
		if (varCount >= ledger->varCount) break;
		//SerialPrint(1, "compactMemory ===================== varCount = %d", varCount);
		if (variable->name[0] == '\0') continue; //skip the empty variable pods
		if (variable->type == VARIABLE_TYPE_STRING) {
			//SerialPrint(1, "compactMemory: variable index is %d; variable variable %s compactOffset = %d", i, variable->name, compactOffset);
			int stringLength = strlen(ledger->memory + variable->value.stringValueIndex);
			if (stringLength > 0)
				stringLength++; //add 1 for the \0, not required if string is already null
			memmove(ledger->memory + compactOffset, ledger->memory + variable->value.stringValueIndex, stringLength);
			variable->value.stringValueIndex = compactOffset;
			compactOffset += stringLength;
			//SerialPrint(1, "compactMemory: Updated compactOffset to %d", compactOffset);
		}
		varCount++;
	}
	ledger->memoryOffset = compactOffset;
}

void printLedger(Ledger* ledger) {
	if (ledger->varCount > 0) SerialPrint(1, "printLedger:");
	else return;
	uint32_t varCount = 0;
	while (varCount < MAX_VARIABLES) {
		Variable* variable = &ledger->variables[varCount];
		//SerialPrint(1, "printLedger ===================== varCount = %d", varCount);
		if (varCount >= ledger->varCount) break;
		if (variable->name[0] == '\0') {
			varCount++;
			continue; //skip the empty variable pods
		}
		if (variable->type == VARIABLE_TYPE_STRING) {
			//SerialPrint(1, "Variable %s at vartable index %d: %s", variable->name, varCount, ledger->memory + variable->value.stringValueIndex);
		} else {
			//SerialPrint(1, "Variable %s at vartable index %d: %.15f", variable->name, varCount, variable->value.doubleValue.real);
		}
		varCount++;
	}
}

void printMemory(Ledger* ledger) {
	char temp[SHORT_STRING_SIZE];
	for (unsigned int i = 0; i < ledger->memoryOffset; i += 16) {
		sprintf(temp, "%04X: ", i);
		SerialPrint(1, temp);
		for (uint32_t j = 0; j < 16; j++) {
			if (i + j < ledger->memoryOffset) {
				sprintf(temp, "%02X: ", (unsigned char)ledger->memory[i + j]);
				SerialPrint(1, temp);
			} else {
				SerialPrint(1, "   ");
			}
		}

		//SerialPrint(1, ": ");
		for (uint32_t j = 0; j < 16; j++) {
			if (i + j < ledger->memoryOffset) {
				char ch = ledger->memory[i + j];
				if (ch >= 32 && ch <= 126) {
					temp[0] = ch; temp[1] = '\0';
					SerialPrint(1, temp);
				} else {
					SerialPrint(1, ".");
				}
			} else {
				SerialPrint(1, " ");
			}
		}
		SerialPrint(1, "\r\n");
	}
}

void stackInit(Strack *s) {
	s->topStr = -1;
	s->topLen = -1;
}

bool stackIsEmpty(Strack *s) {
	if (s->topLen == -1 && s->topStr != -1)
		SerialPrint(1, "Stack warning: Strack top indices out of sync!\r\n");
	return (s->topLen == -1 || s->topStr == -1);
}

int stackIsFull(Strack *s) {
	return (s->topStr >= STACK_SIZE - 1 || s->topLen >= STACK_NUM_ENTRIES - 1);
}

bool push(Strack *s, char* value, int8_t meta) {
	//SerialPrint(1, "push: entering-------- with %s of len = %d (including '\\0')", value, strlen(value)+1);
	size_t len = strlen(value);
	if (len > 0) len++; //include the '\0'
	if (s->topStr + len >= STACK_SIZE - 1 || s->topLen == STACK_SIZE - 1) {
		return false;
	}
	//SerialPrint(1, "push: s->topStr = %d", s->topStr);
	//SerialPrint(1, "push: s->topLen = %d", s->topLen);
	s->stackLen[++s->topLen] = len + (((int8_t)meta & 0xff) << 24); //meta
	//SerialPrint(1, "push: len + meta = %lu", len + (((int8_t)meta & 0xff) << 24));
	strcpy(&s->stackStr[s->topStr + 1], value);
	//SerialPrint(1, "push: string value = %s", &s->stackStr[s->topStr + 1]);
	s->topStr += len;

	//SerialPrint(1, "push: NOW s->topStr = %d", s->topStr);
	//SerialPrint(1, "push: NOW s->topLen = %d", s->topLen);
	//SerialPrint(1, "push: returning--------");
	return true;
}

int8_t pop(Strack *s, char* output) {
	//SerialPrint(1, "pop: entering--------");
	if (stackIsEmpty(s)) {
		output = NULL;
		return -1;
	}
	//SerialPrint(1, "pop: s->topStr = %d", s->topStr);
	//SerialPrint(1, "pop: s->topLen = %d", s->topLen);
	int8_t meta = (s->stackLen[s->topLen] >> 24) & 0xff;
	size_t len = s->stackLen[s->topLen--] & 0xffffff;
	//SerialPrint(1, "pop: length retrieved = %lu", len);
	if (output)
		strcpy(output, &s->stackStr[s->topStr - len + 1]);
	//SerialPrint(1, "pop: string output = %s", output);
	s->topStr -= len;
	//SerialPrint(1, "pop: NOW s->topStr = %d", s->topStr);
	//SerialPrint(1, "pop: NOW s->topLen = %d", s->topLen);
	//SerialPrint(1, "pop: returning--------");
	return meta;
}

int8_t peek(Strack *s, char output[STRING_SIZE]) {
	if (stackIsEmpty(s)) {
		//SerialPrint(1, "Peek: Strack is empty");
		output = NULL;		
		return -1;
	}
	//SerialPrint(1, "peek: s->topStr = %d", s->topStr);
	//SerialPrint(1, "peek: s->topLen = %d", s->topLen);
	int8_t meta = (s->stackLen[s->topLen] >> 24) & 0xff;
	size_t len = s->stackLen[s->topLen] & 0xffffff; 
	if (output)
		strcpy(output, &s->stackStr[s->topStr - len + 1]);
	//SerialPrint(1, "peek: string output = %s", output);
	//SerialPrint(1, "peek: NOW s->topStr = %d", s->topStr);
	//SerialPrint(1, "peek: NOW s->topLen = %d", s->topLen);
	return meta;
}

int8_t peekn(Strack* s, char output[STRING_SIZE], int n) {
	//n = 0: return T
	//n = 1: return T - 1
	if (n == 0) return peek(s, output);
	if (stackIsEmpty(s) || (n > s->topLen)) {
		//SerialPrint(1, "peekn: n is > s->topLen %d", s->topLen);
		output = NULL;
		return -1;
	}
	n++;

	//SerialPrint(1, "peekn: s->topStr = %s", s->topStr);
	//SerialPrint(1, "peekn: s->topLen = %d", s->topLen);
	char* stringPtr;
	size_t stringLen = -1;
	int8_t meta;
	for (int i = 0; i < n; i++) {
		//SerialPrint(1, "peekn: s->stackLen[%d] = %lu", i, s->stackLen[s->topLen - i]);
		stringLen += s->stackLen[s->topLen - i] & 0xffffff;
	}
	//now s->topStr - stringLen will point to the first string
	stringPtr = &s->stackStr[s->topStr - stringLen];
	meta = (s->stackLen[s->topLen - n + 1] >> 24) & 0xff;
	if (output)
		strcpy(output, stringPtr);
	return meta;
}

void printStack(Strack* s, int count, bool firstLast) {
	if (stackIsEmpty(s) || count == 0) return;
	int origCount = count;
	//ensure the count is within the range of the stack elements
	count = (count > s->topLen + 1) ? s->topLen + 1 : count;

	for (int i = 0; i < origCount - count; i++) {
		SerialPrint(1, "..................\r\n"); 
	}

	//SerialPrint(1, "printStack: s->topStr = %d", s->topStr);
	//SerialPrint(1, "printStack: s->topLen = %d", s->topLen);
	//SerialPrint(1, "Printing %d entries from the top of the stack:", count);
	char* stringPtr;
	size_t stringLen = -1;
	int8_t meta;
	if (firstLast) {
		for (int i = 0; i < count; i++) {
			//SerialPrint(1, "printStack: s->stackLen[%d] = %lu", i, s->stackLen[s->topLen - i]);
			stringLen += s->stackLen[s->topLen - i] & 0xffffff;
		}
		//now s->topStr - stringLen will point to the first string
		//SerialPrint(1, "printStack - doing firstLast -- before FOR loop -- topLen = %d, stringLen = %lu", s->topLen, stringLen);
		
		for (int i = 0; i < count; i++) {
			//SerialPrint(1, "printStack - doing firstLast -- s->topLen = %d and stringLen = %lu", s->topLen, stringLen);
			stringPtr = &s->stackStr[s->topStr - stringLen];
			meta = (s->stackLen[s->topLen - count + i + 1] >> 24) & 0xff;
			SerialPrint(6, 
					(meta == 1)? "[": ((meta == 4)? "[[": ""), 
					stringPtr, 
					(meta == 3)? "]": ((meta == 6)? "]]": ""), 
					" meta = ",
					DEBUGMETA[meta], 
					"\r\n");

			stringLen -= strlen(stringPtr) + 1;
		}
	} else {
		for (int i = s->topLen; i > s->topLen - count; i--) {		
			stringLen += s->stackLen[i] & 0xffffff;
			stringPtr = &s->stackStr[s->topStr - stringLen];
			SerialPrint(2, stringPtr, "\r\n");
		}
	}
}

int calcRightOFPos(char* dest) {
	int slen = strlen(dest);
	for (int i = slen - 1; i >= 0; i--) {
		if ((dest[i] == 'e') || (dest[i] == 'E'))
			return i - 1;
	}
	return slen - 1;
}

int makeStringFit(char*dest, const char* src, int len) {
	char debug0[VSHORT_STRING_SIZE];
	char debug1[VSHORT_STRING_SIZE];
	char srcCopy[STRING_SIZE];
	//printf("------------makeStringFit - called with src = %s and len = %d\n", src, len);

	int slen = strlen(src);
	//printf("------------makeStringFit - called with src = %s and slen = %d\n", src, slen);

	strcpy(srcCopy, src);
	int expoIndex = 0;
	for (expoIndex = 0; expoIndex < slen; expoIndex++) {
		if ((srcCopy[expoIndex] == 'e') || (srcCopy[expoIndex] == 'E')) {
			if (srcCopy[expoIndex + 1] == '+') {
				//remove unnecessary '+' after the 'e'
				memmove(&srcCopy[expoIndex + 1], &srcCopy[expoIndex + 2], slen - expoIndex - 2);
				srcCopy[slen - 1] = '\0';
				slen--; //replacing src with srcCopy
			}
			break;
		}
	}
	//printf("------------makeStringFit: srcCopy = %s expoIndex = %d\n", srcCopy, expoIndex);

	if (slen <= len) {
		strcpy(dest, srcCopy);
		//printf("------------makeStringFit: Returning. A src = %s\n", src);
		//printf("------------makeStringFit: Returning. A dest = %s\n", dest);
		return 0;
	}

	////printf("------------makeStringFit - partMantissaLen = ", debug0, " and expoIndex = ", debug1);
	if (expoIndex == slen) {
		//does not have the exponent format
		if (slen > len) {
			//printf("------------makeStringFit: AAA slen %d > len %d\n", slen, len);
			//bigint --  show right overflow
			strncpy(dest, srcCopy, len);
			dest[len] = '\0';
			//printf("------------makeStringFit - B dest = %s len of dest = %lu\n", dest, strlen(dest));
			//printf("------------makeStringFit: Returning. B dest = %s\n", dest);
			return 1;
		} else {
			//printf("------------makeStringFit: BBB slen %d <= len %d\n", slen, len);
			strncpy(dest, srcCopy, slen);
			dest[slen] = '\0';
			//printf("------------makeStringFit - C dest = %s len of dest = %lu\n", dest, strlen(dest));
			//printf("------------makeStringFit: Returning. C dest = ", dest);
			return 0;
		}
	}

	int partMantissaLen = len - (slen - expoIndex);
	//printf("------------makeStringFit: partMantissaLen = %d\n", partMantissaLen);
	//we want to remove some digits from the mantissa
	//part of the real number so that it fits within len
	//partmantissalen = len - (slen - expoIndex) + 1
	//this has to be less than if we have to truncate
	//the mantissa
	//slen - expoIndex --> exponent part
	if (slen > len) {
		//printf("------------makeStringFit: srcCopy = %s slen %d >= len %d and (slen - expoIndex) = %d\n", srcCopy, slen, len, slen - expoIndex);
		strncpy(dest, srcCopy, partMantissaLen);
		//printf("------------makeStringFit - intermediate D dest = %s len of dest = %lu\n", dest, strlen(dest));
		if (dest[partMantissaLen - 1] == '.') { //ending in a '.' - remove it
			strncpy(dest + partMantissaLen - 1, &srcCopy[expoIndex], slen - expoIndex);
			dest[len - 1] = '\0';
			//printf("------------makeStringFit - D1 dest = %s len of dest = %lu\n", dest, strlen(dest));
			//printf("------------makeStringFit: Returning. D dest = %s\n", dest);
		} else {
			strncpy(dest + partMantissaLen, &srcCopy[expoIndex], slen - expoIndex);
			dest[len] = '\0';
			//printf("------------makeStringFit - D2 dest = %s len of dest = %lu\n", dest, strlen(dest));
			//printf("------------makeStringFit: Returning. D dest = %s\n", dest);
		}
		return 1;
	} else {
		//printf("------------makeStringFit: srcCopy = %s slen %d < len %d\n", srcCopy, slen, len);
		strcpy(dest, srcCopy);
		//printf("------------makeStringFit: Returning. E dest = %s\n", dest);
		return 0;
	}
}

int makeComplexStringFit(char*dest, const char* src, int len) {
	char re[VSHORT_STRING_SIZE];
	char im[VSHORT_STRING_SIZE];
	char debug0[VSHORT_STRING_SIZE];
	char debug1[VSHORT_STRING_SIZE];
	int spacePos = 0;
	int charPos = -1;
	//printf("------------makeComplexStringFit - called with  src = %s\n", src);
	//printf("------------makeComplexStringFit - called with len = %d\n", len);

	if (src[0] != '(') {
		//number is real
		charPos = makeStringFit(re, src, len);
		//printf("------------makeComplexStringFit - number is real, charPos = %d\n", charPos);
		//printf("------------makeComplexStringFit - number is real, charPos - 1 = %d\n", charPos - 1);
		strcat(dest, re);
		//return (charPos == -1)? -1: (charPos - 1); //no '('
		if (charPos) //length was modified
			return calcRightOFPos(dest);
		else return -1;
	}

	//code below is only for imaginary or complex numbers
	strcat(dest, "(");

	int slen = strlen(src);
	//printf("------------makeComplexStringFit: src = %s of slen = %d\n", src, slen);
	strcpy(re, &src[1]); //copy source without the '(' and ')'
	re[slen - 2] = '\0'; //remove the ')'
	//printf("------------makeComplexStringFit: re = %s of len = %d\n", re, strlen(re));

	int spaceCharCount = 0;
	while (re[spacePos] != ' ' && re[spacePos++] != '\0');
	//printf("------------makeComplexStringFit: spacePos = %d\n", spacePos);
	if (spacePos > (slen - 2)) {
		//space was not found, so this is a pure imaginary number
		memset(im, 0, sizeof(im));
		charPos = makeStringFit(im, re, len - 2); //keep space for '(' & ')'
		//printf("------------makeComplexStringFit  - a pure imaginary - re = %s\n", re);
		//printf("------------makeComplexStringFit  - a pure imaginary - now dest = %s\n", dest);
		strcat(dest, im);
		strcat(dest, ")");
		if (charPos) //length was modified
			return calcRightOFPos(dest) - 1;
		else return -1;

	} else {
		//printf("------------makeComplexStringFit  - a complex number - re = %s\n", re);
		memset(im, 0, sizeof(im));
		re[spacePos] = '\0'; //just the part before the space - real part
		makeStringFit(im, re, ((len - 2) >> 1) - 1);
		//printf("------------makeComplexStringFit  - a complex number - done real before cat, dest = %s\n", dest);
		strcat(dest, im); //im is now the real part
		strcat(dest, " ");
		//printf("------------makeComplexStringFit  - a complex number - done real now dest = %s\n", dest);
		//printf("------------makeComplexStringFit  - a complex number - done real now &src[spacePos + 2] = %s\n", &src[spacePos + 2]);
		memset(im, 0, sizeof(im));
		strcpy(im, &src[spacePos + 2]); //the first '(' was removed from src before spacePos was found
		//printf("------------makeComplexStringFit  - a complex number - will call for imag now A im = %s\n", im);
		im[slen - spacePos - 3] = '\0'; //not copying the last ')' in src
		//printf("------------makeComplexStringFit  - a complex number - will call for imag now B im = %s\n", im);
		memset(re, 0, sizeof(re));
		makeStringFit(re, im, ((len - 2) >> 1) - 1);
		strcat(dest, re);
		strcat(dest, ")");
		charPos = strlen(dest); //capture dest len at this point

		//printf("------------makeComplexStringFit  - a complex number - done imag now dest = %s\n", dest);
		//printf("------------makeComplexStringFit  - a complex number - slen = %d len = %d\n", slen, len);
		//printf("------------makeComplexStringFit  - a complex number - returning with %d\n", charPos);
		return (slen > len)? charPos: -1;
	}
}

void showStack(Machine* vm) {
	//SerialPrint(1, "===================================\r\n");
	int len;
	int8_t meta;
	char debug0[VSHORT_STRING_SIZE];
	for (int i = 0; i < 3; i++) {
		//print userline status here
		lcd.setCursor(0, 2 - i);
		if ((2 - i) == 0) //row 0
			lcd.print((vm->shiftState)? "S ": "  "); 
		else if ((2 - i) == 1)
			lcd.print((vm->altState)? "A ": "  ");
		else
			lcd.print((vm->modeDegrees)? "D ": "  ");

		for (int j = DISPLAY_STATUS_WIDTH; j < DISPLAY_LINESIZE; j++)
			lcd.print(' ');
		memset(vm->userDisplay, 0, STRING_SIZE);
		lcd.setCursor(DISPLAY_STATUS_WIDTH, 2 - i);
		meta = peekn(&vm->userStack, vm->coadiutor, i);
		if (meta != -1) {
			int rightOfPos = -1;
			SerialPrint(3, "showStack: calling makeString/Complex/Fit. vm->coadiutor = ", vm->coadiutor, "\r\n");
			rightOfPos = makeComplexStringFit(vm->userDisplay, vm->coadiutor, DISPLAY_LINESIZE - DISPLAY_STATUS_WIDTH);
			SerialPrint(3, "showStack: returned from makeComplexStringFit. vm->display = ", vm->display, "\r\n");
			lcd.print(vm->userDisplay);
			if (rightOfPos != -1) {
				//show the right overflow indicator
				lcd.setCursor(DISPLAY_STATUS_WIDTH + rightOfPos, 2 - i);
				lcd.write(RIGHTOFIND);
			}
			int tempLen = strlen(vm->userDisplay);
			//doubleToString(tempLen, debug0);
			SerialPrint(3, "showStack: long str: vm->display = ", vm->userDisplay, "\r\n");
			//SerialPrint(5, "showStack: long str: vm->display = ", vm->display, " len display = ", debug0, "\r\n");
			//SerialPrint(3, "showStack: len display = ", debug0, "\r\n");
		} else {
			lcd.print("..................");
			//lcd.print("                  ");
		}
	}
	printStack(&vm->userStack, 3, true);
	//SerialPrint(1, "-----------------------------------");
	//printRegisters(vm);
	//SerialPrint(3, "Last Op = ", vm->lastFnOp, "\r\n");
	SerialPrint(3, (strcmp(vm->error, "") == 0)? "": "ERR = ", (strcmp(vm->error, "") == 0)? "Ok": vm->error, "\r\n");
	//char buf[129];
	//sprintf(buf, "execStack: matrix = %d, vector = %d IFstate = %x\r\n", (int) (execData >> 4) & 0x1, (int) (execData >> 3) & 0x1, (int) execData & 0x7);
	//SerialPrint(1, buf);
	//SerialPrint(1, "===================================\r\n");

	if (strcmp(vm->error, "") != 0) {
		char line[DISPLAY_LINESIZE];
		eraseUserEntryLine();
		fitstr(line, vm->error, DISPLAY_LINESIZE - DISPLAY_STATUS_WIDTH);
		lcd.setCursor(DISPLAY_STATUS_WIDTH, 3); 
		lcd.print("E:"); 
		lcd.print(line);
		lcd.setCursor(vm->cursorPos, 3); //col, row
	} else if (strcmp(vm->lastFnOp, "") != 0) {
		char line[DISPLAY_LINESIZE];
		SerialPrint(3, "Doing Last Op = ", vm->lastFnOp, "\r\n");
		eraseUserEntryLine();
		//no error, show previous operator or function on user entry line
		fitstr(line, vm->lastFnOp, DISPLAY_LINESIZE - DISPLAY_STATUS_WIDTH);
		lcd.setCursor(DISPLAY_STATUS_WIDTH, 3); lcd.print(line);
		lcd.setCursor(vm->cursorPos, 3); //col, row
	} else {
		eraseUserEntryLine();
	}
}

void showUserEntryLine(int bsp){ 
	char debug0[VSHORT_STRING_SIZE];
	char debug1[VSHORT_STRING_SIZE];
	//print userline status here
	//SerialPrint(3, "showUserEntryLine: userInput = ", vm.userInput, "\r\n");
	memset(vm.userDisplay, 0, STRING_SIZE);
	int len = strlen(vm.userInput);
	doubleToString(vm.cursorPos, debug0);
	SerialPrint(3, "showUserEntryLine: vm.cursorPos = ", debug0, "\n\r");
	doubleToString(vm.userInputPos, debug0);
	SerialPrint(3, "showUserEntryLine: vm.userInputPos = ", debug0, "\n\r");

	//keep 1 char for current input char
	if (len > DISPLAY_LINESIZE - 1) {
		lcd.setCursor(0, 3); //col, row
		//keep 1 char for the scrolled off indication
		lcd.write(LEFTOFIND);
		vm.userInputLeftOflow = 1;
		strncpy(vm.userDisplay, &vm.userInput[len - DISPLAY_LINESIZE + 2], DISPLAY_LINESIZE - 2);
	} else {
		int colPos = DISPLAY_LINESIZE - len - 1;
		if (bsp) {
			doubleToString(colPos, debug0);
			doubleToString(bsp, debug1);
			SerialPrint(5, "showUserEntryLine: colPos = ", debug0, " bsp = ", debug1, "\r\n");
			doubleToString(vm.cursorPos, debug1);
			SerialPrint(5, "showUserEntryLine: colPos = ", debug0, " vm.cursorPos = ", debug1, "\r\n");
			lcd.setCursor(colPos - 1, 3);
			lcd.print(' ');
		}
		lcd.setCursor(colPos, 3); //col, row
		strncpy(vm.userDisplay, vm.userInput, len);
	}
	SerialPrint(3, "showUserEntryLine: vm.userDisplay = ", vm.userDisplay, "\r\n");
	doubleToString(len, debug0);
	SerialPrint(3, "showUserEntryLine: len vm.userDisplay = ", debug0, "\r\n");
	lcd.print(vm.userDisplay);
	lcd.setCursor(vm.cursorPos, 3); //col, row
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
	//SerialPrint(1, "Popping from UintStack");
	if (UintStackIsEmpty(s)) {
		//SerialPrint(1, "Pop warning: UintStack is empty");
		return 0;
	}
	return s->stack[s->top--];
}

size_t UintStackPeek(UintStack *s) {
	if (UintStackIsEmpty(s)) {
		//SerialPrint(1, "Peek: Stack is empty");
		return 0;
	}
	return s->stack[s->top];
}

bool isComplexNumber(const char *input) {
	int i = 0;
	if (strlen(input) < 2) return false;
	bool success = false;
	//SerialPrint(1, "isComplexNumber:------------------- input = %s", input);
	while (isspace(input[i])) {
		i++; //skip leading spaces
	}
	if (input[i++] == '(') success = true;
	else return false;
	//SerialPrint(1, "isComplexNumber:------------------- len input = %lu, i = %d success = %d", strlen(input), i, success);
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
	//SerialPrint(1, "parseComplex: 0. input: '%s'", input);
	int i = 0;
	while (isspace(input[i])) {
		i++; //Skip leading spaces
	}
	//SerialPrint(1, "parseComplex: 0. i = %d", i);
	//SerialPrint(1, "parseComplex: 0. input[%d]: '%c'", i, input[i]);
	if (input[i] == '(') {
		i++;
		while (isspace(input[i])) {
			i++; //skip spaces
		}
	}
	//SerialPrint(1, "parseComplex: 1. i = %d", i);
	//first string
	int j = 0;
	while (isalpha(input[i]) || isdigit(input[i]) || input[i] == '+' || input[i] == '-' || input[i] == '.') {
		//SerialPrint(1, "parseComplex: i = %d, input[%d] = '%c'", i, i,input[i]);
		substring1[j++] = input[i++];
	}
	substring1[j] = '\0'; //null-terminate the first substring
	//SerialPrint(1, "parseComplex: Substring 1: '%s'", substring1);
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
			//SerialPrint(1, "parseComplex: Done Substring 2: '%s'", substring1);
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
	//SerialPrint(1, "stringToDouble: entered ------------------- str = %s", str);
	char* endPtr;
	size_t len = strlen(str);
	char rlc = strIsRLC(str);
	if (rlc) {
		str[len-1] = '\0';
	}
	*dbl = strtod(str, &endPtr);
	//if (endPtr == str) {
	if (*endPtr != '\0') {
		//SerialPrint(1, "stringToDouble: done with false ------------------- dbl = %g", *dbl);
		return false;
	} else {
		//SerialPrint(1, "stringToDouble: done with true ------------------- dbl = %g", *dbl);
		if (rlc) str[len-1] = rlc;
		if (errno == ERANGE) return false;
		if (abs(*dbl) == DOUBLE_EPS) return false;
		return true;
	}
}

bool stringToComplex(const char *input, ComplexDouble* c) {
	//the input is guaranteed not to have to leading or trailing spaces
	//(a, b) -> a + ib
	//(b) -> ib
	//SerialPrint(1, "stringToComplex: just entered input = %s", input);
	if (input[0] != '(' ||  input[strlen(input)-1] != ')') return false;
	char str1[SHORT_STRING_SIZE];
	char str2[SHORT_STRING_SIZE];
	int failpoint = parseComplex(input, str1, str2);
	//SerialPrint(1, "stringToComplex: parseComplex returned %d str1 = %s --- str2 = %s", failpoint, str1, str2);
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

bool isRealNumber(const char *token) {
	int i = 0;
	int len = strlen(token);
	int hasDecimal = 0;
	int hasExponent = 0;
	int hasSign = 0;

	// Check for negative sign
	if (token[i] == '-' || token[i] == '+') {
		hasSign = 1;
		i++;
		if (i == len) {
			return false;  // '-' alone is not a valid number
		}
	}

	// Check for digits or decimal point
	while (i < len) {
		if (isdigit(token[i])) {
			i++;
		} else if (token[i] == '.') {
			if (hasDecimal || hasExponent) {
				return false;  // Multiple decimal points or decimal point after exponent is not valid
			}
			hasDecimal = 1;
			i++;
		} else if (token[i] == 'e' || token[i] == 'E') {
			if (hasExponent) {
				return false;  // Multiple exponents are not valid
			}
			hasExponent = 1;
			i++;
			if (i < len && (token[i] == '+' || token[i] == '-')) {
				i++;  // Allow optional sign after exponent
			}
		} else {
			if ((token[i] == 'r' || token[i] == 'l' || token[i] == 'c') &&
				(i == len - 1)) return true;
			else return false;  // Invalid character
		}
	}

	//make sure the token is fully parsed
	if (hasSign && i == 1) {
		return false;  // Only a sign is not a valid number
	}
	return (bool)(i == len);
}

double sgn(double value) {
	if (signbit(value)) return (-1.0);
	else return (1.0);
}

double cabso (ComplexDouble value) {
	return sqrt(value.real * value.real + value.imag * value.imag);
}

double cargu (ComplexDouble value) {
	return atan(value.imag/value.real);
}

double crealpart (ComplexDouble value) {
	return value.real;
}

double cimagpart (ComplexDouble value) {
	return value.imag;
}

bool alm0(ComplexDouble value) {
	if (cabso(value) < DOUBLE_EPS) return true;
	else return false;
}

bool alm0double(double value) {
	if (abs(value) < DOUBLE_EPS) return true;
	else return false;
}

ComplexDouble cneg(ComplexDouble value) {
	return makeComplex(-value.real, -value.imag);
}

ComplexDouble cadd(ComplexDouble value, ComplexDouble second) {
	return makeComplex(value.real + second.real, value.imag + second.imag);
}

ComplexDouble caddd(ComplexDouble value, double second) {
	return makeComplex(value.real + second, value.imag);
}

ComplexDouble csub(ComplexDouble value, ComplexDouble second) {
	return makeComplex(value.real - second.real, value.imag - second.imag);
}

ComplexDouble csubd(ComplexDouble value, double second) {
	return makeComplex(value.real - second, value.imag);
}

ComplexDouble cmul(ComplexDouble value, ComplexDouble second) {
	return makeComplex(value.real * second.real - value.imag * second.imag, 
		value.real * second.imag + value.imag * second.real);
}

ComplexDouble cdiv(ComplexDouble value, ComplexDouble second) {
	double denom = cabso(second);
	if (alm0double(denom)) return makeComplex(0, 0);
	return makeComplex(value.real / denom, value.imag / denom);
}

ComplexDouble cdivd(ComplexDouble value, double second) {
	return makeComplex(value.real / second, value.imag / second);
}

ComplexDouble cmod(ComplexDouble value, ComplexDouble second) {
	return makeComplex(0, 0);
}

ComplexDouble ceq(ComplexDouble value, ComplexDouble second) {
	if (alm0double(cabso(csub(value, second))))
		return makeComplex(1.0, 0.0);
	else
		return makeComplex(0.0, 0.0);
}

ComplexDouble cneq(ComplexDouble value, ComplexDouble second) {
	if (!alm0double(cabso(csub(value, second))))
		return makeComplex(1.0, 0.0);
	else
		return makeComplex(0.0, 0.0);
}

ComplexDouble cpar(ComplexDouble value, ComplexDouble second) {
	return cdiv(cmul(value, second), cadd(value, second));
}

ComplexDouble clt(ComplexDouble value, ComplexDouble second) {
	ComplexDouble diff = csub(second, value);
	if (alm0double(diff.imag))
		//difference is real
		//LT is true if difference is positive
		return (!alm0double(diff.real))? makeComplex(1.0, 0.0): makeComplex(0.0, 0.0);
	else
		//difference is complex
		//LT is true if magnitude of the diff is positive
		return (!alm0(diff))? makeComplex(1.0, 0.0): makeComplex(0.0, 0.0);
}

ComplexDouble cgt(ComplexDouble value, ComplexDouble second) {
	return (clt(second, value));
}

ComplexDouble cgte(ComplexDouble value, ComplexDouble second) {
	ComplexDouble l = clt(value, second);
	if (l.real == 1.0) return makeComplex(0.0, 0.0);
	else return makeComplex(1.0, 0.0);
}

ComplexDouble clte(ComplexDouble value, ComplexDouble second) {
	ComplexDouble g = cgt(value, second);
	if (g.real == 1.0) return makeComplex(0.0, 0.0);
	else return makeComplex(1.0, 0.0);
}

ComplexDouble cln(ComplexDouble c) {
	double re = c.real;
	double im = c.imag;
	double r, j, imdre;
	if (!alm0double(re) && alm0double(im) && (re > 0.0)) {
		//imag is 0
		//re is positive
		return makeComplex(log(re), 0);
	} else if (!alm0double(re)) {
		if (re > 0.0) {
			r = log(re);
			j = 0.0;
		} else {
			//log of a negative number = log (-1) * log of the negative of the negative number
			//log (-1) = pi * i
			r = log(-re);
			j = __TS_PI__;
		}
		if (!alm0double(im)) {
			imdre = (im/re);
			r += log(imdre*imdre + 1)/2;
			j += atan(imdre);
		} 
	} else if (!alm0double(im)) {
		r = log(im);
		j = 0.5 * __TS_PI__  * sgn(im);
	} else {
		r = 0;
		j = 0;
		strcpy(vm.error, "log undef");
	}
	if (j > __TS_PI__)
		j -= __TS_PI__ * 2;
	return makeComplex(r, j);
}

ComplexDouble cexpo(ComplexDouble c) {
	if (alm0double(c.imag)) {
		return makeComplex(exp(c.real), 0);
	}
	double r = c.real;
	double j = c.imag;
	double rexp = exp(r);
	r = rexp * cos(j);
	if (!alm0double(c.imag))
		j = rexp * sin(j);
	else
		j = 0;
	return makeComplex (r, j);
}

ComplexDouble cpower(ComplexDouble c, ComplexDouble d) {
	if (alm0double(c.imag) && alm0double(d.imag) && (d.real >= 1.0))
		return makeComplex(pow(c.real, d.real), 0.0);
	if (alm0double(c.imag) && alm0double(d.imag) && (d.real == 0.0))
		return makeComplex (1.0, 0.0);
	return (cexpo(cmul(cln(c), d))); 
}

ComplexDouble cpowerd(ComplexDouble c, double d) {
	if (alm0double(c.imag) && (d >= 1.0))
		return makeComplex(pow(c.real, d), 0.0);
	if (alm0double(c.imag) && (d == 0.0))
		return makeComplex (1.0, 0.0);
	return (cexpo(cmul(cln(c), makeComplex(d, 0.0))));
}

ComplexDouble csqroot(ComplexDouble c) {
	return cpowerd(c, 0.5);
}

ComplexDouble conjugate(ComplexDouble c) {
	return makeComplex(c.real, -c.imag);
}

ComplexDouble cbroot(ComplexDouble c) {
	ComplexDouble r = cpower(c, makeComplex((1.0/3.0), 0.0));
	if (!alm0double(r.imag)) {
		//returned data is complex, we want the real root
		ComplexDouble rmagsq = cmul(r, conjugate(r));
		return (cdiv(c, rmagsq));
	} else {
		return r;
	}
}

ComplexDouble csinehyp(ComplexDouble c) {
	return cdivd(csub(cexpo(c), cexpo(cneg(c))), 2.0);
}

ComplexDouble ccosinehyp(ComplexDouble c) {
	return cdivd(cadd(cexpo(c), cexpo(cneg(c))), 2.0);
}

ComplexDouble ctangenthyp(ComplexDouble c) {
	ComplexDouble d = ccosinehyp(c);
	if (alm0(d)) {
		strcpy(vm.error, "bad fn domain");
		return makeComplex(0.0, 0.0);
	}
	return cdiv(csinehyp(c), d);
}

ComplexDouble csine(ComplexDouble c) {
	double r = c.real;
	double j = c.imag;
	return (makeComplex(sin(r) * cosh(j), cos(r) * sinh(j)));
}

ComplexDouble ccosine(ComplexDouble c) {
	double r = c.real;
	double j = c.imag;
	return (makeComplex(cos(r) * cosh(j), sin(r) * sinh(j)));
}

ComplexDouble ctangent(ComplexDouble c) {
	ComplexDouble d = ccosine(c);
	if (alm0(d)) {
		strcpy(vm.error, "result undef");
		return makeComplex(0.0, 0.0);
	}
	return cdiv(csine(c), d);
}

ComplexDouble carcsine(ComplexDouble c) {
	ComplexDouble j = makeComplex(0.0, 1.0);
	ComplexDouble x = csub(csqroot(csub(makeComplex(1.0, 0.0), cmul(c, c))), cmul(j, c));
	return cmul(j, cln(x));
}

ComplexDouble carccosine(ComplexDouble c) {
	return csub(makeComplex(__TS_PI__/2.0, 0.0), csine(c));
}

ComplexDouble carctangent(ComplexDouble c) {
	ComplexDouble j = makeComplex(0.0, 1.0);
	ComplexDouble jx2 = makeComplex(0.0, 2.0);
	if (alm0(csub(c, j))) {
		//atan is not defined for j
		strcpy(vm.error, "bad fn domain");
		return makeComplex(0.0, 0.0);
	}
	ComplexDouble x = cdiv(cadd(c, j), csub(c, j));
	return cdiv(cln(x), jx2);
}

ComplexDouble carcsinehyp(ComplexDouble c) {
	return cln(cadd(c, csqroot(caddd(cmul(c, c), 1.0))));
}

ComplexDouble carccosineh(ComplexDouble c) {
	return cln(cadd(c, csqroot(csubd(cmul(c, c), 1.0))));
}

ComplexDouble carctangenthyp(ComplexDouble c) {
	if (alm0(csub(c, makeComplex(1.0, 0.0)))) {
		strcpy(vm.error, "bad fn domain");
		return makeComplex(0.0, 0.0);
	}
	ComplexDouble x = cdiv(cadd(makeComplex(1.0, 0.0), c), csub(makeComplex(1.0, 0.0), c));
	return cdivd(cln(x), 2.0);
}

ComplexDouble carccotangenthyp(ComplexDouble c) {
	if (alm0(csub(c, makeComplex(1.0, 0.0)))) {
		strcpy(vm.error, "bad fn domain");
		return makeComplex(0.0, 0.0);
	}
	ComplexDouble x = cdiv(cadd(c, makeComplex(1.0, 0.0)), csub(c, makeComplex(1.0, 0.0)));
	return cdivd(cln(x), 2.0);
}

ComplexDouble carcsecanthyp(ComplexDouble c) {
	if (alm0(c)) {
		strcpy(vm.error, "bad fn domain");
		return makeComplex(0.0, 0.0);
	}
	return cln(cdiv(cadd(makeComplex(1.0, 0.0), csqroot(csub(makeComplex(1.0, 0.0), cmul(c, c)))), c));
}

ComplexDouble carccosecanthyp(ComplexDouble c) {
	if (alm0(c)) {
		strcpy(vm.error, "bad fn domain");
		return makeComplex(0.0, 0.0);
	}
	return cln(cdiv(cadd(makeComplex(1.0, 0.0), csqroot(cadd(makeComplex(1.0, 0.0), cmul(c, c)))), c));
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
	if (alm0double(value)) {
		strcpy(buf, "0");
		return true;
	}
	char fmt[7];
	//for small numbers (with exponent -10 etc), 16 decimal places
	//will give wrong digits - since these are beyond the precision
	//of double floats
	sprintf(buf, "%.16g", value);
	if (strcmp(lcase(buf), "inf") == 0 || strcmp(lcase(buf), "-inf") == 0) return false;
	if (strcmp(lcase(buf), "nan") == 0 || strcmp(lcase(buf), "-nan") == 0) return false;

	//printf("doubleToString: ------ At start value string = %s\n", buf);
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
			int numDecimals = 15 - expo;
			if (numDecimals < 6) numDecimals = 6;
			strcpy(fmt, "%.");
			itoa(numDecimals, &fmt[2]);
			strcat(fmt, "g");
		}
	}
	sprintf(buf, fmt, value);

	//printf("doubleToString ------ buf = %s\n", buf);
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
	if (alm0double(c.imag)) {
		//imag value is 0 - return r
		strcpy(value, r);
		//SerialPrint(3, "complexToString: returning with value ", value, "\r\n");
		return true;
	}
	//imag value is not zero, return one of
	//(1) (r i)
	//(2) (i)
	strcpy(value, "(");
	if (!(alm0double(c.real))) {
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

ComplexDouble ccbrt(ComplexDouble x){
	return cpowerd(x, (1.0/3.0));
}

ComplexDouble clog2(ComplexDouble x){
	return cdiv(cln(x), cln(makeComplex(2.0, 0.0)));
}

ComplexDouble clogarithm10(ComplexDouble x){
	return cdiv(cln(x), cln(makeComplex(10.0, 0.0)));
}

ComplexDouble cmax(ComplexDouble a, ComplexDouble b) {
	return cabso(a) > cabso(b) ? a : b;
}

ComplexDouble cmin(ComplexDouble a, ComplexDouble b) {
	return cabso(a) < cabso(b) ? a : b;
}

ComplexDouble carctangent2(ComplexDouble a, ComplexDouble b) {
	return carctangent(cdiv(a, b));
}

typedef double (*RealFunctionPtr)(ComplexDouble);
typedef void (*BigIntVoidFunctionPtr)(const bigint_t*, const bigint_t*, bigint_t*);
typedef int (*BigIntIntFunctionPtr)(const bigint_t*, const bigint_t*);
typedef ComplexDouble (*ComplexFunctionPtr1Param)(ComplexDouble);
typedef ComplexDouble (*ComplexFunctionPtr2Param)(ComplexDouble, ComplexDouble);
typedef ComplexDouble (*ComplexFunctionPtrVector)(ComplexDouble, ComplexDouble, ComplexDouble, int);

const char* mathfn1paramname[] = {
						"sin", "cos",
						"tan", "asin",
						"acos", "atan", 
						"exp", "log10", 
						"log", "log2", 
						"sqrt", "cbrt",
						"conj", 
						"abs", "arg", 
						"re", "im"
						}; //the 1 param functions
const int NUMMATH1PARAMFN = 13;

ComplexFunctionPtr1Param mathfn1param[] = {csine, ccosine, 
											ctangent, carcsine, 
											carccosine, carctangent, 
											cexpo, clogarithm10, 
											cln, clog2, 
											csqroot, cbroot, 
											conjugate};

RealFunctionPtr realfn1param[] = {cabso, cargu, crealpart, cimagpart};

BigIntVoidFunctionPtr bigfnvoid2param[] = {bigint_max, bigint_min, bigint_add, bigint_sub, bigint_mul, bigint_div, bigint_rem};

BigIntIntFunctionPtr bigfnint2param[] = {bigint_gt, bigint_lt, bigint_gte, bigint_lte, bigint_eq,
									bigint_neq};

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

const int NUMMATHOPS = 11;

const char* mathfn3param[] = {"minv", "mexp"}; //the 3 params functions

ComplexFunctionPtr2Param mathfn2param[] = {cpower, carctangent2, cmax, cmin, cadd, csub,
									cmul, cdiv, cmod, cgt, clt, cgte, clte, ceq, cneq, cpar};

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
	//printf ("tokenize: START input = %p\n", input);
	//skip leading spaces
	input += strspn(input, " \t\r\n");
	int quoteOrParens = 0;
	//check if the current token is a double-quoted string or a complex or a print command
	if (*input == '"' || (*input == '?' && input[1] == '"') || (*input == '@' && input[1] == '"'))
		quoteOrParens = 1;
	else if (*input == '(') 
		//a complex number
		quoteOrParens = 2;
	else if ((*input == '[') && (input[1] == '('))
		//a vector with a complex number
		quoteOrParens = 3;

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

		//copy characters until the closing double-quote or closing parenthesis or 
		//the end of the input string is found
		while (*input && *input != ((quoteOrParens >= 2)? ')':'"')) {
			output[i++] = *input++;
		}

		//add null-terminator to the output string
		output[i++] = (quoteOrParens >= 2)? ')':'"';
		output[i] = '\0';

		//if a closing double-quote  or closing parenthesis was found, advance to the next character after it
		if (*input == ((quoteOrParens >= 2)? ')':'"')) {
			input++;
		}

		//skip trailing spaces after the double-quoted string
		input += strspn(input, " \t\r\n");
	}
	else {
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
	//printf ("tokenize: END input = %p\n", input);
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
	FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "no scalar op here.H", NULLFN)
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
		FAILANDRETURN((meta == -1), vm->error, "stack empty", NULLFN)		
		//printf("fnOrOp2Param --- ifcondition is true or is unconditional. meta for var a = %d\n", meta);
		FAILANDRETURN((meta != METANONE), vm->error, "no scalar op here.L.", NULLFN)
		if (fnindex == 0) { //swap
			FAILANDRETURN((meta == -1), vm->error, "stack empty.B", NULLFN)			
			FAILANDRETURN((meta != METANONE), vm->error, "no scalar op here.E", NULLFN)
			pop(&vm->userStack, NULL);
			pop(&vm->userStack, NULL);
			push(&vm->userStack, vm->coadiutor, cmeta);
			push(&vm->userStack, vm->bak, meta);
			return true;
		}
		//printf("fnOrOp2Param --- meta for var b = %d\n", meta);
		FAILANDRETURN((meta == -1), vm->error, "stack empty.C", NULLFN)		
		FAILANDRETURN((meta != METANONE), vm->error, "no scalar op here.A", NULLFN)
		//printf ("fnOrOp2Param: 1 ------------------- coadiutor = %s bak = %s\n", vm->coadiutor, vm->bak);
		//printf ("fnOrOp2Param: 0 ------------------- got ADD\n");
		ComplexDouble c, d;
		success = false;

		if (hasDblQuotes(vm->coadiutor) || hasDblQuotes(vm->bak)) {
			char* coadiutor = removeDblQuotes(vm->coadiutor);
			char* bak = removeDblQuotes(vm->bak);
			if ((isBigIntDecString(coadiutor)) && 
					(isBigIntDecString(bak))) { 
				success = bigint_create_str(coadiutor, &(vm->bigC));
				success = bigint_create_str(bak, &vm->bigB) && success;
				FAILANDRETURN(!success, vm->error, "bad operand X2", NULLFN)
				success = (fnindex > 2) && (fnindex < 16);
				FAILANDRETURN(!success, vm->error, "bad bigint fn", NULLFN)
				if (fnindex > 2) {
					if ((fnindex - 3) < 7) { //void functions
						call2ParamBigIntVoidFunction(fnindex - 3, &vm->bigB, &(vm->bigC), &vm->bigA);
						FAILANDRETURN(((&vm->bigA)->length == -1), vm->error, "bigint div by 0", NULLFN)
						char* acc = &vm->acc[0];
						success = bigint_tostring (&vm->bigA, acc);
						success = addDblQuotes(acc) && success;
						FAILANDRETURN(!success, vm->error, "bigint fail", NULLFN)
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
		c.imag = 0.0;
		if (isComplexNumber(vm->coadiutor)) { //complex
			success = stringToComplex(vm->coadiutor, &c);
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
		} else {
			FAILANDRETURN(true, vm->error, "bad operand.A", NULLFN)
		}
		FAILANDRETURN(!success, vm->error, "bad operand.A2", NULLFN)
		d.imag = 0.0;
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
			FAILANDRETURN(true, vm->error, "bad operand.B", NULLFN)
		}
		FAILANDRETURN(!success, vm->error, "bad operand.B2", NULLFN)
		//SerialPrint(1, "fnOrOp2Param:------------------- d = %g + %gi", d.real, d.imag);
		//call 2-parameter function
		c = call2ParamMathFunction(fnindex - 1, d, c);
		if (abs(c.real) < DOUBLE_EPS) c.real = 0.0;
		if (abs(c.imag) < DOUBLE_EPS) c.imag = 0.0;
		//printf("fnOrOp2Param: rounded c.real = %g c.imag = %g\n", c.real, c.imag);
		success = complexToString(c, vm->coadiutor) && success; //result in coadiutor
		SerialPrint(5, "fnOrOp2Param: 2 ------------------- got ", token, " data returned from function = ", vm->coadiutor, "\r\n");
		//printf ("-------------------success = %d, acc = %s\n", success, vm->coadiutor);
		FAILANDRETURNVAR(!success, vm->error, "Bad fn %s", fitstr(vm->coadiutor, token, 8))
		strcpy(vm->bak, vm->acc); //backup
		strcpy(vm->acc, vm->coadiutor);
		//when no errors are present, actually pop the vars
		pop(&vm->userStack, NULL);
		pop(&vm->userStack, NULL);
		push(&vm->userStack, vm->acc, METANONE);
	}
	return true;
}

bool fn1Param(Machine* vm, const char* token, int fnindex) {
	bool ifCondition, doingIf, conditional;
	size_t execData = UintStackPeek(&vm->execStack);
	FAILANDRETURN((vectorMatrixData(execData) != 0), vm->error, "No scalar op here.Z", NULLFN)
	ifCondition = doingIf = false;
	GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)
	//SerialPrint(1, "is1ParamFunction:------------------- execData = %lu", execData);
	if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
		int8_t meta;
		char debug0[VSHORT_STRING_SIZE];
		meta = peek(&vm->userStack, vm->coadiutor);
		FAILANDRETURN((meta == -1), vm->error, "stack empty.D", NULLFN)
		FAILANDRETURN((meta != METANONE), vm->error, "No scalar op here.Y", NULLFN)
		peek(&vm->userStack, vm->acc);
		if (vm->acc[0] != '\0') {
			ComplexDouble c;
			bool success = false;
			c.imag = 0.0;
			if (isComplexNumber(vm->acc)) { //complex
				success = stringToComplex(vm->acc, &c);
			} else if (isRealNumber(vm->acc)) { //real number
				success = stringToDouble(vm->acc, &c.real);
			}
			FAILANDRETURNVAR(!success, vm->error, "%s bad.A", fitstr(vm->coadiutor, token, 8))
			doubleToString((double)success, debug0);
			SerialPrint(3, "fn1Param: ---  success = ", debug0, "\r\n");
			if (fnindex < NUMMATH1PARAMFN)
				c = call1ParamMathFunction(fnindex, c);
			else if (fnindex < NUMMATH1PARAMFN + NUMREAL1PARAMFN)
				c.real = call1ParamRealFunction(fnindex - NUMMATH1PARAMFN, c);
			FAILANDRETURNVAR((c.real == INFINITY || c.imag == INFINITY || c.real == -INFINITY || c.imag == -INFINITY), 
				vm->error, "'%s' inf!", fitstr(vm->coadiutor, token, 8))
			if (abs(c.real) < DOUBLEFN_EPS) c.real = 0.0;
			if (abs(c.imag) < DOUBLEFN_EPS) c.imag = 0.0;
			success = complexToString(c, vm->acc);
			FAILANDRETURNVAR(!success, vm->error, "%s bad.B", fitstr(vm->coadiutor, token, 8))
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
		//SerialPrint(1, "-----cleanupVec: do-while loop: meta = %d -- %s -- popped data = %s", meta, DEBUGMETA[meta], vm->coadiutor);
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
	//SerialPrint(1, "fnOrOpVec1Param:------------------- execData = %lu", execData);
	//lcase(token);
	if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
		int8_t meta;
		char varName[MAX_VARNAME_LEN];
		ComplexDouble crunningsum = makeComplex(0.0, 0.0);
		ComplexDouble crunningsqsum = makeComplex(0.0, 0.0);
		ComplexDouble crunningrsum = makeComplex(0.0, 0.0);
		ComplexDouble crunning = makeComplex(0.0, 0.0);
		ComplexDouble c;
		ComplexDouble numTCount = makeComplex(0.0, 0.0);
		uint8_t count = 0;
		bool vectorFromStack = true;//get vector from stack, not variables
		meta = peek(&vm->userStack, NULL);
		FAILANDRETURN((meta == -1), vm->error, "Stack empty", NULLFN)		
		//SerialPrint(1, "fnOrOpVec1Param:------------------- called with token = %s fnindex = %d", token, fnindex);
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
			FAILANDRETURN((variableIndex == -1), vm->error, "Expected vector", NULLFN)
			if (variableIndex >= 0) {
				numTCount = fetchVariableComplexValue(&vm->ledger, VACC);
				FAILANDRETURN((numTCount.real == 0.0), vm->error, "Expected vector. A", NULLFN)
				FAILANDRETURN((numTCount.imag != 0.0), vm->error, "Expected vector. B", NULLFN)
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
				//SerialPrint(1, "-----fnOrOpVec1Param: do-while loop: meta = %d -- %s -- popped data = %s", meta, DEBUGMETA[meta], vm->coadiutor);
				success = false;
				c.imag = 0.0;
				if (isComplexNumber(vm->coadiutor)) { //complex
					success = stringToComplex(vm->coadiutor, &c);
				} else if (isRealNumber(vm->coadiutor)) { //real number
					success = stringToDouble(vm->coadiutor, &c.real);
				} else {
					FAILANDRETURN(true, vm->error, "Unexpected string", NULLFN)
				}
				FAILANDRETURNVAR(!success, vm->error, "Math fn %s failed", fitstr(vm->coadiutor, token, 9))
				if (fnindex != -1) strcpy(varName, VACC); //vector operation
				else strncpy(varName, token, MAX_VARNAME_LEN); //vector pop
				makeStrNum(varName, MAX_VARNAME_LEN, count);
				//SerialPrint(1, "-----fnOrOpVec1Param: do-while loop: from stack: variable name is %s of value (%g, %g)", varName, c.real, c.imag);
				createVariable(&vm->ledger, varName, VARIABLE_TYPE_COMPLEX, c, "");
			} else { //from variable
				//since it's from a variable, we can do a vector operation but not pop into a vector
				if (count == (int)numTCount.real) break;
				strcpy(varName, VACC); //vector operation -- can't have pop
				makeStrNum(varName, MAX_VARNAME_LEN, count);
				c = fetchVariableComplexValue(&vm->ledger, varName);
				//SerialPrint(1, "-----fnOrOpVec1Param: do-while loop: from memory: variable name is %s of value (%g, %g)", varName, c.real, c.imag);
			}
			crunningsum = suminternal(crunningsum, c);
			crunningsqsum = suminternal(crunningsqsum, cmul(c, c));
			crunningrsum = suminternal(crunningrsum, cdiv(makeComplex(1.0, 0.0), c));
			count++;
		} while (meta != METASTARTVECTOR); //start of the vector or empty stack

		if (fnindex != -1) strcpy(varName, VACC); //vector operation
		else strncpy(varName, token, MAX_VARNAME_LEN); //vector pop
		if (vectorFromStack)
			createVariable(&vm->ledger, varName, VARIABLE_TYPE_VECMAT, MKCPLX(count), ""); //the meta vector var - num of elements
		if (!isVoidFunction) {
			//not vector vector pop
			crunning = callVectorMath1ParamFunction(fnindex, crunningsum, crunningsqsum, crunningrsum, count);
			c.real = crealpart(crunning);
			c.imag = cimagpart(crunning);
			success = complexToString(c, vm->coadiutor);
			FAILANDRETURNVAR(!success, vm->error, "Math fn %s failed", fitstr(vm->coadiutor, token, 9))
			strcpy(vm->bak, vm->acc); //backup
			strcpy(vm->acc, vm->coadiutor);
			push(&vm->userStack, vm->acc, METANONE);
		}
	}
	return true;
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
	//SerialPrint(1, "------------------- PRINT - called with token = %s", token);
	
	if (strcmp(token, "va") == 0) {
		//FIXME
	} else if (strcmp(token, "b") == 0) {
		//SerialPrint(1, "------------------- PRINT - found ?bak");
		SerialPrint(3, "bak = ", vm->bak, "\r\n");
	} else if (strcmp(token, "a") == 0) {
		//SerialPrint(1, "------------------- PRINT - found ?acc");
		SerialPrint(3, "acc = ", vm->acc, "\r\n");
	} else if (token[0] == '"' && token[strlen(token)-1] == '"') {
		//immediate string
		token = removeDblQuotes(token);
		SerialPrint(2, token, "\r\n");
	} else if (variableVetted(token)) {
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
			if (complexToString(c, vm->coadiutor)) { 
				SerialPrint(4, token, " = ", vm->coadiutor, "\r\n");
			}
		} else if (vt == VARIABLE_TYPE_STRING) {
			if (getVariableStringValue(&vm->ledger, token, vm->coadiutor)) { //returns true
				SerialPrint(4, token, " = ", vm->coadiutor, "\r\n");
			}
		} else {
			FAILANDRETURNVAR(true, vm->error, "no variable %s", fitstr(vm->coadiutor, token, 10))
		}
	} else {
		FAILANDRETURNVAR(true, vm->error, "no variable %s", fitstr(vm->coadiutor, token, 10))
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
		else FAILANDRETURN(true, vm->error, "illegal POP B", NULLFN)
	}
	return true;
}

bool processDefaultPush(Machine* vm, char* token, int8_t meta) {
	//SerialPrint(1, "processDefaultPush:-------------------Default condition with token = %s", token);
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
			//SerialPrint(1, "processDefaultPush:------------------- token = %s 'token[1]' = %d '0' = %d found T for %d = %s", token, token[1], '0', n, vm->coadiutor);
			FAILANDRETURN((localmeta == -1), vm->error, "stack full", NULLFN)
			push(&vm->userStack, vm->coadiutor, METANONE);
			return true;
		}
	} else if (strlen(token) == 3) {
		//handle T10 - T99
		if (token[0] == 'T' && token[1] <= '9' && token[1] > '0' && 
				token[2] <= '9' && token[2] >= '0') {
			int n = (int)((token[1] - '0') * 10 + (token[2] - '0'));
			int8_t localmeta = peekn(&vm->userStack, vm->coadiutor, n);
			FAILANDRETURN((localmeta == -1), vm->error, "stack full", NULLFN)
			push(&vm->userStack, vm->coadiutor, METANONE);
			return true;
		}
	}
	if (strcmp(token, "b") == 0) {
		push(&vm->userStack, vm->bak, meta);
	} else if (strcmp(token, "a") == 0) {
		push(&vm->userStack, vm->acc, meta);
		//SerialPrint(1, "-------------------Default condition - found acc");
	} else if (variableVetted(token) || (strcmp(token, "va") == 0)) {
		if (strcmp(token, "va") == 0) strcpy(token, VACC);;
		if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_VECMAT) {
			c = fetchVariableComplexValue(&vm->ledger, token);
			//SerialPrint(1, "-------------------Default condition - found token got c.real = %g", c.real);
			int count = (int) c.real;
			for (int i = (int) count - 1; i >= 0; i--) {
				int8_t localmeta;
				strcpy(varName, token);
				makeStrNum(varName, MAX_VARNAME_LEN, i);
				c = fetchVariableComplexValue(&vm->ledger, varName);
				if (i == (int) count - 1) localmeta = METASTARTVECTOR;
				else if (i == 0) localmeta = METAENDVECTOR;
				else localmeta = METAMIDVECTOR;
				//SerialPrint(1, "-------------------Default condition - found vacc i = %d got c.real = %g", i, c.real);
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
			FAILANDRETURNVAR(true, vm->error, "no var %s", fitstr(vm->coadiutor, token, 6))
		}
	} else {
		//SerialPrint(1, "processDefaultPush:------------------- ELSE Default condition with token = %s", token);
		double dbl;
		char rlc = strIsRLC(token);
		bool s2d = stringToDouble(token, &dbl);
		if (stringToComplex(token, &c) || s2d) {
			printf("processDefaultPush:------------------- 1. ELSE Default condition with token = %s, no-has-dbl-quotes = %d, dbl = %g\n\n", token, !hasDblQuotes(token), dbl);
			if (s2d) {
				doubleToString(dbl, vm->coadiutor);
				char RLCstr[2];
				RLCstr[1] = '\0';
				if (rlc) {
					RLCstr[0] = rlc;
					//printf("processDefaultPush: RLCstr = %s\n", RLCstr);
					strcat(vm->coadiutor, RLCstr);
				}
				push(&vm->userStack, vm->coadiutor, meta);
			} else {
				push(&vm->userStack, token, meta);
			}
		} else if (!hasDblQuotes(token) && meta != METASTARTVECTOR) {
			//don't want to add double quotes to '['
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
	//SerialPrint(1, "process:------------------- token = %s", token);
	//SerialPrint(1, "process:------------------- UintStackPeek = %lu", execData);
	int is1pfn = is1ParamFunction(token);
	int is2pfn = is2ParamFunction(token);
	int isvecfn = isVec1ParamFunction(token);
	if (is1pfn != -1) {
		//SerialPrint(1, "process:------------------- is1pfn = %d", is1pfn);
		//lcase(token);
		success = fn1Param(vm, token, is1pfn);
	} else if (is2pfn != -1) {
		//SerialPrint(1, "process:------------------- is2pfn|op = %d", is2pfn);
		//lcase(token);
		success = fnOrOp2Param(vm, token, is2pfn); //don't pass is2pop
	} else if (isvecfn != -1) {
		//lcase(token);
		success = fnOrOpVec1Param(vm, token, isvecfn, false);
	} else if (strcmp(token, "dup") == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "No vecmat here.C", NULLFN)
		int8_t meta = peek(&vm->userStack, vm->coadiutor);
		FAILANDRETURN((meta == -1), vm->error, "stack empty", NULLFN)
		FAILANDRETURN((meta != 0), vm->error, "no DUP here", NULLFN)
		if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
			push(&vm->userStack, vm->coadiutor, METANONE);
			return true;
		}
	} else if (strcmp(token, "tim") == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "No vecmat here.F", NULLFN)
		//conditional loop, check for previous if/else
		int8_t meta = peek(&vm->userStack, vm->coadiutor);
		FAILANDRETURN((meta == -1), vm->error, "stack empty", NULLFN)
		FAILANDRETURN((meta != 0), vm->error, "no TIM here", NULLFN)
		if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
			ComplexDouble c;
			rtc_disable_alarm();
			c.imag = 0.0;
			pop(&vm->userStack, vm->coadiutor); //c
			if (isRealNumber(vm->coadiutor)) { //real number
				success = stringToDouble(vm->coadiutor, &c.real);
				FAILANDRETURN(!success, vm->error, "Bad timer.A", NULLFN)
				if (c.real == 0.0) return true;
			} else if (isComplexNumber(vm->coadiutor)) { //complex
				success = stringToComplex(vm->coadiutor, &c);
				FAILANDRETURN(!success, vm->error, "Bad timer.B", NULLFN)
				if (c.real == 0.0 && c.imag == 0.0) return true;
			} else
				FAILANDRETURN(true, vm->error, "Bad timer.C", NULLFN)

			//we don't set the alarm struct values
			//to -1 for repeating alarm
			vm->repeatingAlarm = false;
			if (c.real < 0.0 || c.imag < 0.0) {
				vm->repeatingAlarm = true;
				if (c.real < 0)
					c.real = -c.real;
				if (c.imag < 0)
					c.imag = -c.imag;
			}

			int creal = (int) c.real;
			int cimag = (int) c.imag;

			//set the global values that will be used
			//by the IRQ handler if the timer is repeating
			if (c.imag > 0.0) { //sec and min were specified
				if (cimag > 84600) cimag = 84600;
				if (creal > 3600) creal = 3600;
				vm->alarmSec = (int) (cimag % 60);
				vm->alarmMin = (int) (creal % 60);
				vm->alarmHour = (int) (creal / 60);
			} else { //c.imag == 0.0
				if (creal > 84600) creal = 84600;
				vm->alarmSec = (int) (creal % 60);
				vm->alarmMin = (int) (creal / 60);
				vm->alarmHour = (int) (creal / 3600);
			}
			vm->alarmHour %= 24;

			datetime_t alarm;
			alarm.year  = 2023;
			alarm.month = 9;
			alarm.day   = 15;
			alarm.dotw  = 5; // Friday; 0 is Sunday
			alarm.sec = 0;
			alarm.min = 0;
			alarm.hour = 0;
			rtc_set_datetime(&alarm);
			alarm.sec = vm->alarmSec;
			alarm.min = vm->alarmMin;
			alarm.hour = vm->alarmHour;
			rtc_set_alarm(&alarm, &toggleLED);
		}
		return true;
	} else if (strcmp(token, "do") == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "No vecmat here.F", NULLFN)
		//conditional loop, check for previous if/else
		UintStackPush(&vm->execStack, (1<<(sizeof(size_t)-1)));
		return true;
	} else if (strcmp(token, IFTOKEN) == 0) {
		//SerialPrint(1, "-------------------Found if -- now popping vm->userStack");
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "No vecmat here.G", NULLFN)
		strcpy(vm->bak, vm->acc); //backup
		pop(&vm->userStack, vm->acc);
		ComplexDouble conditionComplex;
		success = false;
		if (isComplexNumber(vm->acc)) { //complex
			success = stringToComplex(vm->acc, &conditionComplex);
			ifCondition = !alm0double(conditionComplex.real) || !alm0double(conditionComplex.imag);
		} else if (isRealNumber(vm->acc)) { //real number
			success = stringToDouble(vm->acc, &conditionComplex.real);
			ifCondition = !alm0double(conditionComplex.real);
			//SerialPrint(1, "-------------------Found if: conditionComplex.real = %.15g", conditionComplex.real);
		} else { //string
			ifCondition = false;
		}
		//SerialPrint(1, "-------------------Found if: ifCondition = %d", ifCondition);
		execData = makeExecStackData(true, ifCondition, true, false, false); //doingIf = true
		//SerialPrint(1, "-------------------Found if: execData calculated = %lu", execData);
		UintStackPush(&vm->execStack, execData);
		return true;
	} else if (strcmp(token, ELSETOKEN) == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "No vecmathere.H", NULLFN)
		//SerialPrint(1, "-------------------Found else -- execData = %lu", execData);
		FAILANDRETURN((conditionalData(execData) == 0x0), vm->error, "else without if. A", NULLFN)
		FAILANDRETURN(!doingIf, vm->error, "el w/o if", NULLFN)
		FAILANDRETURN(UintStackIsEmpty(&vm->execStack), vm->error, "el w/o if.B", NULLFN)
		UintStackPop(&vm->execStack); //discard the if
		execData = makeExecStackData(true, ifCondition, false, false, false); //else
		//SerialPrint(1, "-------------------Found else -- set execData to = %lu", execData);
		UintStackPush(&vm->execStack, execData);
		return true;
	} else if (strcmp(token, ENDIFTOKEN) == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "No vecmat here.I", NULLFN)
		FAILANDRETURN((conditionalData(execData) == 0x0), vm->error, "fi w/o if. A", NULLFN)
		FAILANDRETURN(UintStackIsEmpty(&vm->execStack), vm->error, "fi w/o if. B", NULLFN)
		UintStackPop(&vm->execStack); //discard the if/else
		return true;
	} else {
		uint8_t meta = peek(&vm->userStack, vm->coadiutor);
		FAILANDRETURN((strcmp(token, MATSTARTTOKEN) == 0 || 
				strcmp(token, MATLASTTOKEN) == 0), vm->error, 
				"Dangling '['s", NULLFN)
		//SerialPrint(1, "-------------------Default condition with token = %s", token);
		bool doingMat = vectorMatrixData(execData) == 0x2;
		bool doingVec = vectorMatrixData(execData) == 0x1;
		//SerialPrint(1, "-------------------Default -- execData >> 3 = %lu", execData >> 3);
		//last instruction on exec stack was not an IF or ELSE

		//SerialPrint(1, "-------------------Default: conditional = %d ", conditional);
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
						FAILANDRETURN(!doingMat, vm->error, "Expect mat end.A", NULLFN)
						success = processDefaultPush(vm, &token[1], METAENDMATRIX);
					} else {
						//aa]]
						//SerialPrint(1, "-------------------Default -- aa]] Current execData >> 3 = %lu", execData >> 3);
						FAILANDRETURN(!doingMat, vm->error, "Expect mat end.B", NULLFN)
						//matrix is done
						execData = UintStackPop(&vm->execStack);
						success = processDefaultPush(vm, token, METAENDMATRIX);
					}
				} else if (token_0_eq_VECSTARTTOKENC){ //vector starting and ending
					//[aa]
					if (len_m_1 < 2) return false; //too short
					FAILANDRETURN((len_m_1 < 2), vm->error, "Syntax error.A", NULLFN)
					token[len_m_1] = '\0';
					success = processDefaultPush(vm, &token[1], METANONE);
				} else { //vector ending
					//a] or ]
					if (len_m_1 == 0) { //just a ']'
						FAILANDRETURN(!doingMat && !doingVec, vm->error, "Bad vec end", NULLFN)
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
					FAILANDRETURN(doingMat, vm->error, "Nested mats.A", NULLFN)
					if (!doingVec) { //already vector entry is NOT going on; another '[' starts a matrix entry
						execData = makeExecStackData(false, false, false, false, true); //vector
						UintStackPush(&vm->execStack, execData);
						token[0] = VECSTARTINDICATOR; //ASCII SYNC
						token[1] = '\0';
						success = processDefaultPush(vm, token, METASTARTVECTOR);
					} else {
						//already a vector going on, so this will be start of matrix entry
						//if TOS of userStack has VECSTARTINDICATOR, then ok, else error
						SerialPrint(1, "#### UNSUPPORTED #### UNSUPPORTED ####\r\n");
					}
				} else if (token_1_eq_VECSTARTTOKENC) {//matrix starting
					//[[aa
					FAILANDRETURN(doingMat, vm->error, "Nested mats.B", NULLFN)
					execData = makeExecStackData(false, false, false, true, false); //matrix
					UintStackPush(&vm->execStack, execData);
					success = processDefaultPush(vm, &token[2], METASTARTMATRIX);
				} else { //vector starting
					//[a
					//SerialPrint(1, "-------------------Default -- [a Current execData >> 3 = %lu", execData >> 3);
					FAILANDRETURN(doingVec, vm->error, "Nested vecs.C", NULLFN)
					//in above check for previous vector being the ASCII SYNC char
					FAILANDRETURN(doingMat, vm->error, "Nested mats.C", NULLFN)
					if (!doingVec) { //last '[' entry means another '[' starts a matrix entry
						execData = makeExecStackData(false, false, false, false, true); //vector
						UintStackPush(&vm->execStack, execData);
					}
					success = processDefaultPush(vm, &token[1], METASTARTVECTOR);
				}
			} else if (token[0] == POPTOKENC) { //pop to register or variable
				//SerialPrint(1, "------------------- Got POPTOKEN calling processPop with %s", &token[1]);
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
					//SerialPrint(1, "-------------------Default condition - push token = %s which is char %d", token, (int)token[0]);
					//SerialPrint(1, "-------------------STACKPUSH: execData  = %lu", execData);	
					if (meta == METASTARTVECTOR && (vm->coadiutor[0] == VECSTARTINDICATOR)) {
						pop(&vm->userStack, vm->coadiutor);
						//pop out the placeholder and push the current token, marking as vector start
						success = processDefaultPush(vm, token, METASTARTVECTOR);
					} else {
						success = processDefaultPush(vm, token, METAMIDVECTOR);
					}
				} else if (doingMat) //a matrix end is also a vector end
					success = processDefaultPush(vm, token, METAMIDMATRIX); //mid matrix is also mix vector
				else if (token[0] != '\0')
					success = processDefaultPush(vm, token, METANONE);
			}
		}
	}
	return success;
}

void interpret(Machine* vm, char* sourceCode) {
	char* input;
	char output[STRING_SIZE];
	size_t branchIndex = 0;
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
		success = process(vm, output, branchIndex);
		branchIndex = input - vm->SOURCECODEMEM;
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
	vm->userInputPos = 0;
	vm->shiftState = 0;
	vm->altState = 0;
	vm->viewPage = 0;
	vm->modeDegrees = 0;
	vm->insertState = 0;
	vm->userInputLeftOflow = 0;
	vm->userInputRightOflow = 0;
	vm->userInputCursorEntry = 1; //char DISPLAY_LINESIZE - 1 is empty and for data entry
	vm->cursorPos = DISPLAY_LINESIZE - 1; //between 0 and 19, both inclusive
	vm->timerRunning = false;
	vm->repeatingAlarm = false;
	vm->LEDState = false;
}

char* ltrim(char* str) {
	if (str[0] == '\0') return str;
	return str + strspn(str, " \t\r\f\v\n");
	//while (isspace(*str)) str++;
	//return str;
}

