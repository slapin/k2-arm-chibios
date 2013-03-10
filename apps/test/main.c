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
#include "sspi.h"

static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *p) {

    (void)p;
    chRegSetThreadName("blinker");
    while (TRUE) {
        /*
        palClearPad(IOPORT1, PIOA_LED1);
        chThdSleepMilliseconds(100);
        palSetPad(IOPORT1, PIOA_LED1);
        */
        chThdSleepMilliseconds(800);
    }
    return 0;
}

int cnt = 0;
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
     * Activates the serial driver 1 using the driver default configuration.
     */
    sdStart(&SD1, NULL);
    sdStart(&SD2, NULL);


    sspiInit();

    /*
     * Creates the blinker thread.
     */
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

    chprintf((BaseSequentialStream*)&SD1, "\r\n*** Init\r\n");
    /*
     * Normal main() thread activity.
     */
    while (TRUE) {
        chSemWaitTimeout(&semSspi, MS2ST(1000));
        chprintf((BaseSequentialStream*)&SD1, "SPI: 0x%.2x (cnt:%d)\r\n", sspi_byte, cnt++);
#if 0
        chThdSleepMilliseconds(500);
        if (!palReadPad(IOPORT1, PIOA_B1))
            sdWrite(&SD1, (uint8_t *)"Hello World!\r\n", 14);
        /*
        if (!palReadPad(IOPORT1, PIOB_B2))
            TestThread(&SD1);
            */
        if(cnt & 1) {
            chprintf((BaseSequentialStream*)&SD1, "COM1: %d\r\n", cnt);
        } else {
            chprintf((BaseSequentialStream*)&SD2, "COM2: %d\r\n", cnt);
        }
        cnt++;
#endif
    }

    return 0;
}
