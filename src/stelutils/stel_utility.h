/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _S_UTILITY_H_
#define _S_UTILITY_H_

#include <iostream>
#include <sstream>
#include <string>
#include <cmath>
#include <map>
#include "vecmath.h"

using namespace std;

typedef std::map< std::string, std::string > stringHash_t;
typedef stringHash_t::const_iterator stringHashIter_t;
typedef std::map< std::string, std::wstring > wstringHash_t;
typedef wstringHash_t::const_iterator wstringHashIter_t;

class StelUtility {
public:

	//! Dummy wrapper used to remove a boring warning when using strftime directly
	static size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
	{
		return strftime(s, max, fmt, tm);
	}

	//! @brief Convert an angle in hms format to radian
	//! @param h hour component
	//! @param m minute component
	//!	@param s second component
	//! @return angle in radian
	static double hms_to_rad(unsigned int h, unsigned int m, double s);
			   
	//! @brief Convert an angle in dms format to radian
	//! @param d degree component
	//! @param m arcmin component
	//!	@param s arcsec component
	//! @return angle in radian
	static double dms_to_rad(int d, int m, double s);	  

	//! @brief Obtains a Vec3f from a string
	//! @param s the string describing the Vector with the form "x,y,z"
	//! @return The corresponding vector
	static Vec3f str_to_vec3f(const string& s);
	
	//! @brief Obtains a string from a Vec3f 
	//! @param v The vector
	//! @return the string describing the Vector with the form "x,y,z"
	static string vec3f_to_str(const Vec3f& v);
	
	//! @brief Print the passed angle with the format ddÂ°mm'ss(.ss)"
	//! @param angle Angle in radian
	//! @param decimal Define if 2 decimal must also be printed
	//! @param useD Define if letter "d" must be used instead of Â°
	//! @return The corresponding string
	static wstring printAngleDMS(double angle, bool decimals = false, bool useD = false);
	
	//! @brief Print the passed angle with the format +hh:mm:ss(.ss)"
	//! @param angle Angle in radian
	//! @param decimals Define if 2 decimal must also be printed
	//! @return The corresponding string
	static wstring printAngleHMS(double angle, bool decimals = false);
		
	//! @brief Convert UTF-8 std::string to std::wstring using mbstowcs() function
	//! @param The input string in UTF-8 format
	//! @return The matching wide string
	static wstring stringToWstring(const string& s);
	
	//! @brief Convert std::wstring to UTF-8 std::string using wcstombs() function
	//! @param The input wide character string
	//! @return The matching string in UTF-8 format
	static string wstringToString(const wstring& ws);
	
	//! @brief Format the double value to a wstring (with current locale)
	//! Can't use directly wostringstream because it is not portable to MinGW32/STLPort..
	//! @param d The input double value
	//! @return The matching wstring
	static wstring doubleToWstring(double d);
	
	//! @brief Format the int value to a wstring (with current locale)
	//! Can't use directly wostringstream because it is not portable to MinGW32/STLPort..
	//! @param i The input int value
	//! @return The matching wstring
	static wstring intToWstring(int i);
};

double hms_to_rad(unsigned int h, double m);
double dms_to_rad(int d, double m);


void sphe_to_rect(double lng, double lat, Vec3d& v);
void sphe_to_rect(double lng, double lat, double r, Vec3d& v);
void sphe_to_rect(float lng, float lat, Vec3f& v);
void rect_to_sphe(double *lng, double *lat, const Vec3d& v);
void rect_to_sphe(float *lng, float *lat, const Vec3f& v);

/* Obtains Latitude, Longitude, RA or Declination from a string. */
double get_dec_angle(const string&);


// Provide the luminance in cd/m^2 from the magnitude and the surface in arcmin^2
float mag_to_luminance(float mag, float surface);

// convert string int ISO 8601-like format [+/-]YYYY-MM-DDThh:mm:ss (no timzone offset)
// to julian day
int string_to_jday(string date, double &jd);

#include <time.h>

#ifndef J2000
# define J2000 2451545.0
#endif

// General Calendar Functions

// Human readable (easy printf) date format
typedef struct
{
	int years; 		/*!< Years. All values are valid */
	int months;		/*!< Months. Valid values : 1 (January) - 12 (December) */
	int days; 		/*!< Days. Valid values 1 - 28,29,30,31 Depends on month.*/
	int hours; 		/*!< Hours. Valid values 0 - 23. */
	int minutes; 	/*!< Minutes. Valid values 0 - 59. */
	double seconds;	/*!< Seconds. Valid values 0 - 59.99999.... */
}ln_date;


/* Calculate the julian day from date.*/
double get_julian_day(const ln_date * date);

/* Calculate the date from the julian day. */
void get_date(double JD, ln_date * date);

/* Calculate day of the week. 0 = Sunday .. 6 = Saturday */
unsigned int get_day_of_week (const ln_date *date);
	
/* Calculate julian day from system time. */
double get_julian_from_sys();

/* Calculate gmt date from system date */
void get_ln_date_from_sys(ln_date * date);

// Obtains a ln_date from 2 strings s1 and s2 for date and time
// with the form dd/mm/yyyy for s1 and hh:mm:ss.s for s2.
// Returns NULL if s1 or s2 is not valid.
// Uses the current date if s1 is "today" and current time if s2 is "now"
const ln_date * str_to_date(const char * s1, const char * s2);

// Calculate tm struct from julian day
void get_tm_from_julian(double JD, struct tm * tm_time);

// Calculate time_t from julian day
time_t get_time_t_from_julian(double JD);


//! Return the number of hours to add to gmt time to get the local time at time JD
//! taking the parameters from system. This takes into account the daylight saving
//! time if there is. (positive for Est of GMT)
float get_GMT_shift_from_system(double JD, bool _local=0);

//! Return the time zone name taken from system locale
wstring get_time_zone_name_from_system(double JD);

//! Return the time in ISO 8601 UTC format that is : %Y-%m-%d %H:%M:%S
string get_ISO8601_time_UTC(double JD);

double str_to_double(string str);

// always positive
double str_to_pos_double(string str);

int str_to_int(string str);
int str_to_int(string str, int default_value);

string double_to_str(double dbl);
long int str_to_long(string str);

int fcompare(const string& _base, const string& _sub);
int fcompare(const wstring& _base, const wstring& _sub);

#endif
