/*
(C) Anirban Banerjee 2023
License: GNU GPL v3
*/
#ifndef __TS_CORE_H__
#define __TS_CORE_H__
#include <errno.h>

#define MAX_MATVECSTR_LEN 4900 //enough for one 12x12 matrix of long double complex
#define MAX_MATVECELEMENTS 150
#define MAX_MATLEN 12
#define MAX_CMD_PAGENUM 4
#define MAX_TOKEN_LEN 129
#define PICO_DOUBLE_PROPAGATE_NANS 1
#define DISPLAY_STATUS_WIDTH 2
#define DISPLAY_LINESIZE 20
#define DISPLAY_LINECOUNT 4
#define NUMBER_LINESIZE 18
#define NUMBER_LINESIZE_WO_PARENS 16
#define COMPLEX_NUM_DISP_SIZE 7
#define MAX_VARNAME_LEN 33
#define MAX_VARIABLES 1000
#define MEMORY_SIZE 32000 //in bytes
#define CAPACITY MEMORY_SIZE
#define SOURCECODEMEM_SIZE 32000 //in bytes
#define STACK_SIZE 32000 //in bytes
#define STACK_NUM_ENTRIES 11500 //max stack entries
#define EXEC_STACK_SIZE 200 //in Uint
#define STRING_SIZE 101
#define SHORT_STRING_SIZE 51 //%.15g gives 24 * 2 + 3
#define VSHORT_STRING_SIZE 25

//#define DOUBLE_EPS __LDBL_MIN__
#define DOUBLE_EPS 1e-15
//#define DOUBLEFN_EPS __LDBL_MIN__
#define DOUBLEFN_EPS 1e-15 //for return values of functions
//#define DOUBLE_EPS __LDBL_EPSILON__
//#define DOUBLEFN_EPS __LDBL_EPSILON__
#define NRPOLYSOLV_EPS 1e-6

//For LCD with Japanese char set: HD44780U A00
#define UPIND 1
#define DOWNIND 2
#define ALTIND 3
#define ALTLOCKIND 4
//#define TWOIND 5
#define VARIND 5
#define POLARIND 6
#define DEGPOLARIND 7
#define DEGREEIND char(0xdf)
#define RIGHTARROW char(0x7e)
#define LEFTARROW  char(0x7f)
#define OFLOWIND char(0xeb) 
#define STACKIND char(0xd0)
//#define VARIND char(0xda)
#define VARLISTIND char(0xed)
#define SIGMAIND char(0xe5)
#define ALPHAIND char(0xe0)
#define BARRIERIND char(0xa2)

#define CMDPG_0 char(0xdb)
#define CMDPG_1 char(0xd5)
#define CMDPG_2_ALPHA char(0xe0) //alpha
#define CMDPG_3_PROG char(0xf7) //pi
#define CMDPG_3 char(0xd6)
#define CMDPG_4 char(0xd1)
#define CMDPG_4_MU char(0xe4) //mu

#define COMSTARTTOKENC '('
#define VECSTARTTOKENC '['
#define VECLASTTOKENC ']'
#define MATSTARTTOKENC '{'
#define MATLASTTOKENC '}'
#define PRINTTOKENC '?'
#define POPTOKENC '@'

#define MATSTARTTOKEN "{"
#define MATENDTOKEN "}"
#define VECSTARTTOKEN "["
#define VECLASTTOKEN "]"
#define IFTOKEN "if"
#define ELSETOKEN "el"
#define ENDIFTOKEN "fi"
#define JMPTOKEN "jmp"
#define JPZTOKEN "jpz"
#define JNZTOKEN "jnz"
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
#define DPOPTOKEN "@@"
 
//used for main automatic stack
typedef struct {
	char stackStr[STACK_SIZE];
	int32_t stackLen[STACK_NUM_ENTRIES]; //bits 31:28 are meta data - 
										//used to indicate start/end of vectors/matrices
	int topStr;
	int topLen;
	int itemCount;
} Strack;

//used for branch and condition stack
typedef struct {
	int32_t stack[EXEC_STACK_SIZE];
	int top;
} UintStack;

//view page - 
//view page - 
//0: normal, 
//1: stack view, 
//2: variable view, 
//3: variable list view, 
//4: internal status view
//5: code editor view
typedef enum {
	NORMAL_VIEW,
	STACK_VIEW,
	ENTRY_VIEW,
	VARLIST_VIEW,
	STATUS_VIEW,
	QUICK_VIEW,
	EDIT_VIEW
} ViewPageType;

//used for variables
typedef enum {
	VARIABLE_TYPE_COMPLEX,
	VARIABLE_TYPE_VECMAT,
	VARIABLE_TYPE_STRING
} VariableType;

typedef enum {
	METASCALAR,
	METAVECTOR,
	METAMATRIX,
	METAVECTORPARTIAL,
	METAVECTORMATRIXPARTIAL,
	METAMATRIXPARTIAL
} StrackMeta;

#define METABARRIER 0x8

typedef struct {
	long double real;
	long double imag;
} ComplexDouble;

ComplexDouble makeComplex(long double re, long double im);
#define MKCPLX(a,...) makeComplex(a, (0, ##__VA_ARGS__))

typedef struct Matrix{
	int rows;
	int columns;
	ComplexDouble numbers[MAX_MATLEN][MAX_MATLEN];
} Matrix;

typedef struct Matrixd{
	int rows;
	int columns;
	long double numbers[MAX_MATLEN][MAX_MATLEN];
} Matrixd;

typedef struct {
	char name[MAX_VARNAME_LEN];
	VariableType type;
	union {
		ComplexDouble doubleValue;
		int32_t stringValueIndex;
	} value;
} Variable;

//for garbage collector
typedef struct {
	Variable variables[MAX_VARIABLES];
	char memory[MEMORY_SIZE];
	int32_t memoryOffset;
	int32_t varCount;
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
	/* internal structures */
	char SOURCECODEMEM[SOURCECODEMEM_SIZE];
	Ledger ledger;
	Strack userStack; //the user stack
	UintStack execStack; //the execution stack to keep track of conditionals, loops and calls

	/* internal variables */
	char bak[STRING_SIZE];//backup register
	char acc[STRING_SIZE];//the accumulator
	char coadiutor[STRING_SIZE]; //coadiutor = helper
	char dummy[STRING_SIZE];
	char error[SHORT_STRING_SIZE];//error notification
	bigint_t bigA;
	bigint_t bigB;
	bigint_t bigC;
	char matvecStrA[MAX_MATVECSTR_LEN]; 
	char matvecStrB[MAX_MATVECSTR_LEN];
	char matvecStrC[MAX_MATVECSTR_LEN];
	char lastY[MAX_MATVECSTR_LEN];
	char lastX[MAX_MATVECSTR_LEN];
	int8_t lastXMeta;
	int8_t lastYMeta;

	Matrix matrixA;
	Matrix matrixB;
	Matrix matrixC;

	bigint_t bigMod;
	uint8_t width;

	/* for stack inspection control */
	//view page - 
	//0: normal, 
	//1: stack view, 
	//2: variable view, 
	//3: variable list view, 
	//4: internal status view
	//5: code editor view
	int viewPage;
	int topEntryNum; //stack entry number shown at top row
	int pointerRow; //entry pointed to by right pointing arrow

	int quickViewPage;

	/* for variable viewer */
	//for displaying on screen, a variable/stack item is split
	//into fragments
	int32_t varFragsArray[MAX_MATVECELEMENTS];
	int varFragCount; //total fragment count
	int topVarFragNum; //fragment number at top
	int varLength; //variable length

	/* for operating mode */
	int cmdPage;
	int altState;

	/* for math operations */
	bool modeDegrees;
	bool modePolar;
	long double frequency;

	/* for user entry and display management */
	int cursorPos;
	bool userInputLeftOflow;
	bool userInputRightOflow;
	bool partialVector;
	bool partialMatrix;
	bool partialComplex;
	char userInput[STRING_SIZE];
	char userInputInterpret[STRING_SIZE];
	int userInputPos; //for userInput buffer
	char userDisplay[SHORT_STRING_SIZE];

	/* for timer and astronomy */
	bool timerRunning;
	bool repeatingAlarm;
	bool LEDState;
	float locationLat;
	float locationLong;
	float locationTimeZone;
	uint8_t alarmHour;
	uint8_t alarmMin;
	uint8_t alarmSec;

	uint8_t precision;
	char notationStr[3];
	int month;
	int year;
	char* disp;

} Machine;

extern Machine vm;

extern LiquidCrystal lcd;

bool writeBigintToFile(const char* filename, bigint_t* ptr);
bool writeOneVariableToFile(const char* filename, float* ptr);
bool hasDblQuotes(char* input);
char* removeDblQuotes(char* input);
bool fnOrOp2Param(Machine* vm, const char* token, int fnindex);
void formatVariable(Machine* vm, char* str, int width);
void showVariable(Machine* vm, char* showStr);
char* skip_whitespace(char* input);
void zstrncpy (char*dest, const char* src, int len);
char* splitIntoLines(char* str, int width, char* holdbuffer);
void clearDisplay (Machine* vm);
void cleanUpModes (Machine* vm);
int8_t peekn(Strack* s, char output[STRING_SIZE], int n);
void interpret(Machine* vm, char* sourceCode);
void initMachine(Machine* vm);
bool doubleToString(long double value, char* buf, uint8_t precision, char* notationStr);
void showUserEntryLine(int bsp);
void showStackEntries(Machine* vm, int linecount);
void showCmdPageDegAlt(Machine* vm);
void showModes(Machine* vm);
void showStackHP(Machine* vm, int linestart, int linecount);
void initStacks(Machine* vm);
void eraseUserEntryLine();
void toggleLED();

#define FAILANDRETURN(failcondition,dest,src,fnptr)	\
	if (failcondition) {							\
		sprintf(dest, src);							\
		if (fnptr != NULL) fnptr();					\
		return false;								\
	}

#define FAILANDRETURNVAR(failcondition,dest,src,var)\
	if (failcondition) {							\
		sprintf(dest, src, var);					\
		return false;								\
	}

#endif
