/*
(C) Anirban Banerjee 2023
License: GNU GPL v3
*/
#ifndef __TS_CORE_H__
#define __TS_CORE_H__

#define DISPLAY_STATUS_WIDTH 2
#define DISPLAY_LINESIZE 20
#define DISPLAY_LINECOUNT 4
#define NUMBER_LINESIZE 18
#define MAX_VARNAME_LEN 33
#define MAX_VARIABLES 1000
#define MEMORY_SIZE 32000 //in bytes
#define CAPACITY MEMORY_SIZE
#define USER_SOURCECODEMEM_SIZE 32000 //in bytes
#define SOURCECODEMEM_SIZE 32000 //in bytes
#define STACK_SIZE 32000 //in bytes
#define STACK_NUM_ENTRIES 15000 //max stack entries
#define EXEC_STACK_SIZE 200 //in Uint
#define STRING_SIZE 101
#define SHORT_STRING_SIZE 33
#define VSHORT_STRING_SIZE 25
#define DOUBLE_EPS 2.2250738585072014e-308
#define DOUBLEFN_EPS 1.5e-16 //for return values of functions

#define RIGHTOFIND 1
#define LEFTOFIND 2
#define CMDIND 3
#define ALTIND 4
#define MATSTARTIND 5
#define MATENDIND 6
#define LOCKIND 7
#define UNUSEDIND 0

#define VECSTARTTOKENC '['
#define VECLASTTOKENC ']'
#define PRINTTOKENC '?'
#define POPTOKENC '@'
#define VECSTARTINDICATOR '\22'

#define VECSTARTTOKEN "["
#define MATSTARTTOKEN "[["
#define VECLASTTOKEN "]"
#define MATLASTTOKEN "]]"
#define IFTOKEN "if"
#define ELSETOKEN "el"
#define ENDIFTOKEN "fi"
#define ADDTOKEN "+"
#define SUBTOKEN "-"
#define MULTOKEN "*"
#define DIVTOKEN "/"
#define MODTOKEN "%"
#define GTTOKEN ">"
#define LTTOKEN "<"
#define GTETOKEN ">="
#define LTETOKEN "<="
#define EQTOKEN "="
#define NEQTOKEN "!="
#define PARTOKEN "//"
 
#define POSTFIX_BEHAVE 0

//used for main automatic stack
typedef struct {
	char stackStr[STACK_SIZE];
	size_t stackLen[STACK_NUM_ENTRIES]; //bits 31:24 are meta data - 
										//used to indicate start/end of vectors/matrices
	int topStr;
	int topLen;
} Strack;

//used for branch and condition stack
typedef struct {
	size_t stack[EXEC_STACK_SIZE];
	int top;
} UintStack;

//used for variables
typedef enum {
	VARIABLE_TYPE_COMPLEX,
	VARIABLE_TYPE_VECMAT,
	VARIABLE_TYPE_STRING
} VariableType;

typedef enum {
	METANONE,
	METASTARTVECTOR,
	METAMIDVECTOR,
	METAENDVECTOR,
	METASTARTMATRIX,
	METAMIDMATRIX,
	METAENDMATRIX
} StrackMeta;

typedef struct {
	double real;
	double imag;
} ComplexDouble;

ComplexDouble makeComplex(double re, double im);
#define MKCPLX(a,...) makeComplex(a, (0, ##__VA_ARGS__))

typedef struct {
	char name[MAX_VARNAME_LEN];
	VariableType type;
	union {
		ComplexDouble doubleValue;
		size_t stringValueIndex;
	} value;
} Variable;

//for garbage collector
typedef struct {
	Variable variables[MAX_VARIABLES];
	char memory[MEMORY_SIZE];
	size_t memoryOffset;
	size_t varCount;
} Ledger;

#ifndef __BIGINT_STRUCT__
#define __BIGINT_STRUCT__
typedef struct {
        int32_t length: 31;
        uint32_t negative: 1;
        uint32_t data[129];
} bigint_t;
#endif

typedef struct {
	char USERSOURCECODEMEM[USER_SOURCECODEMEM_SIZE];
	char SOURCECODEMEM[SOURCECODEMEM_SIZE];
	Ledger ledger;
	Strack userStack; //the user stack
	UintStack execStack; //the execution stack to keep track of conditionals, loops and calls
	char bak[STRING_SIZE];//backup register
	char acc[STRING_SIZE];//the accumulator
	char error[SHORT_STRING_SIZE];//error notification
	char userDisplay[SHORT_STRING_SIZE];
	char display[SHORT_STRING_SIZE];
	char coadiutor[STRING_SIZE]; //coadiutor = helper
	char userInput[STRING_SIZE];
	char userInputInterpret[STRING_SIZE];
	char lastFnOp[STRING_SIZE];
	int userInputPos; //for userInput buffer
	int cmdState;
	int altState;
	//view page - 
	//0: normal, 1: variable view, 
	//2: stack view, 3: vector view, 4: matrix view
	//5: internal status view
	int viewPage;
	int modeDegrees;
	int cursorPos;
	int userInputLeftOflow;
	int userInputRightOflow;
	int userInputCursorEntry;
	int insertState; //0: overwrite, 1: insert
	bool timerRunning;
	bool repeatingAlarm;
	bool LEDState;
	int alarmHour;
	int alarmMin;
	int alarmSec;
	unsigned int TS0RTCFreq;
	bigint_t bigA;
	bigint_t bigB;
	bigint_t bigC;
} Machine;

extern Machine vm;

extern unsigned int TS0RTCFreq;
//extern int alarmHour;
//extern int alarmMin;
//extern int alarmSec;

extern LiquidCrystal lcd;

void SerialPrint(const int count, ...);
void interpret(Machine* vm, char* sourceCode);
void initMachine(Machine* vm);
bool doubleToString(double value, char* buf);
void showUserEntryLine(int bsp);
void showStackEntries(Machine* vm, int linecount);
void showStackHP(Machine* vm, int linecount);
void showStack(Machine* vm);
void eraseUserEntryLine();
void toggleLED();
#endif
