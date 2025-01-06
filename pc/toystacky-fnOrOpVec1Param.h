bool fnOrOpVec1Param(Machine* vm, const char* token, int fnindex, bool isTrig, bool returnsVector) {
	//this function can be called for evaluation a math function operating on a vector
	//(returnsVector = true)
	//these functions are:
	//"sin", "cos", "tan", "cot", "sinh", "cosh", "tanh", "coth", 
	//"asin", "acos", "atan", "acot", "asinh", "acosh", "atanh", "acoth",
	//"exp", "log10", "log", "log2", "sqrt", "cbrt", "conj", 
	//"abs", "arg", "re", "im"
	//These will return a vector of real or complex values
	//and result is a vector, e.g., [1 2 3]sin returns a vector [0.84 0.91 0.14].

	//If returnsVector = false, the return value is a scalar (real or complex) -- these functions are:
	//"sum", "sqsum", "var", "stdev", "mean", "rsum"

	char* input = (char*)NULL;
	char output[MAX_TOKEN_LEN];
	bool success;
	if (returnsVector) {
		//ToS is known to have a vector, no need to check for empty stack or non-vector item
		peek(&vm->userStack, vm->matvecStrA);
		input = vm->matvecStrA;
		//printf ("fnOrOpVec1Param: at start, matvecStrA = %s\n", vm->matvecStrA);
		strcpy(vm->matvecStrB, "[");
		while (true) {
			input = tokenize(input, output);
			//printf ("fnOrOpVec1Param: loop, output = %s\n", output);
			if (output[0] == ']') break;
			if (output[0] == '[') continue;
			strcpy(vm->acc, output);
			//function name is in token
			//scalar, function argument is in vm->acc
			success = fn1ParamScalar(vm, token, fnindex, isTrig);
			//fn1ParamScalar has the result in acc
			FAILANDRETURNVAR(!success, vm->error, "%s bad arg.B", fitstr(vm->coadiutor, token, 8))
			strcat(vm->matvecStrB, vm->acc);
			if (input[0] != ']') strcat(vm->matvecStrB, " ");
			//printf ("fnOrOpVec1Param: loop, vm->matvecStrB = %s\n", vm->matvecStrB);
		}
		strcat(vm->matvecStrB, "]");
		pop(&vm->userStack, NULL);
		push(&vm->userStack, vm->matvecStrB, METAVECTOR);
	}
	else {//returns a scalar
		complex double crunningsum = 0;
		complex double crunningsqsum = 0;
		complex double crunningrsum = 0;
		complex double crunning = 0;
		complex double c;
		ComplexDouble cd;
		uint32_t count = 0;

		peek(&vm->userStack, vm->matvecStrA);
		input = vm->matvecStrA;
		//printf ("fnOrOpVec1Param: at not returnsVector, matvecStrA = %s\n", vm->matvecStrA);
		while (true) {
			input = tokenize(input, output);
			//printf ("fnOrOpVec1Param: loop, output = %s\n", output);
			if (output[0] == ']') break;
			if (output[0] == '[') continue;
			strcpy(vm->acc, output);

			success = false;
			//function name is in token
			if (isComplexNumber(vm->acc)) //complex
				success = stringToComplex(vm->acc, &cd);
			else if (isRealNumber(vm->acc)) //real number
				success = stringToDouble(vm->acc, &cd.real);
			FAILANDRETURNVAR(!success, vm->error, "%s bad arg.C", fitstr(vm->coadiutor, token, 8))
			c = cd.real + cd.imag*I; 
			crunningsum = suminternal(crunningsum, c);
			crunningsqsum = suminternal(crunningsqsum, cmul(c, c));
			crunningrsum = suminternal(crunningrsum, cdiv(1, c));
			count++;
		}
		crunning = callVectorMath1ParamFunction(fnindex, crunningsum, crunningsqsum, crunningrsum, count);
		cd.real = creal(crunning);
		cd.imag = cimag(crunning);
		success = complexToString(cd, vm->acc);
		FAILANDRETURNVAR(!success, vm->error, "fn %s failed", fitstr(vm->coadiutor, token, 9))
		//pop(&vm->userStack, NULL);
		//keep vector on stack
		push(&vm->userStack, vm->acc, METANONE);
	}
	return true;
}
