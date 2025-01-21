//push veriables and special vars
bool processDefaultPush(Machine* vm, char* token, char* retstr, StrackMeta* metap) {
	//returns variable value (if var exists) or the token in retstr
	ComplexDouble c;
	*metap = METASCALAR;
	if (varNameIsLegal(token)) {
		//token is a variable present in ledger
		if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_VECMAT) {
			getVariableStringVecMatValue(&vm->ledger, token, retstr);
			if (retstr[0] == '{')
				*metap = METAMATRIX;
			else
				*metap = METAVECTOR;
		} else if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_COMPLEX) {
			c = fetchVariableComplexValue(&vm->ledger, token);
			complexToString(c, retstr, vm->precision, vm->notationStr);
		} else if (findVariable(&vm->ledger, token) == VARIABLE_TYPE_STRING) {
			getVariableStringVecMatValue(&vm->ledger, token, retstr);
		} else {
			if (!hasDblQuotes(token))
				addDblQuotes(token);
			strcpy(retstr, token);
		}

	} else {
		//token is a real or complex number or string literal
		double dbl;
		char rlc = strIsRLC(token);
		bool s2c = stringToComplex(token, &c);
		bool s2d = stringToDouble(token, &dbl);
		//printf("processDefaultPush: A: token = %s\n", token);
		if (s2c || s2d) {
			if (s2d) {
				//token is an RLC type double -- 1.234e3l
				doubleToString(dbl, retstr, vm->precision, vm->notationStr);
				char RLCstr[2];
				RLCstr[1] = '\0';
				if (rlc) {
					RLCstr[0] = rlc;
					strcat(retstr, RLCstr);
				}
			} else {
				//printf("processDefaultPush: got complex = %s\n", token);
				strcpy(retstr, token);
			}
			//printf("processDefaultPush: A: retstr = %s\n", retstr);
		} else { 
			if (!hasDblQuotes(token))
				addDblQuotes(token);
			strcpy(retstr, token);
		}
	}
	return true;
}

