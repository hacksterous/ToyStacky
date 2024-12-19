bool fn1Param(Machine* vm, const char* token, int fnindex, int isTrig) {
	bool ifCondition, doingIf, conditional;
	size_t execData = UintStackPeek(&vm->execStack);
	FAILANDRETURN((vectorMatrixData(execData) != 0), vm->error, "No scalar op here.Z", NULLFN)
	ifCondition = doingIf = false;
	GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)
	//SerialPrint(1, "is1ParamFunction:------------------- execData = %lu", execData);
	if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
		int8_t meta;
		char debug0[VSHORT_STRING_SIZE];
		meta = peek(&vm->userStack, vm->coadiutor);
		FAILANDRETURN((meta == -1), vm->error, "stack empty.D", NULLFN)
		FAILANDRETURN((meta != METANONE), vm->error, "No scalar op here.Y", NULLFN)
		peek(&vm->userStack, vm->acc);
		if (vm->acc[0] != '\0') {
			ComplexDouble c;
			bool success = false;
			c.imag = 0.0;
			if (isComplexNumber(vm->acc)) { //complex
				success = stringToComplex(vm->acc, &c);
			} else if (isRealNumber(vm->acc)) { //real number
				success = stringToDouble(vm->acc, &c.real);
			}
			FAILANDRETURNVAR(!success, vm->error, "%s bad.A", fitstr(vm->coadiutor, token, 8))
			doubleToString((double)success, debug0);
			SerialPrint(3, "fn1Param: ---  success = ", debug0, "\r\n");
			if (fnindex < NUMMATH1PARAMFN) {
				if (vm->modeDegrees && isTrig == 1)
					//convert input to radians for trig fns
					c = makeComplex(c.real * 3.141592653589793L/180.0, c.imag * 3.141592653589793L/180.0);
				c = call1ParamMathFunction(fnindex, c);
				if (vm->modeDegrees && isTrig == 2)
					//convert result to degrees for inverse trig fns
					c = makeComplex(c.real * 180.0/3.141592653589793L, c.imag * 180.0/3.141592653589793L);
			} else if (fnindex < NUMMATH1PARAMFN + NUMREAL1PARAMFN)
				c.real = call1ParamRealFunction(fnindex - NUMMATH1PARAMFN, c);
			FAILANDRETURNVAR((c.real == INFINITY || c.imag == INFINITY || c.real == -INFINITY || c.imag == -INFINITY), 
				vm->error, "'%s' inf!", fitstr(vm->coadiutor, token, 8))
			//take relative values of real and imag parts
			if (abs(c.imag) != 0)
				if (abs(c.real/c.imag) < DOUBLEFN_EPS) c.real = 0.0;
			if (abs(c.real) != 0)
				if (abs(c.imag/c.real) < DOUBLEFN_EPS) c.imag = 0.0;
			success = complexToString(c, vm->acc);
			FAILANDRETURNVAR(!success, vm->error, "%s bad.B", fitstr(vm->coadiutor, token, 8))
			pop(&vm->userStack, NULL);
			push(&vm->userStack, vm->acc, METANONE);
		}
	}
	return true;
}
