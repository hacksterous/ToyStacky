char simpleRemap (char keyc) {
	char retKeyC;
	switch (keyc) {
		case 'a': retKeyC = 'b'; break;
		case 'c': retKeyC = 'd'; break;
		case 'e': retKeyC = 'w'; break;
		case 'x': retKeyC = 'y'; break; 
		case 'f': retKeyC = 'z'; break;
		case 'l': retKeyC = 't'; break;
		case '[': retKeyC = 'u'; break;
		case '(': retKeyC = 'v'; break;
		case ']': retKeyC = 'r'; break;
		case ')': retKeyC = 's'; break;
		case '1': retKeyC = 'g'; break;
		case '2': retKeyC = 'i'; break;
		case '3': retKeyC = 'j'; break;
		case '4': retKeyC = 'k'; break;
		case '5': retKeyC = 'm'; break;
		case '6': retKeyC = 'n'; break;
		case '7': retKeyC = 'o'; break;
		case '8': retKeyC = 'p'; break;
		case '9': retKeyC = 'q'; break;
		default: retKeyC = keyc; break;
	}
	return retKeyC;
}

void processAltImmdOpKeyC (const char* str) {
	//results in key + \n -- modify to ' ' + key + \n
	if (vm.userInputPos >= STRING_SIZE - 18) return;

	processImmdOpKeyC(str);

	//if ((vm.userInputPos > 0) && (vm.userInputPos < STRING_SIZE)) {
	//	vm.userInput[vm.userInputPos++] = ' ';
	//	SerialPrint(1, "\n\r");
	//}
	//strcpy(&vm.userInput[vm.userInputPos], str); 
	//SerialPrint(1, "\n\r");
	//strcpy(vm.userInputInterpret, vm.userInput);
	//clearUserInput();
	//rp2040.fifo.push(CORE0_TO_CORE1_START);
}

int altModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		case '0': // D <-> R
			keyTypePressed = 1;
			vm.modeDegrees ^= true;
			if (vm.altState == 1) vm.altState = 0;
			break;
		case '\n':
			keyTypePressed = 1;
			if (vm.timerRunning) {
				vm.timerRunning = false;
				rtc_disable_alarm();
				vm.repeatingAlarm = false;
			}
			Serial.print(keyc);
			SerialPrint(1, "\r");
	
			strcpy(vm.userInput, "today");
			strcpy(vm.userInputInterpret, vm.userInput);
			clearUserInput();
			rp2040.fifo.push(CORE0_TO_CORE1_START);
			break;
		case '\b':
			//clear whatever was being typed in and drop last entry from stack
			clearUserInput();
			keyTypePressed = 1;
			strcpy(vm.userInputInterpret, "@");
			rp2040.fifo.push(CORE0_TO_CORE1_START);
			SerialPrint(3, "Got backspace: vm.userInput = ", vm.userInput, "\r\n");
			break;
		//these are operators (in normal mode), result in ' ' + key + \n
		case 'm': 
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("sd");
			keyTypePressed = 1;
			break;
		case 'M': 
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("arg");
			keyTypePressed = 1;
			break;
		case 'q': 
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("cbrt");
			keyTypePressed = 1;
			break;
		case 'p': 
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("ln swp ln swp /");
			keyTypePressed = 1;
			break;
		case 's': 
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("asin");
			keyTypePressed = 1;
			break;
		case 'X':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("acos");
			keyTypePressed = 1;
			break;
		case 't':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("atan");
			keyTypePressed = 1;
			break;
		case 'h':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("hex");
			keyTypePressed = 1;
			break;
		case 'b':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("bin");
			keyTypePressed = 1;
			break;
		case 'z':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("10 ln swp ln swp /");
			keyTypePressed = 1;
			break;
		case 'n':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("10 swp pow");
			keyTypePressed = 1;
			break;
		case 'T':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("1 swp /");
			keyTypePressed = 1;
			break;
		case '+':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("sum");
			keyTypePressed = 1;
			break;
		case '-':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("rsum");
			keyTypePressed = 1;
			break;
		case '*':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("dot");
			keyTypePressed = 1;
			break;
		case '/':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("//");
			keyTypePressed = 1;
			break;
		case '.':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("swp");
			keyTypePressed = 1;
			break;
		case '"':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("dec");
			keyTypePressed = 1;
			break;
		case ' ':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("ang");
			keyTypePressed = 1;
			break;
		case '<':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("shl");
			keyTypePressed = 1;
			break;
		case '>':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("shr");
			keyTypePressed = 1;
			break;
		case 'u':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("deg");
			keyTypePressed = 1;
			break;
		case 'd':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC("rad");
			keyTypePressed = 1;
			break;
		case '@':
			//results in key + \n -- simplify to ' ' + key + \n
			processAltImmdOpKeyC(__TS_PI_STR__);
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
