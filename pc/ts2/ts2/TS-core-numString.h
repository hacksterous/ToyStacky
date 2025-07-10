#ifndef __TS_CORE_NUMSTRING__
#define __TS_CORE_NUMSTRING__
#include <stdint.h>
#include <stdbool.h>
#include "TS-core-miscel.h"

void zstrncpy (char*dest, const char* src, int len);
char* fitstr(char*dest, const char* src, size_t len);
bool isRealNumber(const char *token);
bool isComplexNumber(const char *input);
int parseComplex(const char* input, char* substring1, char* substring2);
char strIsRLC(char* str);
bool stringToDouble(char* str, long double* dbl);
bool stringToComplex(char *input, ComplexDouble* c);
void reverse(char s[]);
#ifdef _UNIX
void itoa(int n, char *s);
#endif
char* lcase(char* token);
bool doubleToString(long double value, char* buf, uint8_t precision, char* notationStr);
bool complexToString(ComplexDouble c, char* value, uint8_t precision, char* notationStr);
bool hasDblQuotes(char* input);
char* removeDblQuotes(char* input);
char* removeDblQuotesDONTUSE(char* input);
bool addDblQuotes(char *input);
bool varNameIsLegal(char* var);
char* ltrim(char* str);


#endif

