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
				FAILANDRETURN(true, vm->error, "Unexpected string.", NULLFN)
			}
			FAILANDRETURNVAR(!success, vm->error, "Math fn '%s' failed.", fitstr(vm->coadiutor, token, 9))
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
			FAILANDRETURN((variableIndex == -1), vm->error, "Expected vector.", NULLFN)
			if (variableIndex >= 0) {
				numXCount = fetchVariableComplexValue(&vm->ledger, VACC);
				FAILANDRETURN((numXCount.real == 0.0), vm->error, "Expected vector. A", NULLFN)
				FAILANDRETURN((numXCount.imag != 0.0), vm->error, "Expected vector. B", NULLFN)
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
					FAILANDRETURN(true, vm->error, "Unexpected string.", NULLFN)
				}
				FAILANDRETURNVAR(!success, vm->error, "Math fn '%s' failed.", fitstr(vm->coadiutor, token, 9))
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
