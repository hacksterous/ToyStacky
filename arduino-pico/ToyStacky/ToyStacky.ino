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
//byte rowPins[ROWS] = {15, 14, 13, 12, 3, 2, 1, 0}; //these are inputs
//byte rowPins[ROWS] = {11, 10, 9, 8, 3, 2, 1, 0}; //these are inputs
byte colPins[COLS] = {7, 6, 5, 4}; //these are outputs
Keypad customKeypad = Keypad( makeKeymap(numKeys), rowPins, colPins, ROWS, COLS); 

// 0=user continues to edit; 1=evaluate immediately; 2=backspace; ... others ...
int keyTypePressed = -1; 
	
void printRegisters(Machine* vm) {
}

void clearUserInput(){
	vm.userInputLeftOflow = 0;
	vm.userInputRightOflow = 0;
	memset(vm.userInput, 0, STRING_SIZE);
	memset(vm.userDisplay, 0, SHORT_STRING_SIZE);
	vm.userInputPos = 0;
	vm.cursorPos = DISPLAY_LINESIZE - 1;
}

void eraseUserEntryLine() {
	//SerialPrint(2, "eraseUserEntryLine: eraseUserEntryLine called", "\r\n");
	lcd.setCursor(0, 3); //col, row
	lcd.print("                    ");
	lcd.setCursor(vm.cursorPos, 3); //col, row
}

void updatesForLeftMotion() {
	char debug0[VSHORT_STRING_SIZE];
	char debug1[VSHORT_STRING_SIZE];
	int len = strlen(vm.userInput);

	memset(vm.userDisplay, 0, STRING_SIZE);
	//doubleToString(vm.cursorPos, debug0);
	//SerialPrint(3, "updatesForLeftMotion: 0 -- vm.cursorPos = ", debug0, "\n\r");
	//doubleToString(vm.userInputPos, debug0);
	//SerialPrint(3, "updatesForLeftMotion: 0 -- vm.userInputPos = ", debug0, "\n\r");
	//doubleToString(len, debug0);
	//SerialPrint(3, "updatesForLeftMotion: 0 -- len = ", debug0, "\n\r");
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
				strncpy(vm.userDisplay, vm.userInput, DISPLAY_LINESIZE);
				lcd.print(vm.userDisplay);
				//no decrement of vm.cursorPos
				vm.userInputLeftOflow = 0;
				SerialPrint(2, "updatesForLeftMotion: 10 11 B case ", "\n\r");
			} else if (vm.userInputPos > 2) {
				vm.userInputPos--;
				lcd.setCursor(0, 3); //col, row
				//no decrement of vm.cursorPos
				lcd.print(LEFTOFIND);
				if ((len - vm.userInputPos) == (DISPLAY_LINESIZE - 1)) {
					//no right overflow
					strncpy(vm.userDisplay, &vm.userInput[vm.userInputPos], DISPLAY_LINESIZE - 1);
					lcd.print(vm.userDisplay);
					SerialPrint(2, "updatesForLeftMotion: 10 11 C case ", "\n\r");
				} else if ((len - vm.userInputPos) > (DISPLAY_LINESIZE - 1)) {
					strncpy(vm.userDisplay, &vm.userInput[vm.userInputPos], DISPLAY_LINESIZE - 2);
					lcd.print(vm.userDisplay);
					vm.userInputRightOflow = 1;
					SerialPrint(2, "updatesForLeftMotion: 10 11 D case ", "\n\r");
				} else {
					SerialPrint(2, ">>>>>>> updatesForLeftMotion: 10 11 E case ERROR: can't have this", "\n\r");
				}
			}
		}
		if (vm.userInputRightOflow) {
			lcd.setCursor(DISPLAY_LINESIZE - 1, 3); //col, row
			lcd.print(RIGHTOFIND);
		}
	}
	lcd.setCursor(vm.cursorPos, 3); //col, row
	//doubleToString(vm.cursorPos, debug0);
	//SerialPrint(3, "updatesForLeftMotion: 1 -- vm.cursorPos = ", debug0, "\n\r");
	//doubleToString(vm.userInputPos, debug0);
	//SerialPrint(3, "updatesForLeftMotion: 1 -- vm.userInputPos = ", debug0, "\n\r");
}

void updatesForRightMotion(){
	SerialPrint(2, "-------------------------------------------", "\n\r");
	char debug0[VSHORT_STRING_SIZE];
	char debug1[VSHORT_STRING_SIZE];
	int len = strlen(vm.userInput);
	memset(vm.userDisplay, 0, STRING_SIZE);

	//doubleToString(vm.cursorPos, debug0);
	//SerialPrint(3, "updatesForRightMotion: 0 -- vm.cursorPos = ", debug0, "\n\r");
	//doubleToString(vm.userInputPos, debug0);
	//SerialPrint(3, "updatesForRightMotion: 0 -- vm.userInputPos = ", debug0, "\n\r");
	//doubleToString(len, debug0);
	//SerialPrint(3, "updatesForRightMotion: 0 -- len = ", debug0, "\n\r");
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
					//lcd.write(LEFTOFIND);
					lcd.print(LEFTOFIND);
					vm.userInputLeftOflow = 1;
					strncpy(vm.userDisplay, &vm.userInput[len - DISPLAY_LINESIZE + 2], DISPLAY_LINESIZE - 2);
					SerialPrint(2, "updatesForRightMotion: 00 10 A case ", "\n\r");
					lcd.print(vm.userDisplay);
					lcd.print(' ');
				} else {
					SerialPrint(2, "updatesForRightMotion: 00 10 B case ", "\n\r");
					lcd.setCursor(DISPLAY_LINESIZE - len, 3); //col, row
					strncpy(vm.userDisplay, &vm.userInput[len - DISPLAY_LINESIZE + 1], len);
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
				//lcd.write(LEFTOFIND);
				lcd.print(LEFTOFIND);
				vm.userInputLeftOflow = 1;
				strncpy(vm.userDisplay, &vm.userInput[vm.userInputPos - DISPLAY_LINESIZE + 3], DISPLAY_LINESIZE - 2);
				lcd.print(vm.userDisplay);
			} else if (vm.userInputPos >= len - 3) {
				//right scroll is not required
				vm.userInputPos++;
				if (vm.userInputPos == len)
					vm.cursorPos++;
				//there is one char scrolled off to the right after the indicator
				//recalculate whether a left scroll indicator is required
				lcd.setCursor(0, 3); //col, row
				//lcd.write(LEFTOFIND);
				lcd.print(LEFTOFIND);
				vm.userInputLeftOflow = 1;
				SerialPrint(2, "updatesForRightMotion: 01 11 C case ", "\n\r");
				strncpy(vm.userDisplay, &vm.userInput[vm.userInputPos - DISPLAY_LINESIZE + 3], DISPLAY_LINESIZE - 1);
				lcd.print(vm.userDisplay);
				lcd.print(' ');
				//no increment of vm.cursorPos
				vm.userInputRightOflow = 0;
			}
		}
	}
	lcd.setCursor(vm.cursorPos, 3); //col, row
	//doubleToString(vm.cursorPos, debug0);
	//SerialPrint(3, "updatesForRightMotion: 1 -- vm.cursorPos = ", debug0, "\n\r");
	//doubleToString(vm.userInputPos, debug0);
	//SerialPrint(3, "updatesForRightMotion: 1 -- vm.userInputPos = ", debug0, "\n\r");
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

	vm.cmdState = 0;
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
	  B11011,
	  B11111,
	  B00100,
	  B00100,
	  B00100,
	};
	
	byte downIndicator[8] = {
	  B00100,
	  B00100,
	  B00100,
	  B11111,
	  B11011,
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

	byte matStartIndicator[8] = {
	  B11111,
	  B10100,
	  B10100,
	  B10100,
	  B10100,
	  B10100,
	  B11111,
	};

	byte matEndIndicator[8] = {
	  B11111,
	  B00101,
	  B00101,
	  B00101,
	  B00101,
	  B00101,
	  B11111,
	};

	byte twoIndicator[8] = {
	  B00000,
	  B11110,
	  B00010,
	  B11110,
	  B10000,
	  B10000,
	  B11110,
	};

	// initialize digital pin LED_BUILTIN as an output.
	lcd.createChar(UPIND, upIndicator);
	lcd.createChar(DOWNIND, downIndicator);
	lcd.createChar(ALTIND, altIndicator);
	lcd.createChar(ALTLOCKIND, altLockIndicator);
	lcd.createChar(MATSTARTIND, matStartIndicator);
	lcd.createChar(MATENDIND, matEndIndicator);
	lcd.createChar(TWOIND, twoIndicator);
	lcd.begin(DISPLAY_LINESIZE, DISPLAY_LINECOUNT); //20 cols, 4 rows
	lcd.setCursor(DISPLAY_STATUS_WIDTH, 0);
	lcd.print("....Toy Stacky....");
	lcd.setCursor(DISPLAY_STATUS_WIDTH, 1);
	lcd.print("   ....v1.0....   ");
	lcd.setCursor(DISPLAY_STATUS_WIDTH, 2);
	lcd.print("     ..TS-0..     ");
	lcd.setCursor(0, 3);
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
void loop() {
	char debug0[10];
	char keyc = customKeypad.getKey();

	if (keyc) {
		debug0[0] = keyc; debug0[1] = '\0';
		SerialPrint(3, "Key stroke -- got ", debug0, "\n\r");
		
		if (vm.viewPage == NORMAL_VIEW) {
			if (keyc == 'C' && (vm.altState == 0x0)) { //command page
				processImmdOpKeyC("cmdpg");
				keyTypePressed = 5;
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

				switch ((vm.cmdState << 0x1) + (vm.altState & 0x1)) {
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
					default: //page 0, normal
						keyTypePressed = normalModeKeyhandler(keyc);
						break;
				}
				if (vm.altState == 1) vm.altState = 0;
				showModes(&vm);
			}
			if ((vm.userInput[0] != '\0') && (keyTypePressed == 0)) {
				//user is editing the bottom row
				//showStack(&vm);
				showStackHP(&vm, 0, DISPLAY_LINECOUNT - 1);
				eraseUserEntryLine();
				showUserEntryLine(0);
			}
		} else if (vm.viewPage == STACK_VIEW) { //stack view mode
			showStackHP(&vm, 0, DISPLAY_LINECOUNT);
		}
	}
	if (rp2040.fifo.pop_nb(&core1msg)) { 
		//non-blocking; until core 1 says done
		if (keyTypePressed == 5)
			//for displaying the command page number immediately
			showModes(&vm);
		showStackHP(&vm, 0, DISPLAY_LINECOUNT);
	}
}
