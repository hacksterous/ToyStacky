int normalModeKeyhandler (char keyc) {
	int len;
	int keyTypePressed = -1;
	len = strlen(vm.userInput);
	switch (keyc) {
		case '\n':
			keyTypePressed = 1;
			if (vm.timerRunning) {
				vm.timerRunning = false;
				rtc_disable_alarm();
				vm.repeatingAlarm = false;
			}
			Serial.print(keyc);
			SerialPrint(1, "\r");
	
			if (vm.userInput[0] == '\0') {
				strcpy(vm.userInput, "dup");
			} else {
				vm.userInput[len + 1] = '\0';
				//SerialPrint(3, "Got \n : vm.userInput = ", vm.userInput, "\n\r");
			}
			strcpy(vm.userInputInterpret, vm.userInput);
			clearUserInput();
			rp2040.fifo.push(CORE0_TO_CORE1_START);
			break;
		case '\b':
			if (vm.userInputPos > 0) {
				bool lastChar = (vm.userInputPos == 1) && (vm.cursorPos == DISPLAY_LINESIZE - 1);
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
				if (lastChar) {
					showStackHP(&vm, 0, DISPLAY_LINECOUNT - 1);
					vm.partialVector = false;
					vm.partialMatrix = false;
					vm.partialComplex = false;
				} else {
					showStackHP(&vm, 0, DISPLAY_LINECOUNT - 2);
					showUserEntryLine(1);
				}
			} else {
				//backspace on null entry --> drop command
				vm.partialVector = false;
				vm.partialMatrix = false;
				vm.partialComplex = false;
				clearUserInput();
				keyTypePressed = 1;
				strcpy(vm.userInputInterpret, "1@@"); //<1> <@@> -- drop one existing entry
				rp2040.fifo.push(CORE0_TO_CORE1_START);
			}
			//SerialPrint(3, "Got backspace: vm.userInput = ", vm.userInput, "\r\n");
			break;
		//these are operators (in normal mode), result in ' ' + key + \n
		case 'm':
			processImmdOpKeyC("mean");
			keyTypePressed = 1;
			break;
		case 'M':
			processImmdOpKeyC("abs");
			keyTypePressed = 1;
			break;
		case 'q':
			processImmdOpKeyC("sqrt");
			keyTypePressed = 1;
			break;
		case 'p':
			processImmdOpKeyC("pow");
			keyTypePressed = 1;
			break;
		case 's':
			processImmdOpKeyC("sin");
			keyTypePressed = 1;
			break;
		case 'X':
			processImmdOpKeyC("cos");
			keyTypePressed = 1;
			break;
		case 't':
			processImmdOpKeyC("tan");
			keyTypePressed = 1;
			break;
		case 'z':
			processImmdOpKeyC("log");
			keyTypePressed = 1;
			break;
		case 'n':
			processImmdOpKeyC("exp");
			keyTypePressed = 1;
			break;
		case 'T':
			processImmdOpKeyC("2 pow");
			keyTypePressed = 1;
			break;
		case '%':
			processImmdOpKeyC("100 / *"); //y% of x. ToS = y, ToS-1 = x
			keyTypePressed = 1;
			break;
		case '*':
		case '/':
		case '+':
		case ')':
		case ']':
		case '}':
		case '@':
			//these don't call processImmdOpKeyC, but all other actions are same as above
			keyTypePressed = 1;
			if ((vm.userInputPos > 0) && (vm.userInputPos < STRING_SIZE - 1)) {
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
			if ((vm.userInputPos > 0) && (vm.userInputPos < STRING_SIZE - 1) && vm.userInput[vm.userInputPos-1] != 'e') {
				//non spaces followed by a '-'
				//"1e-" will not cause immediate evaluation as this is part of an incomplete number
				keyTypePressed = 1;
				vm.userInput[vm.userInputPos++] = ' ';
				//SerialPrint(1, "\n\r");
				vm.userInput[vm.userInputPos++] = '-';
				vm.userInput[vm.userInputPos] = '\0';
				//SerialPrint(1, "\n\r");
				strcpy(vm.userInputInterpret, vm.userInput);
				//SerialPrint(3, "Got - non-initial: vm.userInput = ", vm.userInput, "\n\r");
				clearUserInput();
				rp2040.fifo.push(CORE0_TO_CORE1_START);
			} else {
				//first char or -number
				keyTypePressed = 0;
				if (vm.userInputPos < STRING_SIZE - 1) {
					vm.userInput[vm.userInputPos++] = '-';
					if (vm.cursorPos < DISPLAY_LINESIZE - 1)
						vm.cursorPos++;
					//SerialPrint(3, "Got - initial: vm.userInput = ", vm.userInput, "\n\r");
					//SerialPrint(1, "\n\r");
				}
			}
			break;
		//these below need special handling
		case 'u': //up
			keyTypePressed = 3;
			lcd.noCursor();
			updatesForUpMotion();
			showViewPage();
			break;
		case 'D': //down
			//keyTypePressed = 3;
			//lcd.noCursor();
			//updatesForDownMotion();
			//showViewPage();
			processImmdOpKeyC("gcd");
			keyTypePressed = 1;
			break;
		case '<': //left
			//if ((vm.userInputPos > 0) && (vm.userInputPos < STRING_SIZE - 1)) {
			if (vm.userInput[0] != '\0') {
				//user is editing the entry line
				keyTypePressed = 7;
				updatesForLeftMotion();
			}
			else {
				//user is not editing the entry line
				//reuse for a function
				processImmdOpKeyC("lcm");
				keyTypePressed = 1;
			}
			break;
		case '>': //right
			keyTypePressed = 7;
			updatesForRightMotion();
			break;
		default:
			//these need no translation and no immediate operation
			keyTypePressed = 0;
			//Serial.print(keyc);
			if (((keyc != ' ') || (vm.userInputPos > 0)) && (vm.userInputPos < STRING_SIZE - 1)) {
				//space cannot be entered at start when vm.userInputPos = 0
				vm.userInput[vm.userInputPos++] = keyc;
				if (vm.cursorPos < DISPLAY_LINESIZE - 1)
					vm.cursorPos++;
			}
			break;
	}
	return keyTypePressed;
}
