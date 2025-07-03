#include <stdbool.h>
#include <string.h>
#include "yasML.h"
#include "day.h"
#include "TS-core-stack.h"
#include "TS-core-numString.h"
#include "TS-core-llist.h"
#include "TS-core-miscel.h"

bool process(Machine* vm, char* token) {
	bool success = true;
	bool ifCondition, doingIf, conditional;
	ifCondition = doingIf = conditional = false;
	int32_t execData = UintStackPeek(&vm->execStack);

	GETEXECSTACKDATA(conditional, ifCondition, doingIf, execData)
	//printf("process:------------------- token = %s\n", token);
	//printf("process:------------------- UintStackPeek = %lu\n", execData);
	if ((ifCondition && doingIf) || (!ifCondition && !doingIf) || (!conditional)) {
		int is1ParamBigIntFn = is1ParamBigIntFunction(token);
		int ismatfn = isMatFunction(token);
		int is1pfn = is1ParamFunction(token);
		int is2pfn = is2ParamFunction(token);
		int isvecfn = isVec1ParamFunction(token);
		int isTrig = is1ParamTrigFunction(token);
		int8_t meta = peek(&vm->userStack, vm->matvecStrC);
		if (ismatfn != -1) {
			Matrix m;
			ComplexDouble c;
			FAILANDRETURN((meta == -1), vm->error, "stack empty.Y", NULLFN)
			switch (ismatfn) {
				case 0: //det
					FAILANDRETURN((meta != METAMATRIX), vm->error, "require matrix.", NULLFN)
					success = matbuild(&m, vm->matvecStrC);
					FAILANDRETURN((!success), vm->error, "bad input matrix.", NULLFN)
					success = matdeterminant(&m, &c);
					FAILANDRETURN((!success), vm->error, "mat singular.", NULLFN)
					success = complexToString(c, vm->coadiutor, vm->precision, vm->notationStr);
					FAILANDRETURN((!success), vm->error, "bad determinant.", NULLFN)
					//when operating on a vector or matrix and the result is a scalar,
					//don't pop out the vector/matrix
					push(&vm->userStack, vm->coadiutor, METASCALAR);
					break;
				case 1:	//inv
					FAILANDRETURN((meta != METAMATRIX), vm->error, "require matrix.", NULLFN)
					success = matbuild(&m, vm->matvecStrC);
					FAILANDRETURN((!success), vm->error, "bad input matrix.", NULLFN)
					success = matdeterminant(&m, &c);
					FAILANDRETURN((!success), vm->error, "no det.", NULLFN)
					FAILANDRETURN(alm0(c), vm->error, "mat singular.", NULLFN)
					FAILANDRETURN((m.rows != m.columns), vm->error, "non sqr matrix.", NULLFN)
					success = matinversion(&m, &vm->matrixC);
					FAILANDRETURN((!success), vm->error, "mat inv failed.", NULLFN)
					success = matrixToString(&vm->matrixC, vm->matvecStrC, vm->precision, vm->notationStr);
					FAILANDRETURN((!success), vm->error, "bad mat inv.", NULLFN)
					pop(&vm->userStack, NULL);
					push(&vm->userStack, vm->matvecStrC, METAMATRIX);
					break;
				case 2:	//iden
					break;
				case 3:	//trace
					break;
				case 4:	//eival
					break;
				case 5:	//eivec
					break;
				case 6:	//tpose
					break;
			}
		}
		else if (is1pfn != -1 || is1ParamBigIntFn != -1) {
			//printf("process:------------------- is1pfn = %d\n", is1pfn);
			//exp is special
			if (is1pfn == EXPFNINDEX) { //exp, can also compare token with "exp"
				int8_t cmeta = peek(&vm->userStack, vm->matvecStrB);
				FAILANDRETURN((cmeta == -1), vm->error, "stack empty.T", NULLFN)
				if (vm->matvecStrB[0] != '"') //not a bigint, could be vec, mat, real or complex
					success = fn1Param(vm, token, is1pfn, isTrig, is1ParamBigIntFn); //exp(x) function
				else
					//ToS is a bigint, try special exp function
					success = fnOrOp2Param(vm, token, is1pfn);
			}
			else
				success = fn1Param(vm, token, is1pfn, isTrig, is1ParamBigIntFn);
		} else if (is2pfn != -1) {
			//printf("process:------------------- is2pfn|op = %d\n", is2pfn);
			//could call 2-parameter vector function
			success = fnOrOp2Param(vm, token, is2pfn);
		} else if (isvecfn != -1) {
			FAILANDRETURN((meta == -1), vm->error, "stack empty.U", NULLFN)
			FAILANDRETURN((meta != METAVECTOR), vm->error, "only vector.", NULLFN)
			success = fnOrOpVec1Param(vm, token, isvecfn, false, false); //not a trig fn., result is not vector
		#ifndef DESKTOP_PC
		} else if (strcmp(token, "rnd") == 0) {
			uint32_t rnd = truerandom();
			success = doubleToString(rnd, vm->acc, vm->precision, vm->notationStr);
			FAILANDRETURN(!success, vm->error, "Random failed.", NULLFN)
			push(&vm->userStack, vm->acc, METASCALAR);
		#endif
		} else if (strcmp(token, "wid") == 0) {
			long double temp;
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN((meta == -1), vm->error, "stack empty.W", NULLFN)
			FAILANDRETURN((meta != METASCALAR), vm->error, "only scalar.", NULLFN)
			pop(&vm->userStack, vm->matvecStrC);
			success = stringToDouble(vm->matvecStrC, &temp);
			FAILANDRETURN(!success, vm->error, "bad width.", NULLFN)
			FAILANDRETURN(temp > 255, vm->error, "width > 255.", NULLFN)
			FAILANDRETURN(temp < 0, vm->error, "width < 0.", NULLFN)
			vm->width = (uint8_t) temp;
		} else if (strcmp(token, "mod") == 0 || strcmp(token, "savmod") == 0) {
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN((meta == -1), vm->error, "stack empty.W", NULLFN)
			FAILANDRETURN((meta != METASCALAR), vm->error, "only scalar.", NULLFN)
			pop(&vm->userStack, vm->matvecStrC);
			char* strC = removeDblQuotes(vm->matvecStrC);
			if (strC[0] == 'x') success = bigint_from_hex(&vm->bigC, strC);
			else if (strC[0] == 'b') success = bigint_from_bin(&vm->bigC, strC);
			else success = bigint_from_str(&vm->bigC, strC);
			FAILANDRETURN(!success, vm->error, "bad mod val.", NULLFN)
			vm->bigMod = vm->bigC;
			if (strcmp(token, "savmod") == 0) {
				#ifndef DESKTOP_PC
				success = writeBigintToFile(".modulus", &vm->bigMod);	
				FAILANDRETURN(!success, vm->error, "savmod failed.", NULLFN)
				#endif
			}
		} else if (strcmp(token, "solv") == 0) {
			success = polysolve(vm);
		} else if (strcmp(token, "reim") == 0) {
			//swap real and imaginary components
			int8_t meta = peek(&vm->userStack, NULL);
			long double temp;
			FAILANDRETURN((meta == -1), vm->error, "stack empty.W", NULLFN)
			FAILANDRETURN((meta != METASCALAR), vm->error, "only scalar.", NULLFN)
			pop(&vm->userStack, vm->matvecStrC);
			ComplexDouble c;			
			c.imag = 0;
			if (isComplexNumber(vm->matvecStrC)) //complex
				success = stringToComplex(vm->matvecStrC, &c);
			else if (isRealNumber(vm->acc)) //real number -> make it (0 real)
				success = stringToDouble(vm->matvecStrC, &c.real);
			FAILANDRETURN(!success, vm->error, "bad arg for reim.", NULLFN)

			if (vm->modePolar) {
				if (vm->modeDegrees) c.imag *= 0.01745329251994329576923L;
				if (!alm0double(c.imag)) {
					temp = c.real * cosl(c.imag);
					c.imag = c.real * sinl(c.imag);
					c.real = temp;
				}
				//now, the number is in radians in cartesian mode
			}

			temp = c.imag;
			c.imag = c.real;
			c.real = temp;

			if (vm->modePolar) {
				//convert back to polar mode
				if (!alm0double(c.real)) {
					temp = abso(c);
					c.imag = atan(c.imag/c.real);
					c.real = temp;
				} else {
					c.imag = 1.570796326794896619231L;//pi/2
					c.real = 0.0;
				}
			
				//convert back to degrees since this was converted to radians 
				if (vm->modeDegrees)
					c = makeComplex(c.real, c.imag * 57.295779513082320876798L);
			}

			complexToString(c, vm->matvecStrC, vm->precision, vm->notationStr);
			push(&vm->userStack, vm->matvecStrC, METASCALAR);
		#ifdef DESKTOP_PC
		} else if (strcmp(token, "pi") == 0) {
			push(&vm->userStack, "3.141592653589793", METASCALAR);
		} else if (strcmp(token, "e") == 0) {
			push(&vm->userStack, "2.718281828459045", METASCALAR);
		} else if (strcmp(token, "coord") == 0) {
			vm->modePolar ^= true;
		} else if (strcmp(token, "angle") == 0) {
			vm->modeDegrees ^= true;
		#endif
		} else if (strcmp(token, "bar") == 0) { //barrier or unbarrier
			int8_t meta = peekbarrier(&vm->userStack, NULL);
			//meaningless to put barrier on empty stack
			FAILANDRETURN((meta == -1), vm->error, "stack empty.W", NULLFN)
			pop(&vm->userStack, vm->matvecStrC);
			if (meta & 0x8) //already has barrier
				push(&vm->userStack, vm->matvecStrC, (meta & 0x7)); //clear the barrier bit
			else
				push(&vm->userStack, vm->matvecStrC, (meta | 0x8)); //set the barrier bit
			//printf("process: pushed meta = %d\n", (meta | 0x8));
		} else if (strcmp(token, "swp") == 0) {
			int8_t cmeta = popbarrier(&vm->userStack, vm->matvecStrC);
			FAILANDRETURN((cmeta == -1), vm->error, "stack empty.B0", NULLFN)
			int8_t meta = popbarrier(&vm->userStack, vm->matvecStrB);
			if (meta == -1) push(&vm->userStack, vm->matvecStrC, cmeta); //restore on error
			FAILANDRETURN((meta == -1), vm->error, "stack empty.B", NULLFN)
			push(&vm->userStack, vm->matvecStrC, cmeta);
			push(&vm->userStack, vm->matvecStrB, meta);
			return true;
		} else if (strcmp(token, "cmplx") == 0) {
			//make or unmake complex numbers
			long double re;
			long double im;
			ComplexDouble c;
			int8_t cmeta = peek(&vm->userStack, vm->matvecStrC);
			FAILANDRETURN((cmeta == -1), vm->error, "stack empty.B", NULLFN)
			success = isComplexNumber(vm->matvecStrC);
			if (success) {
				stringToComplex(vm->matvecStrC, &c);
				//current ToS is a complex number, uncomplex it
				doubleToString(c.real, vm->bak, vm->precision, vm->notationStr);
				doubleToString(c.imag, vm->acc, vm->precision, vm->notationStr);
				pop(&vm->userStack, NULL);
				//earlier (ToS-1) number will real part, later (ToS) number is imag part
				push(&vm->userStack, vm->bak, METASCALAR);
				push(&vm->userStack, vm->acc, METASCALAR);
			} else {
				//if two real numbers are available at the ToS, create a complex number
				//earlier number is real part, later number is imag part
				int8_t meta = peekn(&vm->userStack, vm->matvecStrB, 1);
				FAILANDRETURN((meta == -1), vm->error, "need 1 more scalar", NULLFN)
				FAILANDRETURN((cmeta != METASCALAR || meta != METASCALAR), vm->error, "need 2 scalars", NULLFN)
				success = stringToDouble(vm->matvecStrB, &re);//earlier (ToS-1) number is real part
				success = stringToDouble(vm->matvecStrC, &im) & success;//later (ToS) number is imag part
				FAILANDRETURN(!success, vm->error, "need 2 reals", NULLFN)
				success = strIsRLC(vm->matvecStrB) && strIsRLC(vm->matvecStrC);
				FAILANDRETURN(success, vm->error, "no RLC here", NULLFN)
				strcpy(vm->matvecStrA, "("); 
				strcat(vm->matvecStrA, vm->matvecStrB); strcat(vm->matvecStrA, " "); strcat(vm->matvecStrA, vm->matvecStrC);
				strcat(vm->matvecStrA, ")");
				pop(&vm->userStack, NULL);
				pop(&vm->userStack, NULL);
				push(&vm->userStack, vm->matvecStrA, METASCALAR);
			}
		} else if (strcmp(token, "vec") == 0) {
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN((meta == -1), vm->error, "stack empty.Z", NULLFN)
			FAILANDRETURN((meta != METAVECTOR && meta != METASCALAR), vm->error, "bad arg.Z", NULLFN)
			if (meta == METAVECTOR) {
				//ToS is a vector, unvectorize it
				char* input = NULL;
				char output[MAX_TOKEN_LEN];
				pop(&vm->userStack, vm->matvecStrA);
				input = vm->matvecStrA;
				while (true) {
					input = tokenize(input, output);
					//printf ("process: loop, output = %s\n", output);
					if (output[0] == ']') break;
					if (output[0] == '[') continue;
					strcpy(vm->acc, output);
					push(&vm->userStack, vm->acc, METASCALAR);
				}
				return true;
			}
			//ToS is a scalar
			int i = 0;
			int j = 0;
			do {
				//break on seeing a stack barrier
				//printf("in do-while loop count i = %d\n", i);
				meta = peeknbarrier(&vm->userStack, NULL, i);
				//printf("in do-while loop at count i = %d and meta returned %d\n", i, meta);
				if ((meta & 0x7) != METASCALAR || (meta & METABARRIER)) {
					break;
				}
				i++;
			} while (true);
			//printf("final count i = %d\n", i);
			if (i == 0) return true; //no vectorizable elements
			strcpy(vm->matvecStrC, "[");
			while (i > 0) {
				peekn(&vm->userStack, vm->acc, i - 1);
				strcat(vm->matvecStrC, vm->acc);
				if (i != 1) strcat(vm->matvecStrC, " ");
				j++;
				i--;
			}
			//printf("j count i = %d\n", j);
			while (j-- > 0) pop(&vm->userStack, NULL);

			strcat(vm->matvecStrC, "]");
			//printf("final vector %s\n", vm->matvecStrC);
			push(&vm->userStack, vm->matvecStrC, METAVECTOR);
		#ifndef DESKTOP_PC
		} else if (strcmp(token, "getm") == 0) {
			sprintf(vm->acc, "\"Month: %d\"", (int) vm->month);
			push(&vm->userStack, vm->acc, METASCALAR | METABARRIER);
		} else if (strcmp(token, "gety") == 0) {
			sprintf(vm->acc, "\"Year: %d\"", (int) vm->year);
			push(&vm->userStack, vm->acc, METASCALAR | METABARRIER);
		} else if (strcmp(token, "getla") == 0) {
			sprintf(vm->acc, "\"Lattitude:%f\"", vm->locationLat);
			push(&vm->userStack, vm->acc, METASCALAR | METABARRIER);
		} else if (strcmp(token, "getlo") == 0) {
			sprintf(vm->acc, "\"Longitude:%f\"", vm->locationLong);
			push(&vm->userStack, vm->acc, METASCALAR | METABARRIER);
		} else if (strcmp(token, "gettz") == 0) {
			sprintf(vm->acc, "\"Timezone:UTC+%f\"", vm->locationTimeZone);
			push(&vm->userStack, vm->acc, METASCALAR | METABARRIER);
		} else if (strcmp(token, "getmode") == 0) {
			sprintf(vm->acc, "\"%s %s\"", (vm->modeDegrees)? "Degrees": "Radians", (vm->modePolar)? "Polar": "Cartesian");
			push(&vm->userStack, vm->acc, METASCALAR | METABARRIER);
		} else if (strcmp(token, "getprec") == 0) {
			sprintf(vm->acc, "\"Precision:%d\"", vm->precision);
			push(&vm->userStack, vm->acc, METASCALAR | METABARRIER);
		} else if (strcmp(token, "getmod") == 0) {
			bigint_tostring(&vm->bigMod, vm->matvecStrB, 0);
			sprintf(vm->matvecStrA, "\"Modulus: %s\"", vm->matvecStrB);
			push(&vm->userStack, vm->matvecStrA, METASCALAR | METABARRIER);
		} else if (strcmp(token, "mode") == 0) {
				//save modePolar, modeDegrees
				float f = (float) vm->modePolar;
				success = writeOneVariableToFile(".polar", &f);
				f = (float) vm->modeDegrees;
				success = writeOneVariableToFile(".degrees", &f) && success;
				f = (float) vm->cmdPage;
				success = writeOneVariableToFile(".cmdpage", &f) && success;
				FAILANDRETURN(!success, vm->error, "mode save failed.", NULLFN)
		} else if (strcmp(token, "mm") == 0 || strcmp(token, "yyyy") == 0 ||
				strcmp(token, "latt") == 0 || strcmp(token, "longt") == 0 ||
				strcmp(token, "prec") == 0 ||
				strcmp(token, "timez") == 0) {
			int8_t meta = peek(&vm->userStack, vm->acc);
			FAILANDRETURN((meta == -1), vm->error, "stack empty.Z1", NULLFN)
			long double d;
			success = stringToDouble(vm->acc, &d);
			FAILANDRETURN(!success, vm->error, "bad arg.Z1", NULLFN)
			float f = (float) d;
			if (strcmp(token, "mm") == 0) {
				//store ToS in a file named ".month"
				success = writeOneVariableToFile(".month", &f);
				FAILANDRETURN(!success, vm->error, "mm failed.", NULLFN)
				vm->month = (int) f;
			} else if (strcmp(token, "prec") == 0) {
				success = writeOneVariableToFile(".precision", &f);
				FAILANDRETURN(!success, vm->error, "prec failed.", NULLFN)
				vm->precision = (uint8_t) f;
			} else if (strcmp(token, "yyyy") == 0) {
				//store ToS in a file named ".year"
				success = writeOneVariableToFile(".year", &f);
				FAILANDRETURN(!success, vm->error, "yyyy failed.", NULLFN)
				vm->year = (int) f;
			} else if (strcmp(token, "latt") == 0) {
				//store ToS in a file named ".lat"
				success = writeOneVariableToFile(".lat", &f);
				FAILANDRETURN(!success, vm->error, "latt failed.", NULLFN)
				vm->locationLat = f;
			} else if (strcmp(token, "longt") == 0) {
				//store ToS in a file named ".long"
				success = writeOneVariableToFile(".long", &f);
				FAILANDRETURN(!success, vm->error, "longt failed.", NULLFN)
				vm->locationLong = f;
			} else if (strcmp(token, "timez") == 0) {
				//store ToS in a file named ".timez"
				success = writeOneVariableToFile(".timez", &f);
				FAILANDRETURN(!success, vm->error, "timez failed.", NULLFN)
				vm->locationTimeZone = f;
			}
			pop(&vm->userStack, NULL);
		#endif
		//} else if (strcmp(token, "daymore") == 0) {
		} else if (strcmp(token, "day") == 0) {
			int dd = 1;
			int mm = (int) vm->month;
			int yyyy = (int) vm->year;
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN((meta == -1), vm->error, "stack empty.Z", NULLFN)
			if (meta == METAVECTOR) {
				//ToS is a vector, has dd, mm, yyyy
				char* input = NULL;
				char output[MAX_TOKEN_LEN];
				pop(&vm->userStack, vm->matvecStrA);
				input = vm->matvecStrA;
				int count = 0;
				ComplexDouble c;
				while (true) {
					input = tokenize(input, output);
					printf ("process: day loop - output = %s count = %d\n", output, count);
					if (output[0] == ']' || count == 3) break;
					if (output[0] == '[') continue;
					if (stringToComplex(output, &c)){
						if (count == 0) dd = (int) c.real;
						else if (count == 1) mm = (int) c.real;
						else if (count == 2) yyyy = (int) c.real;
					}
					else FAILANDRETURN(true, vm->error, "bad arg.V2", NULLFN)
					count++;
				}
			}
			else if (meta == METASCALAR) {
				//assume only date is at ToS
				pop(&vm->userStack, vm->acc);
				long double d;
				success = stringToDouble(vm->acc, &d);
				FAILANDRETURN(!success, vm->error, "bad arg.Z1", NULLFN)
				dd = (int) d;
			}
			else FAILANDRETURN(true, vm->error, "bad arg.V", NULLFN)
			Suntimes times;
			long double srise = sun ((long double) vm->locationLat, (long double) vm->locationLong, (long double) vm->locationTimeZone, dd, mm, yyyy, &times);
			tithiday (dd, mm, yyyy, srise, (long double) vm->locationTimeZone, vm->matvecStrA);
			strcat(vm->matvecStrA, " ");
			vaaraday (dd, mm, yyyy, (long double) vm->locationTimeZone, vm->acc);
			strcat(vm->matvecStrA, vm->acc);
			strcat(vm->matvecStrA, "day ");
			strcat(vm->matvecStrA, "Nakshatra:");
			nakshatraday (dd, mm, yyyy, srise, (long double) vm->locationTimeZone, vm->acc);
			strcat(vm->matvecStrA, vm->acc);
			sprintf(vm->matvecStrB, " Sunrise:%d:%s%d:%s%d Sunset:%d:%s%d:%s%d Duration:%d:%s%d:%s%d", 
				times.sriseh, (times.srisem < 10)? "0":"", times.srisem, (times.srises < 10)? "0": "", times.srises, 
				times.sseth, (times.ssetm < 10)? "0":"", times.ssetm, (times.ssets < 10)? "0": "", times.ssets, 
				times.dlh, (times.dlm < 10)? "0": "", times.dlm, (times.dls < 10)? "0": "", times.dls);
			strcat(vm->matvecStrA, vm->matvecStrB);
			addDblQuotes(vm->matvecStrA);
			push(&vm->userStack, vm->matvecStrA, METASCALAR | METABARRIER);
		} else if (strcmp(token, "lastx") == 0) {
			push(&vm->userStack, vm->lastX, vm->lastXMeta);
		} else if (strcmp(token, "lasty") == 0) {
			push(&vm->userStack, vm->lastY, vm->lastYMeta);
		} else if (strcmp(token, "dot") == 0) {
			int8_t cmeta = peek(&vm->userStack, NULL); //c
			int8_t meta = peekn(&vm->userStack, NULL, 1);  //b
			//fnindex = 6 is multiplication
			return fnOrOpVec2Param(vm, token, 6, cmeta, meta, false); //returnsVector = false
		} else if (strcmp(token, "dup") == 0) {
			int8_t meta = peek(&vm->userStack, vm->matvecStrC);
			FAILANDRETURN((meta == -1), vm->error, "stack empty", NULLFN)
			FAILANDRETURN((meta == METAVECTORPARTIAL), vm->error, "bad vector.2", NULLFN)
			FAILANDRETURN((meta == METAMATRIXPARTIAL), vm->error, "bad matrix.2", NULLFN)
			push(&vm->userStack, vm->matvecStrC, meta);
			return true;
		} else if (strcmp(token, "dupn") == 0) { //duplicates the nth item from ToS, ToS has index of 0
			int8_t meta = peek(&vm->userStack, vm->matvecStrC);
			long double d;
			FAILANDRETURN((meta == -1), vm->error, "stack empty", NULLFN)
			if (isRealNumber(vm->matvecStrC)) { //real number
				success = stringToDouble(vm->matvecStrC, &d);
				pop(&vm->userStack, NULL);
				if (d < 1) return false; 
				//zero negative dup doesn't do anything
				meta = peekn(&vm->userStack, vm->matvecStrC, (int)(d - 1));
				push(&vm->userStack, vm->matvecStrC, meta);
			} else pop(&vm->userStack, NULL);
			return true;
		} else if (strcmp(token, JMPTOKEN) == 0 || strcmp(token, JPZTOKEN) == 0 || strcmp(token, JNZTOKEN) == 0) {
			//jmp, jpz, jnz
			//conditional loop, check for previous if/else
			UintStackPush(&vm->execStack, (1<<(sizeof(int32_t)-1)));
			return true;
		} else if (strcmp(token, IFTOKEN) == 0) {
			//printf ("-------------------Found if -- now popping vm->userStack\n");
			pop(&vm->userStack, vm->matvecStrA);
			ComplexDouble conditionComplex;
			success = false;
			//evaluate the if/else condition using the same variable
			if (isComplexNumber(vm->matvecStrA)) { //complex
				success = stringToComplex(vm->matvecStrA, &conditionComplex);
				ifCondition = !alm0double(conditionComplex.real) || !alm0double(conditionComplex.imag);
			} else if (isRealNumber(vm->matvecStrA)) { //real number
				success = stringToDouble(vm->matvecStrA, &conditionComplex.real);
				ifCondition = !alm0double(conditionComplex.real);
				//printf("-------------------Found if: conditionComplex.real = %.15g\n", conditionComplex.real);
			} else { //string
				ifCondition = false;
			}
			//printf("-------------------Found if: ifCondition = %d\n", ifCondition);
			execData = makeExecStackData(true, ifCondition, true); //doingIf = true
			//printf("-------------------Found if: execData calculated = %lu\n", execData);
			UintStackPush(&vm->execStack, execData);
			return success;
		} else if (strcmp(token, ELSETOKEN) == 0) {
			//printf ("-------------------Found else -- execData = %lu\n", execData);
			FAILANDRETURN((conditionalData(execData) == 0x0), vm->error, "else w/o if A", NULLFN)
			FAILANDRETURN(!doingIf, vm->error, "else w/o if.", NULLFN)
			FAILANDRETURN(UintStackIsEmpty(&vm->execStack), vm->error, "else w/o if B", NULLFN)
			UintStackPop(&vm->execStack); //discard the if
			execData = makeExecStackData(true, ifCondition, false); //condition is same, doing else
			//printf ("-------------------Found else -- set execData to = %lu\n", execData);
			UintStackPush(&vm->execStack, execData);
			return true;
		} else if (strcmp(token, ENDIFTOKEN) == 0) {
			FAILANDRETURN((conditionalData(execData) == 0x0), vm->error, "fi w/o if A", NULLFN)
			FAILANDRETURN(UintStackIsEmpty(&vm->execStack), vm->error, "fi w/o if B", NULLFN)
			UintStackPop(&vm->execStack); //discard the if/else
			return true;
		} else if (token[0] == MATSTARTTOKENC) {
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN(meta == METAVECTORMATRIXPARTIAL || meta == METAMATRIXPARTIAL, vm->error, "nested matrices", NULLFN)
			//push a matrix indicator
			push(&vm->userStack, (char*)"{", METAMATRIXPARTIAL);
		} else if (token[0] == MATLASTTOKENC) {
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN(meta != METAMATRIXPARTIAL, vm->error, "need matrix", NULLFN)
			pop(&vm->userStack, vm->matvecStrC);
			if (strcmp(vm->matvecStrC, "{") != 0) {
				strcat(vm->matvecStrC, "}");
				push(&vm->userStack, vm->matvecStrC, METAMATRIX); //close the matrix
			} //else throw out an empty matrix
		} else if (token[0] == VECSTARTTOKENC) {
			//parse vector first fragment
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN(meta == METAVECTORPARTIAL, vm->error, "nested vectors", NULLFN)
			if (meta == METAMATRIXPARTIAL) { //continuing a matrix
				pop(&vm->userStack, vm->matvecStrC);
				if (vm->matvecStrC[strlen(vm->matvecStrC) - 1] == '{')
					strcat(vm->matvecStrC, "[");
				else
					strcat(vm->matvecStrC, " [");
				push(&vm->userStack, vm->matvecStrC, METAVECTORMATRIXPARTIAL);
			} else { //push a vector indicator
				push(&vm->userStack, (char*)"[", METAVECTORPARTIAL);
			}
		} else if (token[0] == VECLASTTOKENC) {
			int8_t meta = peek(&vm->userStack, NULL);
			FAILANDRETURN(meta != METAVECTORPARTIAL && meta != METAVECTORMATRIXPARTIAL, vm->error, "need vector", NULLFN)
			//have a '[' -- getting a ']' -- discard the vector 
			pop(&vm->userStack, vm->matvecStrC);
			if (strcmp(vm->matvecStrC, "[") != 0) {
				if (meta == METAVECTORPARTIAL) {
					strcat(vm->matvecStrC, "]");
					push(&vm->userStack, vm->matvecStrC, METAVECTOR); //close the vector
				} else { //METAVECTORMATRIXPARTIAL
					//validate matrix and fill in blank cells
					strcat(vm->matvecStrC, "]");
					push(&vm->userStack, vm->matvecStrC, METAMATRIXPARTIAL); //close the vector inside the matrix
				}
			} //else throw out an empty vector
		} else if (token[0] == POPTOKENC || strcmp(token, DPOPTOKEN) == 0) {
			success = processPop(vm, token);
		} else if (token[0] == PRINTTOKENC) { //print register or variable
			success = processPrint(vm, token);
		} else {
			int32_t tokenlen = strlen(token);
			if (token[tokenlen - 1] == '@') {
				//found <variable>@
				token[tokenlen - 1] = '\0';
				success = popIntoVariable (vm, token);
				FAILANDRETURN(!success, vm->error, "bad var", NULLFN)
			}
			else {
				//see if the ToS is a partial matrix or vector
				int8_t meta = peek(&vm->userStack, NULL);
				StrackMeta varmeta;
				//matvecStrA has var value or literal -- variable substitution by value
				processDefaultPush(vm, token, vm->matvecStrA, &varmeta); 
				FAILANDRETURN(varmeta != METASCALAR, vm->error, "only scalar var.", NULLFN)
				if (meta == METAVECTORMATRIXPARTIAL) {
					pop(&vm->userStack, vm->matvecStrC);
					if (vm->matvecStrC[strlen(vm->matvecStrC) - 1] != '[')
						strcat(vm->matvecStrC, " ");
					strcat(vm->matvecStrC, vm->matvecStrA);
					push(&vm->userStack, vm->matvecStrC, METAVECTORMATRIXPARTIAL);
				} else if (meta == METAVECTORPARTIAL) {
					pop(&vm->userStack, vm->matvecStrC);
					if (vm->matvecStrC[strlen(vm->matvecStrC) - 1] != '[')
						strcat(vm->matvecStrC, " ");
					strcat(vm->matvecStrC, vm->matvecStrA);
					push(&vm->userStack, vm->matvecStrC, METAVECTORPARTIAL);
				} else if (meta == METAMATRIXPARTIAL) {
					pop(&vm->userStack, vm->matvecStrC);
					if (vm->matvecStrC[strlen(vm->matvecStrC) - 1] == ']')
						strcat(vm->matvecStrC, " [");
					else if (vm->matvecStrC[0] == '{') 
						strcat(vm->matvecStrC, "[");
					strcat(vm->matvecStrC, vm->matvecStrA);
					push(&vm->userStack, vm->matvecStrC, METAVECTORMATRIXPARTIAL);
				} else {
					push(&vm->userStack, vm->matvecStrA, varmeta);
				}
			}
		}
	}
	return success;
}

