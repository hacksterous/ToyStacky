void showModes (Machine* vm) {
	lcd.setCursor(0, 0);
	if (vm->cmdState == 0x1) {
		lcd.write(CMDIND); 
	} else if (vm->cmdState == 0x3) {
		lcd.write(CMDLOCKIND);
	} else {
		lcd.print(" ");
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
	lcd.noCursor();
}

void showStackEntries(Machine* vm, int linecount) {
	int8_t meta;
	int maxrowval = linecount - 1;
	for (int i = 0; i < linecount; i++) {
		lcd.setCursor(2, maxrowval - i); //col, row
		lcd.print("                  ");
		memset(vm->userDisplay, 0, STRING_SIZE);
		//lcd.setCursor(DISPLAY_STATUS_WIDTH, maxrowval - i);
		meta = peekn(&vm->userStack, vm->coadiutor, i);
		if (meta != -1) {
			int rightOfPos = -1;
			SerialPrint(3, "showStackHP: calling makeString/Complex/Fit. vm->coadiutor = ", vm->coadiutor, "\r\n");
			rightOfPos = makeComplexStringFit(vm->userDisplay, vm->coadiutor, DISPLAY_LINESIZE - DISPLAY_STATUS_WIDTH);
			//SerialPrint(3, "showStackHP: returned from makeComplexStringFit. vm->display = ", vm->display, "\r\n");
			int userDisplayLen = strlen(vm->userDisplay);
			lcd.setCursor(1, maxrowval - i);
			switch (meta) {
				case METANONE:
				case METAMIDVECTOR: lcd.print(" "); break;
				case METASTARTVECTOR: lcd.print("["); break;
				case METAENDVECTOR: lcd.print("]"); break;
			}
			 
			lcd.setCursor(DISPLAY_LINESIZE - userDisplayLen, maxrowval - i);
			lcd.print(vm->userDisplay);
			if (rightOfPos != -1) {
				//show the right overflow indicator
				lcd.setCursor(DISPLAY_STATUS_WIDTH + rightOfPos, maxrowval - i);
				//lcd.write(RIGHTOFIND);
				lcd.print(RIGHTOFIND);
			}
			//doubleToString(userDisplayLen, debug0);
			SerialPrint(3, "showStackHP: long str: vm->userDisplay = ", vm->userDisplay, "\r\n");
			//SerialPrint(5, "showStackHP: long str: vm->display = ", vm->display, " len display = ", debug0, "\r\n");
			//SerialPrint(3, "showStackHP: len display = ", debug0, "\r\n");
		} else {
			lcd.setCursor(1, maxrowval - i);
			lcd.print(" ");
			lcd.setCursor(DISPLAY_STATUS_WIDTH, maxrowval - i);
			lcd.print("..................");
			//lcd.print("                  ");
		}
	}
}

void showStackHP(Machine* vm, int linecount) {
	//SerialPrint(1, "===================================\r\n");
	char debug0[VSHORT_STRING_SIZE];

	//printStack(&vm->userStack, 3, true); //for debug only
	//SerialPrint(1, "-----------------------------------");
	//printRegisters(vm);
	//SerialPrint(3, "Last Op = ", vm->lastFnOp, "\r\n");
	SerialPrint(3, (strcmp(vm->error, "") == 0)? "": "ERR = ", (strcmp(vm->error, "") == 0)? "Ok": vm->error, "\r\n");
	//char buf[129];
	//sprintf(buf, "execStack: matrix = %d, vector = %d IFstate = %x\r\n", (int) (execData >> 4) & 0x1, (int) (execData >> 3) & 0x1, (int) execData & 0x7);
	//SerialPrint(1, buf);
	//SerialPrint(1, "===================================\r\n");

	if (strcmp(vm->error, "") == 0)
		showStackEntries (vm, linecount);
	else {
		char line[DISPLAY_LINESIZE];
		showStackEntries (vm, linecount - 1);
		eraseUserEntryLine();
		fitstr(line, vm->error, DISPLAY_LINESIZE - DISPLAY_STATUS_WIDTH);
		lcd.setCursor(DISPLAY_STATUS_WIDTH, 3); 
		lcd.print("E:"); 
		lcd.print(line);
		lcd.setCursor(vm->cursorPos, 3); //col, row
	}

	if (linecount == DISPLAY_LINECOUNT) 
		lcd.noCursor();
	else {
		lcd.setCursor(vm->cursorPos, 3); //col, row
		lcd.cursor();
	}
}
