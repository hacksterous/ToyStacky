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

int calcRightOFPos(char* str) {
	int slen = strlen(str);
	for (int i = slen - 1; i >= 0; i--) {
		if ((str[i] == 'e') || (str[i] == 'E'))
			return i - 1;
	}
	return slen - 1;
}

int makeStringFit(char*dest, const char* src, int len) {
	//returns 1 if string is to be shortened, 0 if it fits in the size
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

			//NOTE: we could have /should have returned 1 
			//as the non-expo format number has been shortened
			return 0;
		} else {
			//printf("------------makeStringFit: BBB slen %d <= len %d\n", slen, len);
			strncpy(dest, srcCopy, slen);
			dest[slen] = '\0';
			//printf("------------makeStringFit - C dest = %s len of dest = %lu\n", dest, strlen(dest));
			//printf("------------makeStringFit: Returning. C dest = ", dest);
			return 0;
		}
	}

	//Expo-format number
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
		charPos = makeStringFit(re, src, NUMBER_LINESIZE);
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
		//charPos = makeStringFit(im, re, len - 2); //keep space for '(' & ')'
		charPos = makeStringFit(im, re, NUMBER_LINESIZE - 2); //keep space for '(' & ')'
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
		//makeStringFit(im, re, ((len - 2) >> 1) - 1);
		makeStringFit(im, re, ((NUMBER_LINESIZE - 2) >> 1) - 1);
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

#include "TS-core-showStack.h"

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
		//lcd.write(LEFTOFIND);
		lcd.print(LEFTOFIND);
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

#include "TS-core-math.h"

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

typedef double (*RealFunctionPtr)(ComplexDouble);
typedef void (*BigIntVoidFunctionPtr)(const bigint_t*, const bigint_t*, bigint_t*);
typedef int (*BigIntIntFunctionPtr)(const bigint_t*, const bigint_t*);
typedef ComplexDouble (*ComplexFunctionPtr1Param)(ComplexDouble);
typedef ComplexDouble (*ComplexFunctionPtr2Param)(ComplexDouble, ComplexDouble);
typedef ComplexDouble (*ComplexFunctionPtrVector)(ComplexDouble, ComplexDouble, ComplexDouble, int);

const char* mathfn1paramname[] = {
						"sin", 
						"cos",
						"tan", 
						"cot", 
						"sinh", 
						"cosh",
						"tanh", 
						"coth", 

						"asin",
						"acos", 
						"atan",
						"acot",
						"asinh",
						"acosh", 
						"atanh", 
						"acoth",
						
						"exp", "log10", 
						"log", "log2", 
						"sqrt", "cbrt",
						"conj",

						"abs", "arg", 
						"re", "im"
						}; //the 1 param functions
const int NUMMATH1PARAMFN = 23;
const int NUMMATH1PARAMTRIGFN = 8;
const int NUMMATH1PARAMTRIGANTIFN = 16;

//these 21 return ComplexDouble
ComplexFunctionPtr1Param mathfn1param[] = {	csine, 
											ccosine, 
											ctangent, 
											ccotangent, 
											csinehyp, 
											ccosinehyp, 
											ctangenthyp,
											ccotangenthyp,

											carcsine, 
											carccosine, 
											carctangent,
											carccotangent,
											carcsinehyp, 
											carccosinehyp, 
											carctangenthyp,
											carccotangenthyp,

											cexpo, clogarithm10, 
											cln, clog2, 
											csqroot, cbroot, 
											conjugate};

//these 13 return real double
RealFunctionPtr realfn1param[] = {abso, cargu, crealpart, cimagpart};
const int NUMREAL1PARAMFN = 4;

BigIntVoidFunctionPtr bigfnvoid2param[] = {bigint_max, bigint_min, bigint_add, bigint_sub, bigint_mul, bigint_div, bigint_rem};

BigIntIntFunctionPtr bigfnint2param[] = {bigint_gt, bigint_lt, bigint_gte, bigint_lte, bigint_eq,
									bigint_neq};

const char* miscelfn1paramname[] = {"dup"};

const char* vecprefnname[] = {"var", "stdev", "ssdev"}; //these functions require a pre subtraction operation
const char* vecfn1paramname[] = {"sum", "sqsum", "var", "sd", "mean", "rs"};
const int NUMVECFNS = 6;

const char* mathfnop2paramname[] = {"swp", 
						"pow", "atan2", 
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

#include "TS-core-tokenize.h"

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

#include "TS-core-fnOrOpVec2Param.h"

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

#include "TS-core-fnOrOp2Param.h"
#include "TS-core-fn1Param.h"

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

#include "TS-core-fnOrOpVec1Param.h"

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
	FAILANDRETURN((meta == -1), vm->error, "Stack empty", NULLFN)		
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
		} else if (strcmp(token, "@") == 0) { //@@
			//if last item was not a number or 0, all of stack is emptied			
			pop(&vm->userStack, vm->bak);
			if (stringToDouble(vm->bak, &dbl)) howMany = (int) dbl;
			else howMany = 0;
		} else if (strcmp(token, "!") == 0) { //@!
			//clear stack
			howMany = 0;
		}
		if (howMany >= 0) popNItems(vm, howMany);
		else FAILANDRETURN(true, vm->error, "illegal POP B", NULLFN); //should never happen
	}
	return true;
}

#include "TS-core-processDefaultPush.h"
#include "TS-core-process.h"

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
	vm->cmdState = 0;
	vm->altState = 0;
	vm->viewPage = 0;
	vm->modeDegrees = false;
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

