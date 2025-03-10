int altModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		case '\n':
			keyTypePressed = 1;
			//Serial.print(keyc);
			//SerialPrint(1, "\r");
			processImmdOpKeyC("rem"); //remainder op
			break;
		case '\b':
			//clear whatever was being typed in and clear stack
			clearUserInput();
			keyTypePressed = 7;
			initStacks(&vm);
			//SerialPrint(3, "Got clearStack: vm.userInput = ", vm.userInput, "\r\n");
			showStackHP(&vm, 0, DISPLAY_LINECOUNT - 1);
			break;
		//these are operators (in normal mode), result in ' ' + key + \n
		case 'u':
			if (vm.modeDegrees)
				//to radians
				processImmdOpKeyC("rad");
			else
				//to degrees
				processImmdOpKeyC("deg");
			keyTypePressed = 1;
			break;
		case 'P': //alt + pag
			processImmdOpKeyC("reim");
			keyTypePressed = 1;
			break;
		case 'a':
			processImmdOpKeyC("bar");
			keyTypePressed = 1;
			break;
		case 'b':
			processImmdOpKeyC("bin");
			keyTypePressed = 1;
			break;
		case 'c':
			processImmdOpKeyC("sinh");
			keyTypePressed = 1;
			break;
		case 'D': // D <-> R
			keyTypePressed = 1;
			vm.modeDegrees ^= true;
			writeOneVariableToFile(".degrees", (float*) &vm.modeDegrees);
			break;
		case 'd':
			//to decimal
			processImmdOpKeyC("dec");
			keyTypePressed = 1;
			break;
		case 'e':
			processImmdOpKeyC("cosh");
			keyTypePressed = 1;
			break;
		case 'f':
			processImmdOpKeyC("asinh");
			keyTypePressed = 1;
			break;
		case '[':
			processImmdOpKeyC("vec");
			keyTypePressed = 1;
			break;
		case '%':
			processImmdOpKeyC("acosh");
			keyTypePressed = 1;
			break;
		case '"':
			processImmdOpKeyC("tanh");
			keyTypePressed = 1;
			break;
		case '{':
			processImmdOpKeyC("mat");
			keyTypePressed = 1;
			break;
		case '(':
			processImmdOpKeyC("cmplx");
			keyTypePressed = 1;
			break;
		case '@':
			processImmdOpKeyC("__f@");
			keyTypePressed = 1;
			break;
		case 'm':
			processImmdOpKeyC("sd");
			keyTypePressed = 1;
			break;
		case 'M':
			processImmdOpKeyC("arg");
			keyTypePressed = 1;
			break;
		case 'q':
			processImmdOpKeyC("cbrt");
			keyTypePressed = 1;
			break;
		case 'p': //2 input log
			processImmdOpKeyC("logxy");
			keyTypePressed = 1;
			break;
		case 's':
			processImmdOpKeyC("asin");
			keyTypePressed = 1;
			break;
		case 'X':
			processImmdOpKeyC("acos");
			keyTypePressed = 1;
			break;
		case 't':
			processImmdOpKeyC("atan");
			keyTypePressed = 1;
			break;
		case 'x':
			processImmdOpKeyC("hex");
			keyTypePressed = 1;
			break;
		case 'z': //log10 x
			processImmdOpKeyC("log10");
			keyTypePressed = 1;
			break;
		case 'n': //10^x
			processImmdOpKeyC("pow10");
			keyTypePressed = 1;
			break;
		case 'T': //1/x
			processImmdOpKeyC("recip");
			keyTypePressed = 1;
			break;
		case 'l':
			processImmdOpKeyC("atanh");
			keyTypePressed = 1;
			break;
		case ' ':
			processImmdOpKeyC("swp");
			keyTypePressed = 1;
			break;
		case '<':
			processImmdOpKeyC(__TS_E_STR__);
			keyTypePressed = 1;
			break;
		case '>':
			processImmdOpKeyC(__TS_PI_STR__);
			keyTypePressed = 1;
			break;
		case '1':
			processImmdOpKeyC("conj");
			keyTypePressed = 1;
			break;
		case '2':
			if (vm.modePolar)
				processImmdOpKeyC("rect"); //convert ToS to rect if in Polar mode
			else
				processImmdOpKeyC("polar"); //convert ToS to polar
			keyTypePressed = 1;
			break;
		case '3':
			processImmdOpKeyC("atan2");
			keyTypePressed = 1;
			break;
		case '4':
			processImmdOpKeyC("lastx");
			keyTypePressed = 1;
			break;
		case '5':
			vm.modePolar ^= true;
			keyTypePressed = 1;
			writeOneVariableToFile(".polar", (float*) &vm.modePolar);
			break;
		case '6': //negate
			if ((vm.userInputPos > 0) && (vm.userInputPos < STRING_SIZE - 1)) {
				//user is editing the entry line
				char temp[STRING_SIZE];
				zstrncpy(temp, vm.userInput, vm.userInputPos++);
				strcpy(vm.userInput, "-");
				strcat(vm.userInput, temp);
				keyTypePressed = 0;
			}
			else {
				//user has pressed enter and is not currently editing the
				//entry line
				processImmdOpKeyC("neg");
				keyTypePressed = 1;
			}
			break;
		case '7':
			processImmdOpKeyC("lasty");
			keyTypePressed = 1;
			break;
		case '8':
			processImmdOpKeyC("perm");
			keyTypePressed = 1;
			break;
		case '9':
			processImmdOpKeyC("comb");
			keyTypePressed = 1;
			break;
		case '0':
			processImmdOpKeyC("rnd");
			keyTypePressed = 1;
			break;
		case '+':
			processImmdOpKeyC("sum");
			keyTypePressed = 1;
			break;
		case '-':
			processImmdOpKeyC("rsum");
			keyTypePressed = 1;
			break;
		case '*':
			processImmdOpKeyC("dot");
			keyTypePressed = 1;
			break;
		case '.':
			processImmdOpKeyC("solv");
			keyTypePressed = 1;
			break;
		case '/':
			processImmdOpKeyC("//");
			keyTypePressed = 1;
			break;
		default:
			//pass
			keyTypePressed = normalModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
