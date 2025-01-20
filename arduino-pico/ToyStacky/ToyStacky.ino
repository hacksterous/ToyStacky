/*
(C) Anirban Banerjee 2023-2024
License: GNU GPL v3
*/
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#pragma GCC diagnostic ignored "-Wformat-overflow"
#pragma GCC diagnostic ignored "-Wformat-truncation"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <pico/multicore.h>
#include <hardware/rtc.h>
#include <hardware/resets.h>
#include "TS-core.h"

#define CORE0_TO_CORE1_START 0x11
#define CORE1_TO_CORE0_DONE 0x33
const char* __TS_PI_STR__ = "3.141592653589793";
const char* __TS_E_STR__ = "2.718281828459045";

uint32_t core1msg;
const byte ROWS = 12;
const byte COLS = 4;

//define the cymbols on the buttons of the keypads
char numKeys[ROWS][COLS] = 
{
	{'u','a','b','c'},
	{'D','d','e','f'},
	{'C','[','v','"'},
	{'A','(','{','@'},

	{'m','M','q','\b'},
	{'p','s','X','t'},
	{'h','z','n','T'},
	{'l',' ','<','>'},

	{'1','2','3','+'},
	{'4','5','6','-'},
	{'7','8','9','*'},
	{'0','.','/','\n'}
};

//LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
LiquidCrystal lcd(18, 19, 20, 21, 22, 26);
byte rowPins[ROWS] = {15, 14, 13, 12, 11, 10, 9, 8, 3, 2, 1, 0}; //these are inputs
byte colPins[COLS] = {7, 6, 5, 4}; //these are outputs
Keypad customKeypad = Keypad( makeKeymap(numKeys), rowPins, colPins, ROWS, COLS); 

// 0=user continues to edit; 1=evaluate immediately; 2=backspace; ... others ...
int keyTypePressed = -1; 
	
void printRegisters(Machine* vm) {
}

void clearUserInput(){
	vm.userInputLeftOflow = false;
	vm.userInputRightOflow = false;
	memset(vm.userInput, 0, STRING_SIZE);
	memset(vm.userDisplay, 0, SHORT_STRING_SIZE);
	vm.userInputPos = 0;
	vm.cursorPos = DISPLAY_LINESIZE - 1;
}

void eraseUserEntryLine() {
	//SerialPrint(2, "eraseUserEntryLine: eraseUserEntryLine called", "\r\n");
	lcd.setCursor(0, DISPLAY_LINECOUNT - 1); //col, row
	lcd.print("                    ");
	lcd.setCursor(vm.cursorPos, DISPLAY_LINECOUNT - 1); //col, row
}

//view page - 
//0: normal, 
//1: stack view, 
//2: variable view, 
//3: variable list view, 
//4: internal status view
//5: code editor view
//int viewPage;
//int topEntryNum; //stack entry number shown at top row
//int pointerRow; //entry pointed to by right pointing arrow
void updatesForUpMotion() {
	int topRowNum = DISPLAY_LINECOUNT - 1;// == 3
	int8_t lastPointerRow = vm.pointerRow;
	if (vm.viewPage == NORMAL_VIEW) {
		showStackHP(&vm, 0, topRowNum);
		cleanUpModes(&vm);
		vm.viewPage = STACK_VIEW;
		vm.pointerRow = topRowNum;
	} else if (vm.viewPage == STACK_VIEW) {
		if (vm.pointerRow > 0) {
			//pointRow is 0 at topmost row
			if (DISPLAY_LINECOUNT - vm.pointerRow < vm.userStack.itemCount)
				vm.pointerRow--;
		} else if (vm.pointerRow == 0) {
			//int8_t meta = peekn(&vm.userStack, NULL, vm.topEntryNum + 1);
			//if (meta != -1) {
			if (vm.topEntryNum < vm.userStack.itemCount - 1) {
				vm.topEntryNum++;
				showStackHP(&vm, vm.topEntryNum - topRowNum, vm.topEntryNum);
			} else return;
		}
	}
	lcd.setCursor(1, lastPointerRow); //col, row
	lcd.print(' ');
	lcd.setCursor(1, vm.pointerRow); //col, row
	lcd.print(RIGHTARROW);
}

void updatesForDownMotion() {
	int topRowNum = DISPLAY_LINECOUNT - 1;// == 3
	int8_t lastPointerRow = vm.pointerRow;
	if (vm.viewPage == NORMAL_VIEW) {
		showStackHP(&vm, 0, topRowNum);
		cleanUpModes(&vm);
		vm.viewPage = STACK_VIEW;
		vm.pointerRow = topRowNum;
	} else if (vm.viewPage == STACK_VIEW) {
		if (vm.pointerRow < topRowNum) {
			//pointRow is DISPLAY_LINECOUNT - 1 at bottommost row
			vm.pointerRow++;
		} else if (vm.pointerRow == topRowNum) {
			if (vm.topEntryNum > topRowNum)
				vm.topEntryNum--;
			showStackHP(&vm, vm.topEntryNum - topRowNum, vm.topEntryNum);
		}
	}
	lcd.setCursor(1, lastPointerRow); //col, row
	lcd.print(' ');
	lcd.setCursor(1, vm.pointerRow); //col, row
	lcd.print(RIGHTARROW);
}

void cleanUpStackView(){
	lcd.setCursor(1, vm.pointerRow); //col, row
	lcd.print(' ');
}

void updatesForLeftMotion() {
	int len = strlen(vm.userInput);

	memset(vm.userDisplay, 0, SHORT_STRING_SIZE);
	if ((!vm.userInputLeftOflow && !vm.userInputRightOflow) || 
		(!vm.userInputLeftOflow && vm.userInputRightOflow)) {
		//no scrolled indicators
		//limit movement beyond first char
		if (vm.cursorPos > 0)
			vm.cursorPos--;
		if (vm.userInputPos > 0)
			vm.userInputPos--;
		if (vm.cursorPos < DISPLAY_LINESIZE - len - 1)
			vm.cursorPos = DISPLAY_LINESIZE - len - 1;
		SerialPrint(2, "updatesForLeftMotion: 00 01 case ", "\n\r");
	} else if ((vm.userInputLeftOflow && !vm.userInputRightOflow) ||
			(vm.userInputLeftOflow && vm.userInputRightOflow)) {
		//if user input string is larger than 19, so has scrolled left
		//don't decrement cursorPos unless we are showing
		//scrolling right
		//left scrolled indicator
		if (vm.cursorPos >= 2) {
			vm.cursorPos--;
			//the left scroll indicator remains after the left scroll
			if (vm.userInputPos > 0)
				vm.userInputPos--;
			//no change in display, just move the cursor left
			SerialPrint(2, "updatesForLeftMotion: 10 11 A case ", "\n\r");
		} else if (vm.cursorPos == 1) {
			if (vm.userInputPos == 2) {
				//12<34... '<' indicator is cursor position 0, userInputPos points to input[2]='3'
				vm.userInputPos--;
				lcd.setCursor(0, 3); //col, row
				zstrncpy(vm.userDisplay, vm.userInput, DISPLAY_LINESIZE);
				lcd.print(vm.userDisplay);
				//no decrement of vm.cursorPos
				vm.userInputLeftOflow = false;
				SerialPrint(2, "updatesForLeftMotion: 10 11 B case ", "\n\r");
			} else if (vm.userInputPos > 2) {
				vm.userInputPos--;
				lcd.setCursor(0, 3); //col, row
				//no decrement of vm.cursorPos
				lcd.print(LEFTARROW);
				if ((len - vm.userInputPos) == (DISPLAY_LINESIZE - 1)) {
					//no right overflow
					zstrncpy(vm.userDisplay, &vm.userInput[vm.userInputPos], DISPLAY_LINESIZE - 1);
					lcd.print(vm.userDisplay);
					SerialPrint(2, "updatesForLeftMotion: 10 11 C case ", "\n\r");
				} else if ((len - vm.userInputPos) > (DISPLAY_LINESIZE - 1)) {
					zstrncpy(vm.userDisplay, &vm.userInput[vm.userInputPos], DISPLAY_LINESIZE - 2);
					lcd.print(vm.userDisplay);
					vm.userInputRightOflow = true;
					SerialPrint(2, "updatesForLeftMotion: 10 11 D case ", "\n\r");
				} else {
					SerialPrint(2, ">>>>>>> updatesForLeftMotion: 10 11 E case ERROR: can't have this", "\n\r");
				}
			}
		}
		if (vm.userInputRightOflow) {
			lcd.setCursor(DISPLAY_LINESIZE - 1, 3); //col, row
			lcd.print(RIGHTARROW);
		}
	}
	lcd.setCursor(vm.cursorPos, 3); //col, row
}

void updatesForRightMotion(){
	int len = strlen(vm.userInput);
	memset(vm.userDisplay, 0, SHORT_STRING_SIZE);

	if ((!vm.userInputLeftOflow && !vm.userInputRightOflow) || 
		(vm.userInputLeftOflow && !vm.userInputRightOflow)) {
		//no scrolled indicators
		//limit movement beyond first char
		if (vm.cursorPos < DISPLAY_LINESIZE - 1)
			vm.cursorPos++;
		if (vm.userInputPos < len)
			vm.userInputPos++;
		if (vm.cursorPos > DISPLAY_LINESIZE - 1)
			vm.cursorPos = DISPLAY_LINESIZE - 1;
		SerialPrint(2, "updatesForRightMotion: 00 10 case ", "\n\r");
		if (vm.cursorPos == DISPLAY_LINESIZE - 1) {
			if (vm.userInputPos == len) {
				//show cursor entry position on col 19 and move left
				if (len > DISPLAY_LINESIZE - 1) {
					lcd.setCursor(0, 3); //col, row
					lcd.print(LEFTARROW);
					vm.userInputLeftOflow = true;
					zstrncpy(vm.userDisplay, &vm.userInput[len - DISPLAY_LINESIZE + 2], DISPLAY_LINESIZE - 2);
					SerialPrint(2, "updatesForRightMotion: 00 10 A case ", "\n\r");
					lcd.print(vm.userDisplay);
					lcd.print(' ');
				} else {
					SerialPrint(2, "updatesForRightMotion: 00 10 B case ", "\n\r");
					lcd.setCursor(DISPLAY_LINESIZE - len, 3); //col, row
					zstrncpy(vm.userDisplay, &vm.userInput[len - DISPLAY_LINESIZE + 1], len);
					lcd.print(vm.userDisplay);
				}
			}
		}
	} else if ((!vm.userInputLeftOflow && vm.userInputRightOflow) ||
			(vm.userInputLeftOflow && vm.userInputRightOflow)) {
		//right scroll off indicator showing
		if (vm.cursorPos < DISPLAY_LINESIZE - 2) {
			vm.cursorPos++;
			if (vm.userInputPos < len) {
				vm.userInputPos++;
			}
			//no change in display, just move the cursor right
			SerialPrint(2, "updatesForRightMotion: 01 11 A case ", "\n\r");
		} else {
			//cursor is just to the left of the right scroll off indicator
			if (vm.userInputPos < len - 3) {
				//right scroll is required
				vm.userInputPos++;
				SerialPrint(2, "updatesForRightMotion: 01 11 B case ", "\n\r");
				lcd.setCursor(0, 3);
				lcd.print(LEFTARROW);
				vm.userInputLeftOflow = true;
				zstrncpy(vm.userDisplay, &vm.userInput[vm.userInputPos - DISPLAY_LINESIZE + 3], DISPLAY_LINESIZE - 2);
				lcd.print(vm.userDisplay);
			} else if (vm.userInputPos >= len - 3) {
				//right scroll is not required
				vm.userInputPos++;
				if (vm.userInputPos == len)
					vm.cursorPos++;
				//there is one char scrolled off to the right after the indicator
				//recalculate whether a left scroll indicator is required
				lcd.setCursor(0, 3); //col, row
				lcd.print(LEFTARROW);
				vm.userInputLeftOflow = true;
				SerialPrint(2, "updatesForRightMotion: 01 11 C case ", "\n\r");
				zstrncpy(vm.userDisplay, &vm.userInput[vm.userInputPos - DISPLAY_LINESIZE + 3], DISPLAY_LINESIZE - 1);
				lcd.print(vm.userDisplay);
				lcd.print(' ');
				//no increment of vm.cursorPos
				vm.userInputRightOflow = false;
			}
		}
	}
	lcd.setCursor(vm.cursorPos, 3); //col, row
}

void setup1() {
}

void setup() {
	initMachine(&vm);
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);	// turn the LED off (LOW is the voltage level)
	vm.LEDState = false;

	//FIXME - remove once tim command works
    datetime_t alarm = {
		.year  = 2023,
        .month = 9,
        .day   = 15,
        .dotw  = 5, // 0 is Sunday
        .hour  = 0,
        .min   = 0,
        .sec   = 0
    };

	vm.alarmHour = 0;
	vm.alarmMin = 0;
	vm.alarmSec = 10;

	vm.cmdPage = 0;
	vm.altState = 0;

    // Start the RTC
    rtc_init();
    rtc_set_datetime(&alarm);
	vm.TS0RTCFreq = clock_get_hz(clk_rtc);

    alarm.sec  = vm.alarmSec;
	rtc_set_alarm(&alarm, &toggleLED);
	vm.repeatingAlarm = true;

	//various indicator bitmaps
	byte upIndicator[8] = {
	  B00100,
	  B01110,
	  B10101,
	  B10101,
	  B00100,
	  B00100,
	  B00100,
	};
	
	byte downIndicator[8] = {
	  B00100,
	  B00100,
	  B00100,
	  B10101,
	  B10101,
	  B01110,
	  B00100,
	};

	unsigned char altIndicator[8] = {
	  B11110,
	  B10010,
	  B11110,
	  B10010,
	  B10010,
	  B00000,
	  B00000,
	};

	unsigned char altLockIndicator[8] = {
	  B11110,
	  B10010,
	  B11110,
	  B10010,
	  B10010,
	  B10000,
	  B11100,
	};

	byte twoIndicator[8] = {
	  B00000,
	  B11110,
	  B00010,
	  B00010,
	  B11110,
	  B10000,
	  B11110,
	};

	// initialize digital pin LED_BUILTIN as an output.
	lcd.createChar(UPIND, upIndicator);
	lcd.createChar(DOWNIND, downIndicator);
	lcd.createChar(ALTIND, altIndicator);
	lcd.createChar(ALTLOCKIND, altLockIndicator);
	lcd.createChar(TWOIND, twoIndicator);
	lcd.begin(DISPLAY_LINESIZE, DISPLAY_LINECOUNT); //20 cols, 4 rows
	lcd.setCursor(DISPLAY_STATUS_WIDTH, 0);
	lcd.print("....Toy Stacky....");
	lcd.setCursor(DISPLAY_STATUS_WIDTH, 1);
	lcd.print("   ....v1.0....   ");
	lcd.setCursor(DISPLAY_STATUS_WIDTH, 2);
	lcd.print("     ..TS-0..     ");
	lcd.setCursor(0, DISPLAY_LINECOUNT - 1);
	lcd.print("                      ");
	lcd.cursor();
	lcd.setCursor(DISPLAY_LINESIZE - 1, 3); //col, row
	Serial.begin(115200); //works at 9600 ONLY!!
}

// the loop function runs over and over again forever
void loop1() {
	if (rp2040.fifo.pop() == CORE0_TO_CORE1_START) {
		delay(20);
		//blocking; wait for notification from core 0 to start
		interpret(&vm, vm.userInputInterpret);
		rp2040.fifo.push(CORE1_TO_CORE0_DONE);
	}
}

void processImmdOpKeyC (const char* str) {
	if ((vm.userInputPos > 0) && (vm.userInputPos < STRING_SIZE)) {
		vm.userInput[vm.userInputPos++] = ' ';
		SerialPrint(1, "\n\r");
	}
	strcpy(&vm.userInput[vm.userInputPos], str); 
	SerialPrint(1, "\n\r");
	strcpy(vm.userInputInterpret, vm.userInput);
	clearUserInput();
	rp2040.fifo.push(CORE0_TO_CORE1_START);
}

#include "ToyStacky-normal-mode-keyhandler.h"
#include "ToyStacky-alt-mode-keyhandler.h"
#include "ToyStacky-page-1-normal-mode-keyhandler.h"
#include "ToyStacky-page-1-alt-mode-keyhandler.h"
#include "ToyStacky-page-2-normal-mode-keyhandler.h"
#include "ToyStacky-page-2-alt-mode-keyhandler.h"

void loop() {
	//char debug0[10];
	char keyc = customKeypad.getKey();

	if (keyc) {
		//debug0[0] = keyc; debug0[1] = '\0';
		//SerialPrint(3, "Key stroke -- got ", debug0, "\n\r");
		
		if (vm.viewPage == NORMAL_VIEW) {
			if (keyc == 'C' && (vm.altState == 0x0)) { //command page
				//processImmdOpKeyC("cmdpg");
				if (vm.cmdPage < (MAX_CMD_PAGES - 1)) vm.cmdPage++;
				else vm.cmdPage = 0;
				keyTypePressed = 5;
				showModes(&vm);
			} else if (keyc == 'A') { //alt function
				keyTypePressed = 6;
				if (vm.altState == 0) vm.altState = 1;
				else if (vm.altState == 1) vm.altState = 3;
				else vm.altState = 0; //none -> alt -> alt lock -> none
				showModes(&vm);
			} else {
				keyTypePressed = 0;
				if (keyc == '[') {
					if (vm.partialVector){ //doing vector
						keyc = ']';
						vm.partialVector = false;
					} else {
						vm.partialVector = true;
					}
				}
				else if (keyc == '{') {
					if (vm.partialMatrix){ //doing matrix
						keyc = '}';
						vm.partialMatrix = false;
					} else {
						vm.partialMatrix = true;
					}
				}
				else if (keyc == '(') {
					if (vm.partialComplex){ //doing complex
						keyc = ')';
						vm.partialComplex = false;
					} else {
						vm.partialComplex = true;
					}
				}
				switch ((vm.cmdPage << 0x1) + (vm.altState & 0x1)) {
					case 0: //page 0, normal
						keyTypePressed = normalModeKeyhandler(keyc);
						break;
					case 1: //page 0, alt
						keyTypePressed = altModeKeyhandler(keyc);
						break;
					case 2: //page 1, normal
						keyTypePressed = normalPage1ModeKeyhandler(keyc);
						break;
					case 3: //page 1, alt
						keyTypePressed = altPage1ModeKeyhandler(keyc);
						break;
					case 4: //page 2, normal
						keyTypePressed = normalPage2ModeKeyhandler(keyc);
						break;
					case 5: //page 2, alt
						keyTypePressed = altPage2ModeKeyhandler(keyc);
						break;
					default: //page 0, normal
						keyTypePressed = normalModeKeyhandler(keyc);
						break;
				}
				if (vm.altState == 1) vm.altState = 0;
				showModes(&vm);
			}
			if ((vm.userInput[0] != '\0') && (keyTypePressed == 0)) {
				//user is editing the bottom row
				showStackHP(&vm, 0, DISPLAY_LINECOUNT - 2);
				eraseUserEntryLine();
				showUserEntryLine(0);
			}
		} else if (vm.viewPage == STACK_VIEW) { //stack view mode
			switch (keyc) {
				case 'u': //up
					keyTypePressed = 3;
					updatesForUpMotion();
					break;
				case 'D': //down
					keyTypePressed = 4;
					updatesForDownMotion();
					break;
				case '\b':
					//going back to normal view clear variables 
					//used in stack view
					vm.viewPage = NORMAL_VIEW;
					vm.topEntryNum = DISPLAY_LINECOUNT - 1;
					vm.pointerRow = DISPLAY_LINECOUNT - 1;
					lcd.clear();
					showStackHP(&vm, 0, DISPLAY_LINECOUNT - 1);
					showModes(&vm);
					break;
				case '\n':
					int8_t meta;
					vm.viewPage = ENTRY_VIEW;
					lcd.clear();
					//display the entry
					meta = peekn(&vm.userStack, NULL, vm.topEntryNum - vm.pointerRow);
					if (meta != -1) {
						peekn(&vm.userStack, vm.matvecStrC, vm.topEntryNum - vm.pointerRow);
					} else {
						strcpy(vm.matvecStrC, "SOFTWARE BUG!!");
					}
					lcd.clear();
					//stack item display routine for vectors, matrices and complex doubles
					//only 19 columns - leave leftmost col for up/dn indication
					formatVariable(&vm, vm.matvecStrC, DISPLAY_LINESIZE - 1);
					showVariable(&vm);
					break;
				default: 
					break;
			}
		} else if (vm.viewPage == ENTRY_VIEW) { //single entry view mode
			switch (keyc) {
				case 'u': //up
					keyTypePressed = 3;
					if (vm.topVarFragNum > 0) vm.topVarFragNum--;
					showVariable(&vm);
					break;
				case 'D': //down
					keyTypePressed = 4;
					if (vm.topVarFragNum < (vm.varFragCount - DISPLAY_LINECOUNT)) vm.topVarFragNum++;
					showVariable(&vm);
					break;
				case '\b':
					//going back to stack view clear variables 
					//used in entry view
					vm.topVarFragNum = 0;
					vm.varFragCount = 0;
					vm.varLength = 0;
					//variables used in the stack view are preserved
					vm.viewPage = STACK_VIEW;
					lcd.clear();
					//restore previous stack view
					showStackHP(&vm, vm.topEntryNum - DISPLAY_LINECOUNT + 1, vm.topEntryNum);
					lcd.setCursor(1, vm.pointerRow); //col, row
					lcd.print(RIGHTARROW);
					break;
				case '\n':
					//going back to normal view clear variables 
					//used in entry view
					vm.topVarFragNum = 0;
					vm.varFragCount = 0;
					vm.varLength = 0;
					vm.topEntryNum = DISPLAY_LINECOUNT - 1;
					vm.pointerRow = DISPLAY_LINECOUNT - 1;
					vm.viewPage = NORMAL_VIEW;
					lcd.clear();
					//FIXME: can we copy the item into userInput, update userInputPos
					showStackHP(&vm, 0, DISPLAY_LINECOUNT - 1);
					showModes(&vm);
					break;
				default: 
					break;
			}
		}
	}
	if (rp2040.fifo.pop_nb(&core1msg)) { 
		//non-blocking; until core 1 says done
		//if (keyTypePressed == 5)
			//for displaying the command page number immediately
		//	showModes(&vm);
		showStackHP(&vm, 0, DISPLAY_LINECOUNT - 1);
	}
}
