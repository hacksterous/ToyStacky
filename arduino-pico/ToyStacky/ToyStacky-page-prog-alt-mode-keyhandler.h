//Page 3 is the Programming mode
int altPage3ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		default:
			keyTypePressed = altModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
