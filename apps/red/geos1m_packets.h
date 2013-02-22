/*
 * geos1m_packets.h
 *  ________   __  _____  ____  _____  ____    ___
 *  \  /\  /  /_/ /_  _/ /___/ /_  _/ / /\ \  / | |
 *   \/__\/  __    / /  /___    / /  / /_/_/ / /| |
 *    \  /  / /   / /  /___    / /  / /\ \  / /_| |
 *     \/  /_/   /_/  /___/   /_/  /_/  \/ /_/  |_|
 *
 *  Created on: 01.09.2010
 *      Author: A.Afanasiev
 */


#ifndef GEOS1M_PACKETS_H_
#define GEOS1M_PACKETS_H_

#include <stdint.h>


#ifdef __IAR_SYSTEMS_ICC__
#pragma pack(push, _Struct, 1)
#endif



// ----------------------------- 0x20 GEOGRAPHIC -------------------------------
typedef struct _Tgeos1m_packet_geographic {
	double time_s;	//seconds UTC from 00:00:00 01.01.2008
	double lat;	//in rad
	double lon;	//in rad
	double alt;	//over ellipsoid in meters
	double geoid;	//ellipsoid over geoid (?)
	int32_t SATs;	//SATs number in decision
	int32_t res1;	//reserved
	double GDOP;
	double PDOP;
	double TDOP;
	double HDOP;
	double VDOP;
	double todo1;	//TODO
	double todo2;	//TODO
	double todo3;	//TODO
	int32_t noreliable;//reliability of decision
	int32_t res2;	//reserved
	double speed;	//in m/s
	double course;	//in rad
}
#ifndef __IAR_SYSTEMS_ICC__
__attribute__((__packed__))
#endif
Tgeos1m_packet_geographic;


// ----------------------------- 0x21 TELEMETRY -------------------------------
typedef struct _Tgeos1m_packet_cur_telemetry {
	uint32_t status;	//receiver status word
	uint32_t uptime;	//current seconds count after last restart of receiver
	uint16_t satsdec;	//satellites in decision
	uint16_t sats;		//satellites in tracking
}
#ifndef __IAR_SYSTEMS_ICC__
__attribute__((__packed__))
#endif
Tgeos1m_packet_cur_telemetry;

typedef struct _Tgeos1m_packet_cur_telemetry_status {	//receiver status word bit field
	uint8_t :1;			//0) reserved
	uint8_t :1;			//1) reserved
	uint8_t dec :1;		//2) 0-no decision, 1-decision is present
	uint8_t decmode :1;	//3) 0-3D, 1-2D
	uint8_t :1;			//4) reserved
	uint8_t both :1;	//5) 0-one NAV system, 1-both NAV systems (GPS+GLONASS) in decision
	uint8_t :1;			//6) reserved
	uint8_t :1;			//7) reserved
	uint8_t alm :1;		//8) 0-almanac not available, 1-almanac available
	uint8_t :1;			//9) reserved
	uint8_t PPS :1;		//10) 0-PPS is not reliable, 1-PPS is reliable
	uint8_t exp :1;		//11) 0-extrapolation impossible, 1-extrapolation is possible
	uint8_t decns :2;	//12,13) decision on NAV systems: 0-combined(GPS+GLONASS), 1-GPS only, 2-GLONASS only
	uint8_t alw2D :1;	//14) 0-2D not allowed, 0-2D allowed,
	uint8_t fixpos :1;	//15) 0-not fixed coordinates mode, 1-fixed coordinates mode
	uint8_t expalw :1;	//16) 0-extrapolation not allowed, 1-extrapolation allowed
	uint8_t :1;			//17) reserved
	uint8_t dynflt :1;	//18) 0-dynamic filter not allowed, 1-dynamic filter allowed
	uint8_t :1;			//19) reserved
	uint8_t decrate :1;	//20) decision rate: 0-5Hz, 1-1Hz
	uint8_t :1;			//21) reserved
	uint8_t antv :1;	//22) antenna voltage: 0-not normal, 1-normal
	uint8_t synt :1;	//23) 0-state not normal, 1- state in normal
	uint8_t expd :1;	//24) 0-coordinates as-is, 1-coordinates extrapolated
	uint8_t :1;			//25) reserved
	uint8_t dynfltd :1;	//26) 0-dynamic filter not use, 1-dynamic filter in use
	uint8_t rcvid :1;	//27) receiver id: 0-GeoS-1, 1-GeoS-1M
	uint8_t rcvmod :2;	//28,29) receiver mode: 0-normal, 1-test, 2-firmware upgrade
	uint8_t :1;			//30) reserved
	uint8_t :1;			//31) reserved
}
#ifndef __IAR_SYSTEMS_ICC__
__attribute__((__packed__))
#endif
Tgeos1m_packet_cur_telemetry_status;
















#ifdef __IAR_SYSTEMS_ICC__
#pragma pack(pop, _Struct)
#endif






#endif //GEOS1M_PACKETS_H_

