bool processPop(Machine* vm, char* token) {
	int8_t meta;
	//check if the user was in the middle of entering a vector
	//or matrix
	meta = peek(&vm->userStack, NULL);
	FAILANDRETURN((meta == -1), vm->error, "Stack empty", NULLFN)		
	double dbl;
	if (token[0] == '\0') {
		//@ will pop 1 element of a vector/matrix
		//@1 will pop 1 entire vector/matrix
		strcpy(vm->bak, vm->acc); //save acc	
		pop(&vm->userStack, vm->acc);
		//if (meta == METANONE) {
		//	//just pop out a scalar in acc
		//	strcpy(vm->bak, vm->acc); //save acc
		//	pop(&vm->userStack, vm->acc);
		//} else {
		//	//printf("processPop: calling pop1VecMat\n");
		//	pop1VecMat(vm, meta);
		//}
	} else if (strcmp(token, "b") == 0) {
		strcpy(vm->coadiutor, vm->bak); //save bak
		pop(&vm->userStack, vm->bak);
	} else if (variableVetted(token)) {
		ComplexDouble c;
		//POP var -- pop the top of stack into variable 'var'
		meta = pop(&vm->userStack, vm->bak);
		//createVariable will overwrite the variable if  it already exists
		if (stringToComplex(vm->bak, &c)) {
			//printf("processPop:------ calling createVar complex with varname = %s and value = %s\n", token, vm->bak);
			createVariable(&vm->ledger, token, VARIABLE_TYPE_COMPLEX, c, "");
		} else if (stringToDouble(vm->bak, &dbl)) {
			//printf("processPop:------ calling createVar real with varname = %s and value = %s\n", token, vm->bak);
			createVariable(&vm->ledger, token, VARIABLE_TYPE_COMPLEX, MKCPLX(dbl), "");
		} else {
			//printf("processPop:------ calling createVar string with varname = %s and value = %s\n", token, vm->bak);
			createVariable(&vm->ledger, token, VARIABLE_TYPE_STRING, MKCPLX(0), vm->bak);
		}
	} else { 
		//a pop but the var does not have a variable name
		//or is a number -- number means pop n entities out of stack
		int howMany = 0;
		if (POSTFIX_BEHAVE) { //@<number> is the postfix behavior -- we want <number> @@
			if (stringToDouble(token, &dbl)) howMany = (int) dbl;
		} else if (strcmp(token, "@") == 0) {
			pop(&vm->userStack, vm->bak);
			if (stringToDouble(vm->bak, &dbl)) howMany = (int) dbl;
		}
		if (howMany >= 0) popNItems(vm, howMany);
		else FAILANDRETURN(true, vm->error, "illegal POP B", NULLFN); //should never happen
	}
	return true;
}
