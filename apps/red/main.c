/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "k2_serial.h"
#include "eeprom.h"
#include "gnss.h"
#include "1k161.h"

static WORKING_AREA(wa_blink, 128);
static WORKING_AREA(wa_gnss, 1024);

#define EEPROM_PAGE_SIZE        8         /* page size in bytes. Consult datasheet. */
#define EEPROM_SIZE             8192       /* total amount of memory in bytes */
#define EEPROM_I2CD             I2CD1       /* ChibiOS I2C driver used to communicate with EEPROM */
#define EEPROM_I2C_ADDR         0x50   /* EEPROM address on bus */
#define EEPROM_WRITE_TIME_MS    20          /* time to write one page in mS. Consult datasheet! */
#define EEPROM_TX_DEPTH         (EEPROM_PAGE_SIZE + 2)/* temporal transmit buffer depth for eeprom driver */

static msg_t blink_thread(void *p) {

    (void)p;
    chRegSetThreadName("blinker");
    while (TRUE) {
        palClearPad(IOPORT1, PIOA_LED_GPSACT);
        chThdSleepMilliseconds(100);
        palSetPad(IOPORT1, PIOA_LED_GPSACT);
        chThdSleepMilliseconds(100);
    }
    return 0;
}

#define I2C_CLDIV 12
#define I2C_CHDIV 12
#define I2C_CKDIV 5

static const I2CConfig i2cfg = {
	.cwgr = (I2C_CKDIV << 16) | (I2C_CHDIV << 8) | (I2C_CLDIV)
};

struct gnss_thread_config {
	int buf_size;
	int read_timeout;
	void (*packet_detector)(int c);
};

struct gnss_thread_config conf;
msg_t gnss_thread(void *p) {
	uint8_t *gnss_buffer = chHeapAlloc(NULL, conf.buf_size);
	(void)p;
	while(TRUE) {
		int t, i;
	        t = sdReadTimeout(&SD1, gnss_buffer,
			conf.buf_size, conf.read_timeout);
	        for (i = 0; i < t; i++)
			conf.packet_detector(gnss_buffer[i]);
	}
        chHeapFree(gnss_buffer);
	return 0;
}

/*
 * Application entry point.
 */
int main(void) {
   int status;
   uint8_t ch;

   int j, rv;
    /*
     * System initializations.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs the board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     */
    halInit();
    chSysInit();
    k2_init_serials();
    chprintf((BaseSequentialStream*)&SDDBG, "BOOT\r\n");
    
    i2cStart(&I2CD1, &i2cfg);

    chprintf((BaseSequentialStream*)&SDDBG, "FRAM init\r\n");
    fram_init();
    fram_open();
    chprintf((BaseSequentialStream*)&SDDBG, "GNSS init\r\n");
    rv = gnss_init();
    /*
     * Creates the blinker thread.
      */
    chThdCreateStatic(wa_blink, sizeof(wa_blink), NORMALPRIO, blink_thread, NULL);

    /* FIXME */
    switch(rv) {
    case 1:
        conf.buf_size = 1024;
	conf.read_timeout = 250;
	conf.packet_detector = packet_detector_geos;
        chThdCreateStatic(wa_gnss, sizeof(wa_gnss), NORMALPRIO, gnss_thread, NULL);
	break;
    case 2:
/* Danger! Don't do > 512 bytes! */
        conf.buf_size = 512;
	conf.read_timeout = 500;
	conf.packet_detector = packet_detector_1k161;
        chThdCreateStatic(wa_gnss, sizeof(wa_gnss), NORMALPRIO, gnss_thread, NULL);
	break;
    }

    /*
     * Normal main() thread activity.
     */
    chThdSleepMilliseconds(2500);
    while (TRUE) {
        ch = sdGet(&SDDBG);
        chprintf((BaseSequentialStream*)&SDDBG, "got %02x\r\n", ch);
    }

    return 0;
}

