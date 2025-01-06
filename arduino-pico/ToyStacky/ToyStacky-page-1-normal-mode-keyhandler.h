int normalPage1ModeKeyhandler (char keyc) {
	int len;
	int keyTypePressed;
	len = strlen(vm.userInput);
	switch (keyc) {
		case '\n':
			keyTypePressed = 1;
			if (vm.userInput[0] == '\0') {
				if (strcmp(vm.error, "\0") == 0) {
					//no error has happened already
					strcpy(vm.userInput, "dup");
				} else 
					showStackHP(&vm, 0, DISPLAY_LINECOUNT - 1);
			} else {
				vm.userInput[len + 1] = '\0';
				SerialPrint(3, "Got \n : vm.userInput = ", vm.userInput, "\n\r");
			}
			strcpy(vm.userInputInterpret, vm.userInput);
			clearUserInput();
			rp2040.fifo.push(CORE0_TO_CORE1_START);
			break;
		case '\b':
			Serial.print(keyc);
			if (vm.userInputPos > 0) {
				bool lastChar = (vm.userInputPos == 1);
				keyTypePressed = 2;
				//some user entry exists
				if (vm.cursorPos == DISPLAY_LINESIZE - 1) {
					//cursor is at end of line
					vm.userInputPos--;
					vm.userInput[vm.userInputPos] = '\0';
				} else {
					//move the last part of the string 1 char forward
					strcpy(&vm.userInput[vm.userInputPos - 1], &vm.userInput[vm.userInputPos]);
					vm.userInputPos--;
					vm.userInput[len] = '\0';
				}
				if (lastChar) 
					showStackHP(&vm, 0, DISPLAY_LINECOUNT - 1);
				else {
					showStackHP(&vm, 0, DISPLAY_LINECOUNT - 2);
					showUserEntryLine(1);
				}
			} else {
				//backspace on null entry --> drop command
				clearUserInput();
				keyTypePressed = 1;
				strcpy(vm.userInputInterpret, "@");
				rp2040.fifo.push(CORE0_TO_CORE1_START);
			}
			SerialPrint(3, "Got backspace: vm.userInput = ", vm.userInput, "\r\n");
			break;
		//these are operators (in normal mode), result in ' ' + key + \n
		case 'm': 
			processNormalImmdOpKeyC("cot");
			keyTypePressed = 1;
			break;
		case 'M': 
			processNormalImmdOpKeyC("coth");
			keyTypePressed = 1;
			break;
		case 'q': 
			processNormalImmdOpKeyC("sqrt");
			keyTypePressed = 1;
			break;
		case 'p': 
			processNormalImmdOpKeyC("pow");
			keyTypePressed = 1;
			break;
		case 's': 
			processNormalImmdOpKeyC("sin");
			keyTypePressed = 1;
			break;
		case 'X':
			processNormalImmdOpKeyC("cos");
			keyTypePressed = 1;
			break;
		case 't':
			processNormalImmdOpKeyC("tan");
			keyTypePressed = 1;
			break;
		case 'z':
			processNormalImmdOpKeyC("sinh");
			keyTypePressed = 1;
			break;
		case 'n':
			processNormalImmdOpKeyC("cosh");
			keyTypePressed = 1;
			break;
		case 'T':
			processNormalImmdOpKeyC("tanh");
			keyTypePressed = 1;
			break;
		case 'v':
			processNormalImmdOpKeyC("vec");
			keyTypePressed = 1;
			break;
		case '*':
		case '/':
		case '+':
		case ')':
		case ']':
		case '}':
		case '@':
			//these don't call processNormalImmdOpKeyC, but all other actions are same as above
			keyTypePressed = 1;
			if ((vm.userInputPos > 0) && (vm.userInputPos < STRING_SIZE) && (keyc != '@') && (keyc != ')') && (keyc != ']')) {
				vm.userInput[vm.userInputPos++] = ' ';
				SerialPrint(1, "\n\r");
			}
			vm.userInput[vm.userInputPos++] = keyc;
			vm.userInput[vm.userInputPos] = '\0';
			SerialPrint(1, "\n\r");
			strcpy(vm.userInputInterpret, vm.userInput);
			clearUserInput();
			rp2040.fifo.push(CORE0_TO_CORE1_START);
			break;
		case '-': //can result in just a '-' (initial) or '-' + \n (non-initial)
			if ((vm.userInputPos > 0) && (vm.userInputPos < STRING_SIZE) && vm.userInput[vm.userInputPos-1] != 'e') {
				//non spaces followed by a '-'
				//"1e-" will not cause immediate evaluation as this is part of an incomplete number
				keyTypePressed = 1;
				vm.userInput[vm.userInputPos++] = ' ';
				SerialPrint(1, "\n\r");
				vm.userInput[vm.userInputPos++] = '-';
				vm.userInput[vm.userInputPos] = '\0';
				SerialPrint(1, "\n\r");
				strcpy(vm.userInputInterpret, vm.userInput);
				SerialPrint(3, "Got - non-initial: vm.userInput = ", vm.userInput, "\n\r");
				clearUserInput();
				rp2040.fifo.push(CORE0_TO_CORE1_START);
			} else {
				//first char or -number
				keyTypePressed = 0;
				if (vm.userInputPos < STRING_SIZE) {
					vm.userInput[vm.userInputPos++] = '-';
					if (vm.cursorPos < DISPLAY_LINESIZE - 1)
						vm.cursorPos++;
					SerialPrint(3, "Got - initial: vm.userInput = ", vm.userInput, "\n\r");
					//SerialPrint(1, "\n\r");
				}
			}
			break;
		//these below need special handling
		case 'u': //up
			keyTypePressed = 3;
			break;
		case 'D': //down
			keyTypePressed = 4;
			break;
		case '<': //left
			keyTypePressed = 7;
			updatesForLeftMotion();
			break;
		case '>': //right
			keyTypePressed = 8;
			updatesForRightMotion();
			break;
		default:
			keyTypePressed = 0;
			Serial.print(keyc);
			if (((keyc != ' ') || (vm.userInputPos > 0)) && (vm.userInputPos < STRING_SIZE)) {
				//space cannot be entered at start when vm.userInputPos = 0
				vm.userInput[vm.userInputPos++] = keyc;
				if (vm.cursorPos < DISPLAY_LINESIZE - 1)
					vm.cursorPos++;
			}
			break;
	}
	return keyTypePressed;
}
