bool process(Machine* vm, char* token) {
	bool success = true;
	bool ifCondition, doingIf, conditional;
	ifCondition = doingIf = conditional = false;
	int32_t execData = UintStackPeek(&vm->execStack);

	GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)
	//printf("process:------------------- token = %s\n", token);
	//printf("process:------------------- UintStackPeek = %lu\n", execData);
	if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
		int ismatfn = isMatFunction(token);
		int is1pfn = is1ParamFunction(token);
		int is2pfn = is2ParamFunction(token);
		int isvecfn = isVec1ParamFunction(token);
		int isTrig = is1ParamTrigFunction(token);
		//keep top two stack entries in last registers
		peek(&vm->userStack, vm->lastX);
		peekn(&vm->userStack, vm->lastY, 1);
		if (ismatfn != -1) {
			Matrix m;
			ComplexDouble c;
			int8_t meta = peek(&vm->userStack, vm->matvecStrC);
			FAILANDRETURN((meta == -1), vm->error, "stack empty.Y", NULLFN)
			switch (ismatfn) {
				case 0: //det
					FAILANDRETURN((meta != METAMATRIX), vm->error, "require matrix.", NULLFN)
					success = matbuild(&m, vm->matvecStrC);
					FAILANDRETURN((!success), vm->error, "bad input matrix.", NULLFN)
					success = matdeterminant(&m, &c);
					FAILANDRETURN((!success), vm->error, "mat singular.", NULLFN)
					success = complexToString(c, vm->coadiutor, vm->precision, vm->notationStr);
					FAILANDRETURN((!success), vm->error, "bad determinant.", NULLFN)
					//when operating on a vector or matrix and the result is a scalar,
					//don't pop out the vector/matrix
					push(&vm->userStack, vm->coadiutor, METASCALAR);
					break;
				case 1:	//inv
					FAILANDRETURN((meta != METAMATRIX), vm->error, "require matrix.", NULLFN)
					success = matbuild(&m, vm->matvecStrC);
					FAILANDRETURN((!success), vm->error, "bad input matrix.", NULLFN)
					success = matdeterminant(&m, &c);
					FAILANDRETURN((!success), vm->error, "no det.", NULLFN)
					FAILANDRETURN(alm0(c), vm->error, "mat singular.", NULLFN)
					FAILANDRETURN((m.rows != m.columns), vm->error, "non sqr matrix.", NULLFN)
					success = matinversion(&m, &vm->matrixC);
					FAILANDRETURN((!success), vm->error, "mat inv failed.", NULLFN)
					success = matrixToString(&vm->matrixC, vm->matvecStrC, vm->precision, vm->notationStr);
					FAILANDRETURN((!success), vm->error, "bad mat inv.", NULLFN)
					pop(&vm->userStack, NULL);
					push(&vm->userStack, vm->matvecStrC, METAMATRIX);
					break;
				case 2:	//iden
					break;
				case 3:	//trace
					break;
				case 4:	//eival
					break;
				case 5:	//eivec
					break;
				case 6:	//tpose
					break;
			}
		}
		else if (is1pfn != -1) {
			//printf("process:------------------- is1pfn = %d\n", is1pfn);
			success = fn1Param(vm, token, is1pfn, isTrig);
		} else if (is2pfn != -1) {
			//printf("process:------------------- is2pfn|op = %d\n", is2pfn);
			//could call 2-parameter vector function
			success = fnOrOp2Param(vm, token, is2pfn);
		} else if (isvecfn != -1) {
			success = fnOrOpVec1Param(vm, token, isvecfn, false, false); //not a trig fn., result is not vector
		} else if (strcmp(token, "bar") == 0) { //barrier or unbarrier
			int8_t meta = peekbarrier(&vm->userStack, NULL);
			//meaningless to put barrier on empty stack
			FAILANDRETURN((meta == -1), vm->error, "stack empty.W", NULLFN)
			pop(&vm->userStack, vm->matvecStrC);
			if (meta & 0x8) //already has barrier
				push(&vm->userStack, vm->matvecStrC, (meta & 0x7)); //clear the barrier bit
			else
				push(&vm->userStack, vm->matvecStrC, (meta | 0x8)); //set the barrier bit
			//printf("process: pushed meta = %d\n", (meta | 0x8));
		} else if (strcmp(token, "swp") == 0) {
			int8_t cmeta = pop(&vm->userStack, vm->matvecStrC);
			FAILANDRETURN((cmeta == -1), vm->error, "stack empty.B0", NULLFN)
			int8_t meta = pop(&vm->userStack, vm->matvecStrB);
			if (meta == -1) push(&vm->userStack, vm->matvecStrC, cmeta); //restore on error
			FAILANDRETURN((meta == -1), vm->error, "stack empty.B", NULLFN)
			push(&vm->userStack, vm->matvecStrC, cmeta);
			push(&vm->userStack, vm->matvecStrB, meta);
			return true;
		} else if (strcmp(token, "cmplx") == 0) {
			//make or unmake complex numbers
			long double re;
			long double im;
			ComplexDouble c;
			int8_t cmeta = peek(&vm->userStack, vm->matvecStrC);
			FAILANDRETURN((cmeta == -1), vm->error, "stack empty.B", NULLFN)
			success = stringToComplex(vm->matvecStrC, &c);
			if (success) {
				//current ToS is a complex number, uncomplex it
				doubleToString(c.real, vm->bak, vm->precision, vm->notationStr);
				doubleToString(c.imag, vm->acc, vm->precision, vm->notationStr);
				pop(&vm->userStack, NULL);
				//earlier (ToS-1) number will real part, later (ToS) number is imag part
				push(&vm->userStack, vm->bak, METASCALAR);
				push(&vm->userStack, vm->acc, METASCALAR);
			} else {
				//if two real numbers are available at the ToS, create a complex number
				//earlier number is real part, later number is imag part
				int8_t meta = peekn(&vm->userStack, vm->matvecStrB, 1);
				FAILANDRETURN((meta == -1), vm->error, "need 1 more scalar", NULLFN)
				FAILANDRETURN((cmeta != METASCALAR || meta != METASCALAR), vm->error, "need 2 scalars", NULLFN)
				success = stringToDouble(vm->matvecStrB, &re);//earlier (ToS-1) number is real part
				success = stringToDouble(vm->matvecStrC, &im) & success;//later (ToS) number is imag part
				FAILANDRETURN(!success, vm->error, "need 2 reals", NULLFN)
				success = strIsRLC(vm->matvecStrB) && strIsRLC(vm->matvecStrC);
				FAILANDRETURN(success, vm->error, "no RLC here", NULLFN)
				strcpy(vm->matvecStrA, "("); 
				strcat(vm->matvecStrA, vm->matvecStrB); strcat(vm->matvecStrA, " "); strcat(vm->matvecStrA, vm->matvecStrC);
				strcat(vm->matvecStrA, ")");
				pop(&vm->userStack, NULL);
				pop(&vm->userStack, NULL);
				push(&vm->userStack, vm->matvecStrA, METASCALAR);
			}
		} else if (strcmp(token, "vec") == 0) {
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN((meta == -1), vm->error, "stack empty.Z", NULLFN)
			int i = 0;
			int j = 0;
			do {
				//break on seeing a stack barrier
				//printf("in do-while loop count i = %d\n", i);
				meta = peeknbarrier(&vm->userStack, NULL, i);
				//printf("in do-while loop at count i = %d and meta returned %d\n", i, meta);
				if ((meta & 0x7) != METASCALAR || (meta & METABARRIER)) {
					break;
				}
				i++;
			} while (true);
			//printf("final count i = %d\n", i);
			if (i == 0) return true; //no vectorizable elements
			strcpy(vm->matvecStrC, "[");
			while (i > 0) {
				peekn(&vm->userStack, vm->acc, i - 1);
				strcat(vm->matvecStrC, vm->acc);
				if (i != 1) strcat(vm->matvecStrC, " ");
				j++;
				i--;
			}
			//printf("j count i = %d\n", j);
			while (j-- > 0) pop(&vm->userStack, NULL);

			strcat(vm->matvecStrC, "]");
			//printf("final vector %s\n", vm->matvecStrC);
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
			long double d;
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
			pop(&vm->userStack, vm->matvecStrA);
			ComplexDouble conditionComplex;
			success = false;
			//evaluate the if/else condition using the same variable
			if (isComplexNumber(vm->matvecStrA)) { //complex
				success = stringToComplex(vm->matvecStrA, &conditionComplex);
				ifCondition = !alm0double(conditionComplex.real) || !alm0double(conditionComplex.imag);
			} else if (isRealNumber(vm->matvecStrA)) { //real number
				success = stringToDouble(vm->matvecStrA, &conditionComplex.real);
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
			FAILANDRETURN((conditionalData(execData) == 0x0), vm->error, "else w/o if A", NULLFN)
			FAILANDRETURN(!doingIf, vm->error, "else w/o if.", NULLFN)
			FAILANDRETURN(UintStackIsEmpty(&vm->execStack), vm->error, "else w/o if B", NULLFN)
			UintStackPop(&vm->execStack); //discard the if
			execData = makeExecStackData(true, ifCondition, false); //condition is same, doing else
			//printf ("-------------------Found else -- set execData to = %lu\n", execData);
			UintStackPush(&vm->execStack, execData);
			return true;
		} else if (strcmp(token, ENDIFTOKEN) == 0) {
			FAILANDRETURN((conditionalData(execData) == 0x0), vm->error, "fi w/o if A", NULLFN)
			FAILANDRETURN(UintStackIsEmpty(&vm->execStack), vm->error, "fi w/o if B", NULLFN)
			UintStackPop(&vm->execStack); //discard the if/else
			return true;
		} else if (token[0] == MATSTARTTOKENC) {
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN(meta == METAVECTORMATRIXPARTIAL || meta == METAMATRIXPARTIAL, vm->error, "nested matrices", NULLFN)
			//push a matrix indicator
			push(&vm->userStack, (char*)"{", METAMATRIXPARTIAL);
		} else if (token[0] == MATLASTTOKENC) {
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN(meta != METAMATRIXPARTIAL, vm->error, "need matrix", NULLFN)
			pop(&vm->userStack, vm->matvecStrC);
			if (strcmp(vm->matvecStrC, "{") != 0) {
				strcat(vm->matvecStrC, "}");
				push(&vm->userStack, vm->matvecStrC, METAMATRIX); //close the matrix
			} //else throw out an empty matrix
		} else if (token[0] == VECSTARTTOKENC) {
			//parse vector first fragment
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN(meta == METAVECTORPARTIAL, vm->error, "nested vectors", NULLFN)
			if (meta == METAMATRIXPARTIAL) { //continuing a matrix
				pop(&vm->userStack, vm->matvecStrC);
				if (vm->matvecStrC[strlen(vm->matvecStrC) - 1] == '{')
					strcat(vm->matvecStrC, "[");
				else
					strcat(vm->matvecStrC, " [");
				push(&vm->userStack, vm->matvecStrC, METAVECTORMATRIXPARTIAL);
			} else { //push a vector indicator
				push(&vm->userStack, (char*)"[", METAVECTORPARTIAL);
			}
		} else if (token[0] == VECLASTTOKENC) {
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN(meta != METAVECTORPARTIAL && meta != METAVECTORMATRIXPARTIAL, vm->error, "need vector", NULLFN)
			//have a '[' -- getting a ']' -- discard the vector 
			pop(&vm->userStack, vm->matvecStrC);
			if (strcmp(vm->matvecStrC, "[") != 0) {
				if (meta == METAVECTORPARTIAL) {
					strcat(vm->matvecStrC, "]");
					push(&vm->userStack, vm->matvecStrC, METAVECTOR); //close the vector
				} else { //METAVECTORMATRIXPARTIAL
					//validate matrix and fill in blank cells
					strcat(vm->matvecStrC, "]");
					push(&vm->userStack, vm->matvecStrC, METAMATRIXPARTIAL); //close the vector inside the matrix
				}
			} //else throw out an empty vector
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
				FAILANDRETURN(!success, vm->error, "bad var", NULLFN)
			}
			else {
				//see if the ToS is a partial matrix or vector
				int8_t meta = peek(&vm->userStack, NULL);
				StrackMeta varmeta;
				//matvecStrA has var value or literal -- variable substitution by value
				processDefaultPush(vm, token, vm->matvecStrA, &varmeta); 
				FAILANDRETURN(varmeta != METASCALAR, vm->error, "only scalar var.", NULLFN)
				if (meta == METAVECTORMATRIXPARTIAL) {
					pop(&vm->userStack, vm->matvecStrC);
					if (vm->matvecStrC[strlen(vm->matvecStrC) - 1] != '[')
						strcat(vm->matvecStrC, " ");
					strcat(vm->matvecStrC, vm->matvecStrA);
					push(&vm->userStack, vm->matvecStrC, METAVECTORMATRIXPARTIAL);
				} else if (meta == METAVECTORPARTIAL) {
					pop(&vm->userStack, vm->matvecStrC);
					if (vm->matvecStrC[strlen(vm->matvecStrC) - 1] != '[')
						strcat(vm->matvecStrC, " ");
					strcat(vm->matvecStrC, vm->matvecStrA);
					push(&vm->userStack, vm->matvecStrC, METAVECTORPARTIAL);
				} else if (meta == METAMATRIXPARTIAL) {
					pop(&vm->userStack, vm->matvecStrC);
					if (vm->matvecStrC[strlen(vm->matvecStrC) - 1] == ']')
						strcat(vm->matvecStrC, " [");
					else if (vm->matvecStrC[0] == '{') 
						strcat(vm->matvecStrC, "[");
					strcat(vm->matvecStrC, vm->matvecStrA);
					push(&vm->userStack, vm->matvecStrC, METAVECTORMATRIXPARTIAL);
				} else {
					push(&vm->userStack, vm->matvecStrA, varmeta);
				}
			}
		}
	}
	return success;
}

