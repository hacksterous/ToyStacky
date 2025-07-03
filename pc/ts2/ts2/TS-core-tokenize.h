#ifndef __TS_CORE_TOKENIZE__
#define __TS_CORE_TOKENIZE__
char* tokenize(char* input, char* output);
char* extract_realnum(char* input, char* output);
char* extract_complexnum(char* input, char* output);
char* skip_whitespace(char* input);


#endif
