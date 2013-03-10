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

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Setup for the Olimex SAM7-P256 development board.
 */

/*
 * Board identifier.
 */
#define BOARD_OLIMEX_SAM7_P256

/*
 * Select your platform by modifying the following line.
 */
#if !defined(SAM7_PLATFORM)
#define SAM7_PLATFORM   SAM7S256
#endif

#include "at91sam7.h"

#define CLK             18432000
#define MCK             48054857

/*
 * Initial I/O setup.
 */
#define VAL_PIOA_ODSR           0x00000000      /* Output data. */
#define VAL_PIOA_OSR            0x00000000      /* Direction. */
#define VAL_PIOA_PUSR           0xFFFFFFFF      /* Pull-up. */

/*
 * I/O definitions.
 */
#define PIOA_LED1               18
#define PIOA_LED1_MASK          (1 << PIOA_LED1_MASK)
#define PIOA_B1                 19
#define PIOA_B1_MASK            (1 << PIOA_B1)
#define PIOA_B2                 20
#define PIOA_B2_MASK            (1 << PIOA_B2)
#define PIOA_DP_PUP             25
#define PIOA_DD_PUP_MASK        (1 << PIOA_DP_PUP)
#define PIOA_USB_D              26
#define PIOA_USB_D_MASK         (1 << PIOA_USB_D)

#define PIOA_MMC_WP             25
#define PIOA_MMC_WP_MASK        (1 << PIOA_MMC_WP)

/* Software SPI */

// slave NCS input 
#define PIOA_SPI_NCS  		11
#define PIOA_SPI_NCS_MASK	(1 << PIOA_SPI_NCS)

// slave MOSI input 
#define PIOA_SPI_MOSI 		15
#define PIOA_SPI_MOSI_MASK	(1 << PIOA_SPI_MOSI)

// slave MISO output
#define PIOA_SPI_MISO 		16
#define PIOA_SPI_MISO_MASK	(1 << PIOA_SPI_MISO)

// slave SCK input 
#define PIOA_SPI_SCK  		17
#define PIOA_SPI_SCK_MASK	(1 << PIOA_SPI_SCK)

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
