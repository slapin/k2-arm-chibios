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
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "k2_serial.h"
#include "eeprom.h"

static WORKING_AREA(waThread1, 128);
static WORKING_AREA(waThread2, 2048);
static EepromFileStream fram;

#define EEPROM_PAGE_SIZE        128         /* page size in bytes. Consult datasheet. */
#define EEPROM_SIZE             8192       /* total amount of memory in bytes */
#define EEPROM_I2CD             I2CD1       /* ChibiOS I2C driver used to communicate with EEPROM */
#define EEPROM_I2C_ADDR         0x50   /* EEPROM address on bus */
#define EEPROM_WRITE_TIME_MS    20          /* time to write one page in mS. Consult datasheet! */
#define EEPROM_TX_DEPTH         (EEPROM_PAGE_SIZE + 2)/* temporal transmit buffer depth for eeprom driver */

static uint8_t i_buf[EEPROM_TX_DEPTH];
static I2CEepromFileConfig icfg = {
  &EEPROM_I2CD,
  0,
  0,
  EEPROM_SIZE,
  EEPROM_PAGE_SIZE,
  EEPROM_I2C_ADDR,
  MS2ST(EEPROM_WRITE_TIME_MS),
  i_buf,
};

static msg_t Thread1(void *p) {

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

static msg_t gnss(void *p) {

    (void)p;
    uint8_t buf[256];
    chRegSetThreadName("gnss");
    while (TRUE) {
        chThdSleepMilliseconds(100);
        chprintf((BaseSequentialStream*)&SD1, "");
	    int t = sdGet(&SD1);
            chprintf((BaseSequentialStream*)&SDDBG, "z %d\r\n", t);

//	    dbg_hex_dump(buf, t);
#if 0
	// sdWrite(&SDDBG, (uint8_t *)"Hello World!\r\n", 14);
        chprintf((BaseSequentialStream*)&SD1, "COM1: %d\r\n", 0);
        chprintf((BaseSequentialStream*)&SD2, "COM2: %d\r\n", 0);
        chprintf((BaseSequentialStream*)&SD3, "COM3: %d\r\n", 0);
        chprintf((BaseSequentialStream*)&SDDBG, "COMDBG: %d\r\n", 0);
#endif
    }
    return 0;
}

static uint8_t frambuf[1024];
#define I2C_CLDIV 12
#define I2C_CHDIV 12
#define I2C_CKDIV 5

static const I2CConfig i2cfg = {
	.cwgr = (I2C_CKDIV << 16) | (I2C_CHDIV << 8) | (I2C_CLDIV)
};
/*
 * Application entry point.
 */
int main(void) {
   int status;
   uint8_t rxbuf[32];
   uint8_t txbuf[16];
   int j;
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
    
    i2cStart(&I2CD1, &i2cfg);
    for (j = 0; j < 8192; j+= 16) {
	    int k;
	    memset(rxbuf, 0xff, sizeof(rxbuf));
	    for (k = 0; k < 16; k++) {
	        txbuf[0] = ((j + k) >> 8) & 0xff;
	        txbuf[1] = ((j + k) & 0xff);
	    	status = i2cMasterTransmitTimeout(&I2CD1, (0xa << 3), txbuf, 2, &rxbuf[k], 1, 2000);
		if (status != RDY_OK)
			break;
	    }
	    if (status == RDY_OK) {
		    dbg_hex_dump(rxbuf, 16);
	    }
	    if (status == RDY_RESET)
		    chprintf((BaseSequentialStream*)&SDDBG, "i2c reset\r\n");
	    if (status == RDY_TIMEOUT)
		    chprintf((BaseSequentialStream*)&SDDBG, "i2c timeout\r\n");
    }
    chprintf((BaseSequentialStream*)&SDDBG, "fram read status %d byte %02x\n", status, rxbuf[0]);
#if 0

    EepromFileOpen(&fram, &icfg);
    chFileStreamSeek(&fram, 0);
    status = chFileStreamRead(&fram, frambuf, 16);
    if (status == 16)
	    dbg_hex_dump(frambuf, 16);
    else
            chprintf((BaseSequentialStream*)&SDDBG, "fram read failure %d\n", status);
    chFileStreamClose(&fram);
#endif

    /*
     * Creates the blinker thread.
      */
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

    /* FIXME */
    chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, gnss, NULL);

    /*
     * Normal main() thread activity.
     */
    int cnt = 0;
     while (TRUE) {
        chThdSleepMilliseconds(1500);
#if 0
	sdWrite(&SDDBG, (uint8_t *)"Hello World!\r\n", 14);
             sdWrite(&SDDBG, (uint8_t *)"Hello World!\r\n", 14);
         if(cnt & 1)
            chprintf((BaseSequentialStream*)&SDDBG, "COM1: %d\r\n", cnt);
#endif
        cnt++;
    }

    return 0;
}

