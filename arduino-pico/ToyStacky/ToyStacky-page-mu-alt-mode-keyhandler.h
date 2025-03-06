//Page 4 is Miscel (mu) mode
int altPage4ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		default:
			//pass
			keyTypePressed = altModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
