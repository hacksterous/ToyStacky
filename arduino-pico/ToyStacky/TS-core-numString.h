bool isRealNumber(const char *token) {
	int i = 0;
	int len = strlen(token);
	int hasDecimal = 0;
	int hasExponent = 0;
	int hasSign = 0;

	// Check for negative sign
	if (token[i] == '-' || token[i] == '+') {
		hasSign = 1;
		i++;
		if (i == len) {
			return false;  // '-' alone is not a valid number
		}
	}

	// Check for digits or decimal point
	while (i < len) {
		if (isdigit(token[i])) {
			i++;
		} else if (token[i] == '.') {
			if (hasDecimal || hasExponent) {
				return false;  // Multiple decimal points or decimal point after exponent is not valid
			}
			hasDecimal = 1;
			i++;
		} else if (token[i] == 'e' || token[i] == 'E') {
			if (hasExponent) {
				return false;  // Multiple exponents are not valid
			}
			hasExponent = 1;
			i++;
			if (i < len && (token[i] == '+' || token[i] == '-')) {
				i++;  // Allow optional sign after exponent
			}
		} else {
			if ((token[i] == 'r' || token[i] == 'l' || token[i] == 'c') &&
				(i == len - 1)) return true;
			else return false;  // Invalid character
		}
	}

	//make sure the token is fully parsed
	if (hasSign && i == 1) {
		return false;  // Only a sign is not a valid number
	}
	return (bool)(i == len);
}

bool isComplexNumber(const char *input) {
	int i = 0;
	if (strlen(input) < 2) return false;
	bool success = false;
	//SerialPrint(1, "isComplexNumber:------------------- input = %s", input);
	while (isspace(input[i])) {
		i++; //skip leading spaces
	}
	if (input[i++] == '(') success = true;
	else return false;
	//SerialPrint(1, "isComplexNumber:------------------- len input = %lu, i = %d success = %d", strlen(input), i, success);
	while (input[i] != ')') {
		i++; //
	}
	if (input[i] == ')' && success) return true;
	return false;
}

int parseComplex(const char* input, char* substring1, char* substring2) {
	//a      --> real number
	//(b)    --> imaginary number
	//(a b)  --> complex number
	//SerialPrint(1, "parseComplex: 0. input: '%s'", input);
	int i = 0;
	while (isspace(input[i])) {
		i++; //Skip leading spaces
	}
	//SerialPrint(1, "parseComplex: 0. i = %d", i);
	//SerialPrint(1, "parseComplex: 0. input[%d]: '%c'", i, input[i]);
	if (input[i] == '(') {
		i++;
		while (isspace(input[i])) {
			i++; //skip spaces
		}
	}
	//SerialPrint(1, "parseComplex: 1. i = %d", i);
	//first string
	int j = 0;
	while (isalpha(input[i]) || isdigit(input[i]) || input[i] == '+' || input[i] == '-' || input[i] == '.') {
		//SerialPrint(1, "parseComplex: i = %d, input[%d] = '%c'", i, i,input[i]);
		substring1[j++] = input[i++];
	}
	substring1[j] = '\0'; //null-terminate the first substring
	//SerialPrint(1, "parseComplex: Substring 1: '%s'", substring1);
	while (isspace(input[i])) {
		i++; //skip spaces after first string
	}

	if (isalpha(input[i]) || isdigit(input[i]) || input[i] == '+' || input[i] == '-' || input[i] == '.') {
		int k = 0;
		while (isalpha(input[i]) || isdigit(input[i]) || input[i] == '+' || input[i] == '-' || input[i] == '.') {
			substring2[k++] = input[i++];
		}
		substring2[k] = '\0'; // null-terminate the second substring

		while (isspace(input[i])) {
			i++; //skip trailing spaces after second string
		}
		if (input[i] == ')' && j > 0 && k > 0) {
			//successfully parsed the second string
			//SerialPrint(1, "parseComplex: Done Substring 2: '%s'", substring1);
			return 0;
		} else {
			return i;
		}
	} else if (input[i] == ')') {
		substring2[0] = '\0'; //no second part
		return 0;
	}
	return i; //invalid expression format - return bad character position
}

char strIsRLC(char* str) {
	int32_t len = strlen(str);
	if (str[len-1] == 'r' || str[len-1] == 'l' || str[len-1] == 'c') return str[len-1];
	else return 0;
}

bool stringToDouble(char* str, double* dbl) {
	//SerialPrint(1, "stringToDouble: entered ------------------- str = %s", str);
	char* endPtr;
	int32_t len = strlen(str);
	char rlc = strIsRLC(str);
	if (rlc) {
		str[len-1] = '\0';
	}
	*dbl = strtod(str, &endPtr);
	//if (endPtr == str) {
	if (*endPtr != '\0') {
		//SerialPrint(1, "stringToDouble: done with false ------------------- dbl = %g", *dbl);
		return false;
	} else {
		//SerialPrint(1, "stringToDouble: done with true ------------------- dbl = %g", *dbl);
		if (rlc) str[len-1] = rlc;
		if (errno == ERANGE) return false;
		if (abs(*dbl) == DOUBLE_EPS) return false;
		return true;
	}
}

bool stringToComplex(const char *input, ComplexDouble* c) {
	//the input is guaranteed not to have to leading or trailing spaces
	//(a, b) -> a + ib
	//(b) -> ib
	//SerialPrint(1, "stringToComplex: just entered input = %s", input);
	if (input[0] != '(' ||  input[strlen(input)-1] != ')') return false;
	char str1[SHORT_STRING_SIZE];
	char str2[SHORT_STRING_SIZE];
	int failpoint = parseComplex(input, str1, str2);
	//SerialPrint(1, "stringToComplex: parseComplex returned %d str1 = %s --- str2 = %s", failpoint, str1, str2);
	double r;
	double i;
	bool success;
	if (failpoint != 0) return false;
	if (str1[0] != '\0' && str2[0] != '\0') {
		success = stringToDouble(str1, &r);
		success = success && stringToDouble(str2, &i);
		if (!success) {
			return false;
		} else {
			c->real = r;
			c->imag = i;
			return true;
		}
	} else if (str1[0] != '\0' && str2[0] == '\0') {
		success = stringToDouble(str1, &i);
		if (!success) {
			return false;
		} else {
			c->real = 0;
			c->imag = i;
			return true;
		}
	}
	return false;
}

/* reverse:  reverse string s in place */
void reverse(char s[]) {
	int i, j;
	char c;

	for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

/* itoa:  convert n to characters in s */
void itoa(int n, char s[]) {
	int i, sign;

	if ((sign = n) < 0)  /* record sign */
		n = -n;		  /* make n positive */
	i = 0;
	do {	   /* generate digits in reverse order */
		s[i++] = n % 10 + '0';   /* get next digit */
	} while ((n /= 10) > 0);	 /* delete it */
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}

char* lcase(char* token) {
	for (int i = 0; token[i] != '\0'; i++) {
		token[i] = tolower(token[i]);
	}
	return token;
}

bool doubleToString(double value, char* buf) {
	if (value == INFINITY || value == -INFINITY) return false;
	if (alm0double(value)) {
		strcpy(buf, "0");
		return true;
	}
	char fmt[7];
	//for small numbers (with exponent -10 etc), 16 decimal places
	//will give wrong digits - since these are beyond the precision
	//of double floats
	sprintf(buf, "%.16g", value);
	if (strcmp(lcase(buf), "inf") == 0 || strcmp(lcase(buf), "-inf") == 0) return false;
	if (strcmp(lcase(buf), "nan") == 0 || strcmp(lcase(buf), "-nan") == 0) return false;

	//printf("doubleToString: ------ At start value string = %s\n", buf);
	int i = 0;
	if (buf[0] == '-') i++; //skip any opening '-'
	bool expoIsNeg = false;
	for (i = i; buf[i] != '\0'; i++) {
		if (buf[i] == '-') {
			i++;
			expoIsNeg = true;
			break;
		}
	}
	if (expoIsNeg == false) return true; 	
	//printf("doubleToString: ------ Minus found at i = %d\n", i);
	char* endPtr;
	int expo = strtod(buf + i, &endPtr);
	//printf("doubleToString: ------ Expo = %d\n", expo);
	if (expoIsNeg == true) {
		//printf("doubleToString: ------ expoIsNeg = %d\n", expoIsNeg);
		if (expo > 13) {
			strcpy(fmt, "%.3g");
		} else {
			int numDecimals = 15 - expo;
			if (numDecimals < 6) numDecimals = 6;
			strcpy(fmt, "%.");
			itoa(numDecimals, &fmt[2]);
			strcat(fmt, "g");
		}
	}
	sprintf(buf, fmt, value);

	//printf("doubleToString ------ buf = %s\n", buf);
	if (buf == NULL) return false;
	return true;
}

bool complexToString(ComplexDouble c, char* value) {
	char r[SHORT_STRING_SIZE];
	char i[SHORT_STRING_SIZE];
	bool goodnum = doubleToString(c.real, r);
	if (!goodnum) return false;
	goodnum = doubleToString(c.imag, i);
	if (!goodnum) return false;
	if (alm0double(c.imag)) {
		//imag value is 0 - return r
		strcpy(value, r);
		//SerialPrint(3, "complexToString: returning with value ", value, "\r\n");
		return true;
	}
	//imag value is not zero, return one of
	//(1) (r i)
	//(2) (i)
	strcpy(value, "(");
	if (!(alm0double(c.real))) {
		strcat(value, r);
		strcat(value, " ");
	}
	strcat(value, i);
	strcat(value, ")");
	return true;
}

bool hasDblQuotes(char* input) {
    if (input == NULL)
        return NULL; //bad input 
	//must be trimmed of spaces
    int32_t len = strlen(input);
	return (len > 2 && input[0] == '"' && input[len - 1] == '"');
}

char* removeDblQuotes(char* input) {
	//printf ("removeDblQuotes: input is: %s\n", input);
	if (input[0] != '"') return input;
    if (input == NULL)
        return NULL; //bad input 
    int32_t len = strlen(input);
    if (len == 2 && input[0] == '"' && input[1] == '"') {
		input[0] = '\0';
    } else if (len > 2 && input[0] == '"' && input[len - 1] == '"') {
        input[len - 1] = '\0'; 
		input++;
    }
	//printf ("removeDblQuotes: at end input is: %s\n", input);
	return input;//single character -- can't reduce
}

bool addDblQuotes(char *input) {
    if (input == NULL) {
        return false;
    }
    int32_t len = strlen(input);
    if (len >= 2 && input[0] == '"' && input[len - 1] == '"') {
        return true; 
    }
    memmove(input + 1, input, len + 1);
    input[0] = '"';
    input[len + 1] = '"';
    input[len + 2] = '\0';
	return true;
}

bool variableVetted(char* var) {
	//printf("variableVetted: entered with %s returning %d\n", var, (isalpha(var[0]) || var[0] == '_'));
	if (isalpha(var[0]) || var[0] == '_') return true;
	else return false;
}

char* ltrim(char* str) {
	if (str[0] == '\0') return str;
	return str + strspn(str, " \t\n\r\f\v");
	//while (isspace(*str)) str++;
	//return str;
}

