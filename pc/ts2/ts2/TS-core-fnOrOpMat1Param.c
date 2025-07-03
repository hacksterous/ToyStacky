#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "yasML.h"
#include "TS-core-miscel.h"
#include "TS-core-stack.h"
#include "TS-core-numString.h"

bool fnOrOpMat1Param(Machine* vm, const char* token, int fnindex, bool isTrig) {
	//this function can be called for evaluation a math function operating on a matrix
	//and returns a matrix
	//these functions are:
	//"sin", "cos", "tan", "cot", "sinh", "cosh", "tanh", "coth", 
	//"asin", "acos", "atan", "acot", "asinh", "acosh", "atanh", "acoth",
	//"exp", "log10", "log", "log2", "sqrt", "cbrt", "conj", 
	//"abs", "arg", "re", "im"
	//These will return a matrix of real or complex values

	bool success;
	Matrix m;
	int bigInt1Param = -1;

	//ToS is known to have a vector, no need to check for empty stack or non-vector item
	peek(&vm->userStack, vm->matvecStrA);
	strcpy(vm->matvecStrB, "{[");

	matbuild (&m, vm->matvecStrA);
	for (int i = 0; i < m.rows; i++) {
		for (int j = 0; j < m.columns; j++) {
			complexToString(m.numbers[i][j], vm->acc, vm->precision, vm->notationStr);
			//function name is in token
			//scalar, function argument is in vm->acc
			success = fn1ParamScalar(vm, token, fnindex, isTrig, bigInt1Param); //FIXME: always passing -1
			//fn1ParamScalar has the result in acc
			FAILANDRETURNVAR(!success, vm->error, "%s bad arg.", fitstr(vm->coadiutor, token, 8))
			strcat(vm->matvecStrB, vm->acc);
			if (j != m.columns - 1) strcat(vm->matvecStrB, " ");
			//printf ("fnOrOpMat1Param: loop, vm->matvecStrB = %s\n", vm->matvecStrB);
		}
		strcat(vm->matvecStrB, "]");
		if (i != m.rows - 1) strcat(vm->matvecStrB, " [");
	}
	strcat(vm->matvecStrB, "}");
	vm->lastXMeta = pop(&vm->userStack, vm->lastX);
	push(&vm->userStack, vm->matvecStrB, METAVECTOR);
	return true;

}
