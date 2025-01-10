int normalPage1ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		//these are operators (in normal mode), result in ' ' + key + \n
		case 'm': 
			processNormalImmdOpKeyC("cot");
			keyTypePressed = 1;
			break;
		case 'M': 
			processNormalImmdOpKeyC("coth");
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
		default:
			keyTypePressed = normalModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
