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

#include "ch.h"
#include "hal.h"

/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */
#if HAL_USE_PAL || defined(__DOXYGEN__)
const PALConfig pal_default_config =
{
  {VAL_PIOA_ODSR, VAL_PIOA_OSR, VAL_PIOA_PUSR},
#if (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) || \
    (SAM7_PLATFORM == SAM7X512) || (SAM7_PLATFORM == SAM7A3)
  {VAL_PIOB_ODSR, VAL_PIOB_OSR, VAL_PIOB_PUSR}
#endif
};
#endif

/*
 * SYS IRQ handling here.
 */
static CH_IRQ_HANDLER(SYSIrqHandler) {

  CH_IRQ_PROLOGUE();

  if (AT91C_BASE_PITC->PITC_PISR & AT91C_PITC_PITS) {
    (void) AT91C_BASE_PITC->PITC_PIVR;
    chSysLockFromIsr();
    chSysTimerHandlerI();
    chSysUnlockFromIsr();
  }
  
#if USE_SAM7_DBGU_UART
  if (AT91C_BASE_DBGU->DBGU_CSR & 
    (AT91C_US_RXRDY | AT91C_US_TXRDY | AT91C_US_PARE | AT91C_US_FRAME | AT91C_US_OVRE | AT91C_US_RXBRK)) {
    sd_lld_serve_interrupt(&SDDBG);
  }
#endif  
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}

/*
 * Early initialization code.
 * This initialization must be performed just after stack setup and before
 * any other initialization.
 */
void __early_init(void) {
  unsigned long *p, *d;
  int i;
  
  /* Watchdog disabled.*/
  AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;
  
  at91sam7_clock_init();
  /* Copying vectors to SRAM */
  p = (unsigned long *) 0x200000;
  d = (unsigned long *) 0x104000;
  for (i = 0; i < 10; i++) {
	  *p = *d;
	  p++;
	  d++;
  }
}

/*
 * Board-specific initialization code.
 */
void boardInit(void) {
	palSetGroupMode(IOPORT1, PIOA_GPS_NRST_MASK |
				 PIOA_GPS_PIN1_MASK | 
				 PIOA_WCOM_INT_MASK |
				 PIOA_PV_PWEN_MASK |
				 PIOA_LED_GPSACT_MASK,
				 0,
				 PAL_MODE_OUTPUT_PUSHPULL);
	palSetGroupMode(IOPORT1, PIOA_GPS_ANTSTAT_MASK |
				 PIOA_GPS_SP01_MASK | 
				 PIOA_GPS_OTKAZ |
				 PIOA_ALARM_MASK |
				 PIOA_GPS_PPS_MASK |
				 PIOA_GPS_POUT1_MASK,
				 0,
				 PAL_MODE_INPUT);
	palSetGroupMode(IOPORT2, PIOB_KF0B_SDAT_MASK |
				 PIOB_KF0B_SCLK_MASK |
				 PIOB_KF0B_S4_MASK |
				 PIOB_KF0B_S3_MASK |
				 PIOB_KF0B_S1_MASK |
				 PIOB_KF0B_S2_MASK |
				 PIOB_KF0B_NMCLR_MASK,
				 0,
				 PAL_MODE_OUTPUT_PUSHPULL);
	/* Should configure to {(1 << 11), AT91C_BASE_PIOB, AT91C_ID_PIOB,
	 * PIO_OUTPUT_1}, dunno how */
	palSetPadMode(IOPORT2, PIOB_WLAN_RESET, PAL_MODE_OUTPUT_PUSHPULL);
	palSetGroupMode(IOPORT2,  PIOB_WLAN_RED_MASK |
				  PIOB_WLAN_LINK_MASK |
				  PIOB_WLAN_GREEN_MASK |
				  PIOB_WLAN_SW0_MASK |
				  PIOB_KF0B_RXOUT_MASK,
				  0,
				  PAL_MODE_INPUT);
				  

#if 0
  /*
   * LCD pins setup.
   */
  palClearPad(IOPORT2, PIOB_LCD_BL);
  palSetPadMode(IOPORT2, PIOB_LCD_BL, PAL_MODE_OUTPUT_PUSHPULL);

  palSetPad(IOPORT1, PIOA_LCD_RESET);
  palSetPadMode(IOPORT1, PIOA_LCD_RESET, PAL_MODE_OUTPUT_PUSHPULL);

  /*
   * Joystick and buttons setup.
   */
  palSetGroupMode(IOPORT1,
                  PIOA_B1_MASK | PIOA_B2_MASK | PIOA_B3_MASK |
                  PIOA_B4_MASK | PIOA_B5_MASK,
                  0,
                  PAL_MODE_INPUT);
  palSetGroupMode(IOPORT2, PIOB_SW1_MASK | PIOB_SW2_MASK, 0, PAL_MODE_INPUT);

  /*
   * MMC/SD slot setup.
   */
  palSetGroupMode(IOPORT2,
                  PIOB_MMC_WP_MASK | PIOB_MMC_CP_MASK,
                  0,
                  PAL_MODE_INPUT);
#endif

  /*
   * PIT Initialization.
   */
  AIC_ConfigureIT(AT91C_ID_SYS,
                  AT91C_AIC_SRCTYPE_HIGH_LEVEL | (AT91C_AIC_PRIOR_HIGHEST - 1),
                  SYSIrqHandler);
  AIC_EnableIT(AT91C_ID_SYS);
  AT91C_BASE_PITC->PITC_PIMR = (MCK / 16 / CH_FREQUENCY) - 1;
  AT91C_BASE_PITC->PITC_PIMR |= AT91C_PITC_PITEN | AT91C_PITC_PITIEN;

#if 0
  /*
   * RTS/CTS pins enabled for USART0 only.
   */
  AT91C_BASE_PIOA->PIO_PDR   = AT91C_PA3_RTS0 | AT91C_PA4_CTS0;
  AT91C_BASE_PIOA->PIO_ASR   = AT91C_PIO_PA3 | AT91C_PIO_PA4;
  AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PIO_PA3 | AT91C_PIO_PA4;
#endif
}

