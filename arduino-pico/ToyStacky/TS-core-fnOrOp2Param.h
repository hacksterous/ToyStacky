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
		if (coadiutor[0] == 'x')
			success = bigint_from_hex(&vm->bigC, coadiutor);
		else if (coadiutor[0] == 'b')
			success = bigint_from_bin(&vm->bigC, coadiutor);
		else
			success = bigint_from_str(&vm->bigC, coadiutor);

		if (bak[0] == 'x')
			success = bigint_from_hex(&vm->bigB, bak) && success;
		else if (bak[0] == 'b')
			success = bigint_from_bin(&vm->bigB, bak) && success;
		else
			success = bigint_from_str(&vm->bigB, bak) && success;

		FAILANDRETURN(!success, vm->error, "bad operand X2", NULLFN)
		if (fnindex >= NUMVOID2PARAMBIGINTFNMIN && fnindex <= NUMVOID2PARAMBIGINTFNMAX) { //void bigint functions -- pow through mod
			call2ParamBigIntVoidFunction(fnindex - 2, &vm->bigB, &(vm->bigC), &vm->bigA);
			FAILANDRETURN(((&vm->bigA)->length == -1), vm->error, "bigint div by 0", NULLFN)
			success = bigint_tostring (&vm->bigA, vm->acc, 0);
			if (((vm->acc[0] != '-') && !isdigit(vm->acc[0])) ||
				(((strlen(vm->acc) > 15) && (vm->acc[0] == '-')) || ((strlen(vm->acc) > 14) && (vm->acc[0] != '-'))))
				//add quotes for a non-decimal number
				//for a decimal number -- 14 digits of decimal numbers can be handled as non-bigint integers 
				//add quotes
				success = addDblQuotes(vm->acc) && success;
			FAILANDRETURN(!success, vm->error, "bigint fail", NULLFN)
		} else if (fnindex > NUMVOID2PARAMBIGINTFNMAX) { //compare bigint functions, return int
			vm->acc[0] = '0' +  call2ParamBigIntIntFunction(fnindex - 9, &vm->bigC, &vm->bigB);
			vm->acc[1] = '\0';
		} 
		else FAILANDRETURN(!success, vm->error, "bad bigint fn.1", NULLFN)
		pop(&vm->userStack, NULL);
		pop(&vm->userStack, NULL);
		push(&vm->userStack, vm->acc, METASCALAR);
		return true;
	}

	c.imag = 0.0;
	if (isComplexNumber(vm->coadiutor)) { //complex
		success = stringToComplex(vm->coadiutor, &c);
		//this could be in polar format and degrees
		if (vm->modePolar) {
			//convert to radians for all functions when in polar mode if input is in degrees
			if (vm->modeDegrees)
				c = makeComplex(c.real, c.imag * 0.01745329251994329576923L);

			//is complex number in polar mode
			//convert to rect mode
			if (!alm0double(c.imag)) {
				long double temp = c.real * cosl(c.imag);
				c.imag = c.real * sinl(c.imag);
				c.real = temp;
			}
		}
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
		//this is a pure imaginary number in rect mode in radians
	} else {
		FAILANDRETURN(true, vm->error, "bad operand.A", NULLFN)
	}

	FAILANDRETURN(!success, vm->error, "bad operand.A2", NULLFN)
	d.imag = 0.0;
	if (isComplexNumber(vm->bak)) { //looks like complex number '(## ##)'
		success = stringToComplex(vm->bak, &d);
		//this could be in polar format and degrees
		if (vm->modePolar) {
			//convert to radians for all functions when in polar mode if input is in degrees
			if (vm->modeDegrees)
				d = makeComplex(d.real, d.imag * 0.01745329251994329576923L);

			//is complex number in polar mode
			//convert to rect mode
			if (!alm0double(c.imag)) {
				long double temp = d.real * cosl(d.imag);
				d.imag = d.real * sinl(d.imag);
				d.real = temp;
			}
		}

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
		//this is a pure imaginary number in rect mode in radians
	}  else {
		FAILANDRETURN(true, vm->error, "bad operand.B", NULLFN)
	}
	FAILANDRETURN(!success, vm->error, "bad operand.B2", NULLFN)
	//call 2-parameter function
	//printf("fnOrOp2Param: calling call2ParamMathFunction c real = %lf d real = %lf\n", c.real, d.real);
	c = call2ParamMathFunction(fnindex, d, c);
	//printf("fnOrOp2Param: returned from call2ParamMathFunction c.real = %lf\n", c.real);
	FAILANDRETURNVAR((c.real == INFINITY || c.imag == INFINITY || c.real == -INFINITY || c.imag == -INFINITY), 
		vm->error, "'%s' inf!", fitstr(vm->coadiutor, token, 8))
	
	if (vm->modeDegrees && (fnindex == ATAN2FNINDEX) && !vm->modePolar)
		//convert result to degrees for inverse trig fn (atan2)
		c = makeComplex(c.real * 57.295779513082320876798L, c.imag * 57.295779513082320876798L);

	//take relative values of real and imag parts
	long double temp = c.real;
	if (fabs(c.imag) != 0)
		if (fabs(c.real/c.imag) < DOUBLEFN_EPS) c.real = 0.0;
	if (fabs(temp) != 0)
		if (fabs(c.imag/c.real) < DOUBLEFN_EPS) c.imag = 0.0;
	
	if (vm->modePolar) {
		//convert back to polar mode
		if (!alm0double(c.real)) {
			long double temp = abso(c);
			c.imag = atanl(c.imag/c.real);
			c.real = temp;
		} else {
			c.imag = 1.570796326794896619231L;//pi/2
			c.real = 0.0;
		}

		//convert back to degrees if this was converted to radians 
		if (vm->modeDegrees)
			c = makeComplex(c.real, c.imag * 57.295779513082320876798L);
		
	}
	success = complexToString(c, vm->acc, vm->precision, vm->notationStr); //result in acc
	FAILANDRETURNVAR(!success, vm->error, "Bad fn %s", fitstr(vm->coadiutor, token, 8))

	//when no errors are present, actually pop the vars
	vm->lastXMeta = pop(&vm->userStack, vm->lastX);
	vm->lastYMeta = pop(&vm->userStack, vm->lastY);
	push(&vm->userStack, vm->acc, METASCALAR);
	return true;
}
