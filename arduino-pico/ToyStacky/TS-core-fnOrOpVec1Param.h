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
