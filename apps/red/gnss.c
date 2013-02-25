#include <stdio.h>
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "k2_serial.h"
#include "geos1m.h"

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

static int geos_packet_header(int c)
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
static uint8_t packet_buf[256];
static void geos_packet_detector(int c)
{
	static int state = 0;
	static int packetnum = 0;
	static int packetsize = 0;
	static int packet_count = 0;
	int crc;
	if (state != 0)
		crc = geos_crc(c, 0);
	switch(state) {
	case 0:
		if (geos_packet_header(c)) {
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
		break;
	case 5:
	case 6:
	case 7:
	case 8:
		packet_buf[packet_count] = c;
		packet_count++;
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
		if(crc)
			printf("b\r\n");
		else
			printf("%04x\r\n", packetnum);
		packet_count = 0;
		break;
	}
}

static uint8_t gnss_buffer[1024];
msg_t gnss_thread(void *p) {

    (void)p;
    chRegSetThreadName("gnss");
    init_geos1m();
    while (TRUE) {
	    int i;
	    int t = sdReadTimeout(&SD1, gnss_buffer, 1024, 250);
            // chprintf((BaseSequentialStream*)&SDDBG, "z %02x\r\n", t);
#if 0
	    for (i = 0; i < t; i++)
	    	geos1m_parser_input(buf[i]);
#endif
	    for (i = 0; i < t; i++)
		    geos_packet_detector(gnss_buffer[i]);
    }
    return 0;
}

