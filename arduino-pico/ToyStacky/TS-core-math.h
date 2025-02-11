long double sgn(long double value) {
	if (signbit(value)) return (-1.0);
	else return (1.0);
}

long double abso (ComplexDouble value) {
	return sqrtl(value.real * value.real + value.imag * value.imag);
}

bool alm0(ComplexDouble value) {
	//limit of long double precision
	if (fabsl(value.real) < DOUBLE_EPS && fabsl(value.imag) < DOUBLE_EPS) return true;
	else return false;
}

bool alm0double(long double value) {
	//limit of long double precision
	if (fabsl(value) < DOUBLE_EPS) return true;
	else return false;
}

ComplexDouble cneg(ComplexDouble value) {
	return makeComplex(-value.real, -value.imag);
}

ComplexDouble cabso (ComplexDouble value) {
	return makeComplex(abso(value), 0);
}

long double cargu (ComplexDouble value) {
	if (alm0double(value.real)) {
		if (alm0double(value.real)) {
			errno = 10000;
			return 0; //undef arg
		}
		return ((value.imag < 0)? -1.570796326794896619L: 1.570796326794896619L);
	}
	long double princ =  atanl(value.imag/value.real);
	//printf ("cargu: sign of real = %f sign of imag = %f\n", sgn(value.real), sgn(value.imag));
	if (value.real >= 0 && value.imag >= 0)
		return princ; //1st quadrant ok
	else if (value.real < 0 && value.imag >= 0)
		return princ + 3.14159265358979323846L; //2nd quadrant ok
	else if (value.real < 0 && value.imag < 0)
		return princ - 3.14159265358979323846L;//3rd quadrant ok
	else if (value.real >= 0 && value.imag < 0)
		return princ; //4th quadrant ok
	else
		return 0;
}

long double crealpart (ComplexDouble value) {
	return value.real;
}

long double cimagpart (ComplexDouble value) {
	return value.imag;
}

ComplexDouble conjugate(ComplexDouble c) {
	return makeComplex(c.real, -c.imag);
}

ComplexDouble polar(ComplexDouble c) {
	return makeComplex(abso(c), atanl(c.imag/c.real));
}

ComplexDouble rect(ComplexDouble c) {
	return makeComplex(c.real*cosl(c.imag), c.real*sinl(c.imag));
}

ComplexDouble cadd(ComplexDouble value, ComplexDouble second) {
	return makeComplex(value.real + second.real, value.imag + second.imag);
}

ComplexDouble caddd(ComplexDouble value, long double second) {
	return makeComplex(value.real + second, value.imag);
}

ComplexDouble csub(ComplexDouble value, ComplexDouble second) {
	return makeComplex(value.real - second.real, value.imag - second.imag);
}

ComplexDouble csubd(ComplexDouble value, long double second) {
	return makeComplex(value.real - second, value.imag);
}

ComplexDouble cmul(ComplexDouble value, ComplexDouble second) {
	return makeComplex(value.real * second.real - value.imag * second.imag, 
		value.real * second.imag + value.imag * second.real);
}

ComplexDouble cdiv(ComplexDouble value, ComplexDouble second) {
	long double denom = second.real * second.real + second.imag * second.imag;
	if (alm0double(denom)) return makeComplex(0, 0);
	ComplexDouble nom = cmul(value, conjugate(second));
	return makeComplex(nom.real / denom, nom.imag / denom);
}

ComplexDouble cdivd(ComplexDouble value, long double second) {
	return makeComplex(value.real / second, value.imag / second);
}

ComplexDouble cmod(ComplexDouble value, ComplexDouble second) {
	return makeComplex(0, 0);
}

ComplexDouble ceq(ComplexDouble value, ComplexDouble second) {
	if (alm0double(abso(csub(value, second))))
		return makeComplex(1, 0);
	else
		return makeComplex(0, 0);
}

ComplexDouble cneq(ComplexDouble value, ComplexDouble second) {
	if (!alm0double(abso(csub(value, second))))
		return makeComplex(1, 0);
	else
		return makeComplex(0, 0);
}

bool bceq(ComplexDouble value, ComplexDouble second) {
	if (alm0double(abso(csub(value, second))))
		return true;
	else
		return false;
}

bool bcneq(ComplexDouble value, ComplexDouble second) {
	if (!alm0double(abso(csub(value, second))))
		return true;
	else
		return false;
}

ComplexDouble cpar(ComplexDouble value, ComplexDouble second) {
	return cdiv(cmul(value, second), cadd(value, second));
}

ComplexDouble clt(ComplexDouble value, ComplexDouble second) {
	ComplexDouble diff = csub(second, value);
	if (alm0double(diff.imag))
		//difference is real
		//LT is true if difference is positive
		return (!alm0double(diff.real))? makeComplex(1.0, 0.0): makeComplex(0.0, 0.0);
	else
		//difference is complex
		//LT is true if magnitude of the diff is positive
		return (!alm0(diff))? makeComplex(1.0, 0.0): makeComplex(0.0, 0.0);
}

ComplexDouble cgt(ComplexDouble value, ComplexDouble second) {
	return (clt(second, value));
}

ComplexDouble cgte(ComplexDouble value, ComplexDouble second) {
	ComplexDouble l = clt(value, second);
	if (l.real == 1.0) return makeComplex(0.0, 0.0);
	else return makeComplex(1.0, 0.0);
}

ComplexDouble clte(ComplexDouble value, ComplexDouble second) {
	ComplexDouble g = cgt(value, second);
	if (g.real == 1.0) return makeComplex(0.0, 0.0);
	else return makeComplex(1.0, 0.0);
}

ComplexDouble cln(ComplexDouble c) {
	long double re = c.real;
	long double im = c.imag;
	long double r, j, imdre;
	if (!alm0double(re) && alm0double(im) && (re > 0.0)) {
		//imag is 0
		//re is positive
		return makeComplex(logl(re), 0);
	} else if (!alm0double(re)) {
		if (re > 0.0) {
			r = logl(re);
			j = 0.0;
		} else {
			//log of a negative number = log (-1) * log of the negative of the negative number
			//log (-1) = pi * i
			r = logl(-re);
			j = __TS_PI__;
		}
		if (!alm0double(im)) {
			imdre = (im/re);
			r += logl(imdre*imdre + 1)/2;
			j += atanl(imdre);
		} 
	} else if (!alm0double(im)) {
		r = logl(im);
		j = 0.5 * __TS_PI__  * sgn(im);
	} else {
		r = 0;
		j = 0;
		errno = 10001;
	}
	if (j > __TS_PI__)
		j -= __TS_PI__ * 2;
	return makeComplex(r, j);
}

ComplexDouble cexpo(ComplexDouble c) {
	if (alm0double(c.imag)) {
		return makeComplex(expl(c.real), 0);
	}
	long double r = c.real;
	long double j = c.imag;
	long double rexp = expl(r);
	r = rexp * cosl(j);
	if (!alm0double(c.imag))
		j = rexp * sinl(j);
	else
		j = 0;
	return makeComplex (r, j);
}

ComplexDouble cpower(ComplexDouble c, ComplexDouble d) {
	if (alm0double(c.imag) && alm0double(d.imag) && (d.real >= 1.0))
		return makeComplex(pow(c.real, d.real), 0.0);
	if (alm0double(c.imag) && alm0double(d.imag) && (d.real == 0.0))
		return makeComplex (1.0, 0.0);
	return (cexpo(cmul(cln(c), d))); 
}

ComplexDouble cpowerd(ComplexDouble c, long double d) {
	if (alm0double(c.real) && alm0double(c.imag))
		return makeComplex(0.0, 0.0);
	if (alm0double(c.imag) && (d >= 1.0))
		return makeComplex(pow(c.real, d), 0.0);
	if (alm0double(c.imag) && (d == 0.0))
		return makeComplex (1.0, 0.0);
	return (cexpo(cmul(cln(c), makeComplex(d, 0.0))));
}

ComplexDouble csqroot(ComplexDouble c) {
	printf("csqroot: got c.real = %Lg c.imag = %Lg\n", c.real, c.imag);
	return cpowerd(c, 0.5);
}

ComplexDouble cbroot(ComplexDouble c) {
	ComplexDouble r = cpower(c, makeComplex((1.0/3.0), 0.0));
	if (!alm0double(r.imag)) {
		//returned data is complex, we want the real root
		ComplexDouble rmagsq = cmul(r, conjugate(r));
		return (cdiv(c, rmagsq));
	} else {
		return r;
	}
}

ComplexDouble csinehyp(ComplexDouble c) {
	return cdivd(csub(cexpo(c), cexpo(cneg(c))), 2.0);
}

ComplexDouble ccosinehyp(ComplexDouble c) {
	return cdivd(cadd(cexpo(c), cexpo(cneg(c))), 2.0);
}

ComplexDouble ctangenthyp(ComplexDouble c) {
	ComplexDouble d = ccosinehyp(c);
	if (alm0(d)) {
		errno = 10002;
		return makeComplex(0.0, 0.0);
	}
	return cdiv(csinehyp(c), d);
}

ComplexDouble ccotangenthyp(ComplexDouble c) {
	ComplexDouble d = csinehyp(c);
	if (alm0(d)) {
		errno = 10003;
		return makeComplex(0.0, 0.0);
	}
	return cdiv(ccosinehyp(c), d);

}

ComplexDouble csine(ComplexDouble c) {
	if (alm0double(c.imag)) return makeComplex(sinl(c.real), 0);
	ComplexDouble jc = cmul (makeComplex(0.0, 1.0), c);
	return cdiv(csub(cexpo(jc), cexpo(cneg(jc))), makeComplex(0.0, 2.0));
}

ComplexDouble ccosine(ComplexDouble c) {
	if (alm0double(c.imag)) return makeComplex(cosl(c.real), 0);
	ComplexDouble jc = cmul (makeComplex(0.0, 1.0), c);
	return cdivd(cadd(cexpo(jc), cexpo(cneg(jc))), 2.0);
}

ComplexDouble ctangent(ComplexDouble c) {
	ComplexDouble d = ccosine(c);
	if (alm0(d)) {
		errno = 10004;
		return makeComplex(0.0, 0.0);
	}
	return cdiv(csine(c), d);
}

ComplexDouble ccotangent(ComplexDouble c) {
	ComplexDouble d = csine(c);
	if (alm0(d)) {
		errno = 10005;
		return makeComplex(0.0, 0.0);
	}
	return cdiv(ccosine(c), d);
}

ComplexDouble carcsine(ComplexDouble c) {
	ComplexDouble j = makeComplex(0.0, 1.0);
	ComplexDouble c1_m_zz = csub(makeComplex(1.0, 0.0), cmul(c, c));
	ComplexDouble csqrt_c1_m_zz = csqroot(cabso(c1_m_zz));
	printf("carcsine: csqrt_c1_m_zz = %Lg + %Lg(i)\n", c.real, c.imag);
	long double rarg_c1_m_zz = cargu(c1_m_zz);
	ComplexDouble cterm1 = cmul(j, c);
	ComplexDouble cexp_arg_1_m_zz_i_d_2 = cexpo(makeComplex(0.0, rarg_c1_m_zz/2));
	ComplexDouble cterm2 = cmul(csqrt_c1_m_zz, cexp_arg_1_m_zz_i_d_2);
	ComplexDouble  clogparam = cadd(cterm1, cterm2);
	return cdiv(cln(clogparam), j);
}

ComplexDouble carcsineSimple(ComplexDouble c) {
	//from FreeBasic
	//function apcplx.asin() as apcplx
	//	dim as apcplx j = apcplx(0, 1)
	//	dim as apcplx x = (1 - this * this).sqrt() - j * this
	//	return j * x.log()
	//end function

	ComplexDouble j = makeComplex(0.0, 1.0);
	ComplexDouble x = csub(csqroot(csub(makeComplex(1.0, 0.0), cmul(c, c))), cmul (j, c));
	return cmul(j, cln(x));

}

ComplexDouble carccosine(ComplexDouble c) {
	ComplexDouble j = makeComplex(0.0, 1.0);
	ComplexDouble c1_m_zz = csub(makeComplex(1.0, 0.0), cmul(c, c));
	ComplexDouble csqrt_c1_m_zz = csqroot(cabso(c1_m_zz));
	long double rarg_c1_m_zz = cargu(c1_m_zz);
	ComplexDouble cterm1 = c;
	ComplexDouble cexp_arg_1_m_zz_i_d_2 = cexpo(makeComplex(0.0, rarg_c1_m_zz/2));
	ComplexDouble cterm2a = cmul(csqrt_c1_m_zz, cexp_arg_1_m_zz_i_d_2);
	ComplexDouble cterm2 = cmul(j, cterm2a);
	ComplexDouble  clogparam = cadd(cterm1, cterm2);
	return cdiv(cln(clogparam), j);
}

ComplexDouble carctangent(ComplexDouble c) {
	ComplexDouble j = makeComplex(0.0, 1.0);
	ComplexDouble jx2 = makeComplex(0.0, 2.0);
	ComplexDouble jpc = cadd(j, c);
	ComplexDouble jmc = csub(j, c);
	if (alm0(jpc) || alm0(jmc)) {
		errno = 10006;
		return makeComplex(0.0, 0.0);
	}
	return cdiv(cln(cdiv(jmc, jpc)), jx2);
}

ComplexDouble carccotangent(ComplexDouble c) {
	ComplexDouble j = makeComplex(0.0, 1.0);
	ComplexDouble jx2 = makeComplex(0.0, 2.0);
	ComplexDouble cpj = cadd(c, j);
	ComplexDouble cmj = csub(c, j);
	if (alm0(cpj) || alm0(cmj)) {
		errno = 10007;
		return makeComplex(0.0, 0.0);
	}
	return cdiv(cln(cdiv(cpj, cmj)), jx2);
}

ComplexDouble carcsinehyp(ComplexDouble c) {
	ComplexDouble c1_p_zz = cadd(makeComplex(1.0, 0.0), cmul(c, c));
	ComplexDouble csqrt_c1_p_zz = csqroot(cabso(c1_p_zz));
	long double rarg_c1_p_zz = cargu(c1_p_zz);
	ComplexDouble cterm1 = c;
	ComplexDouble cexp_arg_1_p_zz_i_d_2 = cexpo(makeComplex(0.0, rarg_c1_p_zz/2));
	ComplexDouble cterm2 = cmul(csqrt_c1_p_zz, cexp_arg_1_p_zz_i_d_2);
	ComplexDouble  clogparam = cadd(cterm1, cterm2);
	return cln(clogparam);
}

ComplexDouble carccosinehyp(ComplexDouble c) {
	ComplexDouble czz_m_1 = csub(cmul(c, c), makeComplex(1.0, 0.0));
	ComplexDouble csqrt_czz_m_1 = csqroot(cabso(czz_m_1));
	long double rarg_czz_m_1 = cargu(czz_m_1);
	ComplexDouble cterm1 = c;
	ComplexDouble cexp_arg_czz_m_1 = cexpo(makeComplex(0.0, rarg_czz_m_1/2));
	ComplexDouble cterm2 = cmul(csqrt_czz_m_1, cexp_arg_czz_m_1);
	ComplexDouble  clogparam = cadd(cterm1, cterm2);
	return cln(clogparam);
}

ComplexDouble carctangenthyp(ComplexDouble c) {
	if (alm0(csub(c, makeComplex(1.0, 0.0)))) {
		errno = 10006;
		return makeComplex(0.0, 0.0);
	}
	ComplexDouble x = cdiv(cadd(makeComplex(1.0, 0.0), c), csub(makeComplex(1.0, 0.0), c));
	return cdivd(cln(x), 2.0);
}

ComplexDouble carccotangenthyp(ComplexDouble c) {
	if (alm0(csub(c, makeComplex(1.0, 0.0)))) {
		errno = 10006;
		return makeComplex(0.0, 0.0);
	}
	ComplexDouble x = cdiv(cadd(c, makeComplex(1.0, 0.0)), csub(c, makeComplex(1.0, 0.0)));
	return cdivd(cln(x), 2.0);
}

ComplexDouble ccbrt(ComplexDouble x){
	return cpowerd(x, (1.0/3.0));
}

ComplexDouble ctorad(ComplexDouble x){
	//pi/180 = 0.017453292519943295769237L
	return cmul(x, makeComplex(0.017453292519943295769237L, 0.0));
}

ComplexDouble ctodeg(ComplexDouble x){
	//180/pi = 57.295779513082320876798L
	return cmul(x, makeComplex(57.295779513082320876798L, 0.0));
}

ComplexDouble crecip(ComplexDouble x){
	return cdiv(makeComplex(1, 0), x);
}

ComplexDouble clogx(ComplexDouble y, ComplexDouble x){
	return cdiv(cln(y), cln(x));
}

ComplexDouble clog2(ComplexDouble x){
	return cdiv(cln(x), cln(makeComplex(2.0, 0.0)));
}

ComplexDouble clogarithm10(ComplexDouble x){
	return cdiv(cln(x), cln(makeComplex(10.0, 0.0)));
}

ComplexDouble cmax(ComplexDouble a, ComplexDouble b) {
	return (abso(a) > abso(b)) ? a : b;
}

ComplexDouble cmin(ComplexDouble a, ComplexDouble b) {
	return (abso(a) < abso(b)) ? a : b;
}

ComplexDouble carctangent2(ComplexDouble a, ComplexDouble b) {
	return carctangent(cdiv(a, b));
}

