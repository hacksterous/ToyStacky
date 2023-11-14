/*
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
const byte ROWS = 8;
const byte COLS = 4;
//define the cymbols on the buttons of the keypads
char numKeys[ROWS][COLS] = 
{

	//{'u','m','[','('},  //up mean [ (
	//{'d','q',']',')'},  //down sum ] )
	//{'S','V','P','?'},  //shift sdv pi ?
	//{'a','U','J','@'},   //alt ABS J @

	{'B','A','r','\b'}, //B A sqrt backspace
	{'D','s','c','t'},  //D sin cos tan
	{'E','l','n','T'},  //E log10 ln 1/T
	{'F',' ','<','>'},  //F space left right

	{'1','2','3','+'},
	{'4','5','6','-'},
	{'7','8','9','*'},
	{'0','.','/','\n'}

};
//LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
LiquidCrystal lcd(18, 19, 20, 21, 22, 26);
//byte rowPins[ROWS] = {15, 14, 13, 12, 11, 10, 9, 8, 3, 2, 1, 0}; //these are inputs
byte rowPins[ROWS] = {11, 10, 9, 8, 3, 2, 1, 0}; //these are inputs
byte colPins[COLS] = {7, 6, 5, 4}; //these are outputs
Keypad customKeypad = Keypad( makeKeymap(numKeys), rowPins, colPins, ROWS, COLS); 

void printRegisters(Machine* vm) {
}

void clearUserInput(){
	vm.userInputLeftOflow = 0;
	vm.userInputRightOflow = 0;
	vm.userInputCursorEntry = 1; //char DISPLAY_LINESIZE - 1 is empty and for data entry
	memset(vm.userInput, 0, STRING_SIZE);
	memset(vm.display, 0, SHORT_STRING_SIZE);
	memset(vm.userDisplay, 0, SHORT_STRING_SIZE);
}

void eraseUserEntryLine() {
	SerialPrint(2, "eraseUserEntryLine: eraseUserEntryLine called", "\r\n");
	lcd.setCursor(0, 3); //col, row
	for (int j = 0; j < DISPLAY_LINESIZE; j++)
		lcd.print(' ');
	vm.cursorPos = DISPLAY_LINESIZE - 1;
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
				lcd.write(LEFTOFIND);
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
			lcd.write(RIGHTOFIND);
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
				//show cursor entry position on col 19 and shift left
				if (len > DISPLAY_LINESIZE - 1) {
					lcd.setCursor(0, 3); //col, row
					lcd.write(LEFTOFIND);
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
		} else { //if (vm.cursorPos >= DISPLAY_LINESIZE - 2) {
			//cursor is just to the left of the right scroll off indicator
			if (vm.userInputPos < len - 3) {
				//right scroll is required
				vm.userInputPos++;
				SerialPrint(2, "updatesForRightMotion: 01 11 B case ", "\n\r");
				lcd.setCursor(0, 3);
				lcd.write(LEFTOFIND);
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
				lcd.write(LEFTOFIND);
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

    // Start the RTC
    rtc_init();
    rtc_set_datetime(&alarm);
	vm.TS0RTCFreq = clock_get_hz(clk_rtc);

    alarm.sec  = vm.alarmSec;
	rtc_set_alarm(&alarm, &toggleLED);
	vm.repeatingAlarm = true;

	byte rightoflowIndic[8] = {
	  B11000,
	  B11100,
	  B11110,
	  B11111,
	  B11110,
	  B11100,
	  B11000,
	};
	
	byte leftoflowIndic[8] = {
	  B00011,
	  B00111,
	  B01111,
	  B11111,
	  B01111,
	  B00111,
	  B00011,
	};

	byte shiftIndic[8] = {
	  B11100,
	  B10000,
	  B11100,
	  B00100,
	  B11100,
	  B00000,
	  B00000,
	};

	byte altIndic[8] = {
	  B11110,
	  B10010,
	  B11110,
	  B10010,
	  B10010,
	  B00000,
	  B00000,
	};

	byte matStartIndic[8] = {
	  B11111,
	  B10100,
	  B10100,
	  B10100,
	  B10100,
	  B10100,
	  B11111,
	};

	byte matEndIndic[8] = {
	  B11111,
	  B00101,
	  B00101,
	  B00101,
	  B00101,
	  B00101,
	  B11111,
	};

	byte shiftLockIndic[8] = {
	  B11100,
	  B10000,
	  B11100,
	  B00100,
	  B11100,
	  B00000,
	  B11110,
	};

	byte altLockIndic[8] = {
	  B11110,
	  B10010,
	  B11110,
	  B10010,
	  B10010,
	  B00000,
	  B11110,
	};

	// initialize digital pin LED_BUILTIN as an output.
	lcd.createChar(RIGHTOFIND, rightoflowIndic);
	lcd.createChar(LEFTOFIND, leftoflowIndic);
	lcd.createChar(SHIFTIND, shiftIndic);
	lcd.createChar(ALTIND, altIndic);
	lcd.createChar(MATSTARTIND, matStartIndic);
	lcd.createChar(MATENDIND, matEndIndic);
	lcd.createChar(SLOCKIND, shiftLockIndic);
	lcd.createChar((int) ALOCKIND, altLockIndic);
	lcd.begin(DISPLAY_LINESIZE, 4); //20 cols, 4 rows
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
	Serial.begin(115200);
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

void opLookup(char keyc) {
	if (keyc == 'p' && ((vm.altState == 1 && vm.shiftState == 0) || (vm.altState == 0 && vm.shiftState == 0))) {
		//pi and exp are 17 chars long
		if (vm.userInputPos >= STRING_SIZE - 17) {
			return;
		}
	} else {
		//assume worst case 7 for others
		if (vm.userInputPos >= STRING_SIZE - 7) {
			return;
		}
	}
	switch ((vm.shiftState << 1) + vm.altState) {
		case 0: //normal
			switch (keyc) {
				case 'r': strcpy(&vm.userInput[vm.userInputPos], "sqrt"); break;
				case 's': strcpy(&vm.userInput[vm.userInputPos], "sin"); break;
				case 'c': strcpy(&vm.userInput[vm.userInputPos], "cos"); break;
				case 't': strcpy(&vm.userInput[vm.userInputPos], "cbrt"); break; //temp
				case 'l': strcpy(&vm.userInput[vm.userInputPos], "log10"); break;
				case 'n': strcpy(&vm.userInput[vm.userInputPos], "log"); break;
				case 'T': strcpy(&vm.userInput[vm.userInputPos], "1 swp /"); break;
				case '+': strcpy(&vm.userInput[vm.userInputPos], "+"); break;
				case '-': strcpy(&vm.userInput[vm.userInputPos], "-"); break;
				case '*': strcpy(&vm.userInput[vm.userInputPos], "*"); break;
				case '/': strcpy(&vm.userInput[vm.userInputPos], "/"); break;
				case 'q': strcpy(&vm.userInput[vm.userInputPos], "sum"); break;
				case 'V': strcpy(&vm.userInput[vm.userInputPos], "sdv"); break;
				case 'U': strcpy(&vm.userInput[vm.userInputPos], "abs"); break;
				case 'm': strcpy(&vm.userInput[vm.userInputPos], "mean"); break;
				case 'P': strcpy(&vm.userInput[vm.userInputPos], __TS_PI_STR__); break;
				case '?': strcpy(&vm.userInput[vm.userInputPos], "?"); break;
				case '@': strcpy(&vm.userInput[vm.userInputPos], "@"); break;
				case ')': strcpy(&vm.userInput[vm.userInputPos], ")"); break;
			}
			break;
		case 1: //ALT
			switch (keyc) {
				case 'r': strcpy(&vm.userInput[vm.userInputPos], "cbrt"); break;
				case 's': strcpy(&vm.userInput[vm.userInputPos], "sin"); break;
				case 'c': strcpy(&vm.userInput[vm.userInputPos], "cos"); break;
				case 't': strcpy(&vm.userInput[vm.userInputPos], "tan"); break;
				case 'l': strcpy(&vm.userInput[vm.userInputPos], "log2"); break;
				case 'n': strcpy(&vm.userInput[vm.userInputPos], "2 swp pow"); break; // 2 ^ T
				case 'T': strcpy(&vm.userInput[vm.userInputPos], "2 pow"); break; // T ^ 2
				case '+': strcpy(&vm.userInput[vm.userInputPos], "+"); break;
				case '-': strcpy(&vm.userInput[vm.userInputPos], "~"); break; //unary neg
				case '*': strcpy(&vm.userInput[vm.userInputPos], "*"); break;
				case '/': strcpy(&vm.userInput[vm.userInputPos], "//"); break;
				case 'q': strcpy(&vm.userInput[vm.userInputPos], "sum"); break;
				case 'V': strcpy(&vm.userInput[vm.userInputPos], "sdv"); break;
				case 'U': strcpy(&vm.userInput[vm.userInputPos], "abs"); break;
				case 'm': strcpy(&vm.userInput[vm.userInputPos], "mean"); break;
				case 'P': strcpy(&vm.userInput[vm.userInputPos], __TS_E_STR__); break;
				case '?': strcpy(&vm.userInput[vm.userInputPos], "?"); break;
				case '@': strcpy(&vm.userInput[vm.userInputPos], "@"); break;
				case ')': strcpy(&vm.userInput[vm.userInputPos], ")"); break;
			}
			break;
		case 2: //SHIFT
			switch (keyc) {
				case 'r': strcpy(&vm.userInput[vm.userInputPos], "pow"); break;
				case 's': strcpy(&vm.userInput[vm.userInputPos], "sin"); break;
				case 'c': strcpy(&vm.userInput[vm.userInputPos], "cos"); break;
				case 't': strcpy(&vm.userInput[vm.userInputPos], "tan"); break;
				case 'l': strcpy(&vm.userInput[vm.userInputPos], "10 swp pow"); break;
				case 'n': strcpy(&vm.userInput[vm.userInputPos], "exp swp pow"); break;
				case 'T': strcpy(&vm.userInput[vm.userInputPos], "pow"); break; //X ^ T
				case '+': strcpy(&vm.userInput[vm.userInputPos], "+"); break;
				case '-': strcpy(&vm.userInput[vm.userInputPos], "-"); break;
				case '*': strcpy(&vm.userInput[vm.userInputPos], "*"); break;
				case '/': strcpy(&vm.userInput[vm.userInputPos], "/"); break;
				case 'q': strcpy(&vm.userInput[vm.userInputPos], "sum"); break;
				case 'V': strcpy(&vm.userInput[vm.userInputPos], "sdv"); break;
				case 'U': strcpy(&vm.userInput[vm.userInputPos], "abs"); break;
				case 'm': strcpy(&vm.userInput[vm.userInputPos], "mean"); break;
				case 'P': strcpy(&vm.userInput[vm.userInputPos], __TS_PI_STR__); break;
				case '?': strcpy(&vm.userInput[vm.userInputPos], "?"); break;
				case '@': strcpy(&vm.userInput[vm.userInputPos], "@"); break;
				case ')': strcpy(&vm.userInput[vm.userInputPos], ")"); break;
			}
			break;
		case 3: //ATL+SHIFT
			switch (keyc) {
				case 'r': strcpy(&vm.userInput[vm.userInputPos], "cbrt"); break;
				case 's': strcpy(&vm.userInput[vm.userInputPos], "sin"); break;
				case 'c': strcpy(&vm.userInput[vm.userInputPos], "cos"); break;
				case 't': strcpy(&vm.userInput[vm.userInputPos], "tan"); break;
				case 'l': strcpy(&vm.userInput[vm.userInputPos], "log10"); break;
				case 'n': strcpy(&vm.userInput[vm.userInputPos], "log"); break;
				case 'T': strcpy(&vm.userInput[vm.userInputPos], "2 pow"); break;
				case '+': strcpy(&vm.userInput[vm.userInputPos], "+"); break;
				case '-': strcpy(&vm.userInput[vm.userInputPos], "-"); break;
				case '*': strcpy(&vm.userInput[vm.userInputPos], "*"); break;
				case '/': strcpy(&vm.userInput[vm.userInputPos], "/"); break;
				case 'q': strcpy(&vm.userInput[vm.userInputPos], "sum"); break;
				case 'V': strcpy(&vm.userInput[vm.userInputPos], "sdv"); break;
				case 'U': strcpy(&vm.userInput[vm.userInputPos], "abs"); break;
				case 'm': strcpy(&vm.userInput[vm.userInputPos], "mean"); break;
				case 'P': strcpy(&vm.userInput[vm.userInputPos], __TS_PI_STR__); break;
				case '?': strcpy(&vm.userInput[vm.userInputPos], "?"); break;
				case '@': strcpy(&vm.userInput[vm.userInputPos], "@"); break;
				case ')': strcpy(&vm.userInput[vm.userInputPos], ")"); break;
			}
			break;
		}
	Serial.print(&vm.userInput[vm.userInputPos]);
}

void loop() {
	char debug0[10];
	char keyc = customKeypad.getKey();
	int bsp;
	int len;
	if (keyc) {
		bsp = 0;
		len = strlen(vm.userInput);
		if (vm.lastFnOp[0] != '\0') {
			vm.lastFnOp[0] = '\0';
			eraseUserEntryLine();
		}
		switch (keyc) {
			case '\n':
				if (vm.timerRunning) {
					vm.timerRunning = false;
					rtc_disable_alarm();
					vm.repeatingAlarm = false;
				}
				doubleToString(vm.TS0RTCFreq, debug0);
				SerialPrint(3, "setup:  -- vm.TS0RTCFreq = ", debug0, "\n\r");
				Serial.print(keyc);
				SerialPrint(1, "\r");
				if (vm.userInput[0] == '\0') break;
				if (vm.shiftState == 1 && vm.altState == 0)
					strcpy(vm.userInput, "tim"); //alarm/timer
				else vm.userInput[len + 1] = '\0';
				SerialPrint(3, "Got \n : vm.userInput = ", vm.userInput, "\n\r");
				strcpy(vm.userInputInterpret, vm.userInput);
				rp2040.fifo.push(CORE0_TO_CORE1_START);
				clearUserInput();
				vm.userInputPos = 0;
				break;
			case '?': //result in \n + key
			case '@': //result in \n + key
				vm.userInput[vm.userInputPos] = '\0';
				SerialPrint(1, "\n\r");
				Serial.print(keyc);
				strcpy(vm.userInputInterpret, vm.userInput);
				rp2040.fifo.push(CORE0_TO_CORE1_START);
				clearUserInput();
				vm.userInput[0] = keyc;
				vm.userInputPos = 1;
				break;
			case '\b':
				Serial.print(keyc);
				if (vm.userInputPos > 0) {
					if (vm.cursorPos == DISPLAY_LINESIZE - 1) {
						//cursor is at end of line
						vm.userInputPos--;
						vm.userInput[vm.userInputPos] = '\0';
					} else {
						//move the last part of the string 1 char forward
						strcpy(&vm.userInput[vm.userInputPos - 1], &vm.userInput[vm.userInputPos]);
						vm.userInputPos--;
						vm.userInput[len] = '\0';
					}
				}
				SerialPrint(3, "Got backspace: vm.userInput = ", vm.userInput, "\r\n");
				bsp = 1;
				break;
			//these are operators, result in ' ' + key + \n
			case 'r': 
			case 's': 
			case 'c':
			case 't':
			case 'l':
			case 'n':
			case 'T':
			case '*':
			case '/':
			case 'q':
			case 'V':
			case 'U':
			case 'm':
			case '+':
			case 'P': //pi
			case ')': //results in key + \n -- simplify to ' ' + key + \n
				if ((vm.userInputPos > 0) && (vm.userInputPos < STRING_SIZE)) {
					vm.userInput[vm.userInputPos++] = ' ';
					SerialPrint(1, "\n\r");
				}
				opLookup(keyc);
				strcpy(vm.lastFnOp, vm.userInput);
				SerialPrint(1, "\n\r");
				strcpy(vm.userInputInterpret, vm.userInput);
				rp2040.fifo.push(CORE0_TO_CORE1_START);
				clearUserInput();
				vm.userInputPos = 0;
				break;
			case '-': //can result in just a '-' (initial) or '-' + \n (non-initial) 
				if ((vm.userInputPos > 0) && (vm.userInputPos < STRING_SIZE)) {
					vm.userInput[vm.userInputPos++] = ' ';
					SerialPrint(1, "\n\r");
					opLookup(keyc);
					strcpy(vm.lastFnOp, vm.userInput);
					SerialPrint(1, "\n\r");
					strcpy(vm.userInputInterpret, vm.userInput);
					SerialPrint(3, "Got + or - non-initial: vm.userInput = ", vm.userInput, "\n\r");
					rp2040.fifo.push(CORE0_TO_CORE1_START);
					clearUserInput();
					vm.userInputPos = 0;
				} else {
					//first char or -number
					if (vm.userInputPos < STRING_SIZE) {
						opLookup(keyc);
						strcpy(vm.lastFnOp, vm.userInput);
						vm.userInputPos++;
						if (vm.cursorPos < DISPLAY_LINESIZE - 1)
							vm.cursorPos++;
						SerialPrint(3, "Got + or - initial: vm.userInput = ", vm.userInput, "\n\r");
						//SerialPrint(1, "\n\r");
					}
				}
				break;
			//these below need special handling
			case 'u': //up
			case 'd': //down
			case 'S': //shift
			case 'a': //alt function
				break;
			case '<': //left
				updatesForLeftMotion();
				break;
			case '>': //right
				updatesForRightMotion();
				break;
			default:
				Serial.print(keyc);
				if (vm.userInputPos < STRING_SIZE) {
					vm.userInput[vm.userInputPos++] = keyc;
					if (vm.cursorPos < DISPLAY_LINESIZE - 1)
						vm.cursorPos++;
				}
				break;
		}
		if (keyc == '\b') {
			showUserEntryLine(bsp);
		} else if ((vm.userInput[0] != '\0') && (keyc != '<') && (keyc != '>')) {
			eraseUserEntryLine();
			showUserEntryLine(0);
		}
	}
	if (rp2040.fifo.pop_nb(&core1msg)) { 
		//non-blocking; if until core 1 says done
		showStack(&vm);
	}
}
