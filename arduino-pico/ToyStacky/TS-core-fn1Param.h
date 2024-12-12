bool fn1Param(Machine* vm, const char* token, int fnindex) {
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
			if (fnindex < NUMMATH1PARAMFN)
				c = call1ParamMathFunction(fnindex, c);
			else if (fnindex < NUMMATH1PARAMFN + NUMREAL1PARAMFN)
				c.real = call1ParamRealFunction(fnindex - NUMMATH1PARAMFN, c);
			FAILANDRETURNVAR((c.real == INFINITY || c.imag == INFINITY || c.real == -INFINITY || c.imag == -INFINITY), 
				vm->error, "'%s' inf!", fitstr(vm->coadiutor, token, 8))
			if (abs(c.real) < DOUBLEFN_EPS) c.real = 0.0;
			if (abs(c.imag) < DOUBLEFN_EPS) c.imag = 0.0;
			success = complexToString(c, vm->acc);
			FAILANDRETURNVAR(!success, vm->error, "%s bad.B", fitstr(vm->coadiutor, token, 8))
			pop(&vm->userStack, NULL);
			push(&vm->userStack, vm->acc, METANONE);
		}
	}
	return true;
}
