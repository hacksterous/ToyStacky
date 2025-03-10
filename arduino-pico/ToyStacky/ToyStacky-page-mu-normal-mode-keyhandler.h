//Page 4 is Miscel (mu) mode
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

int normalPage4ModeKeyhandler (char keyc) {
	keyTypePressed = 0;
	switch (keyc) {
		case 'm':
			processImmdOpKeyC("gety");
			keyTypePressed = 1;
			break;
		case 'M':
			processImmdOpKeyC("getm");
			keyTypePressed = 1;
			break;
		case 'q':
			processImmdOpKeyC("day");
			keyTypePressed = 1;
			break;
		case 'p':
			processImmdOpKeyC("getla");
			keyTypePressed = 1;
			break;
		case 's':
			processImmdOpKeyC("getlo");
			keyTypePressed = 1;
			break;
		case 'X':
			processImmdOpKeyC("gettz");
			keyTypePressed = 1;
			break;
		default:
			keyTypePressed = normalModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
