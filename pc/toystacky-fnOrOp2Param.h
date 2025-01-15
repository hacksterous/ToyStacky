bool fnOrOp2Param(Machine* vm, const char* token, int fnindex) {
	bool success;
	int8_t cmeta = peek(&vm->userStack, NULL); //c
	//if current stack has full vector, call the 2-param vector function
	int8_t meta = peekn(&vm->userStack, NULL, 1);  //b
	//printf("fnOrOp2Param: A entered. cmeta = %d meta=%d\n", cmeta, meta);
	if (cmeta == METAVECTOR || meta == METAVECTOR) return fnOrOpVec2Param(vm, token, fnindex, cmeta, meta, true); //returnsVector = true
	FAILANDRETURN((cmeta == -1), vm->error, "Error: Stack empty", NULLFN)		
	FAILANDRETURN((cmeta != METANONE), vm->error, "Error: Need scalar op L.", NULLFN)
	FAILANDRETURN((meta == -1), vm->error, "Error: stack empty.C", NULLFN)		
	FAILANDRETURN((meta != METANONE), vm->error, "Error: Need scalar op A.", NULLFN)
	cmeta = peek(&vm->userStack, vm->coadiutor); //c
	meta = peekn(&vm->userStack, vm->bak, 1);  //b
	double complex vala, valb;
	ComplexDouble c, d;
	success = false;

	if (hasDblQuotes(vm->coadiutor) || hasDblQuotes(vm->bak)) {
		//bigint
		char* coadiutor = removeDblQuotes(vm->coadiutor);
		char* bak = removeDblQuotes(vm->bak);
		//bigint_from_str will call isBigIntDecString to check for valid big integer
		success = bigint_from_str(&(vm->bigC), coadiutor);
		success = bigint_from_str(&vm->bigB, bak) && success;
		FAILANDRETURN(!success, vm->error, "Error: Bad operand. X2", NULLFN)
		if (fnindex >= NUMVOIDBIGINTFNMIN && fnindex <= NUMVOIDBIGINTFNMAX) { //void bigint functions -- pow through mod
			//printf("fnOrOp2Param: X ------------------- bigint - doing DIV\n");
			call2ParamBigIntVoidFunction(fnindex - 2, &vm->bigB, &(vm->bigC), &vm->bigA);
			FAILANDRETURN(((&vm->bigA)->length == -1), vm->error, "Error: bigint div by 0.", NULLFN)
			char* acc = &vm->acc[0];
			success = bigint_tostring (&vm->bigA, acc);
			success = addDblQuotes(acc) && success;
			FAILANDRETURN(!success, vm->error, "Error: Unsupported bigint fn.", NULLFN)
		} else if (fnindex > NUMVOIDBIGINTFNMAX) { //compare bigint functions, return int
			vm->acc[0] = '0' +  call2ParamBigIntIntFunction(fnindex - 9, &vm->bigC, &vm->bigB);
			vm->acc[1] = '\0';
		}
		pop(&vm->userStack, NULL);
		pop(&vm->userStack, NULL);
		push(&vm->userStack, vm->acc, METANONE);
		return true;
	}
	c.imag = 0;
	if (isComplexNumber(vm->coadiutor)) { //complex
		success = stringToComplex(vm->coadiutor, &c);
		//printf("fnOrOp2Param: returned from stringToComplex ------------------- c = %g + %gi success = %d\n", c.real, c.imag, success);
	} else if (isRealNumber(vm->coadiutor)) { //real number
		char rlc = strIsRLC(vm->coadiutor);
		success = stringToDouble(vm->coadiutor, &c.real);
		//printf("fnOrOp2Param: d = %s returned %d and c.real = %lf\n", vm->coadiutor, success, c.real);
		if (rlc) {
			if (rlc == 'c'){
				c.imag = -1/(2 * 3.141592653589793L * vm->frequency * c.real);
				c.real = 0;
			} else if (rlc == 'l') {
				c.imag = 2 * 3.141592653589793L * vm->frequency * c.real;
				c.real = 0;
			} //else keep value of c.real
		}
	} else {
		FAILANDRETURN(true, vm->error, "Error: Bad operand. A", NULLFN)
	}
	FAILANDRETURN(!success, vm->error, "Error: Bad operand. A2", NULLFN)
	d.imag = 0;
	if (isComplexNumber(vm->bak)) { //looks like complex number '(## ##)'
		success = stringToComplex(vm->bak, &d);
	} else if (isRealNumber(vm->bak)) { //real number
		char rlc = strIsRLC(vm->bak);
		success = stringToDouble(vm->bak, &d.real);
		if (rlc) {
			//does a frequency variable exist?
			if (rlc == 'c'){
				d.imag = -1/(2 * 3.141592653589793L * vm->frequency * d.real);
				d.real = 0;
			} else if (rlc == 'l') {
				d.imag = 2 * 3.141592653589793L * vm->frequency * d.real;
				d.real = 0;
			} //else keep value of d.real
		}
		//printf("fnOrOp2Param: d = %s returned %d ", vm->bak, success);
	}  else {
		FAILANDRETURN(true, vm->error, "Error: Bad operand. B", NULLFN)
	}
	FAILANDRETURN(!success, vm->error, "Error: Bad operand. B2", NULLFN)
	vala = c.real + c.imag * I;
	//printf("fnOrOp2Param:------------------- d = %g + %gi\n", d.real, d.imag);
	valb = d.real + d.imag * I;
	//call 2-parameter function
	vala = call2ParamMathFunction(fnindex, valb, vala);
	//printf ("fnOrOp2Param: 2 ------------------- got %s, vala = %g + %gi\n", token, creal(vala), cimag(vala));
	c.real = creal(vala);
	c.imag = cimag(vala);
	//printf("fnOrOp2Param: function returned c.real = %g c.imag = %g abs(c.real) = %g\n", c.real, c.imag, dabs(c.real));
	if (dabs(c.real) < DOUBLE_EPS) c.real = 0.0;
	if (dabs(c.imag) < DOUBLE_EPS) c.imag = 0.0;
	//printf("fnOrOp2Param: rounded c.real = %g c.imag = %g\n", c.real, c.imag);
	success = complexToString(c, vm->coadiutor, vm->precision, vm->notationStr); //result in coadiutor
	//printf ("-------------------success = %d, acc = %s\n", success, vm->coadiutor);
	FAILANDRETURNVAR(!success, vm->error, "Error: Op '%s' had error.", fitstr(vm->coadiutor, token, 8))
	strcpy(vm->acc, vm->coadiutor);
	//when no errors are present, actually pop the vars
	pop(&vm->userStack, NULL);
	pop(&vm->userStack, NULL);
	push(&vm->userStack, vm->acc, METANONE);
	return true;
}
