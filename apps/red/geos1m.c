// #include "gnss.h"
#include "geos1m.h"
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
// #include <global.h>
// #include <base.h>
//#include "debug.h"
#define DEBUG(a, b, ...) printf(__VA_ARGS__)
#define DBGPRINTF(a, b, ...) printf(__VA_ARGS__)
#define DEBUGCRLF(a, b) printf("\r\n")
#define CRLF "\r\n"
#define NoYes(c) (c) ? 'Y':'N'

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

typedef enum _Tgeos1m_parser_step {
	GEOS1M_PARSER_HEADER_WAIT,
	GEOS1M_PARSER_GET_NUM_LO,
	GEOS1M_PARSER_GET_NUM_HI,
	GEOS1M_PARSER_GET_SIZE_LO,
	GEOS1M_PARSER_GET_SIZE_HI,
	GEOS1M_PARSER_PACKET_ACCU,
} Tgeos1m_parser_step;

typedef struct _Tgeos1m_parser_state {
	Tgeos1m_parser_step step;
	int header_step;
	int packet_num;
	int packet_size;	// in 32bit words
	int buffer_count;
	Tgeos1m_parser_packet_handler handler;
	uint8_t calc_crc[4];
} Tgeos1m_parser_state;

static Tgeos1m_parser_state state;

#define GEOS1M_BUFFER_SIZE 516
uint8_t geos1m_parser_buffer[GEOS1M_BUFFER_SIZE];

void geos1m_set_packet_handler(Tgeos1m_parser_packet_handler packet_handler)
{
	state.handler = packet_handler;
}

static void geos1m_parser_reset(void)
{
	state.step = GEOS1M_PARSER_HEADER_WAIT;
	state.header_step = 0;
}

void geos1m_parser_init(void)
{
	geos1m_parser_reset();
	state.handler = NULL;
}

void geos1m_parser_input(char c)
{
	switch (state.step) {
	case GEOS1M_PARSER_HEADER_WAIT:{
			char header[] = "PSGG";

			if (header[state.header_step] == c) {
				state.header_step++;
				if (state.header_step ==
				    (sizeof(header) - 1)) {
					state.step = GEOS1M_PARSER_GET_NUM_LO;
					state.calc_crc[0] = 'P';
					state.calc_crc[1] = 'S';
					state.calc_crc[2] = 'G';
					state.calc_crc[3] = 'G';
				}
			} else
				state.header_step = 0;

		}
		break;

	case GEOS1M_PARSER_GET_NUM_LO:{
			state.calc_crc[0] ^= c;
			state.packet_num = c;
			state.step = GEOS1M_PARSER_GET_NUM_HI;
		}
		break;

	case GEOS1M_PARSER_GET_NUM_HI:{
			state.calc_crc[1] ^= c;
			state.packet_num |= (int)c << 8;
			state.step = GEOS1M_PARSER_GET_SIZE_LO;
		} break;

	case GEOS1M_PARSER_GET_SIZE_LO:{
			state.calc_crc[2] ^= c;
			state.packet_size = (unsigned char)c;
			state.step = GEOS1M_PARSER_GET_SIZE_HI;
		}
		break;

	case GEOS1M_PARSER_GET_SIZE_HI:
		state.calc_crc[3] ^= c;
		state.packet_size |= (unsigned char)c << 8;
		state.packet_size *= 4;
		state.buffer_count = 0;
#define GEOGRAPH_SIZE	136
#define TELEMETRY_SIZE	12
		switch(state.packet_num) {
		case GEOS1M_TAG_GEOGRAPH:
			if (state.packet_size != GEOGRAPH_SIZE) {
				state.step = GEOS1M_PARSER_HEADER_WAIT;
				DEBUG(GNSS, NORMAL, "BAD SIZE %d != %d for GEOGRAPH"CRLF, state.packet_size, GEOGRAPH_SIZE);
				return;
			}
			break;
		case GEOS1M_TAG_CUR_TELEMETRY:
			if (state.packet_size != TELEMETRY_SIZE) {
				state.step = GEOS1M_PARSER_HEADER_WAIT;
				DEBUG(GNSS, NORMAL, "BAD SIZE %d != %d for TELEMETRY"CRLF, state.packet_size, TELEMETRY_SIZE);
				return;
			}
			break;
		case GEOS1M_TAG_VIS_SAT:
			/* Variable length packet */
#if 0
			/* Totally ignore */
			state.step = GEOS1M_PARSER_HEADER_WAIT;
			DEBUG(GNSS, NORMAL, "ignoring VIS_SAT"CRLF);
			return;
#endif
			break;
		default:
			break;
		}

             // Ivan A-R: check for too small packet_size
		if(state.packet_size >= 4) {
			if (state.packet_size < GEOS1M_BUFFER_SIZE)
				state.step = GEOS1M_PARSER_PACKET_ACCU;
			else // Ignore packet witch error size
			state.step =
				GEOS1M_PARSER_HEADER_WAIT;
		} else { // hadle zero packet immediate
				state.step =
                    			GEOS1M_PARSER_HEADER_WAIT;
/* No packet - no pain */
#if 0
       		if (state.handler != NULL)
                    state.handler((Tgeos1m_packet_tag)
                            state.packet_num,
                            geos1m_parser_buffer,
                            state.packet_size / 4);
#endif
			DEBUG(GNSS, NORMAL, "Got some crap, bad packet size %d\n", state.packet_size);
		}
		break;

	case GEOS1M_PARSER_PACKET_ACCU:
            	geos1m_parser_buffer[state.buffer_count] = c;
		state.calc_crc[state.buffer_count & 3] ^= c;
		state.buffer_count++;
		/* Reading till size + 4, where 4 is CRC size */
		if (state.buffer_count ==
		    state.packet_size + 4) {
			state.step = GEOS1M_PARSER_HEADER_WAIT;
			if (state.calc_crc[3] +
				state.calc_crc[2] +
				state.calc_crc[1] +
				state.calc_crc[0] == 0) {
			/* CRC OK */
				DEBUG(GNSS, NORMAL, "CRC OK"CRLF);
				if (state.handler != NULL)
					state.packet_size -= 4;
					state.handler((Tgeos1m_packet_tag)
						state.packet_num,
						geos1m_parser_buffer,
						state.packet_size / 4);
			} else {
				DEBUG(GNSS, NORMAL, "BAD CRC (%02x): %02x:%02x:%02x:%02x len=%d"CRLF,
					state.packet_num,
					state.calc_crc[3], state.calc_crc[2],
					state.calc_crc[1], state.calc_crc[0],
					state.packet_size);
//				DEBUGDUMP_P(GNSS, NORMAL,
//					geos1m_parser_buffer, state.packet_size + 4);
//				DEBUG(GNSS, NORMAL, "Eiiiiiieeeeee ==="CRLF);
//				DEBUGCRLF(GNSS, NORMAL);
			}
		}
		break;
	}
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

const char *geos1m_packet_tag_str(Tgeos1m_packet_tag tag)
{
	switch (tag) {
	case GEOS1M_TAG_DEB01:
	case GEOS1M_TAG_DEB02:
	case GEOS1M_TAG_DEB03:
	case GEOS1M_TAG_DEB04:

	case GEOS1M_TAG_DEB06:
	case GEOS1M_TAG_DEB07:
	case GEOS1M_TAG_DEB08:

	case GEOS1M_TAG_DEB0E:
		return "debug";

	case GEOS1M_TAG_CHAN_MEAS:
		return "chan measurement";
	case GEOS1M_TAG_NAV_GPS:
		return "GPS nav frame";
	case GEOS1M_TAG_NAV_GLONASS:
		return "GLONASS nav frame";
	case GEOS1M_TAG_GEOCENT:
		return "geocentric coord";

	case GEOS1M_TAG_GEOGRAPH:
		return "geographic coord";
	case GEOS1M_TAG_CUR_TELEMETRY:
		return "curr telemetry";
	case GEOS1M_TAG_VIS_SAT:
		return "visible SATs";

	case GEOS1M_TAG_PWR_ON:
		return "pwr on frame";
	case GEOS1M_TAG_CMD_ERROR:
		return "bad command";

	case GEOS1M_TAG_SET_DEFS:
		return "set default params";
	case GEOS1M_TAG_SET_UART:
		return "set UART params";
	case GEOS1M_TAG_SET_MODE:
		return "set receiver mode";
	case GEOS1M_TAG_SET_DECISION:
		return "set decision params";
	case GEOS1M_TAG_SET_RATE:
		return "set packet rate";

	case GEOS1M_TAG_SET_PROT:
		return "set protocols";

	case GEOS1M_TAG_SET_ALMANAC_GPS:
		return "set GPS almanac";
	case GEOS1M_TAG_SET_ALMANAC_GLONASS:
		return "set GLONASS almanac";
	case GEOS1M_TAG_SET_EPHEMER_GPS:
		return "set GPS ephemer";
	case GEOS1M_TAG_SET_EPHEMER_GLONASS:
		return "set GLONASS ephemer";
	case GEOS1M_TAG_SET_PPS:
		return "set PPS mode";
	case GEOS1M_TAG_SET_SAT:
		return "config SATs use";
	case GEOS1M_TAG_SET_NMEA:
		return "config NMEA";
	case GEOS1M_TAG_SET_BIN:
		return "config BINary";

	case GEOS1M_TAG_GET_DEFS:
		return "get default params";
	case GEOS1M_TAG_GET_UART:
		return "get UART params";
	case GEOS1M_TAG_GET_MODE:
		return "get receiver mode";
	case GEOS1M_TAG_GET_DECISION:
		return "get decision params";
	case GEOS1M_TAG_GET_RATE:
		return "get packet rate";

	case GEOS1M_TAG_GET_PROT:
		return "get protocols";

	case GEOS1M_TAG_GET_ALMANAC_GPS:
		return "get GPS almanac";
	case GEOS1M_TAG_GET_ALMANAC_GLONASS:
		return "get GLONASS almanac";
	case GEOS1M_TAG_GET_EPHEMER_GPS:
		return "get GPS ephemer";
	case GEOS1M_TAG_GET_EPHEMER_GLONASS:
		return "get GLONASS ephemer";
	case GEOS1M_TAG_GET_PPS:
		return "get PPS mode";
	case GEOS1M_TAG_GET_SAT:
		return "get SATs use conf";
	case GEOS1M_TAG_GET_NMEA:
		return "get NMEA conf";
	case GEOS1M_TAG_GET_BIN:
		return "get BINary conf";

	case GEOS1M_TAG_FIRM_VER:
		return "firmware ver";
	case GEOS1M_TAG_RESTART:
		return "cmd restart";
	case GEOS1M_TAG_SAVE:
		return "flash save";

	default:
		return "invalid or reserved";
	}
}

#define TAG_PARSER(tag) static void parse_ ## tag (void *data_ptr, int size)
#define TAG_PARSER_CALL(tag, d, s) parse_ ## tag (d, s)

TAG_PARSER(GEOS1M_TAG_GEOGRAPH)
{
	Tgeos1m_packet_geographic *Pmsg = data_ptr;
	
	//struct tm tim = {
	//	.tm_sec = 0,
	//	.tm_min = 0,
	//	.tm_hour = 0,
	//	.tm_mday = 1,
	//	.tm_mon = 0,
	//	.tm_year = 2008 - 1900,
	//};
	//time_t tstamp = mktime(&tim) + (int)Pmsg->time_s;
	
	double time_s_int;
	double time_s_fract = modf(Pmsg->time_s, &time_s_int);
	time_t tstamp = 0x47798280 + (int)time_s_int;
	//time_t tstamp = mktime(&tim);
	//DBGPRINTF(GNSS, DETAILED, "|0x%X|", tstamp);
	//tstamp += (int)time_s_int;
	struct tm *gtim = gmtime(&tstamp);
	printf("\r\n\r\n\r\n");
	DEBUG(GNSS, NORMAL, "GEOGRAPH PARSE tstamp = %d\n", tstamp);
	printf("\r\n\r\n\r\n");
	//info.hrs = gtim->tm_hour;
	//info.min = gtim->tm_min;
	//info.sec = gtim->tm_sec;
	//info.day = gtim->tm_mday;
	//info.month = gtim->tm_mon + 1;
	//info.year = gtim->tm_year + 1900;
#if 0
	geoinfo.dtime.msec = (int)(time_s_fract * 1000);
	geoinfo.dtime.sec = gtim->tm_sec;
	geoinfo.dtime.min = gtim->tm_min;
	geoinfo.dtime.hour = gtim->tm_hour;
	geoinfo.dtime.mday = gtim->tm_mday;
	geoinfo.dtime.mon = gtim->tm_mon;	//(Months *since* january: 0-11)
	geoinfo.dtime.year = gtim->tm_year + 1900 - 2000;
	
	geoinfo.lat = Pmsg->lat;
	geoinfo.lon = Pmsg->lon;
	geoinfo.alt = Pmsg->alt;
	geo_motion2D_pol2geo(Pmsg->course, Pmsg->speed, &geoinfo.motion);// ñêîðîñòü, êóðñ
	
	geo_DOP2geo(Pmsg->HDOP, Pmsg->VDOP, Pmsg->TDOP, &geoinfo.prec);
			
	DEBUGCRLF(GNSS, DETAILED);
	geo_time_result();//geo&time debug prited by this function
	dop_result();
#endif
}
TAG_PARSER(GEOS1M_TAG_CUR_TELEMETRY)
{
	Tgeos1m_packet_cur_telemetry *Pmsg = data_ptr;
	uint32_t status = Pmsg->status;
	Tgeos1m_packet_cur_telemetry_status *pstatus = (Tgeos1m_packet_cur_telemetry_status *)&status;
#if 0	
	geoinfo.mode = pstatus->dec ? (pstatus->decmode ? GEOM_2D : GEOM_3D) : GEOM_NONE;
	geoinfo.sats.visible = Pmsg->sats;
	geoinfo.sats.decused = Pmsg->satsdec;
#endif
	
	static const char * const mode[] = {
		[0] = "NORM",
		[1] = "TEST",
		[2] = "FWUP",
	};
	
	//receiver: Id, mode & uptime:
	DBGPRINTF(GNSS, DETAILED, "%c %s%05lu ",
			  pstatus->rcvid ? 'M' : '-', mode[pstatus->rcvmod], Pmsg->uptime);
	
	static const char * const NAVS[] = {
		[0] = "PL",	//GPS+GLONASS
		[1] = "P-",	//GPS only
		[2] = "-L",	//GLONASS only
	};
	
	//navigation system status & satellites info:
	DBGPRINTF(GNSS, DETAILED, "%s%d sat%02dd%02d ",
			  NAVS[pstatus->decns], (int)pstatus->both + 1, Pmsg->sats, Pmsg->satsdec);

	//decision info (validity, rate):
	DBGPRINTF(GNSS, DETAILED, "dc%c%cHz ",
			  NoYes(pstatus->dec), pstatus->decrate ? '1' : '5');
	
	static const char * const DisEn[] = {
		[0] = "DS",	//disable
		[1] = "EN", //enable
	};
	
	//2D/3D modes setting & '~'status; fixed position mode status:
	DBGPRINTF(GNSS, DETAILED, "2D%s~%s fp%c ",
			  DisEn[pstatus->alw2D], pstatus->decmode ? "2D" : "3D", NoYes(pstatus->fixpos));
	
	//coordinates extrapolation setting & status (enable/disable, 'p'possibility, '~'current status (usage in decision)):
	DBGPRINTF(GNSS, DETAILED, "exp%sp%c~%c ",
			  DisEn[pstatus->expalw], NoYes(pstatus->exp), NoYes(pstatus->expd));
	
	//dynamic filter setting & '~'status:
	DBGPRINTF(GNSS, DETAILED, "df%s~%c ",
			  DisEn[pstatus->dynflt], NoYes(pstatus->dynfltd));
	
	//almanac & PPS statuses:
	DBGPRINTF(GNSS, DETAILED, "alm%c pps%c ",
			  NoYes(pstatus->alm), NoYes(pstatus->PPS));
	
	static const char * const AntWarn[] = {
		[0] = "A!",	//warning, bad supply voltage on antenna, may be short circuit
		[1] = "ok",	//ok
	};
	static const char * const SyntWarn[] = {
		[0] = "S!",	//warning, synthesizer status is not normal
		[1] = "ok",	//ok
	};
	
	//warnings: antenna voltage & synthesizer:
	DBGPRINTF(GNSS, DETAILED, "%s %s" CRLF,
			  AntWarn[pstatus->antv], SyntWarn[pstatus->synt]);
	
	// ------------- Trace format description & examples: ---------------
	//('M'=GeoS1M/'-'=Geos1)  (2D mode enabLed/disabled)  (coOrdinates extrapolated)
	// | (2=decision From both systems) | (Current mode)  | (dynamic (Almanac available)
	// | (NAV systems|     (decision    |  | (fixed       |  filter   | (PPS reliability)
	// |     in use*)|       valiD-Y/N) |  | cooRdinates) |  in uSe)  |    | (WarNings: 'A!'=antenna voltage
	// |           | |           |      |  |    |         |      |    |    |  |  |      'S!'=synthesizer)
	// M NORM04273 PL2 sat05d03 dcY1Hz 2DEN~2D fpN expENpY~Y dfEN~Y almY ppsY ok ok
	// - NORM00068 P-1 sat02d00 dcN1Hz 2DDS~3D fpY expENpN~N dfDS~N almY ppsN A! ok
	// M NORM95144 -L1 sat00d00 dcN5Hz 2DEN~3D fpN expDSpN~N dfEN~N almN ppsN ok S!
	//     |   |          |  |     |                  |  |     |
	//     |   | (all     |  | (decIsion rate)        |  | (dynAmic filter enabled/disabled)
	//     |   | satelliteS)(Satellites in decision)  | (Extrapolation possibility)
	//     | (rEceiver uptime in seconds)  (extrapolatIon enabled/disabled)
	// (recEiver mode: NORMal, TEST, or FWUP-firmware upgrade)
	// *: PL=GPS+GLONASS, P-=GPS only, -L=GLONASS only
	
#if 0
	status_gnss.ant_status = (int)(1 - pstatus->antv) << 1;
	
	decision_complete();
#endif
}
void GNSS_GeoS1M_Bin_Msg_Received_proc(Tgeos1m_packet_tag type, void* data_ptr, int size)  		// îáðàáîòêà ñîîáùåíèÿ îò GNSS receiver GeoS-1M
{
	DEBUG(GNSS, DETAILED, "GeoSmsg:%02X,s:%d,", type, size);
	switch(type) {
	case GEOS1M_TAG_GEOGRAPH:
		DEBUG(GNSS, NORMAL, "GOT GEOGRAPH"CRLF);
		TAG_PARSER_CALL(GEOS1M_TAG_GEOGRAPH, data_ptr, size);
		break;
	
	case GEOS1M_TAG_CUR_TELEMETRY:
		DEBUG(GNSS, NORMAL, "GOT TELEMETRY"CRLF);
		TAG_PARSER_CALL(GEOS1M_TAG_CUR_TELEMETRY, data_ptr, size);
		break;
	
	case GEOS1M_TAG_VIS_SAT:
		DEBUG(GNSS, NORMAL, "GOT VIS_SAT"CRLF);
		break;
	default:
		DEBUGCRLF(GNSS, DETAILED);
		DEBUG(GNSS, NORMAL, "Got some crap: %08x size %d\n", type, size);
		break;
	}
#if 0	
	msg_has_arrived();
#endif
}

void do_geos1m_machinery(void)
{
#if 0
	static int _f = 1;	//?for GeoS1M
	if(_f) {
		GPS_NOT_RESET;
		_f = 0;
		timer_handler(NULL);
	}
#endif
}

void init_geos1m(void)
{
	geos1m_parser_init();
	geos1m_set_packet_handler(GNSS_GeoS1M_Bin_Msg_Received_proc);
#if 0
	configure_for_GNSS(18);
#endif
}

