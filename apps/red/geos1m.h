



#ifndef GEOS1M_H_
#define GEOS1M_H_


typedef enum _Tgeos1m_packet_tag{
	GEOS1M_TAG_DEB01 = 0x01,
	GEOS1M_TAG_DEB02,
	GEOS1M_TAG_DEB03,
	GEOS1M_TAG_DEB04,
	
	GEOS1M_TAG_DEB06 = 0x06,
	GEOS1M_TAG_DEB07,
	GEOS1M_TAG_DEB08,
	
	GEOS1M_TAG_DEB0E = 0x0E,
	
	GEOS1M_TAG_CHAN_MEAS = 0x10,
	GEOS1M_TAG_NAV_GPS,
	GEOS1M_TAG_NAV_GLONASS,
	GEOS1M_TAG_GEOCENT,
	
	GEOS1M_TAG_GEOGRAPH = 0x20,
	GEOS1M_TAG_CUR_TELEMETRY,
	GEOS1M_TAG_VIS_SAT,
	
	GEOS1M_TAG_PWR_ON = 0x3E,
	GEOS1M_TAG_CMD_ERROR,
	
	GEOS1M_TAG_SET_DEFS = 0x40,
	GEOS1M_TAG_SET_UART,
	GEOS1M_TAG_SET_MODE,
	GEOS1M_TAG_SET_DECISION,
	GEOS1M_TAG_SET_RATE,
	
	GEOS1M_TAG_SET_PROT = 0x46,
	
	GEOS1M_TAG_SET_ALMANAC_GPS = 0x48,
	GEOS1M_TAG_SET_ALMANAC_GLONASS,
	GEOS1M_TAG_SET_EPHEMER_GPS,
	GEOS1M_TAG_SET_EPHEMER_GLONASS,
	GEOS1M_TAG_SET_PPS,
	GEOS1M_TAG_SET_SAT,
	GEOS1M_TAG_SET_NMEA,
	GEOS1M_TAG_SET_BIN,
	
	GEOS1M_TAG_GET_DEFS = 0x80,
	GEOS1M_TAG_GET_UART,
	GEOS1M_TAG_GET_MODE,
	GEOS1M_TAG_GET_DECISION,
	GEOS1M_TAG_GET_RATE,
	
	GEOS1M_TAG_GET_PROT = 0x86,
	
	GEOS1M_TAG_GET_ALMANAC_GPS = 0x88,
	GEOS1M_TAG_GET_ALMANAC_GLONASS,
	GEOS1M_TAG_GET_EPHEMER_GPS,
	GEOS1M_TAG_GET_EPHEMER_GLONASS,
	GEOS1M_TAG_GET_PPS,
	GEOS1M_TAG_GET_SAT,
	GEOS1M_TAG_GET_NMEA,
	GEOS1M_TAG_GET_BIN,
	
	GEOS1M_TAG_FIRM_VER = 0xC1,
	GEOS1M_TAG_RESTART,
	GEOS1M_TAG_SAVE,

}Tgeos1m_packet_tag;


#include "geos1m_packets.h"




typedef void	(*Tgeos1m_parser_packet_handler)(Tgeos1m_packet_tag type, void* data_ptr, int size);	//size in uint32_t elements

void geos1m_parser_input(char c);
void geos1m_set_packet_handler(Tgeos1m_parser_packet_handler packet_handler);


void geos1m_parser_init(void);




// * * * * * * * * * * * debug * * * * * * * * * * * *

const char *geos1m_packet_tag_str(Tgeos1m_packet_tag tag);





void GNSS_GeoS1M_Bin_Msg_Received_proc(Tgeos1m_packet_tag type, void* data_ptr, int size);  		// îáðàáîòêà ñîîáùåíèÿ îò GNSS receiver GeoS-1M
void do_geos1m_machinery(void);

#endif //GEOS1M_H_
