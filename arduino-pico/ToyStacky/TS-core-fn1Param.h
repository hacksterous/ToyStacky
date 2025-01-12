bool fn1ParamScalar(Machine* vm, const char* fnname, int fnindex, int isTrig) {
	ComplexDouble c;
	bool success = false;
	c.imag = 0.0;
	if (isComplexNumber(vm->acc)) //complex
		success = stringToComplex(vm->acc, &c);
	else if (isRealNumber(vm->acc)) //real number
		success = stringToDouble(vm->acc, &c.real);

	FAILANDRETURNVAR(!success, vm->error, "%s bad.", fitstr(vm->coadiutor, fnname, 8))
	if (fnindex < NUMMATH1PARAMFN) {
		if (vm->modeDegrees && isTrig == 1)
			//convert input to radians for trig fns
			c = makeComplex(c.real * 3.141592653589793L/180.0, c.imag * 3.141592653589793L/180.0);
		c = call1ParamMathFunction(fnindex, c);
		if (vm->modeDegrees && isTrig == 2)
			//convert result to degrees for inverse trig fns
			c = makeComplex(c.real * 180.0/3.141592653589793L, c.imag * 180.0/3.141592653589793L);
	} else if (fnindex < NUMMATH1PARAMFN + NUMREAL1PARAMFN) {
		c.imag = 0.0;
		c.real = call1ParamRealFunction(fnindex - NUMMATH1PARAMFN, c);
	}
	FAILANDRETURNVAR((c.real == INFINITY || c.imag == INFINITY || c.real == -INFINITY || c.imag == -INFINITY), 
		vm->error, "'%s' inf!", fitstr(vm->coadiutor, fnname, 8))

	//take relative values of real and imag parts
	if (abs(c.imag) != 0)
		if (abs(c.real/c.imag) < DOUBLEFN_EPS) c.real = 0.0;
	if (abs(c.real) != 0)
		if (abs(c.imag/c.real) < DOUBLEFN_EPS) c.imag = 0.0;

	return complexToString(c, vm->acc);
}

bool fn1Param(Machine* vm, const char* token, int fnindex, int isTrig) {
	bool success = false;
	int8_t meta = peek(&vm->userStack, NULL);
	FAILANDRETURN((meta == -1), vm->error, "stack empty.D", NULLFN)

	if (meta == METANONE) {
		peek(&vm->userStack, vm->acc);
		//scalar, function argument is in vm->acc
		success = fn1ParamScalar(vm, token, fnindex, isTrig);
		FAILANDRETURNVAR(!success, vm->error, "%s bad.", fitstr(vm->coadiutor, token, 8))
		//fn1ParamScalar has the result in acc
		pop(&vm->userStack, NULL);
		push(&vm->userStack, vm->acc, METANONE);
	} else if (meta == METAVECTOR) {
		success = fnOrOpVec1Param(vm, token, fnindex, isTrig, true); //returnsVector = true
	}
	return success;
}
