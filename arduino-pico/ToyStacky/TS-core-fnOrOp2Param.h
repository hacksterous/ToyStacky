bool fnOrOp2Param(Machine* vm, const char* token, int fnindex) {
	bool success;
	int8_t cmeta = peek(&vm->userStack, NULL); //c
	//if current stack has full vector, call the 2-param vector function
	int8_t meta = peekn(&vm->userStack, NULL, 1);  //b
	if (cmeta == METAVECTOR || meta == METAVECTOR) return fnOrOpVec2Param(vm, token, fnindex, cmeta, meta, true); //returnsVector = true
	FAILANDRETURN((cmeta == -1), vm->error, "stack empty", NULLFN)		
	FAILANDRETURN((cmeta != METASCALAR), vm->error, "no matrix.", NULLFN)
	FAILANDRETURN((meta == -1), vm->error, "stack empty.C", NULLFN)		
	FAILANDRETURN((meta != METASCALAR), vm->error, "no matrix.C", NULLFN)
	cmeta = peek(&vm->userStack, vm->coadiutor); //c
	meta = peekn(&vm->userStack, vm->bak, 1);  //b
	ComplexDouble c, d;
	success = false;

	if (hasDblQuotes(vm->coadiutor) || hasDblQuotes(vm->bak)) {
		//bigint
		char* coadiutor = removeDblQuotes(vm->coadiutor);
		char* bak = removeDblQuotes(vm->bak);
		//bigint_from_str will call isBigIntDecString to check for valid big integer
		success = bigint_from_str(&(vm->bigC), coadiutor);
		success = bigint_from_str(&vm->bigB, bak) && success;
		FAILANDRETURN(!success, vm->error, "bad operand X2", NULLFN)
		if (fnindex >= NUMVOIDBIGINTFNMIN && fnindex <= NUMVOIDBIGINTFNMAX) { //void bigint functions -- pow through mod
			call2ParamBigIntVoidFunction(fnindex - 2, &vm->bigB, &(vm->bigC), &vm->bigA);
			FAILANDRETURN(((&vm->bigA)->length == -1), vm->error, "bigint div by 0", NULLFN)
			char* acc = &vm->acc[0];
			success = bigint_tostring (&vm->bigA, acc);
			success = addDblQuotes(acc) && success;
			FAILANDRETURN(!success, vm->error, "bigint fail", NULLFN)
		} else if (fnindex > NUMVOIDBIGINTFNMAX) { //compare bigint functions, return int
			vm->acc[0] = '0' +  call2ParamBigIntIntFunction(fnindex - 9, &vm->bigC, &vm->bigB);
			vm->acc[1] = '\0';
		}
		pop(&vm->userStack, NULL);
		pop(&vm->userStack, NULL);
		push(&vm->userStack, vm->acc, METASCALAR);
		return true;
	}
	c.imag = 0.0;
	if (isComplexNumber(vm->coadiutor)) { //complex
		success = stringToComplex(vm->coadiutor, &c);
	} else if (isRealNumber(vm->coadiutor)) { //real number
		char rlc = strIsRLC(vm->coadiutor);			
		success = stringToDouble(vm->coadiutor, &c.real);
		if (rlc) {
			//use built-in frequency var (default 1Hz)
			if (rlc == 'c'){
				c.imag = -1/(2 * 3.141592653589793L * vm->frequency * c.real);
				c.real = 0;
			} else if (rlc == 'l') {
				c.imag = 2 * 3.141592653589793L * vm->frequency * c.real;
				c.real = 0;
			} //else keep value of c.real
			//printf("fnOrOp2Param: isRealNumber: coadiutor (RLC) -- vm->frequency = %Lf returned real = %Lf imag = %lf ", vm->frequency, c.real, c.imag);
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
			if (rlc == 'c'){
				d.imag = -1/(2 * 3.141592653589793L * vm->frequency * d.real);
				d.real = 0;
			} else if (rlc == 'l') {
				d.imag = 2 * 3.141592653589793L * vm->frequency * d.real;
				d.real = 0;
			} //else keep value of d.real
			//printf("fnOrOp2Param: isRealNumber: bak (RLC) -- vm->frequency = %lf returned real = %lf imag = %lf ", vm->frequency, d.real, d.imag);
		}
		//printf("fnOrOp2Param: d = %s returned d real = %lf imag = %lf\n", vm->bak, d.real, d.imag);
	}  else {
		FAILANDRETURN(true, vm->error, "bad operand.B", NULLFN)
	}
	FAILANDRETURN(!success, vm->error, "bad operand.B2", NULLFN)
	//call 2-parameter function
	//printf("fnOrOp2Param: calling call2ParamMathFunction c real = %lf d real = %lf\n", c.real, d.real);
	c = call2ParamMathFunction(fnindex, d, c);
	if (fabs(c.real) < DOUBLE_EPS) c.real = 0.0;
	if (fabs(c.imag) < DOUBLE_EPS) c.imag = 0.0;
	//printf("fnOrOp2Param: returned from call2ParamMathFunction c.real = %lf\n", c.real);
	success = complexToString(c, vm->coadiutor, vm->precision, vm->notationStr); //result in coadiutor
	//SerialPrint(5, "fnOrOp2Param: 2 ------------------- got ", token, " data returned from function = ", vm->coadiutor, "\r\n");
	FAILANDRETURNVAR(!success, vm->error, "Bad fn %s", fitstr(vm->coadiutor, token, 8))
	strcpy(vm->acc, vm->coadiutor);
	//when no errors are present, actually pop the vars
	pop(&vm->userStack, NULL);
	pop(&vm->userStack, NULL);
	push(&vm->userStack, vm->acc, METASCALAR);
	return true;
}
