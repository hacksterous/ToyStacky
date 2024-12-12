char* tokenize(char* input, char* output) {
	//printf ("tokenize: START input = %p\n", input);
	//skip leading spaces
	input += strspn(input, " \t\r\n");
	int quoteOrParens = 0;
	//check if the current token is a double-quoted string or a complex or a print command
	if (*input == '"' || (*input == '?' && input[1] == '"') || (*input == '@' && input[1] == '"'))
		quoteOrParens = 1;
	else if (*input == '(') 
		//a complex number
		quoteOrParens = 2;
	else if ((*input == '[') && (input[1] == '('))
		//a vector with a complex number
		quoteOrParens = 3;
	else if (*input == ']') 
		//closing vector, could be with trailing commands
		quoteOrParens = -1;

	if (quoteOrParens == -1) {
		*output = ']';
		input++;
	} else if (quoteOrParens > 0) {
		int i = 0;
		if (*input == '?') {
			output[i++] = '?';
			input++;
		} else if (*input == '@') {
			output[i++] = '@';
			input++;
		}
		input++; // skip the opening delim
		if (quoteOrParens == 3) {
			input++; // skip 2 chars if [( is found
			output[i++] = '[';
		}
		output[i++] = (quoteOrParens >= 2)? '(':'"';

		//copy characters until the closing double-quote or closing parenthesis or 
		//the end of the input string is found
		while (*input && *input != ((quoteOrParens >= 2)? ')':'"')) {
			output[i++] = *input++;
		}

		//add null-terminator to the output string
		output[i++] = (quoteOrParens >= 2)? ')':'"';
		output[i] = '\0';

		//if a closing double-quote  or closing parenthesis was found, advance to the next character after it
		if (*input == ((quoteOrParens >= 2)? ')':'"')) {
			input++;
		}

		//skip trailing spaces after the double-quoted string
		input += strspn(input, " \t\r\n");
	}
	else {
		//if the current token is not a double-quoted string, it is a regular word

		//find the length of the word (until the next space or end of the input)
		size_t wordLength = strcspn(input, " \t\r\n");
		//printf ("tokenize: NOT string. wordLength = %lu\n", wordLength);

		//copy the word to the output buffer
		strncpy(output, input, wordLength);
		output[wordLength] = '\0';
		input += wordLength;
		//printf ("tokenize: NOT string. input = %s\n", input);
	}
	//printf ("tokenize: END input = %p\n", input);
	return input;
}
