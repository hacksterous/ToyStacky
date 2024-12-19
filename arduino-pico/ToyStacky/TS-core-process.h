bool process(Machine* vm, char* token, int branchIndex) {
	bool success = true;
	bool ifCondition, doingIf, conditional;
	ifCondition = doingIf = conditional = false;
	size_t execData = UintStackPeek(&vm->execStack);

	GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)
	//SerialPrint(1, "process:------------------- token = %s", token);
	//SerialPrint(1, "process:------------------- UintStackPeek = %lu", execData);
	int is1pfn = is1ParamFunction(token);
	int is2pfn = is2ParamFunction(token);
	int isvecfn = isVec1ParamFunction(token);
	int isTrig = is1ParamTrigFunction(token);
	if (is1pfn != -1) {
		//SerialPrint(1, "process:------------------- is1pfn = %d", is1pfn);
		//lcase(token);
		success = fn1Param(vm, token, is1pfn, isTrig);
	} else if (is2pfn != -1) {
		//SerialPrint(1, "process:------------------- is2pfn|op = %d", is2pfn);
		//lcase(token);
		success = fnOrOp2Param(vm, token, is2pfn); //don't pass is2pop
	} else if (isvecfn != -1) {
		//lcase(token);
		success = fnOrOpVec1Param(vm, token, isvecfn, false);
	} else if (strcmp(token, "dup") == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "No vecmat here.C", NULLFN)
		int8_t meta = peek(&vm->userStack, vm->coadiutor);
		FAILANDRETURN((meta == -1), vm->error, "stack empty", NULLFN)
		FAILANDRETURN((meta != 0), vm->error, "no DUP here", NULLFN)
		if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
			push(&vm->userStack, vm->coadiutor, METANONE);
			return true;
		}
	} else if (strcmp(token, "tim") == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "No vecmat here.F", NULLFN)
		//conditional loop, check for previous if/else
		int8_t meta = peek(&vm->userStack, vm->coadiutor);
		FAILANDRETURN((meta == -1), vm->error, "stack empty", NULLFN)
		FAILANDRETURN((meta != 0), vm->error, "no TIM here", NULLFN)
		if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
			ComplexDouble c;
			rtc_disable_alarm();
			c.imag = 0.0;
			pop(&vm->userStack, vm->coadiutor); //c
			if (isRealNumber(vm->coadiutor)) { //real number
				success = stringToDouble(vm->coadiutor, &c.real);
				FAILANDRETURN(!success, vm->error, "Bad timer.A", NULLFN)
				if (c.real == 0.0) return true;
			} else if (isComplexNumber(vm->coadiutor)) { //complex
				success = stringToComplex(vm->coadiutor, &c);
				FAILANDRETURN(!success, vm->error, "Bad timer.B", NULLFN)
				if (c.real == 0.0 && c.imag == 0.0) return true;
			} else
				FAILANDRETURN(true, vm->error, "Bad timer.C", NULLFN)

			//we don't set the alarm struct values
			//to -1 for repeating alarm
			vm->repeatingAlarm = false;
			if (c.real < 0.0 || c.imag < 0.0) {
				vm->repeatingAlarm = true;
				if (c.real < 0)
					c.real = -c.real;
				if (c.imag < 0)
					c.imag = -c.imag;
			}

			int creal = (int) c.real;
			int cimag = (int) c.imag;

			//set the global values that will be used
			//by the IRQ handler if the timer is repeating
			if (c.imag > 0.0) { //sec and min were specified
				if (cimag > 84600) cimag = 84600;
				if (creal > 3600) creal = 3600;
				vm->alarmSec = (int) (cimag % 60);
				vm->alarmMin = (int) (creal % 60);
				vm->alarmHour = (int) (creal / 60);
			} else { //c.imag == 0.0
				if (creal > 84600) creal = 84600;
				vm->alarmSec = (int) (creal % 60);
				vm->alarmMin = (int) (creal / 60);
				vm->alarmHour = (int) (creal / 3600);
			}
			vm->alarmHour %= 24;

			datetime_t alarm;
			alarm.year  = 2023;
			alarm.month = 9;
			alarm.day   = 15;
			alarm.dotw  = 5; // Friday; 0 is Sunday
			alarm.sec = 0;
			alarm.min = 0;
			alarm.hour = 0;
			rtc_set_datetime(&alarm);
			alarm.sec = vm->alarmSec;
			alarm.min = vm->alarmMin;
			alarm.hour = vm->alarmHour;
			rtc_set_alarm(&alarm, &toggleLED);
		}
		return true;
	} else if (strcmp(token, "do") == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "No vecmat here.F", NULLFN)
		//conditional loop, check for previous if/else
		UintStackPush(&vm->execStack, (1<<(sizeof(size_t)-1)));
		return true;
	} else if (strcmp(token, IFTOKEN) == 0) {
		//SerialPrint(1, "-------------------Found if -- now popping vm->userStack");
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "No vecmat here.G", NULLFN)
		strcpy(vm->bak, vm->acc); //backup
		pop(&vm->userStack, vm->acc);
		ComplexDouble conditionComplex;
		success = false;
		if (isComplexNumber(vm->acc)) { //complex
			success = stringToComplex(vm->acc, &conditionComplex);
			ifCondition = !alm0double(conditionComplex.real) || !alm0double(conditionComplex.imag);
		} else if (isRealNumber(vm->acc)) { //real number
			success = stringToDouble(vm->acc, &conditionComplex.real);
			ifCondition = !alm0double(conditionComplex.real);
			//SerialPrint(1, "-------------------Found if: conditionComplex.real = %.15g", conditionComplex.real);
		} else { //string
			ifCondition = false;
		}
		//SerialPrint(1, "-------------------Found if: ifCondition = %d", ifCondition);
		execData = makeExecStackData(true, ifCondition, true, false, false); //doingIf = true
		//SerialPrint(1, "-------------------Found if: execData calculated = %lu", execData);
		UintStackPush(&vm->execStack, execData);
		return true;
	} else if (strcmp(token, ELSETOKEN) == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "No vecmathere.H", NULLFN)
		//SerialPrint(1, "-------------------Found else -- execData = %lu", execData);
		FAILANDRETURN((conditionalData(execData) == 0x0), vm->error, "else without if. A", NULLFN)
		FAILANDRETURN(!doingIf, vm->error, "el w/o if", NULLFN)
		FAILANDRETURN(UintStackIsEmpty(&vm->execStack), vm->error, "el w/o if.B", NULLFN)
		UintStackPop(&vm->execStack); //discard the if
		execData = makeExecStackData(true, ifCondition, false, false, false); //else
		//SerialPrint(1, "-------------------Found else -- set execData to = %lu", execData);
		UintStackPush(&vm->execStack, execData);
		return true;
	} else if (strcmp(token, ENDIFTOKEN) == 0) {
		FAILANDRETURN(vectorMatrixData(execData) != 0, vm->error, "No vecmat here.I", NULLFN)
		FAILANDRETURN((conditionalData(execData) == 0x0), vm->error, "fi w/o if. A", NULLFN)
		FAILANDRETURN(UintStackIsEmpty(&vm->execStack), vm->error, "fi w/o if. B", NULLFN)
		UintStackPop(&vm->execStack); //discard the if/else
		return true;
	} else {
		uint8_t meta = peek(&vm->userStack, vm->coadiutor);
		FAILANDRETURN((strcmp(token, MATSTARTTOKEN) == 0 || 
				strcmp(token, MATLASTTOKEN) == 0), vm->error, 
				"Dangling '['s", NULLFN)
		//SerialPrint(1, "-------------------Default condition with token = %s", token);
		bool doingMat = vectorMatrixData(execData) == 0x2;
		bool doingVec = vectorMatrixData(execData) == 0x1;
		//SerialPrint(1, "-------------------Default -- execData >> 3 = %lu", execData >> 3);
		//last instruction on exec stack was not an IF or ELSE

		//SerialPrint(1, "-------------------Default: conditional = %d ", conditional);
		if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
			int len_m_1 = strlen(token) - 1;
			bool token_0_eq_VECSTARTTOKENC = (token[0] == VECSTARTTOKENC);
			bool token_1_eq_VECSTARTTOKENC = (token[1] == VECSTARTTOKENC);
			bool token_l_eq_VECLASTTOKENC = (token[len_m_1] == VECLASTTOKENC);
			bool token_l_m_1_eq_VECLASTTOKENC = (token[len_m_1 - 1] == VECLASTTOKENC);
			if (token_l_eq_VECLASTTOKENC) { //vector or matrix ending 
				if (token_l_m_1_eq_VECLASTTOKENC) { //matrix ending
					if (len_m_1 < 2) return false; //too short
					token[len_m_1 - 1] = '\0';
					if (token_0_eq_VECSTARTTOKENC && token_1_eq_VECSTARTTOKENC){ //both matrix starting and ending
						//[[aa]]
						success = processDefaultPush(vm, &token[2], METANONE);
					} else if (token_0_eq_VECSTARTTOKENC) {
						//[aa]]
						FAILANDRETURN(!doingMat, vm->error, "Expect mat end.A", NULLFN)
						success = processDefaultPush(vm, &token[1], METAENDMATRIX);
					} else {
						//aa]]
						//SerialPrint(1, "-------------------Default -- aa]] Current execData >> 3 = %lu", execData >> 3);
						FAILANDRETURN(!doingMat, vm->error, "Expect mat end.B", NULLFN)
						//matrix is done
						execData = UintStackPop(&vm->execStack);
						success = processDefaultPush(vm, token, METAENDMATRIX);
					}
				} else if (token_0_eq_VECSTARTTOKENC){ //vector starting and ending
					//[aa]
					if (len_m_1 < 2) return false; //too short
					FAILANDRETURN((len_m_1 < 2), vm->error, "Syntax error.A", NULLFN)
					token[len_m_1] = '\0';
					success = processDefaultPush(vm, &token[1], METANONE);
				} else { //vector ending
					//a] or ]
					if (len_m_1 == 0) { //just a ']'
						FAILANDRETURN(!doingMat && !doingVec, vm->error, "Bad vec end", NULLFN)
						pop(&vm->userStack, vm->coadiutor);
						if (meta == METAMIDVECTOR) {
							push(&vm->userStack, vm->coadiutor, METAENDVECTOR);
						} else if (meta == METASTARTVECTOR) { //[2] becomes 2
							if (vm->coadiutor[0] != VECSTARTINDICATOR)
								push(&vm->userStack, vm->coadiutor, METANONE);
						} else if (meta == METAMIDMATRIX) {
							push(&vm->userStack, vm->coadiutor, METAENDMATRIX);
						}
					} else { //a]
						//if the last was a vector ending, we append the new ending
						//by making the previous one a mid-vector
						if (meta == METAENDVECTOR) {
							pop(&vm->userStack, vm->coadiutor);
							push(&vm->userStack, vm->coadiutor, METAMIDVECTOR);
						}
						token[len_m_1] = '\0'; //remove the ']' character
						success = processDefaultPush(vm, token, METAENDVECTOR);
					}
					if (doingVec) { //vector was going on, not matrix
						//vector is done
						execData = UintStackPop(&vm->execStack);
					}
				}
			} else if (token_0_eq_VECSTARTTOKENC) { //vector or matrix starting 
				if (len_m_1 == 0) {
					//[
					FAILANDRETURN(doingMat, vm->error, "Nested mats.A", NULLFN)
					if (!doingVec) { //already vector entry is NOT going on; another '[' starts a matrix entry
						execData = makeExecStackData(false, false, false, false, true); //vector
						UintStackPush(&vm->execStack, execData);
						token[0] = VECSTARTINDICATOR; //ASCII SYNC
						token[1] = '\0';
						success = processDefaultPush(vm, token, METASTARTVECTOR);
					} else {
						//already a vector going on, so this will be start of matrix entry
						//if TOS of userStack has VECSTARTINDICATOR, then ok, else error
						SerialPrint(1, "#### UNSUPPORTED #### UNSUPPORTED ####\r\n");
					}
				} else if (token_1_eq_VECSTARTTOKENC) {//matrix starting
					//[[aa
					FAILANDRETURN(doingMat, vm->error, "Nested mats.B", NULLFN)
					execData = makeExecStackData(false, false, false, true, false); //matrix
					UintStackPush(&vm->execStack, execData);
					success = processDefaultPush(vm, &token[2], METASTARTMATRIX);
				} else { //vector starting
					//[a
					//SerialPrint(1, "-------------------Default -- [a Current execData >> 3 = %lu", execData >> 3);
					FAILANDRETURN(doingVec, vm->error, "Nested vecs.C", NULLFN)
					//in above check for previous vector being the ASCII SYNC char
					FAILANDRETURN(doingMat, vm->error, "Nested mats.C", NULLFN)
					if (!doingVec) { //last '[' entry means another '[' starts a matrix entry
						execData = makeExecStackData(false, false, false, false, true); //vector
						UintStackPush(&vm->execStack, execData);
					}
					success = processDefaultPush(vm, &token[1], METASTARTVECTOR);
				}
			} else if (token[0] == POPTOKENC) { //pop to register or variable
				//SerialPrint(1, "------------------- Got POPTOKEN calling processPop with %s", &token[1]);
				success = processPop(vm, &token[1]);
			} else if (token[0] == PRINTTOKENC) { //print register or variable
				success = processPrint(vm, &token[1]);
			} else {	
				uint32_t tlen = strlen(token);
				if (token[tlen - 1] == PRINTTOKENC) { //pop to register or variable
					//postfix version of pop
					strncpy(vm->coadiutor, token, tlen - 1);
					vm->coadiutor[tlen - 1] = '\0';
					//printf("------------------- Got PRINTTOKEN for postfix version of pop; calling processPop with %s\n", vm->coadiutor);
					success = processPrint(vm, vm->coadiutor);
				} else if (token[tlen - 1] == POPTOKENC) { //pop to register or variable
					//postfix version of pop
					strncpy(vm->coadiutor, token, tlen - 1);
					vm->coadiutor[tlen - 1] = '\0';
					//printf("------------------- Got POPTOKEN for postfix version of pop; calling processPop with %s\n", vm->coadiutor);
					success = processPop(vm, vm->coadiutor);
				} else if (doingVec) {
					//SerialPrint(1, "-------------------Default condition - push token = %s which is char %d", token, (int)token[0]);
					//SerialPrint(1, "-------------------STACKPUSH: execData  = %lu", execData);	
					if (meta == METASTARTVECTOR && (vm->coadiutor[0] == VECSTARTINDICATOR)) {
						pop(&vm->userStack, vm->coadiutor);
						//pop out the placeholder and push the current token, marking as vector start
						success = processDefaultPush(vm, token, METASTARTVECTOR);
					} else {
						success = processDefaultPush(vm, token, METAMIDVECTOR);
					}
				} else if (doingMat) //a matrix end is also a vector end
					success = processDefaultPush(vm, token, METAMIDMATRIX); //mid matrix is also mix vector
				else if (token[0] != '\0')
					success = processDefaultPush(vm, token, METANONE);
			}
		}
	}
	return success;
}
