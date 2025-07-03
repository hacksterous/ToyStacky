#ifndef __TS_CORE_LEDGER__
#define __TS_CORE_LEDGER__
void initializeLedger(Ledger* ledger);
Variable* createVariable(Ledger* ledger, const char* name, VariableType type, ComplexDouble doubleValue, const char* stringValue);
bool getVariableComplexValue(Ledger*, const char*, ComplexDouble*);
bool getVariableStringVecMatValue(Ledger* ledger, const char* name, char* value);
ComplexDouble fetchVariableComplexValue(Ledger* ledger, const char* name);
char* fetchVariableStringVecMatValue(Ledger* ledger, const char* name);
void updateComplexVariable(Ledger* ledger, const char* name, ComplexDouble doubleValue);
void deleteVariable(Ledger* ledger, const char* name);
int findVariable(Ledger* ledger, const char* name);
bool updateLedger(Ledger* ledger, int32_t memIndexThreshold, int memIndexIncr);
bool updateStringVecMatVariable(Ledger* ledger, const char* name, const char* newString);
void compactMemory(Ledger* ledger);
#endif
