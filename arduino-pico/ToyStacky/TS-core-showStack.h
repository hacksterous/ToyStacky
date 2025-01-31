void cleanUpModes (Machine* vm) {
	lcd.noCursor();
	for (int i = 0; i < DISPLAY_LINECOUNT; i += 2){
		//clean up cmd page and degree mode indicators
		//don't need to clean up alt indicator
		lcd.setCursor(0, i); //col, row
		lcd.print(' ');
	}
}

void showModes (Machine* vm) {
	if (vm->viewPage != NORMAL_VIEW) return;
	lcd.setCursor(0, 0);
	switch(vm->cmdPage) {
		case 0:
			lcd.print(CMDPG_0); break;
		case 1:
			lcd.print(CMDPG_1); break;
		case 2:
			lcd.write(TWOIND); break;
		case 3:
			lcd.print(CMDPG_3); break;
		case 4:
			lcd.print(CMDPG_4); break;
		default:
			lcd.print(CMDPG_0); break;
	}

	lcd.setCursor(0, 1);

	if (vm->altState == 0x1) {
		lcd.write(ALTIND); 
	} else if (vm->altState == 0x3) {
		lcd.write(ALTLOCKIND);
	} else {
		lcd.print(" ");
	}
	lcd.setCursor(0, 2);
	(vm->modeDegrees)? lcd.print(DEGREEIND): lcd.print(" ");
	lcd.setCursor(0, 3);
	lcd.print("  ");
	lcd.setCursor(vm->cursorPos, 3);
}

int32_t splitStringOnSpaces(char* str, int width, bool* overlongp){
	int i, j;
	bool complexFound = false;
	for (i = 0; i < width; i++) {
		if (str[i] == ' ' || str[i] == '\0') {
			break;
		}
	}
	//handle when word is longer than width
	if (i == width) {
		if (str[width] != '\0' && str[width] != ' ') {
			*overlongp = true;
		}
		return width;
	}
	return i + 1;
}

void formatVariable(Machine* vm, char* str, int width){
	int i;
	int32_t next = 0;
	bool overlong;
	vm->varLength = strlen(str);
	for (i = 0;; i++) {
		overlong = false;
		if (next >= vm->varLength) break;
		int32_t incr = splitStringOnSpaces(str + next, width - 1, &overlong);
		if (incr == 1) {
			next += 1;
			i--; //skip empty string
			continue;
		} else {
			vm->varFragsArray[i] = next & 0x0ffff; //lower 16 bits 15:0
			next += incr;
		}
		vm->varFragsArray[i] |= ((next & 0x0ffff) << 16); //bits 30:16
		if (overlong) vm->varFragsArray[i] |= 0x80000000;
	}
	vm->varFragCount = i;
}

void showVariable(Machine* vm) {
	//formatVariable has already been called before
	int32_t next;
	int32_t index;
	bool overlong;
	lcd.clear();
	for (int i = vm->topVarFragNum; i < vm->topVarFragNum + DISPLAY_LINECOUNT; i++){
		index = vm->varFragsArray[i] & 0x0ffff;
		next = (vm->varFragsArray[i] & 0x7fff0000) >> 16;
		overlong = (vm->varFragsArray[i] & 0x80000000) >> 31;
		//original variable string is in matvecStrC
		zstrncpy(vm->coadiutor, &vm->matvecStrC[index], next - index);
		lcd.setCursor(1, i - vm->topVarFragNum);
		lcd.print(vm->coadiutor);
		if (overlong) lcd.print(OFLOWIND);
		if (next >= vm->varLength) break;
	}
	lcd.setCursor(0, 0);
	if (vm->topVarFragNum > 0)lcd.write(UPIND);
	else lcd.print(' ');
	lcd.setCursor(0, DISPLAY_LINECOUNT - 1);
	if ((vm->topVarFragNum + DISPLAY_LINECOUNT) < vm->varFragCount) lcd.write(DOWNIND);
	else lcd.print(' ');
}

void showStackEntries(Machine* vm, int linestart, int lineend) {
	int8_t meta;
	int8_t barrier;
	int maxrowval = lineend;
	for (int i = linestart; i <= lineend; i++) {
		lcd.setCursor(2, lineend - i); //col, row
		lcd.print("                  ");
		memset(vm->userDisplay, 0, SHORT_STRING_SIZE);
		meta = peeknbarrier(&vm->userStack, vm->coadiutor, i);
		barrier = (meta & 0x8) >> 3;
		if (meta != -1) {
			makeComplexMatVecStringFit(vm->userDisplay, vm->coadiutor, NUMBER_LINESIZE, barrier);
			lcd.setCursor(1, lineend - i);
			lcd.print(" ");
			lcd.setCursor(DISPLAY_LINESIZE - strlen(vm->userDisplay), lineend - i);
			lcd.print(vm->userDisplay);
		} else {
			lcd.setCursor(1, lineend - i);
			lcd.print(" ");
			lcd.setCursor(DISPLAY_STATUS_WIDTH, lineend - i);
			lcd.print("..................");
		}
	}
	//in normal mode, if stack count exceeds display line count, show up arrow
	if (vm->viewPage == NORMAL_VIEW) {
		lcd.setCursor(1, 0);
		if (vm->userStack.itemCount > DISPLAY_LINECOUNT) 
			lcd.write(UPIND);
		else
			lcd.print(' ');
	}
}

void showStackHP(Machine* vm, int linestart, int lineend) {
	if (vm->error[0]== '\0' && errno == 0) //no errors
		showStackEntries (vm, linestart, lineend);
	else {
		char line[DISPLAY_LINESIZE];
		int err = errno - 10000;
		if (err >= 0) strcpy(vm->error, __TS_GLOBAL_ERRORCODE[err]);
		showStackEntries (vm, linestart, lineend - 1);
		eraseUserEntryLine();
		lcd.setCursor(DISPLAY_STATUS_WIDTH, DISPLAY_LINECOUNT - 1); 
		lcd.print("E:"); 
		if (errno == 0) {
			fitstr(line, vm->error, DISPLAY_LINESIZE - DISPLAY_STATUS_WIDTH);
			lcd.print(line);
		}
		else lcd.print(errno);
		lcd.setCursor(vm->cursorPos, DISPLAY_LINECOUNT - 1); //col, row
	}

	if (vm->viewPage != NORMAL_VIEW) return; //for stack mode, don't reenable cursor
	if (lineend == DISPLAY_LINECOUNT - 1) 
		lcd.noCursor();
	else {
		lcd.setCursor(vm->cursorPos, DISPLAY_LINECOUNT - 1); //col, row
		lcd.cursor();
	}
}
