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

static WORKING_AREA(waThread1, 128);
/* GNSS */
static const SerialConfig usart1_config = {
	  115200 /* depends */,
	  AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK |
	  AT91C_US_CHRL_8_BITS | AT91C_US_PAR_NONE | AT91C_US_NBSTOP_1_BIT
};
/* PV */
static const SerialConfig usart2_config = {
	  115200,
	  AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK |
	  AT91C_US_CHRL_8_BITS | AT91C_US_PAR_NONE | AT91C_US_NBSTOP_1_BIT
};
/* SEKOP */
static const SerialConfig usart3_config = {
	  115200,
	  AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK |
	  AT91C_US_CHRL_8_BITS | AT91C_US_PAR_NONE | AT91C_US_NBSTOP_1_BIT
};
/* DBGU console */
static const SerialConfig dbgu_config = {
	  115200,
	  AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK |
	  AT91C_US_CHRL_8_BITS | AT91C_US_PAR_NONE | AT91C_US_NBSTOP_1_BIT
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

/*
 * Application entry point.
 */
int main(void) {

    /*
     * System initializations.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs the board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     */
    halInit();
    chSysInit();

    /*
     * Activates the serial driver.
     */
    sdStart(&SD1, &usart1_config);
    sdStart(&SD2, &usart2_config);
    sdStart(&SD3, &usart3_config);
    sdStart(&SDDBG, &dbgu_config);

    /*
     * Creates the blinker thread.
     */
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

    /*
     * Normal main() thread activity.
     */
    int cnt = 0;
    while (TRUE) {
        chThdSleepMilliseconds(500);
            sdWrite(&SDDBG, (uint8_t *)"Hello World!\r\n", 14);
        if(cnt & 1) {
            chprintf((BaseSequentialStream*)&SDDBG, "COM1: %d\r\n", cnt);
        } else {
            chprintf((BaseSequentialStream*)&SDDBG, "COM2: %d\r\n", cnt);
        }
        cnt++;
    }

    return 0;
}

