bool process(Machine* vm, char* token) {
	bool success = true;
	bool ifCondition, doingIf, conditional;
	ifCondition = doingIf = conditional = false;
	int32_t execData = UintStackPeek(&vm->execStack);

	GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)
	//printf("process:------------------- token = %s\n", token);
	//printf("process:------------------- UintStackPeek = %lu\n", execData);
	if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
		int is1pfn = is1ParamFunction(token);
		int is2pfn = is2ParamFunction(token);
		int isvecfn = isVec1ParamFunction(token);
		int isTrig = is1ParamTrigFunction(token);
		if (is1pfn != -1) {
			//printf("process:------------------- is1pfn = %d\n", is1pfn);
			success = fn1Param(vm, token, is1pfn, isTrig);
		} else if (is2pfn != -1) {
			//printf("process:------------------- is2pfn|op = %d\n", is2pfn);
			//could call 2-parameter vector function
			success = fnOrOp2Param(vm, token, is2pfn); //don't pass is2pop
		} else if (isvecfn != -1) {
			success = fnOrOpVec1Param(vm, token, isvecfn, false, false); //not a trig fn., result is not vector
		} else if (strcmp(token, "cmdpg") == 0) {
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN((meta != METANONE), vm->error, "no cmdpg here", NULLFN)
			FAILANDRETURN((meta == -1), vm->error, "stack empty.C", NULLFN)
			peek(&vm->userStack, vm->coadiutor);
			char *endptr;
			unsigned long int n = strtoul(vm->coadiutor, &endptr, 10);
			//if ToS has a complex number, n will be 0
			if (n < MAX_CMD_PAGES) {
				//if the page number is allowed, load into cmdPage 
				vm->cmdPage = n;
				pop(&vm->userStack, vm->coadiutor);
			}
		} else if (strcmp(token, "swp") == 0) {
			int8_t cmeta = pop(&vm->userStack, vm->coadiutor);
			FAILANDRETURN((cmeta == -1), vm->error, "stack empty.B0", NULLFN)
			int8_t meta = pop(&vm->userStack, vm->bak);
			if (meta == -1) push(&vm->userStack, vm->coadiutor, cmeta); //restore on error
			FAILANDRETURN((meta == -1), vm->error, "stack empty.B", NULLFN)
			push(&vm->userStack, vm->coadiutor, cmeta);
			push(&vm->userStack, vm->bak, meta);
			return true;
		} else if (strcmp(token, "vec") == 0) {
			int8_t meta;
			strcpy(vm->matvecStrC, "]");
			while (true) {
				meta = peek(&vm->userStack, NULL);
				if (meta == METANONE) {
					pop(&vm->userStack, vm->acc);
					//printf("found %s\n", vm->acc);
					strcat(vm->matvecStrC, vm->acc);
					//attach a spacer if next entry is a scalar
					if (peek(&vm->userStack, NULL) == METANONE)
						strcat(vm->matvecStrC, " ");
				} else {
					break;
				}
			}
			//printf("before rev vector %s\n", vm->matvecStrC);
			strcat(vm->matvecStrC, "[");
			reverse(vm->matvecStrC);
			//printf("final vector %s\n", vm->matvecStrC);
			//printf("count = %d\n", count);
			push(&vm->userStack, vm->matvecStrC, METAVECTOR);
		} else if (strcmp(token, "dot") == 0) {
			int8_t cmeta = peek(&vm->userStack, NULL); //c
			int8_t meta = peekn(&vm->userStack, NULL, 1);  //b
			//fnindex = 6 is multiplication
			return fnOrOpVec2Param(vm, token, 6, cmeta, meta, false); //returnsVector = false
		} else if (strcmp(token, "dup") == 0) {
			int8_t meta = peek(&vm->userStack, vm->matvecStrC);
			FAILANDRETURN((meta == -1), vm->error, "stack empty", NULLFN)
			FAILANDRETURN((meta == METAVECTORPARTIAL), vm->error, "bad vector.2", NULLFN)
			FAILANDRETURN((meta == METAMATRIXPARTIAL), vm->error, "bad matrix.2", NULLFN)
			push(&vm->userStack, vm->matvecStrC, meta);
			return true;
		} else if (strcmp(token, "dupn") == 0) { //duplicates the nth item from ToS, ToS is 1
			int8_t meta = peek(&vm->userStack, vm->matvecStrC);
			double d;
			FAILANDRETURN((meta == -1), vm->error, "stack empty", NULLFN)
			if (isRealNumber(vm->matvecStrC)) { //real number
				success = stringToDouble(vm->matvecStrC, &d);
				pop(&vm->userStack, NULL);
				if (d < 1) return false; 
				//zero negative dup doesn't do anything
				meta = peekn(&vm->userStack, vm->matvecStrC, (int)(d - 1));
				push(&vm->userStack, vm->matvecStrC, meta);
			} else pop(&vm->userStack, NULL);
			return true;
		} else if (strcmp(token, JMPTOKEN) == 0 || strcmp(token, JPZTOKEN) == 0 || strcmp(token, JNZTOKEN) == 0) {
			//jmp, jpz, jnz
			//conditional loop, check for previous if/else
			UintStackPush(&vm->execStack, (1<<(sizeof(int32_t)-1)));
			return true;
		} else if (strcmp(token, IFTOKEN) == 0) {
			//printf ("-------------------Found if -- now popping vm->userStack\n");
			pop(&vm->userStack, vm->acc);
			ComplexDouble conditionComplex;
			success = false;
			//evaluate the if/else condition using the same variable
			if (isComplexNumber(vm->acc)) { //complex
				success = stringToComplex(vm->acc, &conditionComplex);
				ifCondition = !alm0double(conditionComplex.real) || !alm0double(conditionComplex.imag);
			} else if (isRealNumber(vm->acc)) { //real number
				success = stringToDouble(vm->acc, &conditionComplex.real);
				ifCondition = !alm0double(conditionComplex.real);
				//printf("-------------------Found if: conditionComplex.real = %.15g\n", conditionComplex.real);
			} else { //string
				ifCondition = false;
			}
			//printf("-------------------Found if: ifCondition = %d\n", ifCondition);
			execData = makeExecStackData(true, ifCondition, true); //doingIf = true
			//printf("-------------------Found if: execData calculated = %lu\n", execData);
			UintStackPush(&vm->execStack, execData);
			return success;
		} else if (strcmp(token, ELSETOKEN) == 0) {
			//printf ("-------------------Found else -- execData = %lu\n", execData);
			FAILANDRETURN((conditionalData(execData) == 0x0), vm->error, "E:else w/o if A", NULLFN)
			FAILANDRETURN(!doingIf, vm->error, "E:else w/o if.", NULLFN)
			FAILANDRETURN(UintStackIsEmpty(&vm->execStack), vm->error, "E:else w/o if B", NULLFN)
			UintStackPop(&vm->execStack); //discard the if
			execData = makeExecStackData(true, ifCondition, false); //condition is same, doing else
			//printf ("-------------------Found else -- set execData to = %lu\n", execData);
			UintStackPush(&vm->execStack, execData);
			return true;
		} else if (strcmp(token, ENDIFTOKEN) == 0) {
			FAILANDRETURN((conditionalData(execData) == 0x0), vm->error, "E:fi w/o if A", NULLFN)
			FAILANDRETURN(UintStackIsEmpty(&vm->execStack), vm->error, "E:fi w/o if B", NULLFN)
			UintStackPop(&vm->execStack); //discard the if/else
			return true;
		} else if (token[0] == MATSTARTTOKENC) {
		} else if (token[0] == MATLASTTOKENC) {
		} else if (token[0] == VECSTARTTOKENC) {
			//parse vector first fragment
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN(meta == METAVECTORPARTIAL, vm->error, "E:nested vectors", NULLFN)
			//only METANONE, METAVECTOR, METAMATRIX, METAMATRIXPARTIAL are allowed
			//push a vector indicator
			push(&vm->userStack, (char*)"[", METAVECTORPARTIAL);
		} else if (token[0] == VECLASTTOKENC) {
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN(meta != METAVECTORPARTIAL, vm->error, "E: need vector", NULLFN)
			//have a '[' -- getting a ']' -- discard the vector 
			pop(&vm->userStack, vm->matvecStrC);
			if (strcmp(vm->matvecStrC, "[") != 0) {
				strcat(vm->matvecStrC, "]");
				push(&vm->userStack, vm->matvecStrC, METAVECTOR); //close the vector
			}
		} else if (token[0] == POPTOKENC || strcmp(token, DPOPTOKEN) == 0) {
			success = processPop(vm, token);
		} else if (token[0] == PRINTTOKENC) { //print register or variable
			success = processPrint(vm, token);
		} else {
			int32_t tokenlen = strlen(token);
			if (token[tokenlen - 1] == '@') {
				//found <variable>@
				token[tokenlen - 1] = '\0';
				success = popIntoVariable (vm, token);
				FAILANDRETURN(!success, vm->error, "E:bad var", NULLFN)
			}
			else {
				//see if the ToS is a partial matrix or vector
				int8_t meta = peek(&vm->userStack, NULL);
				if (meta == METAMATRIXPARTIAL) {
				} else if (meta == METAVECTORPARTIAL) {
					pop(&vm->userStack, vm->matvecStrC);
					if (strcmp(vm->matvecStrC, "[") != 0)
						strcat(vm->matvecStrC, " ");
					strcat(vm->matvecStrC, token);
					push(&vm->userStack, vm->matvecStrC, METAVECTORPARTIAL);
				} else {
					success = processDefaultPush(vm, token);
				}
			}
		}
	}
	return success;
}

