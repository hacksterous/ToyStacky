#ifndef __DAY_H__
#define __DAY_H__

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

long double sun (long double latt, long double longt, long double zhr, int dd, int mm, int yyyy, Suntimes* times);
void tithiday (int dd, int mm, int yyyy, long double hr, long double tz, char* tithiday);
void vaaraday (int dd, int mm, int yyyy, long double tz, char* vaaraday);
void nakshatraday (int dd, int mm, int yyyy, long double hr, long double tz, char* nakshatraday);

#endif
