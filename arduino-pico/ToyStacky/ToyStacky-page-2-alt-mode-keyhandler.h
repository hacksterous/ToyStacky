//Page 2 is the Programming mode
int altPage2ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		case 'm': 
			processImmdOpKeyC("tpose");
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
		default:
			keyTypePressed = altModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
