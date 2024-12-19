#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#define DOUBLE_EPS 2.2250738585072014e-308

typedef struct {
	double real;
	double imag;
} ComplexDouble;

const double __TS_PI__ = 3.141592653589793;
void (*NULLFN)(void) = NULL;

char errmsg[100];

ComplexDouble makeComplex(double re, double im) {
	ComplexDouble ret;
	ret.real = re;
	ret.imag = im;
	return ret;
}

#define SHORT_STRING_SIZE 33
typedef struct {
	char error[SHORT_STRING_SIZE];//error notification
} Machine;

Machine vm;

#define MKCPLX(a,...) makeComplex(a, (0, ##__VA_ARGS__))

#include "../TS-core-math.h"

int main () {

	ComplexDouble x = makeComplex(4.0, 4.0);
	//ComplexDouble x = makeComplex(1.141593, -2);
	ComplexDouble c;
	c = makeComplex(-1.0, -1.0);
	double rarguc = cargu(c);
	//printf ("rarguc = %lf\n\n", rarguc);

	ComplexDouble sinx = csine(x);
	ComplexDouble cosx = ccosine(x);

	ComplexDouble asinxsinx = carcsine(sinx);
	ComplexDouble acoscosx = carccosine(cosx);

	printf ("x.re = %lf x.im = %lf\n", x.real, x.imag);
	printf ("sinx.re = %lf z.im = %lf\n", sinx.real, sinx.imag);
	printf ("recovered x.re = %lf x.im = %lf\n\n", asinxsinx.real, asinxsinx.imag);
	printf ("x.re = %lf x.im = %lf\n", x.real, x.imag);
	printf ("cosx.re = %lf z.im = %lf\n", cosx.real, cosx.imag);
	printf ("recovered x.re = %lf x.im = %lf\n\n", acoscosx.real, acoscosx.imag);

	//ComplexDouble one = makeComplex(1, 0);
	//ComplexDouble j = makeComplex(0.0, 1.0);
	//ComplexDouble onedivj = cdiv(one, j);
	//printf ("one.re = %lf one.im = %lf\n", one.real, one.imag);
	//printf ("onedivj.re = %lf z.im = %lf\n\n", onedivj.real, onedivj.imag);
	//
	//
	//ComplexDouble y = makeComplex(2.0, 2.0);
	//ComplexDouble cabsy = cabso(y);
	//ComplexDouble sqry = csqroot(y);
	//printf ("y.re = %lf y.im = %lf\n", y.real, y.imag);
	//printf ("cabsy.re = %lf cabsy.im = %lf\n", cabsy.real, cabsy.imag);
	//printf ("sqry.re = %lf sqry.im = %lf\n\n", sqry.real, sqry.imag);
	//ComplexDouble jdsqry = cdiv(j, sqry);
	//printf ("jdsqry.re = %lf jdsqry.im = %lf\n\n", jdsqry.real, jdsqry.imag);
	//printf ("recovered sqry.re = %lf sqry.im = %lf\n\n", sqry.real, sqry.imag);

	return 0;
}
