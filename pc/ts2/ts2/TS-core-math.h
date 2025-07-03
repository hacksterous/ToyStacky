#ifndef __TS_CORE_MATH__
#define __TS_CORE_MATH__
#include <math.h>
#include <errno.h>
#include "TS-core-miscel.h"

ComplexDouble makeComplex(double re, long double im);
/*
ComplexDouble makeComplex(double re, long double im) {
	ComplexDouble ret;
	ret.real = re;
	ret.imag = im;
	return ret;
}
*/
extern const long double __TS_PI__;
#define MKCPLX(a,...) makeComplex(a, (0, ##__VA_ARGS__))

long double sgn(long double value);
long double abso (ComplexDouble value);
bool alm0(ComplexDouble value);
bool alm0double(long double value);
ComplexDouble cneg(ComplexDouble value);
ComplexDouble cabso (ComplexDouble value);
long double cargu (ComplexDouble value);
long double crealpart (ComplexDouble value);
long double cimagpart (ComplexDouble value);
ComplexDouble conjugate(ComplexDouble c);
ComplexDouble polar(ComplexDouble c);
ComplexDouble rect(ComplexDouble c);
ComplexDouble cadd(ComplexDouble value, ComplexDouble second);
ComplexDouble caddd(ComplexDouble value, long double second);
ComplexDouble csub(ComplexDouble value, ComplexDouble second);
ComplexDouble csubd(ComplexDouble value, long double second);
ComplexDouble cmul(ComplexDouble value, ComplexDouble second);
ComplexDouble cdiv(ComplexDouble value, ComplexDouble second);
ComplexDouble crem(ComplexDouble value, ComplexDouble second);
ComplexDouble cdivd(ComplexDouble value, long double second);
ComplexDouble cmod(ComplexDouble value, ComplexDouble second);
ComplexDouble ceq(ComplexDouble value, ComplexDouble second);
ComplexDouble cneq(ComplexDouble value, ComplexDouble second);
bool bceq(ComplexDouble value, ComplexDouble second);
bool bcneq(ComplexDouble value, ComplexDouble second);
ComplexDouble cpar(ComplexDouble value, ComplexDouble second);
ComplexDouble clt(ComplexDouble value, ComplexDouble second);
ComplexDouble cgt(ComplexDouble value, ComplexDouble second);
ComplexDouble cgte(ComplexDouble value, ComplexDouble second);
ComplexDouble clte(ComplexDouble value, ComplexDouble second);
ComplexDouble cln(ComplexDouble c);
ComplexDouble cexpo(ComplexDouble c);
ComplexDouble cpower(ComplexDouble c, ComplexDouble d);
ComplexDouble cpowerd(ComplexDouble c, long double d);
ComplexDouble csqroot(ComplexDouble c);
ComplexDouble cbroot(ComplexDouble c);
ComplexDouble csinehyp(ComplexDouble c);
ComplexDouble ccosinehyp(ComplexDouble c);
ComplexDouble ctangenthyp(ComplexDouble c);
ComplexDouble ccotangenthyp(ComplexDouble c);
ComplexDouble csine(ComplexDouble c);
ComplexDouble ccosine(ComplexDouble c);
ComplexDouble ctangent(ComplexDouble c);
ComplexDouble ccotangent(ComplexDouble c);
ComplexDouble carcsine(ComplexDouble c);
ComplexDouble carcsineSimple(ComplexDouble c);
ComplexDouble carccosine(ComplexDouble c);
ComplexDouble carctangent(ComplexDouble c);
ComplexDouble carccotangent(ComplexDouble c);
ComplexDouble carcsinehyp(ComplexDouble c);
ComplexDouble carccosinehyp(ComplexDouble c);
ComplexDouble carctangenthyp(ComplexDouble c);
ComplexDouble carccotangenthyp(ComplexDouble c);
ComplexDouble ccbrt(ComplexDouble x);
ComplexDouble crad(ComplexDouble x);
ComplexDouble cdeg(ComplexDouble x);
ComplexDouble crecip(ComplexDouble x);
ComplexDouble clogx(ComplexDouble y, ComplexDouble x);
ComplexDouble clog2(ComplexDouble x);
ComplexDouble clogarithm10(ComplexDouble x);
ComplexDouble cmax(ComplexDouble a, ComplexDouble b);
ComplexDouble cmin(ComplexDouble a, ComplexDouble b);
ComplexDouble carctangent2(ComplexDouble a, ComplexDouble b);

typedef long double (*RealFunctionPtr)(ComplexDouble);
typedef void (*BigIntVoid1ParamFunctionPtr)(const bigint_t*, char*);

typedef void (*BigIntVoid2ParamFunctionPtr)(const bigint_t*, const bigint_t*, bigint_t*);

typedef int (*BigIntIntFunctionPtr)(const bigint_t*, const bigint_t*);
typedef ComplexDouble (*ComplexFunctionPtr1Param)(ComplexDouble);
typedef ComplexDouble (*ComplexFunctionPtr2Param)(ComplexDouble, ComplexDouble);
typedef ComplexDouble (*ComplexFunctionPtrVector)(ComplexDouble, ComplexDouble, ComplexDouble, int);

extern const char* matrixfnname[];
extern const int NUMMATRIXPARAMFN;
extern const char* mathfn1paramname[];
extern const int EXPFNINDEX;
extern const int NUMMATH1PARAMFN;
extern const int NUMREAL1PARAMFN;
extern const int NUMMATH1PARAMTRIGFN;
extern const int NUMMATH1PARAMTRIGANTIFN;
extern ComplexFunctionPtr1Param mathfn1param[];
extern RealFunctionPtr realfn1param[];
extern BigIntVoid1ParamFunctionPtr bigfnvoid1param[];
extern BigIntVoid2ParamFunctionPtr bigfnvoid2param[];
extern BigIntIntFunctionPtr bigfnint2param[];

extern const char* bigfnvoid1paramname[];
extern const int NUMBIGINT1FNS;
extern const char* vecfn1paramname[];
extern const char* vecfn2paramname[];
extern const int NUMVECFNS;
extern const int NUMVEC2FNS;

extern const char* mathfnop2paramname[];
extern const int POWFNINDEX; 
extern const int NUMMATH2PARAMFNOP; 
extern const int NUMVOID2PARAMBIGINTFNMIN;
extern const int NUMVOID2PARAMBIGINTFNMAX;
extern const int ATAN2FNINDEX;

extern ComplexFunctionPtr2Param mathfn2param[];


ComplexDouble suminternal(ComplexDouble running, ComplexDouble next);
ComplexDouble sum(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n);
ComplexDouble sqsum(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n); 
ComplexDouble sqsum(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n); 
ComplexDouble var(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n);
ComplexDouble sdv(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n);
ComplexDouble mean(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n);
ComplexDouble rsum(ComplexDouble summed, ComplexDouble sqsummed, ComplexDouble rsummed, int n);

extern ComplexFunctionPtrVector mathfnvec1param[];

#endif
