/*
 * geo.h    // Geographic navigation storage and processing library.
 *  ________   __  _____  ____  _____  ____    ___
 *  \  /\  /  /_/ /_  _/ /___/ /_  _/ / /\ \  / | |
 *   \/__\/  __    / /  /___    / /  / /_/_/ / /| |
 *    \  /  / /   / /  /___    / /  / /\ \  / /_| |
 *     \/  /_/   /_/  /___/   /_/  /_/  \/ /_/  |_|
 *
 *  Created on: 01.09.2010
 *      Author: A.Afanasiev
 */

#ifndef GEO_H_
#define GEO_H_

#include <stdint.h>
#include <time.h>
#include <math.h>
#ifndef M_PI	//in the case of using IAR
#	define M_PI		3.14159265358979323846
#endif


//TODO(?): define систем отсчета, с которыми мы хотим работать, а не хранить в непонятно каких, как сейчас.
//TODO: и переделать все упакованные градусы в радианы (может быть за исключением самих координат, хотя лучше и их).

// * * * * * * * * * * * * * * * * * * * * * * * * GEO * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

typedef enum _Tgeo_prec_type {//precision information level & information type
	GEOPREC_NONE,	// no info about precision
	GEOPREC_DOP,	// dilution of precision
	GEOPREC_SD,		// standard deviation
} Tgeo_prec_type;

typedef union _Tgeo_prec {
	float H;		// Horizontal precision (DOP or standard deviation)
	float V;		// Vertical precision (DOP or standard deviation)
	float T;		// Time precision (DOP or standard deviation)
	Tgeo_prec_type type;
} Tgeo_prec;

//-----------------------------------------------------------------------------------------
//| DOP | Rating    |          Description
//|-----|-----------|-----------------------------------------------------------------------------
//|1    |Ideal      |=O This is the highest possible confidence level to be used for applications demanding the highest possible precision at all times.
//|2-3  |Excellent  |=) At this confidence level, positional measurements are considered accurate enough to meet all but the most sensitive applications.
//|4-6  |Good       |:) Represents a level that marks the minimum appropriate for making business decisions. Positional measurements could be used to make reliable in-route navigation suggestions to the user.
//|7-8  |Moderate   |:| Positional measurements could be used for calculations, but the fix quality could still be improved. A more open view of the sky is recommended.
//|9-20 |Fair       |:( Represents a low confidence level. Positional measurements should be discarded or used only to indicate a very rough estimate of the current location.
//|21-50|Poor       |;( At this level, measurements are inaccurate by as much as 300 meters with a 6 meter accurate device (50 DOP ? 6 meters) and should be discarded.
//-----------------------------------------------------------------------------------------------
// PDOP^2 = HDOP^2 + VDOP^2; GDOP^2 = PDOP^2 + TDOP^2;

typedef enum _Tgeo_mode { //navigation information level
	GEOM_NONE = 0,	// no information about position & motion
	GEOM_2D,		// position - lat & lon; speeds of lat & lon in m/s, or azimuth & speed(m/s)
	GEOM_3D,		// position - lat & lon & alt; speeds of lat & lon & alt in m/s, or azimuth & inclination & speed(m/s)
} Tgeo_mode;

typedef enum _Tgeo_motion_type {//motion information type
	GEOMT_POL = 0,	// polar system
	GEOMT_XYZ,		// cartesian system
} Tgeo_motion_type;

typedef struct _Tgeo_motion {
	float lataz;	// latitude speed m/s or azimuth rad direction
	float loninc;	// longitude speed m/s or inclination rad direction
	float altspd;	// altitude speed m/s or speed m/s
	Tgeo_motion_type type;	// cartesian(xyz) or polar
	//Tgeo_motion_mode mode;	// NONE or 2D or 3D   - replaced by Tgeo_mode
} Tgeo_motion;

typedef struct _Tgeo_time {  //same as tm(from time.h), but with more compact types, more accuracy (milliseconds), and without unnecessary parameters
	uint8_t  sec	;//:8;	// Seconds: 0-61	// > 59 -> leap seconds
	uint8_t  min	;//:8;	// Minutes: 0-59
	uint8_t  hour	;//:8;	// Hours since midnight: 0-23
	uint8_t  mday	;//:8;	// Day of the month: 1-31
	uint8_t  mon	;//:8;	// Months *since* january: 0-11
	uint8_t  year	;//:8;	// Years since 2000	//TODO: century flag?
	uint16_t msec	;//:16;	// MilliSeconds: 0-999
} Tgeo_time;

typedef struct _Tgeo_sat {
	//uint32_t satugp;		// Set of Used GPS Satellites IDs
	//uint32_t satugl;		// Set of Used GLONASS Satellites IDs
	//uint8_t  satunumgp :6;	// Number of Used GPS Satellites
	//uint8_t  satunumgl :6;	// Number of Used GLONASS Satellites
	uint8_t  visible :8;	// Number of Visible Satellites
	uint8_t  decused :8;	// Number of Satellites Used in Decision
} Tgeo_sat;

//typedef struct _Tgeo_flags {
//		uint8_t v	:1;		// Validity of decision  - replaced by Tgeo_mode
//} Tgeo_flags;

typedef struct _Tgeo_pos {
	double lat;		// Latitude in +/-radians	(большинство приемников в бинарном виде отдают радианы, так же с радианами работают стандартные тригонометрич. функции)
	double lon;		// Longitude in +/-radians
	float alt;		// Altitude in meters
	//float mdcl;	// Magnetic variation radians (Easterly var. subtracts from true course)
	Tgeo_motion	motion;	// Motion
	Tgeo_time dtime;// Positional decision UTC time
	Tgeo_mode mode;	// NONE or 2D or 3D - mode & validity level
	Tgeo_prec prec;	// Precision
	Tgeo_sat sats;	// Satellites info
	//Tgeo_flags flags; //Decision flags  - replaced by Tgeo_mode
} Tgeo_pos;


typedef Tgeo_pos Tgeo;	//TODO: Tgeo_pos - position only, Tgeo - position+time


float geo_speed(const Tgeo_motion *motion);
float geo_course(const Tgeo_motion *motion);
time_t geo_time_t(const Tgeo_time *gtime);
float geo_PDOP(const Tgeo_prec *precision);
uint8_t geo_correctness(const Tgeo *geo);


void geo_time_t2geo(time_t loc_time, Tgeo_time *gtime);
void geo_motion2D_pol2geo(float course, float speed, Tgeo_motion *motion);
void geo_motion2D_cart2geo(float spd_lat, float spd_lon, Tgeo_motion *motion);
void geo_DOP2geo(float HDOP, float VDOP, float TDOP, Tgeo_prec *precision);


// * * * * * * * * * * * * * * * * * * * * * * Packed GEO  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// compact representation of geodata for storage, or transfer between platforms

#ifdef __IAR_SYSTEMS_ICC__
#pragma pack(push, _Struct, 1)
#endif
// отсчет времени:
//      ------------packed-sample-time-format------------
//byte#:|   0   |   1   |   2   |   3   |   4   |   5   | 6bytes
//bit#: | 0->7  | 8->15 |16->23 |24->31 |32->39 |40->47 |
//      |-----------------------------------------------|
//field:|  year     |mnt|mday|hour| min | sec |  msec   |
//      |-----------------------------------------------|
//bits: | 12        | 4 | 5  | 5  | 6   | 6   | 10      | 48bits
//      |-----------------------------------------------|
//min:  | 0000      |0  | 1  | 0  | 0   | 0   | 0       |
//max:  | 4095      |11 | 31 | 24 | 59  | 61* | 999     | * - GPS/GLONASS leap second
//      -------------------------------------------------
typedef struct _Ttime_packed {
	unsigned year	:12;	// Year (0...4095) (century available if >99)
	unsigned month	:4;		// Months *since* january: 0-11
	unsigned mday	:5;
	unsigned hour	:5;
	unsigned min	:6;
	unsigned sec	:6;
	unsigned msec	:10;
}
#ifndef __IAR_SYSTEMS_ICC__
__attribute__((__packed__))
#endif
Ttime_packed;

typedef struct _Tgeo_packed_pos//altitude? - at this time does not need, TODO later
{

	uint16_t HDOP;		// Horizontal dilution of precision (in 0.1 parts) (up to 6553.5) TODO: 8bit with logarithmic scale, or 16bit float
	uint16_t VDOP;		// Vertical dilution of precision (in 0.1 parts)
	uint16_t TDOP;		// Time dilution of precision (in 0.1 parts)
	//double PRMS_err;	// Position rms error
	uint16_t speed;		// Speed in 0.01m/s (0.00m/s...655.35m/s)(0cm/s...65535cm/s)
	uint16_t direction;	// Track angle in 0.01degrees (0.00degr...359.99degr)
	int16_t alt;		// Altitude in 0.1m (-3276.8m...3276.7m)
	int32_t lat;		// Latitude LSB cost is 0.000'000'1degree (10^-7), -90...90degree (-900000000...900000000)
	int32_t lon;		// Longitude LSB cost is 0.000'000'1degree (10^-7), -180...180degree (-1800000000...1800000000)
}
#ifndef __IAR_SYSTEMS_ICC__
__attribute__((__packed__))
#endif
Tgeo_packed_pos;


typedef struct _Tgeo_packed_sat {
	//uint32_t satugp;	// Set of Used GPS Satellites IDs
	//uint32_t satugl;	// Set of Used GLONASS Satellites IDs
	//uint8_t  satunumgp;	// Number of Used GPS Satellites
	//uint8_t  satunumgl;	// Number of Used GLONASS Satellites
	uint8_t  visible :8;	// Number of Visible Satellites
	uint8_t  decused :8;	// Number of Satellites Used in Decision
}
#ifndef __IAR_SYSTEMS_ICC__
__attribute__((__packed__))
#endif
Tgeo_packed_sat;

typedef struct _Tgeo_packed_flags {
		uint8_t mode	:2;		// Mode & validity of decision (0-invalid, 1-2D, 2-3D)
}
#ifndef __IAR_SYSTEMS_ICC__
__attribute__((__packed__))
#endif
Tgeo_packed_flags;

typedef struct _Tgeo_packed {
	Tgeo_packed_pos pos;	// Position
	Ttime_packed timedcs;	// Positional decision UTC time
	Tgeo_packed_flags flags;
	Tgeo_packed_sat sats;	// Satellites used to decision
}
#ifndef __IAR_SYSTEMS_ICC__
__attribute__((__packed__))
#endif
Tgeo_packed;


#ifdef __IAR_SYSTEMS_ICC__
#pragma pack(pop, _Struct)
#endif


int32_t geo_pack_coordinate(double latorlon);
double geo_unpack_coordinate(int32_t latorlon);
uint16_t geo_pack_speed(float speed);
uint16_t geo_pack_direction(float direction);


void geo_pack(Tgeo_packed *pgeo, const Tgeo_pos *pos);
void geo_unpack(Tgeo_pos *pos, const Tgeo_packed *pgeo);



// * * * * * * * * * * * * * * * * * * * * * * MEASUREMENT SYSTEMS * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// - - - Angle: - - -
#define RAD2DEGR(rad)	((rad) * 180.0 / M_PI)
#define DEGR2RAD(degr)	((degr) * M_PI / 180.0)

//#define RAD2DEGRMIN(rad)	((rad) * )

// - - - Speed: - - -
float geospeed2kmh(float speed);
float knots2geospeed(float speed_knots);



// * * * * * * * * * * * * * * * * * * * * * * * * * * * DEBUG * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#ifdef __OAT_API_VERSION__	// compiler predefined symbol
//#	define GEOF_CRD	"%-+10.5f"//coordinates format str
//#	define GEOF_ALT	"%-+8.2f"//altitude format str
#else
#	define GEOF_CRD	"%-+10.5f"//coordinates format str
#	define GEOF_ALT	"%-+8.2f"//altitude format str
#endif

#define GEOF_POS		"La" GEOF_CRD " Lo" GEOF_CRD " Alt" GEOF_ALT
#define GEOI_POS(gpos)	RAD2DEGR((gpos)->lat), RAD2DEGR((gpos)->lon), (gpos)->alt

#define GEOF_TIME	"%2hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%04u"
#define GEOI_TIME(dtime)	(dtime)->hour, (dtime)->min, (dtime)->sec, (dtime)->msec, (dtime)->mday, (dtime)->mon + 1, (dtime)->year + 2000

#define GEOF			GEOF_TIME " " GEOF_POS
#define GEOI(geo)		GEOI_TIME(&((geo)->dtime)), GEOI_POS(geo)

/*
void geo_time_print(const Tgeo_time *dtime);
void geo_pos_print(const Tgeo_pos *geo);

#define DEBUG_PRINT_GEOTIME(topic, level, dtime)	\
	if(DBGCONDITION(topic, level)) geo_time_print(dtime)

#define DEBUG_PRINT_GEOPOS(topic, level, geopos)	\
	if(DBGCONDITION(topic, level)) geo_pos_print(geopos)
*/

static inline int geo_crd_strn(char *str, int n, double latlon);
static inline int geo_alt_strn(char *str, int n, float alt);


//#define DEBUG_GEO(topic, level, pgeo)	DEBUGF(topic, level, GEOF CRLF, GEOI(pgeo))
void geo_print(const Tgeo *geo);
#define DBGPRINTF_GEO(topic, level, pgeo)	if(DBGCONDITION(topic, level)){ geo_print(pgeo); }






//--------------inline-implementations----------------
int geo_dbl_strn(char *str, int n, double a, int prec);
static inline int geo_crd_strn(char *str, int n, double latlon) {
	return geo_dbl_strn(str, n, RAD2DEGR(latlon), 6);
}
static inline int geo_alt_strn(char *str, int n, float alt) {
	return geo_dbl_strn(str, n, alt, 1);
}


#endif /* GEO_H_ */
