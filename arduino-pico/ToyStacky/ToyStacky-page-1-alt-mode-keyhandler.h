//Page 1 is the Advanced Trig mode
//differs from Page 0 only for these keys:
//'m','M',
//'z','n','T', ' '

int altPage1ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		case '\n':
			keyTypePressed = 1;
			Serial.print(keyc);
			SerialPrint(1, "\r");
			processAltImmdOpKeyC("%");
			break;
		case '\b':
			//clear whatever was being typed in and drop last entry from stack
			clearUserInput();
			keyTypePressed = 7;
			initStacks(&vm);
			SerialPrint(3, "Got clearStack: vm.userInput = ", vm.userInput, "\r\n");
			showStackHP(&vm, 0, DISPLAY_LINECOUNT - 1);
			break;
		//these are operators (in normal mode), result in ' ' + key + \n
		case 'u': 
			//to degrees
			processAltImmdOpKeyC("180 * __TS_PI_STR__ /");
			keyTypePressed = 1;
			break;
		case 'b':
			processAltImmdOpKeyC("bin");
			keyTypePressed = 1;
			break;
		case 'D': 
			//to radians
			processAltImmdOpKeyC("__TS_PI_STR__ * 180 /");
			keyTypePressed = 1;
			break;
		case 'd': 
			//to decimal
			processAltImmdOpKeyC("dec");
			keyTypePressed = 1;
			break;
		case '"': //percent
			processAltImmdOpKeyC("100 / *"); //y% of x. ToS = y, ToS-1 = x
			keyTypePressed = 1;
			break;
		case '@':
			processAltImmdOpKeyC("__f@"); //store into internal var
			keyTypePressed = 1;
			break;
		case 'm': 
			processAltImmdOpKeyC("acot");
			keyTypePressed = 1;
			break;
		case 'M': 
			processAltImmdOpKeyC("acoth");
			keyTypePressed = 1;
			break;
		case 'q': 
			processAltImmdOpKeyC("cbrt");
			keyTypePressed = 1;
			break;
		case 'p': //2 input log
			processAltImmdOpKeyC("ln swp ln swp /");
			keyTypePressed = 1;
			break;
		case 's': 
			processAltImmdOpKeyC("asin");
			keyTypePressed = 1;
			break;
		case 'X':
			processAltImmdOpKeyC("acos");
			keyTypePressed = 1;
			break;
		case 't':
			processAltImmdOpKeyC("atan");
			keyTypePressed = 1;
			break;
		case 'h':
			processAltImmdOpKeyC("hex");
			keyTypePressed = 1;
			break;
		case 'z': //log10 x
			processAltImmdOpKeyC("asinh");
			keyTypePressed = 1;
			break;
		case 'n':
			processAltImmdOpKeyC("acosh");
			keyTypePressed = 1;
			break;
		case 'T':
			processAltImmdOpKeyC("atanh");
			keyTypePressed = 1;
			break;
		case ' ':
			processAltImmdOpKeyC("atan2"); //we could have done "/ atan"
			keyTypePressed = 1;
			break;
		case '<':
			processAltImmdOpKeyC(__TS_E_STR__);
			keyTypePressed = 1;
			break;
		case '>': //right key is now enter frequency
			processAltImmdOpKeyC(__TS_PI_STR__);
			keyTypePressed = 1;
			break;
		case '0': // D <-> R
			keyTypePressed = 1;
			vm.modeDegrees ^= true;
			break;
		case '6': //negate
			processAltImmdOpKeyC("0 swp -");
			keyTypePressed = 1;
			break;
		case '+':
			processAltImmdOpKeyC("sum");
			keyTypePressed = 1;
			break;
		case '-':
			processAltImmdOpKeyC("rsum");
			keyTypePressed = 1;
			break;
		case '*':
			processAltImmdOpKeyC("dot");
			keyTypePressed = 1;
			break;
		case '.':
			processAltImmdOpKeyC("swp");
			keyTypePressed = 1;
			break;
		case '/':
			processAltImmdOpKeyC("//");
			keyTypePressed = 1;
			break;
		default:
			//adds to user entry but does not execute immediately
			keyTypePressed = 0;
			Serial.print(keyc);
			if (((keyc != ' ') || (vm.userInputPos > 0)) && (vm.userInputPos < STRING_SIZE)) {
				vm.userInput[vm.userInputPos++] = simpleRemap(keyc);
				if (vm.cursorPos < DISPLAY_LINESIZE - 1)
					vm.cursorPos++;
			}
			break;
	}
	return keyTypePressed;
}
