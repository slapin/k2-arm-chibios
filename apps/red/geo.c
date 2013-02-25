/*
 * geo.c    // Geographic navigation storage and processing library.
 *  ________   __  _____  ____  _____  ____    ___
 *  \  /\  /  /_/ /_  _/ /___/ /_  _/ / /\ \  / | |
 *   \/__\/  __    / /  /___    / /  / /_/_/ / /| |
 *    \  /  / /   / /  /___    / /  / /\ \  / /_| |
 *     \/  /_/   /_/  /___/   /_/  /_/  \/ /_/  |_|
 *
 *  Created on: 01.09.2010
 *      Author: A.Afanasiev
 */

#include <string.h>
#include "ch.h"
#include "hal.h"
#include "geo.h"
#include "gnss.h"

extern status_gnss_t status_gnss;

// * * * * * * * * * * * * * * * * * * * * * * * * GEO * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
float geo_speed(const Tgeo_motion *motion){	// returns m/s
	//TODO учет mode&type
	return motion->altspd;
}

float geo_course(const Tgeo_motion *motion){	//Horizontal direction - azimuth
	//TODO учет mode&type
	return motion->lataz;
}

time_t geo_time_t(const Tgeo_time *gtime){
	struct tm tim;
	memset(&tim, 0x00, sizeof(tim));
	tim.tm_sec = gtime->sec;
	tim.tm_min = gtime->min;
	tim.tm_hour = gtime->hour;
	tim.tm_mday = gtime->mday;
	tim.tm_mon = gtime->mon;
	tim.tm_year = gtime->year + 100;
	return mktime(&tim);
}

float geo_PDOP(const Tgeo_prec *precision){
	//TODO: учет precision type
	float HDOP = precision->H;
	float VDOP = precision->V;
	return sqrt(HDOP*HDOP + VDOP*VDOP);
}

uint8_t geo_correctness(const Tgeo *geo){
	//TODO: more accuracy correctness detect

	return geo_PDOP(&geo->prec) < 20 &&
		geo_PDOP(&geo->prec) > 0 &&
		status_gnss.valid;
}

void geo_time_t2geo(time_t loc_time, Tgeo_time *gtime){
	struct tm *gmt = gmtime(&loc_time);
	gtime->msec = 0;
	gtime->sec = gmt->tm_sec;
	gtime->min = gmt->tm_min;
	gtime->hour = gmt->tm_hour;
	gtime->mday = gmt->tm_mday;
	gtime->mon = gmt->tm_mon;
	gtime->year = gmt->tm_year;
}

// course - course in radians
//  speed - speed in m/s
void geo_motion2D_pol2geo(float course, float speed, Tgeo_motion *motion){
	motion->lataz = course;
	motion->altspd = speed;
	motion->type = GEOMT_POL;
	//motion->mode = GEOMM_2D;
}

//spd_lat - latitude speed in m/s
//spd_lon - longitude speed in m/s
void geo_motion2D_cart2geo(float spd_lat, float spd_lon, Tgeo_motion *motion){
	//TODO: не конвертировать в polar, а сохранять как есть. Написать функции конвертации Tgeo_motion из cart в polar и обратно.
	float spd, dir;

	spd = sqrt(spd_lat * spd_lat + spd_lon * spd_lon);

	if(spd){
		//float spd_lon_norm = spd_lon / spd;
		float spd_lat_norm = spd_lat / spd;	//TODO: check lat/lon accordance
		dir = acos(spd_lat_norm);
		if(spd_lon < 0)
			dir = 2 * M_PI - dir;
	}else
		dir = 0;

	geo_motion2D_pol2geo(dir, spd, motion);
}


void geo_DOP2geo(float HDOP, float VDOP, float TDOP, Tgeo_prec *precision){
	precision->H = HDOP;
	precision->V = VDOP;
	precision->T = TDOP;
	precision->type = GEOPREC_DOP;
}


// * * * * * * * * * * * * * * * * * * * * * * Packed GEO  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#define COORD_MULTIPLIER	(10000000.0 * (180.0 / M_PI))// 10^7
#define SPEED_MULTIPLIER	(100.0)// 10^2
#define DIR_MULTIPLIER		(100.0 * 180.0 / M_PI)
#define DOP_MULTIPLIER		(10.0)// 10^2
#define	knots2ms(knots)		((knots) * 0.514444444444444444444444)
#define fround(a)			floor(a)//floor((a + 0.5)) - what if a is negative?
#define abs(a)				(uint32_t)fabs(a)	//TODO: use labs() for int


int32_t geo_pack_coordinate(double latorlon){
	return (int32_t)fround(latorlon * COORD_MULTIPLIER);
}
double geo_unpack_coordinate(int32_t latorlon){
	return (double)latorlon / COORD_MULTIPLIER;
}
//void geo_pack_motion_2Dpolar_knots(Tgeo_packed_pos *ppos, float spdknots, float direction){
//	ppos->speed = (uint16_t)fround(knots2ms(spdknots) * SPEED_MULTIPLIER);
//	ppos->direction = (uint16_t)fround(direction * DIR_MULTIPLIER);
//}
uint16_t geo_pack_speed(float speed){
	return (uint16_t)fround(speed * SPEED_MULTIPLIER);
}
uint16_t geo_pack_direction(float direction){
	return (uint16_t)fround(direction * DIR_MULTIPLIER);
}

void geo_pack_motion(Tgeo_packed_pos *ppos, const Tgeo_motion *motion){
	ppos->speed = geo_pack_speed(geo_speed(motion));
	ppos->direction = geo_pack_direction(geo_course(motion));
}
void geo_unpack_motion(Tgeo_motion *motion, const Tgeo_packed_pos *ppos){
	geo_motion2D_pol2geo((float)ppos->direction / DIR_MULTIPLIER, (float)ppos->speed / SPEED_MULTIPLIER, motion);
}

void geo_pack_precision(Tgeo_packed_pos *ppos, const Tgeo_prec *precision){//TODO: учет precision type
	if(precision->H > 6553.5)
		ppos->HDOP = 65535;
	else
		ppos->HDOP = (uint16_t)fround(precision->H * DOP_MULTIPLIER);
	
	if(precision->V > 6553.5)
		ppos->VDOP = 65535;
	else
		ppos->VDOP = (uint16_t)fround(precision->V * DOP_MULTIPLIER);
	
	if(precision->T > 6553.5)
		ppos->TDOP = 65535;
	else
		ppos->TDOP = (uint16_t)fround(precision->T * DOP_MULTIPLIER);
}
void geo_unpack_precision(Tgeo_prec *precision, const Tgeo_packed_pos *ppos){
	geo_DOP2geo((float)ppos->HDOP / DOP_MULTIPLIER, (float)ppos->VDOP / DOP_MULTIPLIER, (float)ppos->TDOP / DOP_MULTIPLIER, precision);
}

void geo_pack_pos(Tgeo_packed_pos *ppos, const Tgeo_pos *pos){
	ppos->lat = geo_pack_coordinate(pos->lat);
	ppos->lon = geo_pack_coordinate(pos->lon);
	float alt = pos->alt;
	if(alt > 3276.7)
		ppos->alt = 32767;
	else if(alt < -3276.8)
		ppos->alt = -32768;
	else
		ppos->alt = (int16_t)fround(alt * 10);
	geo_pack_motion(ppos, &pos->motion);
	geo_pack_precision(ppos, &pos->prec);
}

void geo_unpack_pos(Tgeo_pos *pos, const Tgeo_packed_pos *ppos){
	pos->lat = geo_unpack_coordinate(ppos->lat);
	pos->lon = geo_unpack_coordinate(ppos->lon);
	pos->alt = (float)ppos->alt / 10.0;
	geo_unpack_motion(&pos->motion, ppos);
	geo_unpack_precision(&pos->prec, ppos);
}



static void time_pack(Ttime_packed *ptime, const Tgeo_time *gtime){
	ptime->year	= gtime->year + 2000;	// Year (0...4095) (century available if >99)	TODO: correct century conversion
	ptime->month = gtime->mon;			// Months *since* january: 0-11
	ptime->mday	= gtime->mday;
	ptime->hour = gtime->hour;
	ptime->min = gtime->min;
	ptime->sec = gtime->sec;
	ptime->msec = gtime->msec;
}

static void time_unpack(Tgeo_time *gtime, const Ttime_packed *ptime){
	int year = ptime->year;
	if(year > 99)
		year -= 2000;
	gtime->year = year;
	gtime->mon = ptime->month;
	gtime->mday = ptime->mday;
	gtime->hour = ptime->hour;
	gtime->min = ptime->min;
	gtime->sec = ptime->sec;
	gtime->msec = ptime->msec;
}

static void sats_pack(Tgeo_packed_sat *psat, const Tgeo_sat *sat){
	//psat->satugp = sat->satugp;
	//psat->satugl = sat->satugl;
	//psat->satunumgp = sat->satunumgp;
	//psat->satunumgl = sat->satunumgl;
	(*psat).visible = (*sat).visible;
	(*psat).decused = (*sat).decused;
}

static void sats_unpack(Tgeo_sat *sat, const Tgeo_packed_sat *psat){
	//sat->satugp = psat->satugp;
	//sat->satugl = psat->satugl;
	//sat->satunumgp = psat->satunumgp;
	//sat->satunumgl = psat->satunumgl;
	(*sat).visible = (*psat).visible;
	(*sat).decused = (*psat).decused;
}


void geo_pack(Tgeo_packed *pgeo, const Tgeo *pos){
	geo_pack_pos(&pgeo->pos, pos);
	time_pack(&pgeo->timedcs, &pos->dtime);
	sats_pack(&pgeo->sats, &pos->sats);
	pgeo->flags.mode = pos->mode;
}

void geo_unpack(Tgeo *pos, const Tgeo_packed *pgeo){
	geo_unpack_pos(pos, &pgeo->pos);
	time_unpack(&pos->dtime, &pgeo->timedcs);
	sats_unpack(&pos->sats, &pgeo->sats);
	pos->mode = (Tgeo_mode)pgeo->flags.mode;
}


// * * * * * * * * * * * * * * * * * * * * * * MEASUREMENT SYSTEMS * * * * * * * * * * * * * * * * * * * * * * * * * * * *

float geospeed2kmh(float speed){
	return speed * 3.6;
}
float knots2geospeed(float speed_knots){
	return knots2ms(speed_knots);
}




// * * * * * * * * * * * * * * * * * * * * * * * * * * * DEBUG * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#include <stdio.h>


static const double pwr10[] = {
	1.0,
	10.0,
	100.0,
	1000.0,
	10000.0,
	100000.0,
	1000000.0,
	10000000.0,
	100000000.0,
	1000000000.0,
	10000000000.0,
	100000000000.0,
	1000000000000.0, //10^12, 0.5*10^-12
	//10000000000000.0,
	//100000000000000.0,
	//1000000000000000.0,
	//10000000000000000.0, //10^16, 0.5*10^-16
};

int geo_dbl_strn(char *str, int n, double a, int prec) {//prec - precision (digits after'.')
	if(prec) {
		double f, i;
		f = modf(a/* + frac10[prec] what if the number is negative?*/, &i) * pwr10[prec];// <latitude or longitude>
		return snprintf(str, n, "%d.%0*d", (int)i, prec, (int)fabs(f));
	} else {
		return snprintf(str, n, "%d", (int)a);
	}
}



// ti:me:00.000 da.te.2000 Lalat.itude Lolong.itude ^alt.itude Mmtp P[]
// ti:me:00.000 da.te.2000 lat.itudee/long.itudee ^alt.itude ~N
//                                             or~2C12/5 (lat/lon) or~3C12/5/8 (lat/lon/alt)
//                                             or~2P12/132 (spd/az) or~3P12/132/15  (spd/az/incl)
// ti:me:00.000 da.te.2000 lat.itudee/long.itudee ^alt.itude *2.5/113 Dp6.8
void geo_debug_strn(char *str, int str_size, const Tgeo *geo)
{
	// в следствие нерабочести %f под open_at, используем другие способы печати float
	//Time:
	if(str_move_ptr(&str, &str_size, snprintf(str, str_size, "" GEOF_TIME " ", GEOI_TIME(&geo->dtime))))
		return;
	//Position:
	if(str_move_ptr(&str, &str_size, geo_dbl_strn(str, str_size, RAD2DEGR(geo->lat), 6)))
		return;
	if(str_move_ptr(&str, &str_size, snprintf(str, str_size, "/")))//Lo")))
		return;
	if(str_move_ptr(&str, &str_size, geo_dbl_strn(str, str_size, RAD2DEGR(geo->lon), 6)))
		return;
	if(str_move_ptr(&str, &str_size, snprintf(str, str_size, " ^")))
		return;
	if(str_move_ptr(&str, &str_size, geo_dbl_strn(str, str_size, geo->alt, 1)))
		return;

	//Motion:
	if(str_move_ptr(&str, &str_size, snprintf(str, str_size, " *")))
		return;
	if(str_move_ptr(&str, &str_size, geo_dbl_strn(str, str_size, geo_speed(&geo->motion), 1)))
		return;
	if(str_move_ptr(&str, &str_size, snprintf(str, str_size, "/")))
		return;
	//if(str_move_ptr(&str, &str_size, geo_dbl_strn(str, str_size, RAD2DEGR(geo_course(&geo->motion)), 0)))
	if(str_move_ptr(&str, &str_size, snprintf(str, str_size, "%03d", (int)(float)RAD2DEGR(geo_course(&geo->motion)))))
		return;

	//Precision:
	if(str_move_ptr(&str, &str_size, snprintf(str, str_size, " Dp")))
		return;
	if(str_move_ptr(&str, &str_size, geo_dbl_strn(str, str_size, geo_PDOP(&geo->prec), 1)))
		return;

	//Mode:
	static const char * const mode[] = {
		[GEOM_NONE] = " nv",	//no validity
		[GEOM_2D] = " 2D",		//validly, 2D mode
		[GEOM_3D] = " 3D",		//validly, 3D mode
	};
	
	if(str_move_ptr(&str, &str_size, snprintf(str, str_size, mode[geo->mode])))
		return;

	//Satellites:
	if(str_move_ptr(&str, &str_size, snprintf(str, str_size, " s")))
		return;
	if(str_move_ptr(&str, &str_size, snprintf(str, str_size, "%d", (int)geo->sats.visible)))
		return;
	if(str_move_ptr(&str, &str_size, snprintf(str, str_size, ",")))
		return;
	if(str_move_ptr(&str, &str_size, snprintf(str, str_size, "%d", (int)geo->sats.decused)))
		return;

}


void geo_print(const Tgeo *geo) {
	char str[80];//40];
	geo_debug_strn(str, sizeof(str), geo);
//	DBGPRINTF(0,0, "%s", str);
}

