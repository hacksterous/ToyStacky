bool fnOrOp2Param(Machine* vm, const char* token, int fnindex) {
	bool ifCondition, doingIf, conditional;
	size_t execData = UintStackPeek(&vm->execStack);
	bool success;
	FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "no scalar op here.H", NULLFN)
	ifCondition = doingIf = false;
	GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)
	//printf ("fnOrOp2Param: 0 ------------------- last = %s bak = %s\n", vm->coadiutor, vm->bak);
	//printf("fnOrOp2Param: ------------------- token = %s\n", token);
	int8_t cmeta = peek(&vm->userStack, vm->coadiutor); //c
	//if current stack has end of vector, call the 2-param vector function
	if (cmeta == METAENDVECTOR) return fnOrOpVec2Param(vm, token, fnindex);
	int8_t meta = peekn(&vm->userStack, vm->bak, 1);  //b
	//printf("fnOrOp2Param --- meta for var a = %d\n", meta);
	if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
		FAILANDRETURN((meta == -1), vm->error, "stack empty", NULLFN)		
		//printf("fnOrOp2Param --- ifcondition is true or is unconditional. meta for var a = %d\n", meta);
		FAILANDRETURN((meta != METANONE), vm->error, "no scalar op here.L.", NULLFN)
		if (fnindex == 0) { //swap
			FAILANDRETURN((meta == -1), vm->error, "stack empty.B", NULLFN)			
			FAILANDRETURN((meta != METANONE), vm->error, "no scalar op here.E", NULLFN)
			pop(&vm->userStack, NULL);
			pop(&vm->userStack, NULL);
			push(&vm->userStack, vm->coadiutor, cmeta);
			push(&vm->userStack, vm->bak, meta);
			return true;
		}
		//printf("fnOrOp2Param --- meta for var b = %d\n", meta);
		FAILANDRETURN((meta == -1), vm->error, "stack empty.C", NULLFN)		
		FAILANDRETURN((meta != METANONE), vm->error, "no scalar op here.A", NULLFN)
		//printf ("fnOrOp2Param: 1 ------------------- coadiutor = %s bak = %s\n", vm->coadiutor, vm->bak);
		//printf ("fnOrOp2Param: 0 ------------------- got ADD\n");
		ComplexDouble c, d;
		success = false;

		if (hasDblQuotes(vm->coadiutor) || hasDblQuotes(vm->bak)) {
			char* coadiutor = removeDblQuotes(vm->coadiutor);
			char* bak = removeDblQuotes(vm->bak);
			if ((isBigIntDecString(coadiutor)) && 
					(isBigIntDecString(bak))) { 
				success = bigint_create_str(coadiutor, &(vm->bigC));
				success = bigint_create_str(bak, &vm->bigB) && success;
				FAILANDRETURN(!success, vm->error, "bad operand X2", NULLFN)
				success = (fnindex > 2) && (fnindex < 16);
				FAILANDRETURN(!success, vm->error, "bad bigint fn", NULLFN)
				if (fnindex > 2) {
					if ((fnindex - 3) < 7) { //void functions
						call2ParamBigIntVoidFunction(fnindex - 3, &vm->bigB, &(vm->bigC), &vm->bigA);
						FAILANDRETURN(((&vm->bigA)->length == -1), vm->error, "bigint div by 0", NULLFN)
						char* acc = &vm->acc[0];
						success = bigint_tostring (&vm->bigA, acc);
						success = addDblQuotes(acc) && success;
						FAILANDRETURN(!success, vm->error, "bigint fail", NULLFN)
					} else { //compare functions, return int
						vm->acc[0] = '0' +  call2ParamBigIntIntFunction(fnindex - 5, &vm->bigC, &vm->bigB);
						vm->acc[1] = '\0';
					}
				}
				pop(&vm->userStack, NULL);
				pop(&vm->userStack, NULL);
				push(&vm->userStack, vm->acc, METANONE);
				return true;
			}
		}
		c.imag = 0.0;
		if (isComplexNumber(vm->coadiutor)) { //complex
			success = stringToComplex(vm->coadiutor, &c);
		} else if (isRealNumber(vm->coadiutor)) { //real number
			char rlc = strIsRLC(vm->coadiutor);			
			success = stringToDouble(vm->coadiutor, &c.real);
			if (rlc) {
				//does a frequency variable exist?
				ComplexDouble f = fetchVariableComplexValue(&vm->ledger, "f");
				if (f.real <= 0) f.real = 1;
				if (rlc == 'c'){
					c.imag = -1/(2 * 3.141592653589793 * f.real * c.real);
					c.real = 0;
				} else if (rlc == 'l') {
					c.imag = 2 * 3.141592653589793 * f.real * c.real;
					c.real = 0;
				} //else keep value of c.real
			}
		} else {
			FAILANDRETURN(true, vm->error, "bad operand.A", NULLFN)
		}
		FAILANDRETURN(!success, vm->error, "bad operand.A2", NULLFN)
		d.imag = 0.0;
		if (isComplexNumber(vm->bak)) { //looks like complex number '(## ##)'
			success = stringToComplex(vm->bak, &d);
		} else if (isRealNumber(vm->bak)) { //real number
			char rlc = strIsRLC(vm->bak);
			success = stringToDouble(vm->bak, &d.real);
			if (rlc) {
				//does a frequency variable exist?
				ComplexDouble f = fetchVariableComplexValue(&vm->ledger, "f");
				if (f.real <= 0) f.real = 1;
				if (rlc == 'c'){
					d.imag = -1/(2 * 3.141592653589793 * f.real * d.real);
					d.real = 0;
				} else if (rlc == 'l') {
					d.imag = 2 * 3.141592653589793 * f.real * d.real;
					d.real = 0;
				} //else keep value of d.real
			}
			//printf("fnOrOp2Param: d = %s returned %d ", vm->bak, success);
		}  else {
			FAILANDRETURN(true, vm->error, "bad operand.B", NULLFN)
		}
		FAILANDRETURN(!success, vm->error, "bad operand.B2", NULLFN)
		//SerialPrint(1, "fnOrOp2Param:------------------- d = %g + %gi", d.real, d.imag);
		//call 2-parameter function
		c = call2ParamMathFunction(fnindex - 1, d, c);
		if (abs(c.real) < DOUBLE_EPS) c.real = 0.0;
		if (abs(c.imag) < DOUBLE_EPS) c.imag = 0.0;
		//printf("fnOrOp2Param: rounded c.real = %g c.imag = %g\n", c.real, c.imag);
		success = complexToString(c, vm->coadiutor) && success; //result in coadiutor
		SerialPrint(5, "fnOrOp2Param: 2 ------------------- got ", token, " data returned from function = ", vm->coadiutor, "\r\n");
		//printf ("-------------------success = %d, acc = %s\n", success, vm->coadiutor);
		FAILANDRETURNVAR(!success, vm->error, "Bad fn %s", fitstr(vm->coadiutor, token, 8))
		strcpy(vm->bak, vm->acc); //backup
		strcpy(vm->acc, vm->coadiutor);
		//when no errors are present, actually pop the vars
		pop(&vm->userStack, NULL);
		pop(&vm->userStack, NULL);
		push(&vm->userStack, vm->acc, METANONE);
	}
	return true;
}
