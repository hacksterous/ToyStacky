bool fnOrOpVec2Param(Machine* vm, const char* token, int fnindex, int8_t cmeta, int8_t meta, bool returnsVector) {
	//this function can be called for evaluation a math function operating on a vector
	//(returnsVector = true)
	//these functions are:
	//	"atan2", "pow",
	//	"max", "min",
	//	ADDTOKEN, SUBTOKEN,
	//	MULTOKEN, DIVTOKEN,
	//	MODTOKEN, GTTOKEN,
	//	LTTOKEN, GTETOKEN,
	//	LTETOKEN, EQTOKEN,
	//	NEQTOKEN, PARTOKEN
	//These will return a vector of real or complex values
	//and result is a vector, e.g., [1 2 3]sin returns a vector [0.84 0.91 0.14].

	//If returnsVector = false, the return value is a scalar (real or complex) -- these functions are:
	//"dot" (dot product = sum (x*y)
	//[1 2 3] [4 5 6]dot returns 32

	bool success = false;
	char* input = (char*) NULL;
	char output[MAX_TOKEN_LEN];
	char* input2 = (char*) NULL;
	char output2[MAX_TOKEN_LEN];
	bool bothVectors = false;
	if (returnsVector) {
		if (cmeta == METAVECTOR && meta == METANONE) {
			//ToS is a vector, ToS-1 is a scalar
			peek(&vm->userStack, vm->matvecStrA);
			peekn(&vm->userStack, vm->bak, 1);
			input = vm->matvecStrA;
		}
		else if (cmeta == METANONE && meta == METAVECTOR) {
			//ToS-1 is a vector, ToS is a scalar
			peek(&vm->userStack, vm->bak);
			peekn(&vm->userStack, vm->matvecStrA, 1);
			input = vm->matvecStrA;
		} else if (cmeta == METAVECTOR && meta == METAVECTOR) {
			//both ToS-1 and ToS are vectors
			peek(&vm->userStack, vm->matvecStrA);
			peekn(&vm->userStack, vm->matvecStrB, 1);
			bothVectors = true;
			input = vm->matvecStrA;
			input2 = vm->matvecStrB;
		}
		//printf("fnOrOpVec2Param: at start -- input = %s, input2 = %s\n", input, input2);
		strcpy(vm->matvecStrC, "[");
		while (true) {
			if (bothVectors) {
				input2 = tokenize(input2, output2);
				if (output2[0] == ']') break;
				if (output2[0] != '[') strcpy(vm->bak, output2);
			}
			//printf ("fnOrOpVec2Param: loop, output = %s, output2 = %s\n", output, output2);
			input = tokenize(input, output);
			if (output[0] == ']') break;
			if (output[0] == '[' || output2[0] == '[') continue;
			strcpy(vm->acc, output);
			//function name is in token
			printf("fnOrOpVec2Param: while loop -- acc = %s, bak = %s\n", vm->acc, vm->bak);

			//(X) push the two scalars
			push(&vm->userStack, vm->bak, METANONE); //push vector element
			push(&vm->userStack, vm->acc, METANONE); //push vector element
			
			success = fnOrOp2Param(vm, token, fnindex);
			if (!success) {
				//if the scalar op fails, pop out the scalars pushed in (X)
				pop(&vm->userStack, NULL);
				pop(&vm->userStack, NULL);
			}
			FAILANDRETURNVAR(!success, vm->error, "%s bad arg.B", fitstr(vm->coadiutor, token, 8))
			
			//fnOrOp2Param has pushed the result onto the stack and 
			//popped out the two scalar items pushed earlier in (X)
			pop(&vm->userStack, vm->acc); 
			strcat(vm->matvecStrC, vm->acc);
			if (bothVectors) {
				if (input[0] != ']' && input2[0] != ']') strcat(vm->matvecStrC, " ");
			} else {
				if (input[0] != ']') strcat(vm->matvecStrC, " ");
			}
			//printf ("fnOrOpVec2Param: loop, vm->matvecStrC = %s\n", vm->matvecStrC);
		}
		strcat(vm->matvecStrC, "]");
		pop(&vm->userStack, NULL);
		pop(&vm->userStack, NULL);
		push(&vm->userStack, vm->matvecStrC, METAVECTOR);
	} else {
		//only the dot product operator returns a scalar
		FAILANDRETURN((cmeta != METAVECTOR || meta != METAVECTOR), vm->error, "E: require vectors.", NULLFN)
		//both ToS-1 and ToS are vectors
		peek(&vm->userStack, vm->matvecStrA);
		peekn(&vm->userStack, vm->matvecStrB, 1);
		input = vm->matvecStrA;
		input2 = vm->matvecStrB;

		double complex c = 0.0;
		ComplexDouble cd;
		cd.real = cd.imag = 0.0;
		//printf("fnOrOpVec2Param: scalar result at start -- input = %s, input2 = %s\n", input, input2);
		while (true) {
			input2 = tokenize(input2, output2);
			if (output2[0] == ']') break;
			if (output2[0] != '[') strcpy(vm->bak, output2);
			//printf ("fnOrOpVec2Param: scalar result loop, output = %s, output2 = %s\n", output, output2);
			input = tokenize(input, output);
			if (output[0] == ']') break;
			if (output[0] == '[' || output2[0] == '[') continue;
			strcpy(vm->acc, output);
			//function name is in token
			//printf("fnOrOpVec2Param: scalar result while loop -- acc = %s, bak = %s\n", vm->acc, vm->bak);

			//(X) push the two scalars
			push(&vm->userStack, vm->bak, METANONE); //push vector element
			push(&vm->userStack, vm->acc, METANONE); //push vector element
			
			success = fnOrOp2Param(vm, token, fnindex); //fnindex = 6 is multiplication
			if (!success) {
				//if the scalar op fails, pop out the scalars pushed in (X)
				pop(&vm->userStack, NULL);
				pop(&vm->userStack, NULL);
			}
			FAILANDRETURNVAR(!success, vm->error, "%s bad arg.B", fitstr(vm->coadiutor, token, 8))
			
			//fnOrOp2Param has pushed the result onto the stack and 
			//popped out the two scalar items pushed earlier in (X)
			pop(&vm->userStack, vm->acc);

			if (isComplexNumber(vm->acc)) //complex
				success = stringToComplex(vm->acc, &cd);
			else if (isRealNumber(vm->acc)) //real number
				success = stringToDouble(vm->acc, &cd.real);

			c += cd.real + cd.imag * I;
		}
		cd.real = creal(c);
		cd.imag = cimag(c);
		if (dabs(cd.real) < DOUBLE_EPS) cd.real = 0.0;
		if (dabs(cd.imag) < DOUBLE_EPS) cd.imag = 0.0;
		success = complexToString(cd, vm->acc) && success; //result in coadiutor
		pop(&vm->userStack, NULL);
		pop(&vm->userStack, NULL);
		push(&vm->userStack, vm->acc, METANONE);
	}

	return true;
}
