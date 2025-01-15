//push veriables and special vars
bool processDefaultPush(Machine* vm, char* token) {
	//printf("processDefaultPush:------------------- with token = %s", token);
	//handle shorthands
	ComplexDouble c;
	if (varNameIsLegal(token)) {
		//token is a variable present in ledger
		if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_VECMAT) {
			if (getVariableStringVecMatValue(&vm->ledger, token, vm->matvecStrA)) {
				push(&vm->userStack, vm->matvecStrA, METAVECTOR);
			}
		} else if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_COMPLEX) {
			c = fetchVariableComplexValue(&vm->ledger, token);
			if (complexToString(c, vm->coadiutor, vm->precision, vm->notationStr)) {
				push(&vm->userStack, vm->coadiutor, METANONE);
			}
		} else if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_STRING) {
			if (getVariableStringVecMatValue(&vm->ledger, token, vm->coadiutor)) {
				push(&vm->userStack, vm->coadiutor, METANONE);
			}
		} else {
			if (!hasDblQuotes(token)) {
				addDblQuotes(token);
				push(&vm->userStack, token, METANONE);
				//FAILANDRETURNVAR(true, vm->error, "no var %s", fitstr(vm->coadiutor, token, 6))
			} else {
				push(&vm->userStack, token, METANONE);
			}
		}

	} else {
		//token is a real or complex number or string literal
		//printf("processDefaultPush:------------------- 0. ELSE Default condition with token = %s\n", token);
		double dbl;
		char rlc = strIsRLC(token);
		bool s2d = stringToDouble(token, &dbl);
		if (stringToComplex(token, &c) || s2d) {
			//printf("processDefaultPush:------------------- 1. ELSE Default condition with token = %s, no-has-dbl-quotes = %d, dbl = %g\n", token, !hasDblQuotes(token), dbl);
			//printf("processDefaultPush:------------------- 1. ELSE Default condition with len token = %lu\n", strlen(token));
			if (s2d) {
				//token is an RLC type double -- 1.234e3l
				doubleToString(dbl, vm->coadiutor, vm->precision, vm->notationStr);
				char RLCstr[2];
				RLCstr[1] = '\0';
				if (rlc) {
					RLCstr[0] = rlc;
					//printf("processDefaultPush: RLCstr = %s\n", RLCstr);
					strcat(vm->coadiutor, RLCstr);
				}
				//printf("processDefaultPush: len coad = %lu\n", strlen(vm->coadiutor));
				push(&vm->userStack, vm->coadiutor, METANONE);
			} else {
				push(&vm->userStack, token, METANONE);
			}
		} else if (!hasDblQuotes(token)) {
			//printf("processDefaultPush:------------------- 2. ELSE Default condition with token = %s, no-has-dbl-quotes = %d\n", token, !hasDblQuotes(token));
			addDblQuotes(token);
			push(&vm->userStack, token, METANONE);
		} else {
			push(&vm->userStack, token, METANONE);
		}
	}

	return true;
}
