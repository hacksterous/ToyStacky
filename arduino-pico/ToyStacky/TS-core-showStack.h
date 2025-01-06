void showModes (Machine* vm) {
	lcd.setCursor(0, 0);
	switch(vm->cmdState) {
		case 0:
			lcd.print(char(0xdb)); break;
		case 1:
			lcd.print(char(0xd5)); break;
		case 2:
			lcd.write(TWOIND); break;
		case 3:
			lcd.print(char(0xd6)); break;
		default:
			lcd.print(char(0xdb)); break;
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

void showStackEntries(Machine* vm, int linestart, int linecount) {
	int8_t meta;
	int maxrowval = linecount - 1;
	for (int i = linestart; i < linecount; i++) {
		lcd.setCursor(2, maxrowval - i); //col, row
		lcd.print("                  ");
		memset(vm->userDisplay, 0, STRING_SIZE);
		meta = peekn(&vm->userStack, vm->coadiutor, i);
		if (meta != -1) {
			makeComplexStringFit(vm->userDisplay, vm->coadiutor, NUMBER_LINESIZE);
			lcd.setCursor(1, maxrowval - i);
			lcd.setCursor(DISPLAY_LINESIZE - strlen(vm->userDisplay), maxrowval - i);
			lcd.print(vm->userDisplay);
		} else {
			lcd.setCursor(1, maxrowval - i);
			lcd.print(" ");
			lcd.setCursor(DISPLAY_STATUS_WIDTH, maxrowval - i);
			lcd.print("..................");
		}
	}
}

void showStackHP(Machine* vm, int linestart, int linecount) {
	//SerialPrint(1, "===================================\r\n");
	char debug0[VSHORT_STRING_SIZE];

	//printStack(&vm->userStack, 3, true); //for debug only
	//SerialPrint(1, "-----------------------------------");
	//printRegisters(vm);
	//SerialPrint(3, "Last Op = ", vm->lastFnOp, "\r\n");
	//SerialPrint(3, (strcmp(vm->error, "") == 0)? "": "ERR = ", (strcmp(vm->error, "") == 0)? "Ok": vm->error, "\r\n");
	//char buf[129];
	//sprintf(buf, "execStack: matrix = %d, vector = %d IFstate = %x\r\n", (int) (execData >> 4) & 0x1, (int) (execData >> 3) & 0x1, (int) execData & 0x7);
	//SerialPrint(1, buf);
	//SerialPrint(1, "===================================\r\n");

	if (strcmp(vm->error, "") == 0)
		showStackEntries (vm, linestart, linecount);
	else {
		char line[DISPLAY_LINESIZE];
		showStackEntries (vm, linestart, linecount - 1);
		eraseUserEntryLine();
		fitstr(line, vm->error, DISPLAY_LINESIZE - DISPLAY_STATUS_WIDTH);
		lcd.setCursor(DISPLAY_STATUS_WIDTH, DISPLAY_LINECOUNT - 1); 
		lcd.print("E:"); 
		lcd.print(line);
		lcd.setCursor(vm->cursorPos, DISPLAY_LINECOUNT - 1); //col, row
	}

	if (linecount == DISPLAY_LINECOUNT) 
		lcd.noCursor();
	else {
		lcd.setCursor(vm->cursorPos, 3); //col, row
		lcd.cursor();
	}
}
