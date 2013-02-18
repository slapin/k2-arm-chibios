/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012 Giovanni Di Sirio.

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

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Setup for the Olimex SAM7-EX256 development board.
 */

/*
 * Board identifier.
 */
#define BOARD_ITETRA_K2
#define BOARD_NAME "ITETRA K2"

/*
 * Select your platform by modifying the following line.
 */
#if !defined(SAM7_PLATFORM)
#define SAM7_PLATFORM   SAM7A3
#endif

#include "at91sam7.h"

#define CLK             18432000
#define MCK             60000000

/*
 * Initial I/O setup.
 */
#define VAL_PIOA_ODSR           0x00000000      /* Output data. */
#define VAL_PIOA_OSR            0x00000000      /* Direction. */
#define VAL_PIOA_PUSR           0xFFFFFFFF      /* Pull-up. */

#define VAL_PIOB_ODSR           0x00000000      /* Output data. */
#define VAL_PIOB_OSR            0x00000000      /* Direction. */
#define VAL_PIOB_PUSR           0xFFFFFFFF      /* Pull-up. */
/* K2 pins */
/* After RESET - RESET GPS/GLONASS module */
#define PIOA_GPS_NRST		4
#define PIOA_GPS_NRST_MASK	(1 << PIOA_GPS_NRST)
/* Antenna status from GPS/GLONASS module */
#define PIOA_GPS_ANTSTAT	5
#define PIOA_GPS_ANTSTAT_MASK	(1 << PIOA_GPS_ANTSTAT)
#define PIOA_GPS_PIN1		6
#define PIOA_GPS_PIN1_MASK	(1 << PIOA_GPS_PIN1)
/* SPO1 status from GPS/GLONASS module (TRUE/FALSE) */
#define PIOA_GPS_SP01		12
#define PIOA_GPS_SP01_MASK	(1 << PIOA_GPS_SP01)
/* OTKAZ status from GPS/GLONASS module (TRUE/FALSE) */
#define PIOA_GPS_OTKAZ		13
#define PIOA_GPS_OTKAZ_MASK	(1 << PIOA_GPS_OTKAZ)
#define PIOA_WCOM_INT		14
#define PIOA_WCOM_INT_MASK	(1 << PIOA_WCOM_INT)
/* Alarm key pin */
#define PIOA_ALARM		18
#define PIOA_ALARM_MASK		(1 << PIOA_ALARM)
#define PIOA_PV_PWEN		19
#define PIOA_PV_PWEN_MASK	(1 << PIOA_PV_PWEN)
#define PIOA_GPS_PPS		22
#define PIOA_GPS_PPS_MASK	(1 << PIOA_GPS_PPS)
#define PIOA_GPS_POUT1		23
#define PIOA_GPS_POUT1_MASK	(1 << PIOA_GPS_POUT1)
#define PIOA_LED_GPSACT		29
#define PIOA_LED_GPSACT_MASK	(1 << PIOA_LED_GPSACT)

#define PIOB_KF0B_SDAT		0
#define PIOB_KF0B_SDAT_MASK	(1 << PIOB_KF0B_SDAT)
#define PIOB_KF0B_SCLK		1
#define PIOB_KF0B_SCLK_MASK	(1 << PIOB_KF0B_SCLK)
#define PIOB_KF0B_S4		2
#define PIOB_KF0B_S4_MASK	(1 << PIOB_KF0B_S4)
#define PIOB_KF0B_S3		3
#define PIOB_KF0B_S3_MASK	(1 << PIOB_KF0B_S3)
#define PIOB_KF0B_S1		4
#define PIOB_KF0B_S1_MASK	(1 << PIOB_KF0B_S1)
#define PIOB_KF0B_S2		5
#define PIOB_KF0B_S2_MASK	(1 << PIOB_KF0B_S2)
#define PIOB_KF0B_NMCLR		8
#define PIOB_KF0B_NMCLR_MASK	(1 << PIOB_KF0B_NMCLR)
#define PIOB_WLAN_RED		9
#define PIOB_WLAN_RED_MASK	(1 << PIOB_WLAN_RED)
#define PIOB_WLAN_LINK		10
#define PIOB_WLAN_LINK_MASK	(1 << PIOB_WLAN_LINK)
#define PIOB_WLAN_RESET		11
#define PIOB_WLAN_RESET_MASK	(1 << PIOB_WLAN_RESET)
#define PIOB_WLAN_GREEN		12
#define PIOB_WLAN_GREEN_MASK	(1 << PIOB_WLAN_GREEN)
#define PIOB_WLAN_SW0		13
#define PIOB_WLAN_SW0_MASK	(1 << PIOB_WLAN_SW0)
#define PIOB_KF0B_RXOUT		21
#define PIOB_KF0B_RXOUT_MASK	(1 << PIOB_KF0B_RXOUT)

/*
/// DBGU pins (DTXD and DRXD) definitions.
#define PINS_DBGU  {0xC0000000, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
PIOA, 30, 31, PIO_PERIPH_A

/// USART0 TXD & RXD pins definition.
#define PINS_USART0  {((1 << 2) | (1 << 3)), AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
PIOA, 2, 3, PIO_PERIPH_A

/// USART1 TXD & RXD pins definition.
#define PINS_USART1  {((1 << 7) | (1 << 8)), AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
PIOA, 7, 8, PIO_PERIPH_A

/// USART2 TXD & RXD pins definition.
#define PINS_USART2  {((1 << 9) | (1 << 10)), AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
PIOA, 9, 10, PIO_PERIPH_A

ADC:
PIOB: 14, 15, 16, 17, 18, 19, 20

I2C:
PIOA, 0, 1, PIO_PERIPH_A
*/

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */
