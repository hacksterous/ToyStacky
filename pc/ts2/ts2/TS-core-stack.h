#ifndef __TS_CORE_STACK__
#define __TS_CORE_STACK__
#include "TS-core-miscel.h"
extern const char* DEBUGMETA[6];

#define METABARRIER 0x8

void stackInit(Strack *s);
bool stackIsEmpty(Strack *s);
int stackIsFull(Strack *s);
bool push(Strack *s, char* value, int8_t meta);
int8_t pop(Strack *s, char* output);
int8_t peek(Strack *s, char output[STRING_SIZE]);
int8_t peekn(Strack* s, char output[STRING_SIZE], int n);
int8_t popbarrier(Strack *s, char* output);
int8_t peekbarrier(Strack *s, char output[STRING_SIZE]);
int8_t peeknbarrier(Strack* s, char output[STRING_SIZE], int n);
void UintStackInit(UintStack *s);
bool UintStackIsEmpty(UintStack *s);
bool UintStackIsFull(UintStack *s);
bool UintStackPush(UintStack *s, char value);
int32_t UintStackPop(UintStack *s);
int32_t UintStackPeek(UintStack *s);
int makeExecStackData(bool conditional, bool ifCondition, bool doingIf);
bool popNItems(Machine* vm, int maxCount);
#endif
