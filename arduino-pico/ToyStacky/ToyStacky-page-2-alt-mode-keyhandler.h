//Page 2 is the Matrix/Vector mode
int altPage2ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		case 'm': 
			processAltImmdOpKeyC("tpose");
			keyTypePressed = 1;
			break;
		case 'n': 
			processAltImmdOpKeyC("trace");
			keyTypePressed = 1;
			break;
		case 'T':
			processAltImmdOpKeyC("eivec");
			keyTypePressed = 1;
			break;
		default:
			keyTypePressed = altModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
