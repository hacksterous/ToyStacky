#ifndef __TS2_H__
#define __TS2_H__
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "TS-core-miscel.h"

//////////////////////////////////////////////////////////////
// All global variables/storage are declared here.
// This is included only from ts2.c
//////////////////////////////////////////////////////////////

#define __DEBUG_TOYSTACKY__
#ifdef __DEBUG_TOYSTACKY__
void SerialPrint(const int count, ...) {
	char *str;
    va_list args;
	va_start(args, count);
	for (int i = 0; i < count; i++) {
		str = va_arg(args, char*);
		printf("%s\n", str);
	}
    va_end(args);
}
#else
void SerialPrint(const int count, ...) {}
#endif

const char* DEBUGMETA[6] = {
	"METASCALAR",
	"METAVECTOR",
	"METAMATRIX",
	"METAVECTORPARTIAL",
	"METAVECTORMATRIXPARTIAL",
	"METAMATRIXPARTIAL"};

ComplexFunctionPtrVector mathfnvec1param[] = {sum, sqsum, var, sdv, mean, rsum};
const char* mathfnop2paramname[] = {
						"logxy",
						"atan2", "pow",
						"max", "min",
						"+", "-",
						"*", "/",
						"%", ">",
						"<", ">=",
						"<=", "=",
						"!=", "//",
						"rem"
						}; //the 2 params functions
const int POWFNINDEX = 2; 
const int NUMMATH2PARAMFNOP = 18; 
const int NUMVOID2PARAMBIGINTFNMIN = 2; //bigint fns start from pow
const int NUMVOID2PARAMBIGINTFNMAX = 9;

const int ATAN2FNINDEX = 1;

ComplexFunctionPtr2Param mathfn2param[] = {clogx, carctangent2, cpower, cmax, cmin, cadd, csub,
									cmul, cdiv, cmod, cgt, clt, cgte, clte, ceq, cneq, cpar, crem};

const char* matrixfnname[] = {
						"det", "inv",
						"idn", "trc",
						"eival", "eivec",
						"tpose"
						};
const int NUMMATRIXPARAMFN = 7;
const char* mathfn1paramname[] = {
						"sin", "cos", "tan", "cot", 
						"rect",
						"asin", "acos", "atan", "acot", 
						"polar", 
						"sinh", "cosh", "tanh", "coth", 
						"asinh", "acosh", "atanh", "acoth", 
						"exp", "log10", "log", "log2", "sqrt", "cbrt", "conj",
						"torad", "todeg", "recip", "neg",						
						"abs", "arg", "re", "im"}; //the 1 param functions
const int EXPFNINDEX = 18;
const int NUMMATH1PARAMFN = 29;
const int NUMREAL1PARAMFN = 4;
const int NUMMATH1PARAMTRIGFN = 5;
const int NUMMATH1PARAMTRIGANTIFN = 10;

//these 23 return ComplexDouble
ComplexFunctionPtr1Param mathfn1param[] = {	csine,
											ccosine,
											ctangent,
											ccotangent,
											rect,

											carcsine,
											carccosine,
											carctangent,
											carccotangent,
											polar,

											csinehyp,
											ccosinehyp,
											ctangenthyp,
											ccotangenthyp,

											carcsinehyp, 
											carccosinehyp, 
											carctangenthyp,
											carccotangenthyp,

											cexpo, clogarithm10, 
											cln, clog2, 
											csqroot, cbroot, 
											conjugate,

											crad,
											cdeg,
											crecip,
											cneg};

//the 1 param functions that have real result
RealFunctionPtr realfn1param[] = {abso, cargu, crealpart, cimagpart};

BigIntVoid1ParamFunctionPtr bigfnvoid1param[] = {bigint_hex, bigint_dec, bigint_bin, bigint_oct, bigint_neg};
BigIntVoid2ParamFunctionPtr bigfnvoid2param[] = {bigint_pow, bigint_max, bigint_min, bigint_add, bigint_sub, bigint_mul, bigint_div, bigint_rem};

BigIntIntFunctionPtr bigfnint2param[] = {bigint_gt, bigint_lt, bigint_gte, bigint_lte, bigint_eq, bigint_neq};

const char* bigfnvoid1paramname[] = {"hex", "dec", "bin", "oct", "neg"};
const int NUMBIGINT1FNS = 5;
const char* vecfn1paramname[] = {"sum", "sqsum", "var", "sd", "mean", "rsum"};
const char* vecfn2paramname[] = {"dot"};
const int NUMVECFNS = 6;
const int NUMVEC2FNS = 1;

const long double __TS_PI__ = 3.14159265358979323846L;
#endif
