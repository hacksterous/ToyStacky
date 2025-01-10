void stackInit(Strack *s) {
	s->topStr = -1;
	s->topLen = -1;
	s->itemCount = 0;
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
	int32_t len = strlen(value);
	if (len > 0) len++; //include the '\0'
	if (s->topStr + len >= STACK_SIZE - 1 || s->topLen == STACK_SIZE - 1) {
		return false;
	}
	//SerialPrint(1, "push: s->topStr = %d", s->topStr);
	//SerialPrint(1, "push: s->topLen = %d", s->topLen);
	s->stackLen[++s->topLen] = len + (((int8_t)meta & 0xf) << 28); //meta
	strcpy(&s->stackStr[s->topStr + 1], value);
	//SerialPrint(1, "push: string value = %s", &s->stackStr[s->topStr + 1]);
	s->topStr += len;

	//SerialPrint(1, "push: NOW s->topStr = %d", s->topStr);
	//SerialPrint(1, "push: NOW s->topLen = %d", s->topLen);
	//SerialPrint(1, "push: returning--------");
	s->itemCount++;
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
	int8_t meta = (s->stackLen[s->topLen] >> 28) & 0xf;
	int32_t len = s->stackLen[s->topLen--] & 0x0fffffff;
	//SerialPrint(1, "pop: length retrieved = %lu", len);
	if (output)
		strcpy(output, &s->stackStr[s->topStr - len + 1]);
	//SerialPrint(1, "pop: string output = %s", output);
	s->topStr -= len;
	//SerialPrint(1, "pop: NOW s->topStr = %d", s->topStr);
	//SerialPrint(1, "pop: NOW s->topLen = %d", s->topLen);
	//SerialPrint(1, "pop: returning--------");
	s->itemCount--;
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
	int8_t meta = (s->stackLen[s->topLen] >> 28) & 0xf;
	int32_t len = s->stackLen[s->topLen] & 0x0fffffff; 
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
	int32_t stringLen = -1;
	int8_t meta;
	for (int i = 0; i < n; i++) {
		//SerialPrint(1, "peekn: s->stackLen[%d] = %lu", i, s->stackLen[s->topLen - i]);
		stringLen += s->stackLen[s->topLen - i] & 0x0fffffff;
	}
	//now s->topStr - stringLen will point to the first string
	stringPtr = &s->stackStr[s->topStr - stringLen];
	meta = (s->stackLen[s->topLen - n + 1] >> 28) & 0xf;
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
	//SerialPrint(1, "Popping from UintStack");
	if (UintStackIsEmpty(s)) {
		//SerialPrint(1, "Pop warning: UintStack is empty");
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

