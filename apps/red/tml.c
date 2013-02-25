/*
 * tml.c
 *  ________   __  _____  ____  _____  ____    ___
 *  \  /\  /  /_/ /_  _/ /___/ /_  _/ / /\ \  / | |
 *   \/__\/  __    / /  /___    / /  / /_/_/ / /| |
 *    \  /  / /   / /  /___    / /  / /\ \  / /_| |
 *     \/  /_/   /_/  /___/   /_/  /_/  \/ /_/  |_|
 *
 *  Created on: 09.08.2010
 *      Author: Alexander_Afanasiev
 */


#ifndef __OAT_API_VERSION__	// compiler predefined symbol
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "tml.h"
// #include "debug.h"
//#include "config.h"
#include "geo.h"
#define DEBUG_ISON 1
#define PTR_F		"%0*X:%0*X"
#define PTR_SIZE			((unsigned int)sizeof(void*))
#define PTR_HALFSIZE_BITS	((unsigned int)(sizeof(void*) * 4))
#define PTR_I(ptr)	PTR_SIZE, ((unsigned int)(ptr) >> PTR_HALFSIZE_BITS), PTR_SIZE, ((unsigned int)(ptr) & PTR_HALFLO_MASK)
#define PTR_HALFLO_MASK		((unsigned int)(((unsigned int)1 << PTR_HALFSIZE_BITS) - 1))

const uint16_t TmlTagSize[256] = {
		[TML_LAT] = 4,
		[TML_LON] = 4,
		[POSDECISION] = 1,
		[POSDIRECTION] = 2,
		[POSSPEED] = 2,
		[POSSATEL] = 1,
		[POSSATELDECISION] = 1,
		[DEVICEID] = 4,
		[TIMEBEGIN] = 4,
		[TIMEEND] = 4,
		[ITEMSCOUNT] = 4,
		[DEVICEUPTIME] = 4,
		[DEVICEGNSS] = 1,
		[DEVICEGSM] = 1,
		[DEVICEGPS] = 1,
		[DEVICESEKOP] = 1,
		[DEVICETETRA] = 1,
		[DEVICEETH] = 1,
		[DEVICERS485] = 1,
		[ADCDATA] = 2,
		[KMZDATA] = 6,
		[KMZERRORCODE] = 2,
		[PROTOCOLVERSION] = 2,
		[ALARM] = 1,
		[RADIOALARM] = 4,
		[RFIDDATA] = 4,
		[RFIDSTATUS] = 1,
		[SERIALNUMBER] = 4,
		[MODVERSION] = 2,
		[GEOPDOP] = 1,
};


// TODO: добавить во все функции пишущие по указателям ограничение на кол-во записываемых данных (как snprintf)






// ================================ TML network methods =============================

//TODO: проверять не просто TML_ENDIAN_BIG а его отличие от endiannes системы
#ifdef TML_ENDIAN_BIG
u16 htotmls(u16 data){
    return (u16)((data >> 8) | ((data & 0x00ff) << 8));
}
u16 tmltohs(u16 ndata){
    return htotmls(ndata);
}
u32 tmltohl(u32 ndata){
    return (u32)(tmltohs(ndata>>16) | (tmltohs((u16) ndata & 0xffff) << 16));
}
u32 htotmll(u32 data){
    return tmltohl(data);
}
void memcpy_htotml(void *dest, const void *src, int size){
	char *_dest = dest;
	char *_src = (char*)src + size;
	while(size --)
		*(_dest++) = *(--_src);
}
#else
u16 htotmls(u16 data){    return data;}
u16 tmltohs(u16 ndata){    return ndata;}
u32 tmltohl(u32 ndata){    return ndata;}
u32 htotmll(u32 data){    return data;}
void memcpy_htotml(void *dest, const void *src, int size){	memcpy(dest, src, size);}
#endif













uint16_t tml_version(uint8_t ver, uint8_t subver)
{
	return ver * 256 + subver;
}






// ====================================== TML debug =================================

#if DEBUG_ISON





char *tml_tag_content_to_str(char *str, int str_size, TML_TAG tag, void *data){
	int dsize = TmlTagSize[tag];
	if(dsize){
		uint32_t udata = 0;
		memcpy_tmltoh(&udata, data, dsize);

		switch(tag){
			case TML_LAT:
			case TML_LON:{
				snprintf(str, str_size, "{" GEOF_CRD "}", geo_unpack_coordinate(udata));
			}break;
			case TIMEBEGIN:
			case TIMEEND:{
				snprintf(str, str_size, "{%s}", ctime_light((time_t*)&udata));
			}break;

			default:{
				snprintf(str, str_size, "{%0*X}", dsize * 2, udata);
			}break;
		}
	}else{
		snprintf(str, str_size, "{" PTR_F "}", PTR_I(data));
	}
	return str;
}



char *tml_tag_debug_str(TML_TAG tag, void *data){
	static char str[60];
	int n;
	int dsize = TmlTagSize[tag];
	snprintf(str, sizeof(str), "%s%s%s %n", dsize?"":"<", tml_tag_str(tag), dsize?"":">", &n);
	tml_tag_content_to_str(&str[n], sizeof(str) - n, tag, data);
	return str;
}



char *tml_tag_str(TML_TAG tag){
	switch(tag){
		case TML_NONE: return "Null Tag";
		case TML_LAT: return "Latitude";
		case TML_LON: return "Longitude";
		case POSDECISION: return "PosDecision";
		case POSDIRECTION: return "PosDirection";
		case POSSPEED: return "PosSpeed";
		case POSSATEL: return "PosSatel";
		case POSSATELDECISION: return "PosSatelDecision";	//WTF?
		case DEVICEID: return "DeviceID";
		case TIMEBEGIN: return "TimeBegin";
		case TIMEEND: return "TimeEnd";
		case ITEMSCOUNT: return "ItemsCount";

		case DEVICEUPTIME: return "DeviceUpTime";
		case DEVICEGNSS: return "DeviceGNSS";
		case DEVICEGSM: return "DeviceGSM";
		case DEVICEGPS: return "DeviceGPS";
		case DEVICESEKOP: return "DeviceSEKOP";
		case DEVICETETRA: return "DeviceTETRA";
		case DEVICEETH: return "DeviceETH";
		case DEVICERS485: return "DeviceRS485";
		case ADCDATA: return "ADCdata";
		case KMZDATA: return "KMZdata";
		case KMZERRORCODE: return "KMZerrorCode";
		case PROTOCOLVERSION: return "ProtocolVersion";
		case ALARM: return "Alarm";
		case RADIOALARM: return "AlarmRadio";
		case RFIDDATA: return "RFIDdata";
		case RFIDSTATUS: return "RFIDstatus";
		case SERIALNUMBER: return "SerialNumber";
		case MODVERSION: return "ModVersion";
		case GEOPDOP: return "GeoPDOP";
			//#Data types (TLV)
		case ISTLV: return "ISTLV";
		case POSNMEA: return "PosNMEA";
		case NETADDRESS: return "NetAddress";
		case SEKOPTEXT: return "SEKOPtext";
		case SEKOPSTATUS: return "SEKOPstsus";
		case SEKOPSTATUSDATA: return "SEKOPStatusData";
		case SEKOPTRANSACTIONFILE: return "SEKOPtransactFile";
		case VERSION: return "Version";
		case MODNAME: return "ModName";
		case SMSTEXT: return "SMStext";
		case QUEUELIST: return "QueueList";
			//#Containers (TLV)
		case DIAGGNSS: return "DiagGNSS";
		case DIAGGSM: return "DiagGSM";
		case DIAGGPRS: return "DiagGPRS";
		case DIAGSEKOP: return "DiagSEKOP";
		case DIAGTETRA: return "DiagTETRA";
		case DIAGETH: return "GiagETH";
		case DIAGRS485: return "DiagRS485";
		case SEKOPTRANSACTION: return "SEKOPtransaction";
		case DEVICESWVERSION: return "DeviceSWVersion";
		case MODINFO: return "ModInfo";
		case GEODATA: return "GeoData";
		case SENSORSDATA: return "SensorsData";
		case ALARMDATA: return "AlarmData";
		case AUTHDATA: return "AuthData";
		case RFID: return "RFID";
		case QUEUESTATUS: return "QueueStatus";
		case QUEUECLEAN: return "QueueClean";
		case OLDPROTOCOL: return "OldProtocol";
			//#Special containers (TLXV and others)
		case CONTAINERCRC16: return "ContainerCRC16";
		case ERR: return "Err";     // ERROR
			//#Base-level containers
		case REQREAD: return "ReqRead";
		case REPLY: return "Reply";
		case REQWRITE: return "ReqWrite";
		case SEKOPDATAIN: return "SekopDataIn";
		case SEKOPDATAOUT: return "SekopDataOut";

		default: return "INVALID_container_tag";
	}
}




/*
void tml_constr_test(void)
{
	printf("\nTML constructor test ('+'start cont, '-'end cont, '*'add data tag, 'a'add data to cont, 'p'print mem, 'q' or ESC - exit)\n\n");
	tml_init();
	char buff[100];
	memset(buff, 0, sizeof(buff));
	tml_constr(buff, sizeof(buff));
	char c;
	do{
		c = _getch();
		switch(c){
			case '+':{
				int itag;
				printf("enter tag:");
				scanf("%d", &itag);
				tml_start_container(itag);
			}break;
			case '-':{
				tml_end_container();
			}break;
			case '*':{
				int itag;
				printf("enter tag:");
				scanf("%d", &itag);
				uint32_t data = 0x87654321;
				tml_add_tag(itag, data);
			}break;
			case 'a':{
				uint8_t data[] = {0xFF, 0xFE, 0xFD, 0xFC};
				tml_add_data(data, sizeof(data));
			}break;
			case 'p':{
				print_mem((void*)tml_constr_ptr_base, tml_constr_max_size);
				printf("\n");
			}break;
		}
	}while((c != 27) && (c != 'q'));
}*/





#endif //DEBUG_ISON


