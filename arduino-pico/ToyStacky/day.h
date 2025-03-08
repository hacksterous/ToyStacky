//(c) 202224 Anirban Banerjee
//Part of ToyStacky
//Licensed under:
//GNU GENERAL PUBLIC LICENSE
//Version 3, 29 June 2007
#include <stdbool.h>
#include <math.h>
#include <string.h>

static const char* monthList[] = {"January","February","March","April","May","June",
   "July","August","September","October","November","December"};

static const char* rashiList[] = {"Mesha","Vrisha","Mithuna","Karka","Simha","Kanya","Tula",
   "Vrischika","Dhanu","Makara","Kumbha","Meena"};

static const char* dayList[] = {"Sun","Mon","Tues","Wednes","Thurs","Fri","Satur"};

static const char* tithiList[] = {"Pratipad","Dvitiya","Tritiya","Chaturthi","Panchami",
		"Shashthi","Saptami","Ashtami","Navami","Dashami","Ekadashi",
		"Dvadashi","Trayodashi","Chaturdashi","Purnima","Pratipad",
		"Dvitiya","Tritiya","Chaturthi","Panchami","Shashthi",
		"Saptami","Ashtami","Navami","Dashami","Ekadashi","Dvadashi",
		"Trayodashi","Chaturdashi","Amaavasya"};

static const char* karanaList[] = {"Bava","Baalava","Kaulava","Taitula","Garija","Vanija",
   "Vishti","Shakuni","Chatushpada","Naga","Kimstughna"};

static const char* yogaList[] = {"Vishakumbha","Preeti","Ayushman","Saubhagya","Shobhana",
   "Atiganda","Sukarman","Dhriti","Shula","Ganda","Vriddhi",
   "Dhruva","Vyaghata","Harshana","Vajra","Siddhi","Vyatipata",
   "Variyan","Parigha","Shiva","Siddha","Saadhya","Shubha","Shukla",
   "Brahma","Indra","Vaidhriti"};

static const char* nakshatraList[] = {"Ashvini","Bharani","Krittika","Rohini","Mrigashira","Ardra",
		"Punarvasu","Pushya","Ashlesa","Magha","Purva Phalguni","Uttara Phalguni",
		"Hasta","Chitra","Svaati","Vishakha","Anuradha","Jyeshtha","Mula",
		"Purva Ashadha","Uttara Ashadha","Shravana","Dhanishtha","Shatabhisha",
		"Purva Bhaadra","Uttara Bhaadra","Revati"};

//zhr=5.5, latt=22.5726, longt=88.3639): Kolkata
//zhr=5.5, latt=12.9716, longt=77.5946): Bangalore

typedef struct {
	int sriseh, srisem, srises;
	int sseth, ssetm, ssets;
	int dlh, dlm, dls;
} Suntimes;

static long double ts_at_mn 
(int dd, int mm, int yyyy, long double tz){
	//timescale at midnight
	//https://stjarnhimlen.se/comp/ppcomp.html#3
	//The time scale in these formulae are counted in days. 
	//Hours, minutes, seconds are expressed as fractions of a day. 
	//Day 0.0 occurs at 2000 Jan 0.0 UT (or 1999 Dec 31, 24:00 UT). 
	//This "day number" d is computed as follows (y=year, m=month, D=date, 
	//UT=UT in hours+decimals):
	//d = 367*y - 7 * ( y + (m+9)/12 ) / 4 - 3 * ( ( y + (m-9)/7 ) / 100 + 1 ) / 4 + 275*m/9 + D - 730515

	//int64_t d = 367*yyyy 
	//	- 7 * (yyyy + (mm+9)/12)/4 
	//	- 3*((yyyy + (mm-9)/7)/100 + 1)/4 
	//	+ 275*mm/9 + dd - 730515;

	int64_t x0 = 367*yyyy;
		int64_t x10 = (mm+9)/12;
	int64_t x1 = 7 * (yyyy + x10)/4;
		int64_t x20 = (mm-9)/7;
		int64_t x21 = (yyyy + x20)/100;
	int64_t x2 =  3*(x21 + 1)/4;
		int64_t x30 = 275*mm/9;
	int64_t x3 = x30 + dd - 730515;

	int64_t d = x0 - x1 - x2 + x3;

	long double ret = (long double) d - (tz/24.0);
	//printf("ts_at_mn: x1 = %Ld\n", x1);
	//printf("ts_at_mn: d = %Ld\n", d);
	//printf("ts_at_mn: ret = %Lf\n", ret);

	return ret; //add 2451543.5 to get Julian Date
}

static long double toradians 
(long double degrees){
	return (3.1415926535897932 * degrees / 180.0);
}

static long double todegrees 
(long double radians){
	return (180.0 * radians /3.1415926535897932);
}

static long double sun_esrl 
(long double latt, long double longt, long double zhr, long double julian_day, long double* sunriseLST, long double* sunsetLST) {

	long double julian_century = (julian_day - 2451545.0)/36525.0; //G
	long double sun_geom_mean_long_deg = fmodl((280.46646+julian_century*(36000.76983+julian_century*0.0003032)), 360.0); //I
	long double sun_geom_mean_anom_deg = 357.52911+julian_century*(35999.05029-0.0001537*julian_century); //J
	long double earth_orbit_eccentricity = 0.016708634-julian_century*(0.000042037+0.0000001267*julian_century); //K
	long double sun_eqn_of_centre = sinl(toradians(sun_geom_mean_anom_deg))* 
						(1.914602-julian_century*(0.004817+0.000014*julian_century))+ 
						sinl(toradians(2*sun_geom_mean_anom_deg))* 
						(0.019993-0.000101*julian_century)+ 
						sinl(toradians(3*sun_geom_mean_anom_deg))*0.000289; //L
	long double sun_true_long_deg = sun_geom_mean_long_deg + sun_eqn_of_centre; //M
	long double sun_true_anom_deg = sun_geom_mean_anom_deg + sun_eqn_of_centre; //N
	long double sun_rad_vector_au = (1.000001018*(1-earth_orbit_eccentricity*earth_orbit_eccentricity))/ 
							(1+earth_orbit_eccentricity*cosl(toradians(sun_true_anom_deg))); //O
	long double sun_apparent_long_deg = sun_true_long_deg-0.00569-0.00478* 
						sinl(toradians(125.04-1934.136*julian_century)); //P
	long double mean_oblique_ecliptic_deg = 23+(26+((21.448-julian_century*(46.815+ 
						julian_century*(0.00059-julian_century*0.001813))))/60.0)/60.0; //Q
	long double oblique_correction_deg = mean_oblique_ecliptic_deg+ 
						0.00256*cosl(toradians(125.04-1934.136*julian_century)); //R
	long double sun_right_ascension_deg = todegrees(atan2(cosl(toradians(oblique_correction_deg))*sinl(toradians(sun_apparent_long_deg)), \
								cosl(toradians(sun_apparent_long_deg)))); //S
	long double sun_declination_deg = todegrees(asinl(sinl(toradians(oblique_correction_deg))* 
						sinl(toradians(sun_apparent_long_deg)))); //T
	long double var_y = tanl(toradians(oblique_correction_deg/2))*tanl(toradians(oblique_correction_deg/2)); //U
	long double equation_of_time_min = 4*todegrees(var_y*sinl(2*toradians(sun_geom_mean_long_deg))- 
						2*earth_orbit_eccentricity*sinl(toradians(sun_geom_mean_anom_deg))+ 
						4*earth_orbit_eccentricity*var_y*sinl(toradians(sun_geom_mean_anom_deg))* 
						cosl(2*toradians(sun_geom_mean_long_deg))- 
						0.5*var_y*var_y*sinl(4*toradians(sun_geom_mean_long_deg))- 
						1.25*earth_orbit_eccentricity*earth_orbit_eccentricity* 
						sinl(2*toradians(sun_geom_mean_anom_deg))); //V
	long double hour_angle_cosine = cosl(toradians(90.833))/(cosl(toradians(latt))* 
						cosl(toradians(sun_declination_deg)))-tanl(toradians(latt))* 
						tanl(toradians(sun_declination_deg));

	if (hour_angle_cosine > 1.0) {
		//sun never rises
		*sunriseLST = 0.0;
		*sunsetLST = 0.0;
		return 0.0;
	}
	else if (hour_angle_cosine < -1.0) {
		//sun never sets
		*sunriseLST = 0.0;
		*sunsetLST = 0.0;
		return 24.0;
	}

	long double hour_angle_sunrise_deg = todegrees(acosl(cosl(toradians(90.833))/(cosl(toradians(latt))*
						cosl(toradians(sun_declination_deg)))-tanl(toradians(latt))*
						tanl(toradians(sun_declination_deg)))); //W

	long double solar_noon_LST = (720.0-4*longt-equation_of_time_min+zhr*60)/1440.0; //X

	*sunriseLST = (solar_noon_LST - (hour_angle_sunrise_deg*4)/1440.0) * 24; //Y
	//printf("sun_esrl: sunriseLST = %Lf\n", *sunriseLST);
	*sunsetLST = (solar_noon_LST + (hour_angle_sunrise_deg*4)/1440.0)*24; //Z

	long double sunlight_duration_hrs = hour_angle_sunrise_deg * 8 / 60; //AA

	return sunlight_duration_hrs;
}

static long double rev 
//revolutions
(long double x) {
	return  x - floorl(x/360.0)*360.0;
}

static long double ayanansha 
(long double d){
	long double t = (d + 36523.5)/36525;
	long double o = rev(259.183275-1934.142008333206*t + 0.0020777778*t*t);
	long double l = rev(279.696678+36000.76892*t + 0.0003025*t*t);
	long double ayan = 17.23*sinl(toradians(o))+1.27*sinl(toradians(l*2))-(5025.64+1.11*t)*t;
	//Based on Lahiri
	ayan = (ayan-80861.27)/3600.0;
	return ayan;
}

static long double ts_at_hr 
(int dd, int mm, int yyyy, long double hr, long double tz){
	return ts_at_mn (dd, mm, yyyy, tz) + (hr/24.0);
}

static void calculate_moon_sun_long
(long double d, long double* mlon, long double* slon) {
	//https://www.stjarnhimlen.se/comp/tutorial.html#7
	//printf ("calculate_moon_sun_long d= %.8f\n", d)

	//------------------------------Sun
	//below are in radians
	long double w = toradians(rev(282.9404 + 4.70935e-5 * d));		//Sun's longitude of perihelion
	long double Ms = toradians(rev(356.0470 + 0.9856002585 * d));	//Sun's mean anomaly
	long double oblecl = 23.4393 - 3.563e-7 * d;		//obliquity of the ecliptic -- unused

	long double e = 0.016709 - 1.151e-9 * d;		//eccentricity

	long double E = Ms + e * sinl(Ms) * (1 + e * cosl(Ms)); //eccentricity anomaly

	//Sun's mean longitude
	long double Ls = w + Ms;

	//Sun's rectangular coordinates
	long double x = cosl(E) - e;
	long double y = sinl(E) * sqrtl(1 - e*e);

	//distance from Sun and true anomaly
	long double r = sqrtl(x*x + y*y);	//in Earth radii
	long double v = atan2l( y, x );		//true anomaly
	*slon = rev(todegrees(v + w));
	//printf ("Sun's longitude = %.8f\n", slon)

	//------------------------------Moon
	//all below are in radians
	long double N = toradians(rev(125.1228 - 0.0529538083 * d)); //Longt of ascending node
	long double i = 0.089804;			//Inclination in degrees is 5.1454
	w = toradians(rev(318.0634 + 0.1643573223 * d));		//Arg. of perigee
	long double Mm = toradians(rev(115.3654 + 13.0649929509 * d)) ; //Mean eccentricity anomaly

	long double a = 60.2666; //Mean distance in Earth equatorial radii
	e = 0.054900; //Eccentricity

	//iterate for accurate eccentricity anomaly
	E = Mm + e * sinl(Mm) * (1 + e * cosl(Mm));
	int iteration = 0;
	long double eps;
	while (true) {
		eps	= (E - e * sinl(E) - Mm) / (1 - e * cosl(E));
		E = E - eps;
		if (iteration++ > 50 || fabsl(eps) < 1e-5) break;
	}

	//compute rectangular (x,y) coordinates in the plane of the lunar orbit
	x = a * (cosl(E) - e);
	y = a * sqrtl(1 - e*e) * sinl(E);

	r = sqrtl(x*x + y*y); //distance Earth radii
	v = atan2l(y, x); //true anomaly

	long double xeclip = r * (cosl(N) * cosl(v+w) - sinl(N) * sinl(v+w) * cosl(i));
	long double yeclip = r * (sinl(N) * cosl(v+w) + cosl(N) * sinl(v+w) * cosl(i));
	long double zeclip = r * sinl(v+w) * sinl(i);

	long double localmlon =  rev(todegrees(atan2l(yeclip, xeclip)));
	long double latt  =  atan2l(zeclip, sqrt( xeclip*xeclip + yeclip*yeclip));
	r =  sqrtl(xeclip*xeclip + yeclip*yeclip + zeclip*zeclip);

	//Compensate for Moon's perturbations
	//Sun's  mean longitude:		Ls	 (already computed as Ls)
	//Moon's mean longitude:		Lm  =  N + w + Mm (for the Moon)
	//Sun's  mean anomaly:		  Ms	 (already computed as Ms)
	//Moon's mean anomaly:		  Mm	 (already computed in this function)
	//Moon's mean elongation:	   D   =  Lm - Ls
	//Moon's argument of latitude:  F   =  Lm - N
	
	long double Lm = N + w + Mm;
	long double D  = Lm - Ls;
	long double F = Lm - N;
	//printf ("Moon's longt before perturb fix is %f\n", localmlon)
	//printf ("Moon's uncorrected ecl. longitude = %.8f\n", localmlon)
	
	//localmlon in degrees
	localmlon += -1.27388888 * sinl(Mm - 2*D);	//Evection -- stjarnhimlen gives -1.274
	localmlon += +0.65833333 * sinl(2*D);		//Variation -- stjarnhimlen give +0.658
	localmlon += -0.185 * sinl(Ms);				//Yearly equation -- stjarnhimlen gives -0.186, but
												//[Chapront-Touzé and Chapront 1988] has 666 arc-seconds
	localmlon += -0.059 * sinl(2*Mm - 2*D)
			-0.057 * sinl(Mm - 2*D + Ms)
			+0.053 * sinl(Mm + 2*D)
			+0.046 * sinl(2*D - Ms)
			+0.041 * sinl(Mm - Ms)
			-0.034722222 * sinl(D);			//Parallactic equation [Chapront-Touzé and Chapront 1988] has
											//125 arc-seconds = 0.034722222
											//http://www.stjarnhimlen.se/comp/tutorial.html has 0.035
	localmlon += -0.031 * sinl(Mm + Ms)
			-0.015 * sinl(2*F - 2*D);		//reduction to the ecliptic from stjarnhimlen -- Wikipedia value is 0.0144
											//stjarnhimlen has 0.015
	localmlon += +0.011 * sinl(Mm - 4*D);
	//printf ("Moon's longt after perturb fix in radians is %f\n", localmlon)

	//printf ("Sun's ecl. longitude = %.8f\n", slon)
	//printf ("Moon's ecl. longitude = %.8f\n", localmlon)
	*mlon = localmlon;
}

static int nakshatra 
(int dd, int mm, int yyyy, long double hr, long double tz){
	long double d = ts_at_mn (dd, mm, yyyy, tz) + (hr/24.0);
	long double mlon;
	long double slon;
	calculate_moon_sun_long(d, &mlon, &slon);
	int nakshindex = ((int)(rev(mlon + ayanansha(d))*6)/80) % 27;
	//print ("nakshatra =", nakshatraList[nakshindex])
	return nakshindex;
}
	
static int yoga 
(int dd, int mm, int yyyy, long double hr, long double tz){
	long double d = ts_at_mn (dd, mm, yyyy, tz) + (hr/24.0);
	long double mlon;
	long double slon;
	calculate_moon_sun_long (d, &mlon, &slon);
	//return index to yoga table
	int yogaindex = ((int)rev(mlon + slon + 2 * ayanansha(d))*6/80) % 27;
	//print ("yoga=", yogaList[yogaindex])
	return yogaindex;
}

static int nround 
(long double x, int n){
	//round up for +
	//round down for -
	//n is a power of 10
	if (x == 0)
		return 0;
	else if (x > 0)
		return (int)floorl(x*n + 0.5)/n;
	else
		return (int)floorl(x*n - 0.5)/n;
}

static int tithi 
(long double d, long double div){
	//Calculate Tithi and Paksha
	long double mlon;
	long double slon;
	calculate_moon_sun_long (d, &mlon, &slon);
	long double fuzz = 0.03;
	int tithiindex = nround((rev(mlon - slon + fuzz)/div), 100000) % 30;
	//print ("Diff between Moons and Sun's longitudes =", mlon - slon)
	//print ("Index of diff between Moons and Sun's longitudes =", (mlon - slon)/12.0, " and index =", n)
	//print ("Tithi index is n=", n)
	return tithiindex;
}

static int karana 
(int dd, int mm, int yyyy, long double hr, long double tz){
	long double d = ts_at_mn (dd, mm, yyyy, tz) + (hr/24.0);
	int karindex = tithi (d, 6.0);
	if (karindex == 0) karindex = 10;
	else if (karindex >= 57) karindex -= 50;
	if (karindex > 0 && karindex < 57) 
		karindex = (karindex - 1) - (floorl((karindex-1)/7)*7);
	karindex = karindex % 11;
	//print ("karana=", karanaList[karindex])
	return karindex;
}

static int mrashi 
(int dd, int mm, int yyyy, long double hr, long double tz){
	long double mlon;
	long double slon;
	long double d = ts_at_mn (dd, mm, yyyy, tz) + (hr/24.0);
	calculate_moon_sun_long(d, &mlon, &slon);
	int rashindex = ((int)rev(mlon + ayanansha(d))/30) % 12;
	//print ("rashi=", rashiList[rashindex])
	return rashindex;
}

static int vaara 
(int dd, int mm, int yyyy, long double tz){
	long double d = ts_at_mn (dd, mm, yyyy, tz);
	d += (tz/24.0);
	int vaarindex = (int)((int)(d + 5) % 7);
	//print ("vaarindex = ", vaarindex)
	//print ("vaara=", vaaraList[vaarindex])
	return vaarindex;
}

static void vaaraday 
(int dd, int mm, int yyyy, long double tz, char* vaaraday){
	int vaarindex = vaara(dd, mm, yyyy, tz);
	strcpy(vaaraday, dayList[vaarindex]);
}

static long double fpart 
(long double x){
	long double y = x - floorl(x);
	if (y < 0) y += 1;
	return y;
}

static void tithiday 
(int dd, int mm, int yyyy, long double hr, long double tz, char* tithiday){
	long double timescale = ts_at_mn (dd, mm, yyyy, tz) + (hr/24.0);
	int t = tithi (timescale, 12);
	//print ("tithi index t =", t)
	strcpy(tithiday, (t <= 14)? "Shukla-": "Krishna-");
	strcat(tithiday, tithiList[t]);
}

static long double sun 
(long double latt, long double longt, long double zhr, int dd, int mm, int yyyy, Suntimes* times){
	long double srise;
	long double sset;
	long double ts = ts_at_mn (dd, mm, yyyy, zhr);
	long double julian_day = ts + 2451544; //2451543.5 + 12/24 (at noon)
	long double sunlightduration = sun_esrl (latt, longt, zhr, julian_day, &srise, &sset);
	//printf("day: sun - srise = %Lf\n", srise);
	if (sunlightduration != 24.0 && sunlightduration != 0.0) {
		times->sriseh =	(int)(srise);
		times->srisem =	(int)(fpart(srise)*60);
		times->srises =	(int)(nround(fpart(fpart(srise)*60)*60, 1000));
		times->sseth = (int)(sset);
		times->ssetm = (int)(fpart(sset)*60);
		times->ssets = (int)(nround(fpart(fpart(sset)*60)*60, 1000));
		times->dlh = (int)(sunlightduration);
		times->dlm = (int)(fpart(sunlightduration)*60);
		times->dls = (int)(nround(fpart(fpart(sunlightduration)*60)*60, 1000));
	}
	else {
		times->sriseh =	0;
		times->srisem =	0;
		times->srises =	0;
		times->sseth = 0;
		times->ssetm = 0;
		times->ssets = 0;
		times->dlh = 0;
		times->dlm = 0;
		times->dls = 0;
	}
	return srise;
}
