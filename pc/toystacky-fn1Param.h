bool fn1ParamScalar(Machine* vm, const char* fnname, int fnindex, int isTrig) {
	//printf("fn1ParamScalar: entered ------------------- fnname = %s fnindex = %d with vm->acc = %s\n", fnname, fnindex, vm->acc);
	ComplexDouble c;
	bool success = false;
	c.imag = 0.0;
	if (isComplexNumber(vm->acc)) { //complex
		success = stringToComplex(vm->acc, &c);
	} else if (isRealNumber(vm->acc)) { //real number
		success = stringToDouble(vm->acc, &c.real);
	}
	FAILANDRETURNVAR(!success, vm->error, "E:Math fn '%s' failed.", fitstr(vm->coadiutor, fnname, 8))
	//printf("fn1Param: ---  success = %d\n", success);
	double complex ctemp = c.real + c.imag * I;
	if (fnindex < NUMMATH1PARAMFN) {
		if (vm->modeDegrees && isTrig == 1) {
			//convert input to radians for trig fns
			c.real = c.real * 3.141592653589793L/180.0;
			c.imag = c.imag * 3.141592653589793L/180.0;
			ctemp = c.real + c.imag * I;
		}
		ctemp = call1ParamMathFunction(fnindex, ctemp);
		c.real = creal(ctemp);
		c.imag = cimag(ctemp);
		if (vm->modeDegrees && isTrig == 2) {
			//convert result to degrees for inverse trig fns
			c.real = c.real * 180.0/3.141592653589793L;
			c.imag = c.imag * 180.0/3.141592653589793L;
		}
	} else if (fnindex < NUMMATH1PARAMFN + NUMREAL1PARAMFN) {
		c.imag = 0.0;
		c.real = call1ParamRealFunction(fnindex - NUMMATH1PARAMFN, ctemp);
	}
	//printf("fn1Param: function returned c.real = %g c.imag = %g\n", c.real, c.imag);
	FAILANDRETURNVAR((c.real == INFINITY || c.imag == INFINITY || c.real == -INFINITY || c.imag == -INFINITY), 
		vm->error, "E:Fn '%s' out of range. A", fitstr(vm->coadiutor, fnname, 8))
	//take relative values of real and imag parts
	if (dabs(c.imag) != 0)
		if (dabs(c.real/c.imag) < DOUBLEFN_EPS) c.real = 0.0;
	if (dabs(c.real) != 0)
		if (dabs(c.imag/c.real) < DOUBLEFN_EPS) c.imag = 0.0;

	return complexToString(c, vm->acc, vm->precision, vm->notationStr);
}

bool fn1Param(Machine* vm, const char* token, int fnindex, int isTrig) {
	//printf("fn1Param: entered ------------------- token = %s fnindex = %d\n", token, fnindex);
	bool success = false;
	int8_t meta = peek(&vm->userStack, NULL);
	FAILANDRETURN((meta == -1), vm->error, "stack empty.D", NULLFN)

	if (meta == METANONE) {
		//scalar, keep in acc
		peek(&vm->userStack, vm->acc);
		success = fn1ParamScalar(vm, token, fnindex, isTrig);
		FAILANDRETURNVAR(!success, vm->error, "E:Math fn '%s' failed. X", fitstr(vm->coadiutor, token, 8))
		pop(&vm->userStack, NULL);
		//fn1ParamScalar has the result in acc
		push(&vm->userStack, vm->acc, METANONE);
	} else if (meta == METAVECTOR) {
		success = fnOrOpVec1Param(vm, token, fnindex, isTrig, true);
	}

	return success;
}

