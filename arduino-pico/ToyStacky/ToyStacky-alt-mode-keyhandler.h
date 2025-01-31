int altModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		case '\n':
			keyTypePressed = 1;
			Serial.print(keyc);
			SerialPrint(1, "\r");
			processImmdOpKeyC("%"); //FIXME remainder op
			break;
		case '\b':
			//clear whatever was being typed in and clear stack
			clearUserInput();
			keyTypePressed = 7;
			initStacks(&vm);
			SerialPrint(3, "Got clearStack: vm.userInput = ", vm.userInput, "\r\n");
			showStackHP(&vm, 0, DISPLAY_LINECOUNT - 1);
			break;
		//these are operators (in normal mode), result in ' ' + key + \n
		case 'u':
			//to degrees
			processImmdOpKeyC("180 * 3.1415926535897932 /");
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
		case 'D':
			//to radians
			processImmdOpKeyC("3.1415926535897932 * 180 /");
			keyTypePressed = 1;
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
		case 'P': // D <-> R
			keyTypePressed = 1;
			vm.modeDegrees ^= true;
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
			processImmdOpKeyC("log swp log swp /");
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
			processImmdOpKeyC("10 log swp log swp /");
			keyTypePressed = 1;
			break;
		case 'n': //10^x
			processImmdOpKeyC("10 swp pow");
			keyTypePressed = 1;
			break;
		case 'T': //1/x
			processImmdOpKeyC("1 swp /");
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
			processImmdOpKeyC("coth");
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
			processImmdOpKeyC("polar"); //switch to polar entry
			keyTypePressed = 1;
			break;
		case '6': //negate
			processImmdOpKeyC("0 swp -");
			keyTypePressed = 1;
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
			processImmdOpKeyC("fac");
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
