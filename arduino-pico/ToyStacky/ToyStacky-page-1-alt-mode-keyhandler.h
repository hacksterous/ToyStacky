//Page 1 is the Advanced Trig mode
//differs from Page 0 only for these keys:
//'m','M',
//'z','n','T', ' '

int altPage1ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		//these are operators (in normal mode), result in ' ' + key + \n
		case 'm': 
			processAltImmdOpKeyC("acot");
			keyTypePressed = 1;
			break;
		case 'M': 
			processAltImmdOpKeyC("acoth");
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
		default:
			//adds to user entry but does not execute immediately
			keyTypePressed = altModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
