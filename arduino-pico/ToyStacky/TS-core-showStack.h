void showStackEntries(Machine* vm, int linecount) {
	int8_t meta;
	int maxrowval = linecount - 1;
	for (int i = 0; i < linecount; i++) {
		//print userline status here
		lcd.setCursor(0, maxrowval - i);
		if ((maxrowval - i) == 0) //row 0
			lcd.print((vm->cmdState)? "S ": "  "); 
		else if ((maxrowval - i) == 1)
			lcd.print((vm->altState)? "A ": "  ");
		else
			lcd.print((vm->modeDegrees)? "D ": "  ");

		lcd.print("                  ");
		memset(vm->userDisplay, 0, STRING_SIZE);
		lcd.setCursor(DISPLAY_STATUS_WIDTH, maxrowval - i);
		meta = peekn(&vm->userStack, vm->coadiutor, i);
		if (meta != -1) {
			int rightOfPos = -1;
			SerialPrint(3, "showStackHP: calling makeString/Complex/Fit. vm->coadiutor = ", vm->coadiutor, "\r\n");
			rightOfPos = makeComplexStringFit(vm->userDisplay, vm->coadiutor, DISPLAY_LINESIZE - DISPLAY_STATUS_WIDTH);
			SerialPrint(3, "showStackHP: returned from makeComplexStringFit. vm->display = ", vm->display, "\r\n");
			int userDisplayLen = strlen(vm->userDisplay);
			lcd.setCursor(DISPLAY_LINESIZE - userDisplayLen, maxrowval - i);
			lcd.print(vm->userDisplay);
			if (rightOfPos != -1) {
				//show the right overflow indicator
				lcd.setCursor(DISPLAY_STATUS_WIDTH + rightOfPos, maxrowval - i);
				lcd.write(RIGHTOFIND);
			}
			//doubleToString(userDisplayLen, debug0);
			SerialPrint(3, "showStackHP: long str: vm->display = ", vm->userDisplay, "\r\n");
			//SerialPrint(5, "showStackHP: long str: vm->display = ", vm->display, " len display = ", debug0, "\r\n");
			//SerialPrint(3, "showStackHP: len display = ", debug0, "\r\n");
		} else {
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
		strcpy(vm->error, "");
	}

	if (linecount == DISPLAY_LINECOUNT) 
		lcd.noCursor();
	else {
		lcd.setCursor(vm->cursorPos, 3); //col, row
		lcd.cursor();
	}
}

void showStack(Machine* vm) {
	//SerialPrint(1, "===================================\r\n");
	int8_t meta;
	char debug0[VSHORT_STRING_SIZE];
	for (int i = 0; i < 3; i++) {
		//print userline status here
		lcd.setCursor(0, 2 - i);
		if ((2 - i) == 0) //row 0
			lcd.print((vm->cmdState)? "S ": "  "); 
		else if ((2 - i) == 1)
			lcd.print((vm->altState)? "A ": "  ");
		else
			lcd.print((vm->modeDegrees)? "D ": "  ");

		lcd.print("                  ");
		memset(vm->userDisplay, 0, STRING_SIZE);
		lcd.setCursor(DISPLAY_STATUS_WIDTH, 2 - i);
		meta = peekn(&vm->userStack, vm->coadiutor, i);
		if (meta != -1) {
			int rightOfPos = -1;
			SerialPrint(3, "showStack: calling makeString/Complex/Fit. vm->coadiutor = ", vm->coadiutor, "\r\n");
			rightOfPos = makeComplexStringFit(vm->userDisplay, vm->coadiutor, DISPLAY_LINESIZE - DISPLAY_STATUS_WIDTH);
			SerialPrint(3, "showStack: returned from makeComplexStringFit. vm->display = ", vm->display, "\r\n");
			int userDisplayLen = strlen(vm->userDisplay);
			lcd.setCursor(DISPLAY_LINESIZE - userDisplayLen, 2 - i);
			lcd.print(vm->userDisplay);
			if (rightOfPos != -1) {
				//show the right overflow indicator
				lcd.setCursor(DISPLAY_STATUS_WIDTH + rightOfPos, 2 - i);
				lcd.write(RIGHTOFIND);
			}
			//doubleToString(userDisplayLen, debug0);
			SerialPrint(3, "showStack: long str: vm->display = ", vm->userDisplay, "\r\n");
			//SerialPrint(5, "showStack: long str: vm->display = ", vm->display, " len display = ", debug0, "\r\n");
			//SerialPrint(3, "showStack: len display = ", debug0, "\r\n");
		} else {
			lcd.print("..................");
			//lcd.print("                  ");
		}
	}
	//printStack(&vm->userStack, 3, true); //for debug only
	//SerialPrint(1, "-----------------------------------");
	//printRegisters(vm);
	//SerialPrint(3, "Last Op = ", vm->lastFnOp, "\r\n");
	SerialPrint(3, (strcmp(vm->error, "") == 0)? "": "ERR = ", (strcmp(vm->error, "") == 0)? "Ok": vm->error, "\r\n");
	//char buf[129];
	//sprintf(buf, "execStack: matrix = %d, vector = %d IFstate = %x\r\n", (int) (execData >> 4) & 0x1, (int) (execData >> 3) & 0x1, (int) execData & 0x7);
	//SerialPrint(1, buf);
	//SerialPrint(1, "===================================\r\n");

	if (strcmp(vm->error, "") != 0) {
		char line[DISPLAY_LINESIZE];
		eraseUserEntryLine();
		fitstr(line, vm->error, DISPLAY_LINESIZE - DISPLAY_STATUS_WIDTH);
		lcd.setCursor(DISPLAY_STATUS_WIDTH, 3); 
		lcd.print("E:"); 
		lcd.print(line);
		lcd.setCursor(vm->cursorPos, 3); //col, row
	} else if (strcmp(vm->lastFnOp, "") != 0) {
		char line[DISPLAY_LINESIZE];
		SerialPrint(3, "Doing Last Op = ", vm->lastFnOp, "\r\n");
		eraseUserEntryLine();
		//no error, show previous operator or function on user entry line
		fitstr(line, vm->lastFnOp, DISPLAY_LINESIZE - DISPLAY_STATUS_WIDTH - 1);
		lcd.setCursor(DISPLAY_STATUS_WIDTH, 3); lcd.print(vm->lastFnOp);
		lcd.setCursor(vm->cursorPos, 3); //col, row
	} else {
		eraseUserEntryLine();
	}
}
