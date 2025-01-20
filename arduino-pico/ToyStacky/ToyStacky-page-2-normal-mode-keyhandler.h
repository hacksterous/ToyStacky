/*
	{'u','a','b','c'},
	{'D','d','e','f'},
	{'C','[','v','"'},
	{'A','(','{','@'},

	{'m','M','q','\b'},
	{'p','s','X','t'},
	{'h','z','n','T'},
	{'l',' ','<','>'},

	{'1','2','3','+'},
	{'4','5','6','-'},
	{'7','8','9','*'},
	{'0','.','/','\n'}

*/
int normalPage2ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		//these are operators (in normal mode), result in ' ' + key + \n
		case 'm': 
			processNormalImmdOpKeyC("mat");
			keyTypePressed = 1;
			break;
		case 'M': 
			processNormalImmdOpKeyC("det");
			keyTypePressed = 1;
			break;
		case 'z':
			processNormalImmdOpKeyC("unmat");
			keyTypePressed = 1;
			break;
		case 'n':
			processNormalImmdOpKeyC("inv");
			keyTypePressed = 1;
			break;
		case 'T':
			processNormalImmdOpKeyC("eival");
			keyTypePressed = 1;
			break;
		case ' ':
			processNormalImmdOpKeyC("unvec");
			keyTypePressed = 1;
			break;
		default:
			keyTypePressed = normalModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
