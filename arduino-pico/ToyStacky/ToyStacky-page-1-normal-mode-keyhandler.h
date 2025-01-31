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

int normalPage1ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		case 'a': 
			processImmdOpKeyC("isp");
			keyTypePressed = 1;
			break;
		case 'b': 
			processImmdOpKeyC("nxp");
			keyTypePressed = 1;
			break;
		case 'c':
			processImmdOpKeyC("join");
			keyTypePressed = 1;
			break;
		case 'd':
			processImmdOpKeyC("mod");
			keyTypePressed = 1;
			break;
		case 'e':
			processImmdOpKeyC("ror");
			keyTypePressed = 1;
			break;
		case 'f':
			processImmdOpKeyC("shr");
			keyTypePressed = 1;
			break;
		case '%':
			processImmdOpKeyC("rol");
			keyTypePressed = 1;
			break;
		case '"':
			processImmdOpKeyC("shl");
			keyTypePressed = 1;
			break;
		case 'm':
			processImmdOpKeyC("tran");
			keyTypePressed = 1;
			break;
		case 'M':
			processImmdOpKeyC("det");
			keyTypePressed = 1;
			break;
		case 'q':
			processImmdOpKeyC("or");
			keyTypePressed = 1;
			break;
		case 'p':
			processImmdOpKeyC("dec");
			keyTypePressed = 1;
			break;
		case 's':
			processImmdOpKeyC("and");
			keyTypePressed = 1;
			break;
		case 'X':
			processImmdOpKeyC("exp");
			keyTypePressed = 1;
			break;
		case 't':
			processImmdOpKeyC("binv");
			keyTypePressed = 1;
			break;
		case 'x':
			processImmdOpKeyC("hex");
			keyTypePressed = 1;
			break;
		case 'z':
			processImmdOpKeyC("proj");
			keyTypePressed = 1;
			break;
		case 'n':
			processImmdOpKeyC("inv");
			keyTypePressed = 1;
			break;
		case 'T':
			processImmdOpKeyC("eival");
			keyTypePressed = 1;
			break;
		case 'l':
			processImmdOpKeyC("bit");
			keyTypePressed = 1;
			break;
		default:
			keyTypePressed = normalModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
