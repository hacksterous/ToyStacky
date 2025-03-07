//Page 2 is the alpha mode
/*
	{'u','a','b','c'},
	{'D','d','e','f'},
	{'P','[','%','"'},
	{'A','{','(','l'},

	{'m','M','q','\b'},
	{'p','s','X','t'},
	{'x','z','n','T'},
	{'@',' ','<','>'},

	{'1','2','3','+'},
	{'4','5','6','-'},
	{'7','8','9','*'},
	{'0','.','/','\n'}

*/
char simpleRemap (char keyc) {
	char retKeyC;
	switch (keyc) {
		case 'a': retKeyC = 'x'; break;
		case 'c': retKeyC = 'y'; break;
		case 'e': retKeyC = 'z'; break;
		case 'f': retKeyC = 't'; break;
		case 'C': retKeyC = 'y'; break;
		case '[': retKeyC = 'u'; break;
		case ']': retKeyC = 'u'; break;
		case '(': retKeyC = 'q'; break;
		case ')': retKeyC = 'q'; break;
		case '{': retKeyC = 'r'; break;
		case '}': retKeyC = 'r'; break;
		case '1': retKeyC = 'g'; break;
		case '2': retKeyC = 'i'; break;
		case '3': retKeyC = 'j'; break;
		case '4': retKeyC = 'k'; break;
		case '5': retKeyC = 'm'; break;
		case '7': retKeyC = 'o'; break;
		case '8': retKeyC = 'p'; break;
		case '9': retKeyC = 'n'; break;
		default: retKeyC = keyc; break;
	}
	return retKeyC;
}
int normalPage2ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		default:
			keyTypePressed = normalModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
