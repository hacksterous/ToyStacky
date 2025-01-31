bool matMulDiv(Machine* vm, int fnindex, Matrix* m1, Matrix* m2) {
	bool success;
	if (fnindex == 6) { //multiply
		//m1.columns should equal m2.rows for matrix multiplication
		success = matmultiply(m1, m2, &vm->matrixC);
		FAILANDRETURN((!success), vm->error, "mat * failed.", NULLFN)
		success = matrixToString (&vm->matrixC, vm->matvecStrC, vm->precision, vm->notationStr);
		return success;
	} else if (fnindex == 7) { //divide
		//m1 / m2 = m1 * inverse(m2)
		ComplexDouble c;
		FAILANDRETURN((m2->rows != m2->columns), vm->error, "non sqr mat2.", NULLFN)
		success = matdeterminant(m2, &c);
		FAILANDRETURN((!success), vm->error, "mat2 singular.", NULLFN)
		success = matinversion(m2, &vm->matrixC); //vm->matrixC has the inversion
		FAILANDRETURN((!success), vm->error, "mat inv failed.", NULLFN)
		success = matmultiply(m1, &vm->matrixC, m2); //m2 has the result
		FAILANDRETURN((!success), vm->error, "mat / failed.", NULLFN)
		success = matrixToString (m2, vm->matvecStrC, vm->precision, vm->notationStr);
		return success;
	} else if (fnindex == 15) { //parallel -- does four matrix operations, mul, add, inv, mul
		ComplexDouble c;
		//m1.columns should equal m2.rows for matrix multiplication
		//m1 = vm->matrixA
		//m2 = vm->matrixB
		success = matmultiply(m1, m2, &vm->matrixC);
		FAILANDRETURN((!success), vm->error, "mat // failed.", NULLFN)
		success = matadd(m1, m2); //result is in m1 (vm->matrixA)
		FAILANDRETURN((!success), vm->error, "mat // failed2.", NULLFN)
		FAILANDRETURN((m1->rows != m1->columns), vm->error, "non sqr mat div.", NULLFN)
		success = matdeterminant(m1, &c);
		FAILANDRETURN((!success), vm->error, "mat div singular.", NULLFN)
		success = matinversion(m1, m2); //m2 (vm->matrixB) has the inversion
		FAILANDRETURN((!success), vm->error, "mat // failed3.", NULLFN)
		success = matmultiply(&vm->matrixC, m2, m1); //result in m1
		FAILANDRETURN((!success), vm->error, "mat * failed.", NULLFN)
		success = matrixToString (m1, vm->matvecStrC, vm->precision, vm->notationStr);
	}
	return false;
}

bool fnOrOpMat2Param(Machine* vm, const char* token, int fnindex, int8_t cmeta, int8_t meta) {
	//this function can be called for evaluation a math function operating on a matrix
	//these functions are:
	//	"atan2", "pow",
	//	"max", "min",
	//	ADDTOKEN, SUBTOKEN,
	//	MULTOKEN, DIVTOKEN,
	//	MODTOKEN, GTTOKEN,
	//	LTTOKEN, GTETOKEN,
	//	LTETOKEN, EQTOKEN,
	//	NEQTOKEN, PARTOKEN
	//These will return a matrix of real or complex values

	bool success = false;
	int matrixCombo = 0;
	if (cmeta == METAMATRIX && meta == METASCALAR) {
		//ToS is a matrix, ToS-1 is a scalar
		peek(&vm->userStack, vm->matvecStrA);
		peekn(&vm->userStack, vm->dummy, 1);
		matbuild (&vm->matrixA, vm->matvecStrA);
		matrixCombo = 2;
	}
	else if (cmeta == METASCALAR && meta == METAMATRIX) {
		//ToS-1 is a matrix, ToS is a scalar
		peek(&vm->userStack, vm->dummy);
		peekn(&vm->userStack, vm->matvecStrA, 1);
		matbuild (&vm->matrixA, vm->matvecStrA);
		matrixCombo = 1;
		//printf("fnOrOpMat2Param: at start -- matrixCombo %d, dummy = %s\n", matrixCombo, vm->dummy);
	} 
	else if (cmeta == METAMATRIX && meta == METAMATRIX) {
		//both ToS-1 and ToS are vectors
		peek(&vm->userStack, vm->matvecStrA);
		peekn(&vm->userStack, vm->matvecStrB, 1);
		matrixCombo = 3;
		matbuild (&vm->matrixA, vm->matvecStrA);
		matbuild (&vm->matrixB, vm->matvecStrB);
		//for matrix multiply and divide (= multiply vm->matrixA with inversion of vm->matrixB),
		//result will be in matvecStrC
		if (fnindex == 6 || fnindex == 7) {
			success = matMulDiv(vm, fnindex, &vm->matrixA, &vm->matrixB);
			if (success) {
				//matMulDiv will copy any error message to vm->error
				//and the result in vm->matvecStrC
				pop(&vm->userStack, vm->lastX);
				pop(&vm->userStack, vm->lastY);
				push(&vm->userStack, vm->matvecStrC, METAMATRIX);
			}
			return success;
		}
		else if (fnindex == 15) { //parallel operator FIXME: don't support at present
			FAILANDRETURN(!success, vm->error, "no par support.", NULLFN)
		}
		//for other operators, matrices must be of same size
		FAILANDRETURNVAR((vm->matrixA.rows != vm->matrixB.rows) || (vm->matrixA.columns != vm->matrixB.columns), vm->error, "not same size %s", fitstr(vm->coadiutor, token, 6))
	}
	else if (cmeta == -1 || meta == -1) {
		FAILANDRETURNVAR(!success, vm->error, "stack empty %s", fitstr(vm->coadiutor, token, 6))
	}
	else {
		FAILANDRETURNVAR(!success, vm->error, "no support %s", fitstr(vm->coadiutor, token, 7))
	}
	//printf("fnOrOpMat2Param: at start -- input = %s, input2 = %s\n", input, input2);
	strcpy(vm->matvecStrC, "{[");
	for (int i = 0; i < vm->matrixA.rows; i++) {
		for (int j = 0; j < vm->matrixA.columns; j++) {
			if (matrixCombo == 3) {
			//printf ("fnOrOpMat2Param: loop, element = %s, element2 = %s\n", element, element2);
				complexToString(vm->matrixB.numbers[i][j], vm->dummy, vm->precision, vm->notationStr);
			}
			//function name is in token
			//printf("fnOrOpMat2Param: while loop -- acc = %s, dummy = %s\n", vm->acc, vm->dummy);
			
			complexToString(vm->matrixA.numbers[i][j], vm->acc, vm->precision, vm->notationStr);
			//(X) push the two scalars for the math op
			if (matrixCombo == 1) {
				push(&vm->userStack, vm->acc, METASCALAR); //push matrix element
				push(&vm->userStack, vm->dummy, METASCALAR);
			}
			else if (matrixCombo == 2 || matrixCombo == 3) {
				push(&vm->userStack, vm->dummy, METASCALAR); //push matrix element
				push(&vm->userStack, vm->acc, METASCALAR);
			}
			
			success = fnOrOp2Param(vm, token, fnindex);
			if (!success) {
				//if the scalar op fails, pop out the scalars pushed in (X)
				pop(&vm->userStack, NULL);
				pop(&vm->userStack, NULL);
			}
			FAILANDRETURNVAR(!success, vm->error, "%s bad arg.", fitstr(vm->coadiutor, token, 8))
			
			//fnOrOp2Param has pushed the result onto the stack and 
			//popped out the two scalar items pushed earlier in (X)
			pop(&vm->userStack, vm->acc); 
			strcat(vm->matvecStrC, vm->acc);
			if (j != vm->matrixA.columns -1) strcat(vm->matvecStrC, " ");
			//printf ("fnOrOpMat2Param: loop, vm->matvecStrC = %s\n", vm->matvecStrC);
			strcat(vm->matvecStrC, "]");
		}
		strcat(vm->matvecStrB, "]");
		if (i != vm->matrixA.rows - 1) strcat(vm->matvecStrB, " [");
	}
	strcat(vm->matvecStrB, "}");
	pop(&vm->userStack, vm->lastX);
	pop(&vm->userStack, vm->lastY);
	push(&vm->userStack, vm->matvecStrC, METAMATRIX);

	return true;
}
