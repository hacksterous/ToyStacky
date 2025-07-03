#include "TS-core-stack.h"
#include <string.h>

void stackInit(Strack *s) {
	s->topStr = -1;
	s->topLen = -1;
	s->itemCount = 0;
}

bool stackIsEmpty(Strack *s) {
	//if (s->topLen == -1 && s->topStr != -1)
		//SerialPrint(1, "Stack warning: Strack top indices out of sync!\r\n");
	return (s->topLen == -1 || s->topStr == -1);
}

int stackIsFull(Strack *s) {
	return (s->topStr >= STACK_SIZE - 1 || s->topLen >= STACK_NUM_ENTRIES - 1);
}

bool push(Strack *s, char* value, int8_t meta) {
	int32_t len = strlen(value);
	if (len > 0) len++; //include the '\0'
	if (s->topStr + len >= STACK_SIZE - 1 || s->topLen == STACK_SIZE - 1) {
		return false;
	}
	s->stackLen[++s->topLen] = len + (((int8_t)meta & 0xf) << 28); //meta
	strcpy(&s->stackStr[s->topStr + 1], value);
	s->topStr += len;

	s->itemCount++;
	return true;
}

int8_t pop(Strack *s, char* output) {
	if (stackIsEmpty(s)) {
		output = NULL;
		return -1;
	}
	int8_t meta = (s->stackLen[s->topLen] >> 28) & 0x7; //keep bit[3] for BARRIER
	int32_t len = s->stackLen[s->topLen--] & 0x0fffffff;
	if (output)
		strcpy(output, &s->stackStr[s->topStr - len + 1]);
	s->topStr -= len;
	s->itemCount--;
	return meta;
}

int8_t peek(Strack *s, char output[STRING_SIZE]) {
	if (stackIsEmpty(s)) {
		output = NULL;		
		return -1;
	}
	int8_t meta = (s->stackLen[s->topLen] >> 28) & 0x7; //keep bit[3] for BARRIER
	int32_t len = s->stackLen[s->topLen] & 0x0fffffff; 
	if (output)
		strcpy(output, &s->stackStr[s->topStr - len + 1]);
	return meta;
}

int8_t peekn(Strack* s, char output[STRING_SIZE], int n) {
	//n = 0: return T
	//n = 1: return T - 1
	if (n == 0) return peek(s, output);
	if (stackIsEmpty(s) || (n > s->topLen)) {
		output = NULL;
		return -1;
	}
	n++;

	char* stringPtr;
	int32_t stringLen = -1;
	int8_t meta;
	for (int i = 0; i < n; i++) {
		stringLen += s->stackLen[s->topLen - i] & 0x0fffffff;
	}
	//now s->topStr - stringLen will point to the first string
	stringPtr = &s->stackStr[s->topStr - stringLen];
	meta = (s->stackLen[s->topLen - n + 1] >> 28) & 0x7; //keep bit[3] for BARRIER
	if (output)
		strcpy(output, stringPtr);
	return meta;
}

//the 'barrier' versions of pop, peek and peekn provide the METABARRIER bit value
int8_t popbarrier(Strack *s, char* output) {
	if (stackIsEmpty(s)) {
		output = NULL;
		return -1;
	}
	int8_t meta = (s->stackLen[s->topLen] >> 28) & 0xf;
	int32_t len = s->stackLen[s->topLen--] & 0x0fffffff;
	if (output)
		strcpy(output, &s->stackStr[s->topStr - len + 1]);
	s->topStr -= len;
	s->itemCount--;
	return meta;
}

int8_t peekbarrier(Strack *s, char output[STRING_SIZE]) {
	if (stackIsEmpty(s)) {
		output = NULL;
		return -1;
	}
	int8_t meta = (s->stackLen[s->topLen] >> 28) & 0xf;
	int32_t len = s->stackLen[s->topLen] & 0x0fffffff; 
	if (output)
		strcpy(output, &s->stackStr[s->topStr - len + 1]);
	return meta;
}

int8_t peeknbarrier(Strack* s, char output[STRING_SIZE], int n) {
	//n = 0: return T
	//n = 1: return T - 1
	//printf("Called peeknbarrier with n = %d stackEmpty = %d topLen = %d\n", n, stackIsEmpty(s), s->topLen);
	if (n == 0) return peekbarrier(s, output);
	if (stackIsEmpty(s) || (n > s->topLen)) {
		output = NULL;
		return -1;
	}
	n++;

	char* stringPtr;
	int32_t stringLen = -1;
	int8_t meta;
	for (int i = 0; i < n; i++) {
		stringLen += s->stackLen[s->topLen - i] & 0x0fffffff;
	}
	//now s->topStr - stringLen will point to the first string
	stringPtr = &s->stackStr[s->topStr - stringLen];
	meta = (s->stackLen[s->topLen - n + 1] >> 28) & 0xf;
	//printf("Called peeknbarrier with n = %d now returning with %s\n", n, stringPtr);
	if (output)
		strcpy(output, stringPtr);
	return meta;
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

int32_t UintStackPop(UintStack *s) {
	if (UintStackIsEmpty(s)) {
		return 0;
	}
	return s->stack[s->top--];
}

int32_t UintStackPeek(UintStack *s) {
	if (UintStackIsEmpty(s)) {
		//SerialPrint(1, "Peek: Stack is empty");
		return 0;
	}
	return s->stack[s->top];
}

int makeExecStackData(bool conditional, bool ifCondition, bool doingIf) {
	//exactly one of the last two parameters can be true
	return  (conditional << 2) + (ifCondition << 1) + doingIf;
}

bool popNItems(Machine* vm, int maxCount) {
	int count = 0;
	if (maxCount == 0) {
		stackInit(&vm->userStack);
		return true;
	}
	while (!stackIsEmpty(&vm->userStack) && count < maxCount) {
		//if count is 0, clean up the stack
		pop(&vm->userStack, NULL);
		count++;
	}
	return true;
}

