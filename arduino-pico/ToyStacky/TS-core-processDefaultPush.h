bool processDefaultPush(Machine* vm, char* token, int8_t meta) {
	//SerialPrint(1, "processDefaultPush:-------------------Default condition with token = %s", token);
	//handle shorthands
	ComplexDouble c;
	char varName[MAX_VARNAME_LEN];
	if (strlen(token) == 1) {
		switch (token[0]) {
			case 'T': strcpy(token, "T0"); break;
			case 'X': strcpy(token, "T1"); break;
			case 'Y': strcpy(token, "T2"); break;
			case 'Z': strcpy(token, "T3"); break;
			case 'W': strcpy(token, "T4"); break;
			case 'V': strcpy(token, "T5"); break;
		}
	} 
	if (strlen(token) == 2) {
		//handle T0 - T9
		if (token[0] == 'T' && token[1] <= '9' && token[1] >= '0') {
			int n = (int)(token[1] - '0');
			int8_t localmeta = peekn(&vm->userStack, vm->coadiutor, n);
			//SerialPrint(1, "processDefaultPush:------------------- token = %s 'token[1]' = %d '0' = %d found T for %d = %s", token, token[1], '0', n, vm->coadiutor);
			FAILANDRETURN((localmeta == -1), vm->error, "stack full", NULLFN)
			push(&vm->userStack, vm->coadiutor, METANONE);
			return true;
		}
	} else if (strlen(token) == 3) {
		//handle T10 - T99
		if (token[0] == 'T' && token[1] <= '9' && token[1] > '0' && 
				token[2] <= '9' && token[2] >= '0') {
			int n = (int)((token[1] - '0') * 10 + (token[2] - '0'));
			int8_t localmeta = peekn(&vm->userStack, vm->coadiutor, n);
			FAILANDRETURN((localmeta == -1), vm->error, "stack full", NULLFN)
			push(&vm->userStack, vm->coadiutor, METANONE);
			return true;
		}
	}
	if (strcmp(token, "b") == 0) {
		push(&vm->userStack, vm->bak, meta);
	} else if (strcmp(token, "a") == 0) {
		push(&vm->userStack, vm->acc, meta);
		//SerialPrint(1, "-------------------Default condition - found acc");
	} else if (variableVetted(token) || (strcmp(token, "va") == 0)) {
		if (strcmp(token, "va") == 0) strcpy(token, VACC);;
		if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_VECMAT) {
			c = fetchVariableComplexValue(&vm->ledger, token);
			//SerialPrint(1, "-------------------Default condition - found token got c.real = %g", c.real);
			int count = (int) c.real;
			for (int i = (int) count - 1; i >= 0; i--) {
				int8_t localmeta;
				strcpy(varName, token);
				makeStrNum(varName, MAX_VARNAME_LEN, i);
				c = fetchVariableComplexValue(&vm->ledger, varName);
				if (i == (int) count - 1) localmeta = METASTARTVECTOR;
				else if (i == 0) localmeta = METAENDVECTOR;
				else localmeta = METAMIDVECTOR;
				//SerialPrint(1, "-------------------Default condition - found vacc i = %d got c.real = %g", i, c.real);
				if (complexToString(c, vm->coadiutor)) {
					push(&vm->userStack, vm->coadiutor, localmeta);
				}
			}
		} else if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_COMPLEX) {
			c = fetchVariableComplexValue(&vm->ledger, token);
			if (complexToString(c, vm->coadiutor)) {
				push(&vm->userStack, vm->coadiutor, meta);
			}
		} else if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_STRING) {
			if (getVariableStringValue(&vm->ledger, token, vm->coadiutor)) {
				push(&vm->userStack, vm->coadiutor, meta);
			}
		} else {
			FAILANDRETURNVAR(true, vm->error, "no var %s", fitstr(vm->coadiutor, token, 6))
		}
	} else {
		//SerialPrint(1, "processDefaultPush:------------------- ELSE Default condition with token = %s", token);
		double dbl;
		char rlc = strIsRLC(token);
		bool s2d = stringToDouble(token, &dbl);
		if (stringToComplex(token, &c) || s2d) {
			printf("processDefaultPush:------------------- 1. ELSE Default condition with token = %s, no-has-dbl-quotes = %d, dbl = %g\n\n", token, !hasDblQuotes(token), dbl);
			if (s2d) {
				doubleToString(dbl, vm->coadiutor);
				char RLCstr[2];
				RLCstr[1] = '\0';
				if (rlc) {
					RLCstr[0] = rlc;
					//printf("processDefaultPush: RLCstr = %s\n", RLCstr);
					strcat(vm->coadiutor, RLCstr);
				}
				push(&vm->userStack, vm->coadiutor, meta);
			} else {
				push(&vm->userStack, token, meta);
			}
		} else if (!hasDblQuotes(token) && meta != METASTARTVECTOR) {
			//don't want to add double quotes to '['
			addDblQuotes(token);
			push(&vm->userStack, token, meta);
		} else {
			push(&vm->userStack, token, meta);
		}
	}
	return true;
}
