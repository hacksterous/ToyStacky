//Page 1 is the Advanced Trig mode
//differs from Page 0 only for these keys:
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
int altPage1ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		case 'a': 
			processImmdOpKeyC("bar");
			keyTypePressed = 1;
			break;
		case 'c':
			processImmdOpKeyC("proj");
			keyTypePressed = 1;
			break;
		case 'd':
			processImmdOpKeyC("mod");
			keyTypePressed = 1;
			break;
		case '[':
			processImmdOpKeyC("vec");
			keyTypePressed = 1;
			break;
		case '%':
			processImmdOpKeyC("mont");
			keyTypePressed = 1;
			break;
		case '{':
			processImmdOpKeyC("mat");
			keyTypePressed = 1;
			break;
		case '(':
			processImmdOpKeyC("comp");
			keyTypePressed = 1;
			break;
		case '@':
			processImmdOpKeyC("__f@");
			keyTypePressed = 1;
			break;
		case 'm':
			processImmdOpKeyC("ctran");
			keyTypePressed = 1;
			break;
		case 'M':
			processImmdOpKeyC("rank");
			keyTypePressed = 1;
			break;
		case 'q':
			processImmdOpKeyC("xor");
			keyTypePressed = 1;
			break;
		case 'p':
			processImmdOpKeyC("bin");
			keyTypePressed = 1;
			break;
		case 's':
			processImmdOpKeyC("wid");
			keyTypePressed = 1;
			break;
		case 'X':
			processImmdOpKeyC("set");
			keyTypePressed = 1;
			break;
		case 't':
			processImmdOpKeyC("clr");
			keyTypePressed = 1;
			break;
		case 'x':
			processImmdOpKeyC("oct");
			keyTypePressed = 1;
			break;
		case 'z':
			processImmdOpKeyC("ortho");
			keyTypePressed = 1;
			break;
		case 'n':
			processImmdOpKeyC("trace");
			keyTypePressed = 1;
			break;
		case 'T':
			processImmdOpKeyC("eivec");
			keyTypePressed = 1;
			break;
		case '1':
			processImmdOpKeyC("onesc"); //1's complement
			keyTypePressed = 1;
			break;
		case '2':
			processImmdOpKeyC("twosc"); //2's complement
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
			processImmdOpKeyC("bar");
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
			processImmdOpKeyC("count0");
			keyTypePressed = 1;
			break;
		case '9':
			processImmdOpKeyC("count1");
			keyTypePressed = 1;
			break;
		case '0':
			processImmdOpKeyC("rnd");
			keyTypePressed = 1;
			break;
		case '-':
			processImmdOpKeyC("rsum");
			keyTypePressed = 1;
			break;
		case '.':
			processImmdOpKeyC("seed");
			keyTypePressed = 1;
			break;
		default:
			keyTypePressed = altModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
