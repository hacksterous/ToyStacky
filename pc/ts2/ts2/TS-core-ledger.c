#include <string.h>
#include "TS-core-miscel.h"
#include "TS-core-math.h"
#include "TS-core-numString.h"

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
	int32_t stringLength = strlen(stringValue);
	
	if ((stringLength == 0) && (type == VARIABLE_TYPE_STRING || type == VARIABLE_TYPE_VECMAT)) return NULL;
	if (stringLength > 0) stringLength++;

	if (ledger->memoryOffset + stringLength > MEMORY_SIZE) {
		//SerialPrint(1, "createVariable: just entered FAIL ledger->memoryOffset + stringLength > MEMORY_SIZE");
		return NULL;
	}
	uint32_t index = hash(name, MAX_VARIABLES);
	//SerialPrint(1, "createVariable: just entered ledger->varCount is %lu; creating variable %s with type %d", ledger->varCount, name, (int) type);
	Variable* variable = &ledger->variables[index];

	zstrncpy(variable->name, name, sizeof(variable->name) - 1);
	variable->name[sizeof(variable->name) - 1] = '\0';
	variable->type = type;

	if (type == VARIABLE_TYPE_STRING || type == VARIABLE_TYPE_VECMAT) {
		variable->value.stringValueIndex = ledger->memoryOffset;
		//SerialPrint(1, "createVariable: stringValueIndex is set to %X for variable %s", (unsigned int)variable->value.stringValueIndex, name);
		zstrncpy(ledger->memory + ledger->memoryOffset, stringValue, stringLength);
		ledger->memoryOffset += stringLength;
	} else if (type == VARIABLE_TYPE_COMPLEX) {
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
	if (strcmp(variable->name, name) == 0 && (variable->type == VARIABLE_TYPE_COMPLEX)) {
		value->real = variable->value.doubleValue.real;
		value->imag = variable->value.doubleValue.imag;
		return true;
	} else return false;
}

bool getVariableStringVecMatValue(Ledger* ledger, const char* name, char* value) {
	uint32_t index = hash(name, MAX_VARIABLES);
	Variable* variable = &ledger->variables[index];
	if (variable->name[0] == '\0') return NULL;
	if (strcmp(variable->name, name) == 0 && (variable->type == VARIABLE_TYPE_STRING || variable->type == VARIABLE_TYPE_VECMAT)) {
		strcpy(value, ledger->memory + variable->value.stringValueIndex);
		return true;
	} else return false;
}

ComplexDouble fetchVariableComplexValue(Ledger* ledger, const char* name) {
	uint32_t index = hash(name, MAX_VARIABLES);
	Variable* variable = &ledger->variables[index];
	if (variable->name[0] == '\0') return MKCPLX(0);
	if (strcmp(variable->name, name) == 0 && (variable->type == VARIABLE_TYPE_COMPLEX)) {
		//SerialPrint(1, "fetchVariableComplexValue: returning with value.real = %g for var '%s'", variable->value.doubleValue.real, name);
		return variable->value.doubleValue;
	} else return MKCPLX(0);
}

char* fetchVariableStringVecMatValue(Ledger* ledger, const char* name) {
	uint32_t index = hash(name, MAX_VARIABLES);
	Variable* variable = &ledger->variables[index];
	if (variable->name[0] == '\0') return NULL;
	if (strcmp(variable->name, name) == 0 && (variable->type == VARIABLE_TYPE_STRING || variable->type == VARIABLE_TYPE_VECMAT)) {
		return ledger->memory + variable->value.stringValueIndex;
	} else return NULL;
}

void updateComplexVariable(Ledger* ledger, const char* name, ComplexDouble doubleValue) {
	for (int32_t i = 0; i < ledger->varCount; i++) {
		Variable* variable = &ledger->variables[i];
		if (strcmp(variable->name, name) == 0 && (variable->type == VARIABLE_TYPE_COMPLEX)) {
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
		if (variable->type == VARIABLE_TYPE_STRING || variable->type == VARIABLE_TYPE_VECMAT) {
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

bool updateLedger(Ledger* ledger, int32_t memIndexThreshold, int memIndexIncr) {
	if (ledger->varCount == 0) return true; //nothing to update
	int32_t varCount = 0;
	int32_t i = 0;
	while (i < MAX_VARIABLES) {
		Variable* variable = &ledger->variables[i++];
		//SerialPrint(1, "updateLedg===================== varCount = %d", varCount);
		if (varCount >= ledger->varCount) break;
		if (variable->name[0] == '\0') continue; //skip the empty variable pods
		if (variable->type == VARIABLE_TYPE_STRING || variable->type == VARIABLE_TYPE_VECMAT) {
			if (variable->value.stringValueIndex > memIndexThreshold) {
				if (variable->value.stringValueIndex + memIndexIncr > MEMORY_SIZE) return false;
				if (variable->value.stringValueIndex + memIndexIncr < 0) return false;
				variable->value.stringValueIndex += memIndexIncr;
				//SerialPrint(1, "updateLedgVariable %s at vartable index %d: updated memoryIndex to %s", variable->name, i, ledger->memory + variable->value.stringValueIndex);
			}
		}
		varCount++;
	}
	return true;
}

bool updateStringVecMatVariable(Ledger* ledger, const char* name, const char* newString) {
	int variableIndex = findVariable(ledger, name);
	if (variableIndex == -1) {
		return false;  // variable not found
	}

	int32_t newSize = strlen(newString);

	Variable* variable = &ledger->variables[variableIndex];
	if (variable->type == VARIABLE_TYPE_COMPLEX) {
		return false;  // variable is not of type VARIABLE_TYPE_STRING or VARIABLE_TYPE_VECMAT
	}

	char* stringValue = ledger->memory + variable->value.stringValueIndex;
	int32_t currentSize = strlen(stringValue);

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

	zstrncpy(stringValue, newString, newSize);
	return true;  // No need to resize
}

void compactMemory(Ledger* ledger) {
	int compactOffset = 0;

	int32_t varCount = 0;
	int32_t i = 0;
	while (i < MAX_VARIABLES) {
		Variable* variable = &ledger->variables[i++];
		if (varCount >= ledger->varCount) break;
		//SerialPrint(1, "compactMemory ===================== varCount = %d", varCount);
		if (variable->name[0] == '\0') continue; //skip the empty variable pods
		if (variable->type == VARIABLE_TYPE_STRING || variable->type == VARIABLE_TYPE_VECMAT) {
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

