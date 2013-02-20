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

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "k2_serial.h"
#include "eeprom.h"

static WORKING_AREA(waThread1, 128);
static WORKING_AREA(waThread2, 128);
static EepromFileStream fram;

#define EEPROM_PAGE_SIZE        32         /* page size in bytes. Consult datasheet. */
#define EEPROM_SIZE             8192       /* total amount of memory in bytes */
#define EEPROM_I2CD             I2CD1       /* ChibiOS I2C driver used to communicate with EEPROM */
#define EEPROM_I2C_ADDR         0b1010000   /* EEPROM address on bus */
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

static msg_t dbguw(void *p) {

    (void)p;
    chRegSetThreadName("dbguw");
    while (TRUE) {
        chThdSleepMilliseconds(1500);
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
/*
 * Application entry point.
 */
int main(void) {
   int status;
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

    EepromFileOpen(&fram, &icfg);
    chFileStreamSeek(&fram, 0);
    status = chFileStreamRead(&fram, frambuf, 1024);
    if (status == 1024)
	    dbg_hex_dump(frambuf, 1024);
    else
            chprintf((BaseSequentialStream*)&SDDBG, "fram read failure %d\n", status);
    chFileStreamClose(&fram);
#if 0
#endif

    /*
     * Creates the blinker thread.
      */
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

    /* FIXME */
    chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, dbguw, NULL);

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

