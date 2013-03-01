#include <stdio.h>
#include <time.h>
#include <math.h>
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "k2_serial.h"
#include "gnss.h"
#include "geos1m.h"
#include "1k161.h"
#include "geo.h"

static msg_t mbox_geo_buffer[10];
static msg_t mbox_tel_buffer[10];
static msg_t mbox_sat_buffer[10];
static MAILBOX_DECL(mbox_geo, mbox_geo_buffer, 10);
static MAILBOX_DECL(mbox_tel, mbox_tel_buffer, 10);
static MAILBOX_DECL(mbox_sat, mbox_sat_buffer, 10);
static WORKING_AREA(wa_geo, 2048);
static WORKING_AREA(wa_tel, 128);
static WORKING_AREA(wa_sat, 128);

status_gnss_t status_gnss;
static Tgeo geoinfo;

static void set_antenna_status(int status)
{
	status_gnss.ant_status = status;
}

static int geos_crc(int c, int init)
{
	static int crcdata[] = {0, 0, 0, 0};
	static int crccounter = 0;
	if (init) {
		crcdata[0] = 0;
		crcdata[1] = 0;
		crcdata[2] = 0;
		crcdata[3] = 0;
		crccounter = 0;
	}
	crcdata[crccounter] ^= c;
	crccounter++;
	crccounter &= 3;
	return crcdata[0] + crcdata[1] + crcdata[2] + crcdata[3];
}

void decision_complete(void)
{
#if 0
	msg_arrives.dec = 1;
	if(status_gnss.ant_status == 2) {
		led_mode(LED_MODE_GNSS_EANT);
	} else	if(geoinfo.mode != GEOM_NONE) {
		status_gnss.valid = 1;
		led_mode(LED_MODE_GNSS_VALID);
	} else {
		status_gnss.valid = 0;
		if(geoinfo.sats.visible)
			led_mode(LED_MODE_GNSS_RECV);
		else
			led_mode(LED_MODE_GNSS_OK);
	}
#endif
	
	Tgeo_packed pgeo;
	geo_pack(&pgeo, &geoinfo);
#if 0
	awc_send_msg(AWC_NAV, &pgeo, sizeof(pgeo));

	DEBUG(GNSS, NORMAL, "GEO ");
	DBGPRINTF_GEO(GNSS, NORMAL, &geoinfo);
	DEBUGCRLF(GNSS, NORMAL);
#endif
}

static int packet_header_geos(int c)
{
	char header[] = "PSGG";
	static int header_step = 0;
	static int skipped_chars = 0;
	if (header[header_step] == c) {
		header_step++;
		if (header_step == sizeof(header) - 1) {
			if (skipped_chars > 0)
				printf("Skip %d\r\n", skipped_chars);
			header_step = 0;
			skipped_chars = 0;
			return 1;
		}
	} else {
		header_step = 0;
		skipped_chars++;
	}
	return 0;
}
struct packet_msg {
	int id;
	int len;
	uint8_t data[];
};
static void process_packet(struct packet_msg *p)
{
	int res;
	switch(p->id) {
	case 0x20: /* Geodata */
		res = chMBPost(&mbox_geo, (msg_t)p, TIME_IMMEDIATE);
		break;
	case 0x21: /* Telemetry */
		res = chMBPost(&mbox_tel, (msg_t)p, TIME_IMMEDIATE);
		break;
	case 0x22: /* Sats channels */
		res = chMBPost(&mbox_sat, (msg_t)p, TIME_IMMEDIATE);
		break;
	default:
		pr_debug("bad %02x\r\n", p->id);
		chHeapFree(p);
		return;
		break;
	}
	if (res != RDY_OK) {
		pr_debug("timeout\r\n");
		chHeapFree(p);
	}
}
void packet_detector_geos(int c)
{
	static int state = 0;
	static int packetnum = 0;
	static int packetsize = 0;
	static int packet_count = 0;
	int crc;
	static struct packet_msg *packet = NULL;
	if (state != 0)
		crc = geos_crc(c, 0);
	switch(state) {
	case 0:
		if (packet_header_geos(c)) {
			state = 1;
			geos_crc('P', 1);
			geos_crc('S', 0);
			geos_crc('G', 0);
			geos_crc('G', 0);
		}
		break;
	case 1:
		packetnum = c;
		state = 2;
		break;
	case 2:
		packetnum |= (c << 8);
		state = 3;
		break;
	case 3:
		packetsize = c;
		state = 4;
		break;
	case 4:
		packetsize |= (c << 8);
		state = 5;
		packet = chHeapAlloc(NULL,
			sizeof(struct packet_msg) + packetsize * 4);
		if (!packet) {
			printf("Drop\r\n");
			state = 0;
			break;
		}
		packet->len = packetsize *4;
		packet->id = packetnum;
		break;
	case 5:
	case 6:
	case 7:
	case 8:
		if (packet) {
			if (packet->len > packet_count) {
				packet->data[packet_count] = c;
				packet_count++;
			}
		}
		if (state == 8) {
			packetsize--;
			if(!packetsize)
				state = 100;
			else
				state = 5;
		} else
			state++;
		break;
	/* CRC */
	case 100:
	case 101:
	case 102:
		state++;
		break;
	case 103:
		state = 0;
		if(crc) {
			printf("b\r\n");
			chHeapFree(packet);
		} else {
			process_packet(packet);
		}
		packet_count = 0;
		break;
	}
}

static msg_t geo_thread(void *p)
{
	msg_t msg, result;
	struct packet_msg *pkt;
	(void)p;
	while (TRUE) {
		result = chMBFetch(&mbox_geo, &msg, TIME_INFINITE);
		pkt = (struct packet_msg *) msg;
		Tgeos1m_packet_geographic *Pmsg = (Tgeos1m_packet_geographic *)pkt->data;
		double time_s_int;
		double time_s_fract = modf(Pmsg->time_s, &time_s_int);
		time_t tstamp = 0x47798280 + (int)time_s_int;
		struct tm *gtim = gmtime(&tstamp);
		chprintf((BaseSequentialStream *)&SDDBG, "GEOGRAPH PARSE tstamp = %ld\r\n", tstamp);
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
				
#if 0
		DEBUGCRLF(GNSS, DETAILED);
		geo_time_result();//geo&time debug prited by this function
		dop_result();
#endif
		chHeapFree(pkt);
	}
	return 0;
}

#define GEOS1M_STATUS_ANTV (1 << 22) /* Antenna voltage */
static msg_t tel_thread(void *p)
{
	msg_t msg, result;
	struct packet_msg *pkt;
	uint32_t status;
	(void)p;
	Tgeos1m_packet_cur_telemetry *Pmsg;
	while (TRUE) {
		result = chMBFetch(&mbox_tel, &msg, TIME_INFINITE);
		pkt = (struct packet_msg *) msg;
		chprintf((BaseSequentialStream *)&SDDBG, "TELEMETRY\r\n");
		Pmsg = (Tgeos1m_packet_cur_telemetry *)pkt->data;
		status = Pmsg->status;
		set_antenna_status(!(status & GEOS1M_STATUS_ANTV));
		decision_complete();
		chHeapFree(pkt);
	}
	return 0;
}
static msg_t sat_thread(void *p)
{
	msg_t msg, result;
	struct packet_msg *pkt;
	(void)p;
	while (TRUE) {
		result = chMBFetch(&mbox_sat, &msg, TIME_INFINITE);
		pkt = (struct packet_msg *) msg;
		chprintf((BaseSequentialStream *)&SDDBG, "SAT\r\n");
		chHeapFree(pkt);
	}
	return 0;
}

static int detect_geos(void)
{
	int i, j;
	uint8_t *gnss_buffer = chHeapAlloc(NULL, 1024);
	int detected = 0;
	palSetPad(IOPORT1, PIOA_GPS_NRST);
        chThdSleepMilliseconds(100);
	k2_usart1_geos();
	for (i = 0; i < 20; i++) {
		int t = sdReadTimeout(&SD1, gnss_buffer, 1024, 250);
		if (t > 0) {
			for (j = 0; j < t; j++) {
				if (packet_header_geos(gnss_buffer[j]))
					detected = 1;
			}
		}
	}
	chHeapFree(gnss_buffer);
	return detected;
}

int gnss_init(void)
{
	int use_geos = 0, use_1k161 = 0;
	/* Detecting receiver */
	while(!use_geos && !use_1k161) {
		if (detect_geos()) {
			pr_debug("Geostar GeoS-1M detected\r\n");
			use_geos = 1;
		} else if (detect_1k161()) {
			pr_debug("RIRV 1K-161 detected\r\n");
			use_1k161 = 1;
		}
	}
	if (use_geos) {
		chThdCreateStatic(wa_geo, sizeof(wa_geo), NORMALPRIO, geo_thread, NULL);
		chThdCreateStatic(wa_tel, sizeof(wa_tel), NORMALPRIO, tel_thread, NULL);
		chThdCreateStatic(wa_sat, sizeof(wa_sat), NORMALPRIO, sat_thread, NULL);
	}
	if (use_1k161) {
		chThdCreateStatic(wa_geo, sizeof(wa_geo), NORMALPRIO, geo_thread_1k161, NULL);
		chThdCreateStatic(wa_tel, sizeof(wa_tel), NORMALPRIO, misc_thread_1k161, NULL);
	}
	if (use_geos)
		return 1;
	else if (use_1k161)
		return 2;
	return 0;
}

