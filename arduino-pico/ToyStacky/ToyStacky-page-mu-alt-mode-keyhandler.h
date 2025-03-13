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
int altPage4ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		case 'm':
			processImmdOpKeyC("yyyy");
			keyTypePressed = 1;
			break;
		case 'M':
			processImmdOpKeyC("mm");
			keyTypePressed = 1;
			break;
		case 'q':
			processImmdOpKeyC("daymore");
			keyTypePressed = 1;
			break;
		case 'p':
			processImmdOpKeyC("latt");
			keyTypePressed = 1;
			break;
		case 's':
			processImmdOpKeyC("longt");
			keyTypePressed = 1;
			break;
		case 'X':
			processImmdOpKeyC("timez");
			keyTypePressed = 1;
			break;
		case 'x':
			processImmdOpKeyC("mode");
			keyTypePressed = 1;
			break;
		case 'z':
			processImmdOpKeyC("prec");
			keyTypePressed = 1;
			break;
		case 'n':
			processImmdOpKeyC("savmod");
			keyTypePressed = 1;
			break;
		default:
			//pass
			keyTypePressed = altModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
