bool fn1ParamScalar(Machine* vm, const char* fnname, int fnindex, int isTrig, int bigInt1Param) {
	ComplexDouble c;
	bool success = false;

	char* acc = &vm->acc[0]; //acc is an 'alias' for vm->acc
	//printf ("fn1ParamScalar: called with fn index = %d and bigInt1Param = %d\n", fnindex, bigInt1Param);
	//if has double quotes (assume bigint) and bigInt1Param >= 0 or
	//does not have double quotes and bigInt1Param >= 0 and fnindex == -1
	if ((!hasDblQuotes(vm->acc) && bigInt1Param != -1 && fnindex == -1) ||
		(hasDblQuotes(vm->acc) && bigInt1Param != -1)) {
		if (hasDblQuotes(vm->acc)) acc = removeDblQuotes(vm->acc);

		if (acc[0] == 'x')
			success = bigint_from_hex(&vm->bigC, acc);
		else if (acc[0] == 'b')
			success = bigint_from_bin(&vm->bigC, acc);
		else
			success = bigint_from_str(&vm->bigC, acc);

		//printf ("fn1ParamScalar: bigInt1Param != -1 called with bigInt1Param = %d, (after \" removal) acc = %s and extracted bigC = ", 
		//	bigInt1Param, acc);
		//bigint_print(&vm->bigC);
		if (!success) return false; //let fn1Param generate 'bad arg' error
		call1ParamBigIntVoidFunction(bigInt1Param, &vm->bigC, vm->acc);
		//printf ("fn1ParamScalar: returned from  call1ParamBigIntVoidFunction success = %d and res = %s first char of res = %c\n", success, vm->acc, vm->acc[0]);
		if (vm->acc[0] == '\0') return false; //error can happen for bigint_bin if result is longer than 100
		if (((vm->acc[0] != '-') && !isdigit(vm->acc[0])) || 
			(((strlen(vm->acc) > 15) && (vm->acc[0] == '-')) || ((strlen(vm->acc) > 14) && (vm->acc[0] != '-'))))
			//add quotes for a non-decimal number
			//for a decimal number -- 14 digits of decimal numbers can be handled as non-bigint integers 
			//add quotes
			success = addDblQuotes(vm->acc) && success;
		if (!success) return false;
		return true;
	}
	else if (hasDblQuotes(vm->acc)) {
		//math functions for bigints
		acc = removeDblQuotes(vm->acc);
		strcpy(vm->acc, acc);
		//printf ("fn1ParamScalar: fallthrough with acc = %s\n", vm->acc);
	}
	
	c.imag = 0.0;
	if (isComplexNumber(vm->acc)) //complex
		success = stringToComplex(vm->acc, &c);
	else if (isRealNumber(vm->acc)) //real number
		success = stringToDouble(vm->acc, &c.real);

	FAILANDRETURNVAR(!success, vm->error, "%s bad arg.", fitstr(vm->coadiutor, fnname, 8))
	if (vm->modePolar) {
		//convert to radians for all functions when in polar mode if input is in degrees
		if (vm->modeDegrees)
			c = makeComplex(c.real, c.imag * 0.01745329251994329576923L);
		
		//is complex number in polar mode
		//convert to rect mode
		//printf("fn1ParamScalar: currently in Polar, converting to Cartesian, real = %Lg imag = %Lg\n", c.real, c.imag);
		if (!alm0double(c.imag)) {
			long double temp = c.real * cosl(c.imag);
			c.imag = c.real * sinl(c.imag);
			c.real = temp;
		}
		//printf("fn1ParamScalar: currently in Polar, converted to Cartesian, real = %Lg imag = %Lg\n", c.real, c.imag);
	}

	if (fnindex < NUMMATH1PARAMFN) {
		if (vm->modeDegrees && isTrig == 1 && !vm->modePolar)
			//convert input to radians for trig fns
			//but not if it has been converted before
			c = makeComplex(c.real * 0.01745329251994329576923L, c.imag * 0.01745329251994329576923L);
		printf("fn1ParamScalar: calling fn %s fnindex = %d with args real = %Lg imag = %Lg\n", fnname, fnindex, c.real, c.imag);
		c = call1ParamMathFunction(fnindex, c);
		//output of the inv trig function is always in radians in cartesian mode
		if (vm->modeDegrees && isTrig == 2 && vm->modePolar)
			//convert result to degrees for inverse trig fns
			//if in polar mode, only the angle is converted to degrees
			c = makeComplex(c.real, c.imag * 57.295779513082320876798L);
		else if (vm->modeDegrees && isTrig == 2)
			//convert result to degrees for inverse trig fns
			//in cartesian (rect) mode, both re and im are converted
			c = makeComplex(c.real * 57.295779513082320876798L, c.imag * 57.295779513082320876798L);
	} else if (fnindex < NUMMATH1PARAMFN + NUMREAL1PARAMFN) {
		//real function outputs "abs", "arg", "re", "im"
		c.real = call1ParamRealFunction(fnindex - NUMMATH1PARAMFN, c);
		c.imag = 0.0;
	}

	FAILANDRETURNVAR((c.real == INFINITY || c.imag == INFINITY || c.real == -INFINITY || c.imag == -INFINITY), 
		vm->error, "'%s' inf!", fitstr(vm->coadiutor, fnname, 8))

	//take relative values of real and imag parts
	long double temp = c.real;
	if (fabs(c.imag) != 0)
		if (fabs(c.real/c.imag) < DOUBLEFN_EPS) c.real = 0.0;
	if (fabs(temp) != 0)
		if (fabs(c.imag/c.real) < DOUBLEFN_EPS) c.imag = 0.0;

	//printf("fn1ParamScalar: returned from fn call with result real = %Lg imag = %Lg\n", c.real, c.imag);
	if (vm->modePolar) {
		//convert back to polar mode
		if (!alm0double(c.real)) {
			long double temp = abso(c);
			c.imag = atan(c.imag/c.real);
			c.real = temp;
		} else {
			c.imag = 1.570796326794896619231L;//pi/2
			c.real = 0.0;
		}

		//convert back to degrees since this was converted to radians 
		if (vm->modeDegrees)
			c = makeComplex(c.real, c.imag * 57.295779513082320876798L);
	}
	success = complexToString(c, vm->acc, vm->precision, vm->notationStr);
	return success;
}

bool fn1Param(Machine* vm, const char* token, int fnindex, int isTrig, int bigInt1Param) {
	bool success = false;
	int8_t meta = peek(&vm->userStack, NULL);
	FAILANDRETURN((meta == -1), vm->error, "stack empty.D", NULLFN)

	if (meta == METASCALAR) {
		peek(&vm->userStack, vm->acc);
		//scalar, function argument is in vm->acc
		success = fn1ParamScalar(vm, token, fnindex, isTrig, bigInt1Param);
		FAILANDRETURNVAR(!success, vm->error, "%s bad arg.2", fitstr(vm->coadiutor, token, 8))
		//fn1ParamScalar has the result in acc
		vm->lastXMeta = pop(&vm->userStack, vm->lastX);
		push(&vm->userStack, vm->acc, METASCALAR);
	}
	else if (meta == METAVECTOR) {
		success = fnOrOpVec1Param(vm, token, fnindex, isTrig, true); //returnsVector = true
	}
	else if (meta == METAMATRIX) {
		success = fnOrOpMat1Param(vm, token, fnindex, isTrig);
	}
	return success;
}
