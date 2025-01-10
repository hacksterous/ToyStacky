bool popIntoVariable (Machine* vm, char* var) {
	ComplexDouble c;
	double dbl;

	printf("popIntoVariable: entered with var %s\n", var);
	//POP var -- pop the top of stack into variable 'var'
	//If there is no variable var, then pop into a variable 'a'
	//createVariable will overwrite the variable if it already exists
	
	int8_t meta = peek(&vm->userStack, NULL);
	if (meta == METANONE) {
		pop(&vm->userStack, vm->bak);
		if (strcmp(var, "__f") == 0) {
			//special variable to set frequency for impedance calculation
			if (stringToDouble(vm->bak, &dbl)) dbl = fabs(dbl);
			else if (stringToComplex(vm->bak, &c)) dbl = fabs(c.real);
			if (dbl < DOUBLE_EPS) dbl = 0.0;
			vm->frequency = dbl;
			//printf("processPop: set vm->frequency with %lf\n", vm->frequency);
		} else if (stringToComplex(vm->bak, &c)) {
			printf("popIntoVariable:------ calling createVar complex with varname = %s and value = %s\n", var, vm->bak);
			createVariable(&vm->ledger, var, VARIABLE_TYPE_COMPLEX, c, "");
		} else if (stringToDouble(vm->bak, &dbl)) {
			//printf("popIntoVariable:------ calling createVar real with varname = %s and value = %s\n", var, vm->bak);
			createVariable(&vm->ledger, var, VARIABLE_TYPE_COMPLEX, MKCPLX(dbl), "");
		} else {
			//printf("popIntoVariable:------ calling createVar string with varname = %s and value = %s\n", var, vm->bak);
			createVariable(&vm->ledger, var, VARIABLE_TYPE_STRING, MKCPLX(0), vm->bak);
		}
	} else if (meta == METAVECTOR || meta == METAMATRIX) {
		pop(&vm->userStack, vm->matvecStrB);
		//printf("popIntoVariable:------ calling createVar complex with varname = %s and value = %s\n", var, vm->matvecStrB);
		createVariable(&vm->ledger, var, VARIABLE_TYPE_VECMAT, MKCPLX(0), vm->matvecStrB);
	} else return false;
	
	return true;
}

bool processPop(Machine* vm, char* token) {
	int8_t meta;
	double dbl;
	char accliteral[] = "acc"; 
	//printf("processPop: entered with token %s\n", token);
	//check if the user was in the middle of entering a vector
	//or matrix
	meta = peek(&vm->userStack, NULL);
	FAILANDRETURN((meta == -1), vm->error, "E:Stack empty", NULLFN)		
	if (strcmp(token, "@") == 0) {
		//'@' found
		//the value in ToS-1 is popped into the variable name at ToS
		if (meta == METANONE) {
			peek(&vm->userStack, vm->acc); //this is the destination variable name
			char* acc = &(vm->acc[0]);
			//printf("processPop: A: acc = %s\n", vm->acc);
			if (hasDblQuotes(vm->acc)) {
				removeDblQuotes(vm->acc);
				acc = &(vm->acc[1]);
			}
			if (varNameIsLegal(acc)) {
				strcpy(vm->acc, acc);
				//pop the variable name (already in vm->acc)
				pop(&vm->userStack, NULL);
			} else {
				//don't pop var name
				strcpy(vm->acc, accliteral); //default variable
			}
			//printf("processPop: calling popIntoVariable with %s\n", vm->acc);
			popIntoVariable (vm, vm->acc);
		} else {
			//vector or matrix
			popIntoVariable (vm, accliteral);
		}
	} else if (strcmp(token, "@!") == 0) {
		//clear stack
		initStacks(vm);
	} else if (strcmp(token, "@@") == 0) {
		FAILANDRETURN((meta != METANONE), vm->error, "need real", NULLFN)
		peek(&vm->userStack, vm->acc); //this is the destination variable name
		bool success = stringToDouble(vm->acc, &dbl);
		FAILANDRETURN(!success, vm->error, "need real", NULLFN)
		int howMany = (int) dbl;
		//pop the variable count already in vm->acc
		pop(&vm->userStack, NULL);
		//printf("processPop: B: acc = %s, dbl = %g\n", acc, dbl);
		if (howMany >= 0) popNItems(vm, howMany);
	}
	return true;
}

