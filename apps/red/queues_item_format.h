/* -*- mode: C; coding: utf8 -*-
 * queues_item_format.h
 *
 *  Created on: 06.11.2010
 *      Author: Alexander Afanasiev <afalsoft[at]gmail.com>
 */

#ifndef QUEUES_ITEM_FORMAT_H_
#define QUEUES_ITEM_FORMAT_H_

#include "geo.h"


#ifdef __IAR_SYSTEMS_ICC__
#pragma pack(push, _Struct, 1)
#endif


#define ALARM_SRC_WRD		1	// тревога - проводная кнопка
#define ALARM_SRC_KEYFOB	2	// тревога - брелок
#define ALARM_SRC_PV		3	// тревога - кнопка на пульте


typedef struct _Tqitem_alarm{
	Tgeo_packed pgeo;	//alarm event position & time
	uint8_t src;		//source of alarm (1-Alarm, 2-RadioAlarm, 3-PVAlarm, ...)
}
#ifndef __IAR_SYSTEMS_ICC__
__attribute__((__packed__))
#endif
Tqitem_alarm;


typedef struct _Tqitem_rfid{
	uint32_t data;		//RFID data (RFID id number, telemetry)
	uint8_t status;		//1-enter the zone, 0-left the zone
}
#ifndef __IAR_SYSTEMS_ICC__
__attribute__((__packed__))
#endif
Tqitem_rfid;




#ifdef __IAR_SYSTEMS_ICC__
#pragma pack(pop, _Struct)
#endif


#endif /* QUEUES_ITEM_FORMAT_H_ */
