//Page 2 is the alpha mode
int altPage2ModeKeyhandler (char keyc) {
	int keyTypePressed;
	switch (keyc) {
		default:
			keyTypePressed = altModeKeyhandler(keyc);
			break;
	}
	return keyTypePressed;
}
