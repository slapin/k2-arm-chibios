#include "sspi.h"

#include <hal.h>

SEMAPHORE_DECL(semSspi, 0);
uint8_t sspi_byte = 0;

/* Receive byte buffer */
static uint8_t _shift;
/* Receive byte counter */
static uint8_t _shift_count;

static void extSpiNCS(EXTDriver *extp, expchannel_t channel)
{
    (void)extp;
    (void)channel;
#if 0
    if(!palReadPad(IOPORT1, PIOA_SPI_NCS))
    {   /* Start SPI transfer */
        _shift = 0;
        _shift_count = 7; /* 7+1 = 8 bits */
    }
    else
    {   /* End SPI transfer */
    }
#endif
    /*
    palClearPad(GPIOC, GPIOC_LED);
    chSysLockFromIsr();
    if (!chVTIsArmedI(&vt))
        chVTSetI(&vt, MS2ST(200), ledoff, NULL);
    chSysUnlockFromIsr();
    */
}

static void extSpiCLK(EXTDriver *extp, expchannel_t channel)
{
    (void)extp;
    (void)channel;
    if(!palReadPad(IOPORT1, PIOA_SPI_NCS))
    {
        _shift |= palReadPad(IOPORT1, PIOA_SPI_MOSI);
        if(_shift_count == 0) /* Byte done */
        {
            sspi_byte = _shift;
            _shift = 0;
            _shift_count = 7;
            chSysLockFromIsr();
            chSemSignalI(&semSspi);
            chSysUnlockFromIsr();
        }
        else
        {
            _shift_count--;
            _shift <<= 1;
        }
    }
    // if(palReadPad(IOPORT1, PIOA_SPI_MOSI))
    /*
    palClearPad(GPIOC, GPIOC_LED);
    chSysLockFromIsr();
    if (!chVTIsArmedI(&vt))
        chVTSetI(&vt, MS2ST(200), ledoff, NULL);
    chSysUnlockFromIsr();
    */
}

static const EXTConfig extcfg = {
    {
        {EXT_CH_MODE_DISABLED, NULL}, // PA0
        {EXT_CH_MODE_DISABLED, NULL}, // PA1
        {EXT_CH_MODE_DISABLED, NULL}, // PA2
        {EXT_CH_MODE_DISABLED, NULL}, // PA3
        {EXT_CH_MODE_DISABLED, NULL}, // PA4
        {EXT_CH_MODE_DISABLED, NULL}, // PA5
        {EXT_CH_MODE_DISABLED, NULL}, // PA6
        {EXT_CH_MODE_DISABLED, NULL}, // PA7
        {EXT_CH_MODE_DISABLED, NULL}, // PA8
        {EXT_CH_MODE_DISABLED, NULL}, // PA9
        {EXT_CH_MODE_DISABLED, NULL}, // PA10
        {EXT_CH_MODE_ENABLED, extSpiNCS}, // PA11
        {EXT_CH_MODE_DISABLED, NULL}, // PA12
        {EXT_CH_MODE_DISABLED, NULL}, // PA13
        {EXT_CH_MODE_DISABLED, NULL}, // PA14
        {EXT_CH_MODE_DISABLED, NULL}, // PA15

        {EXT_CH_MODE_DISABLED, NULL}, // PA16
        {EXT_CH_MODE_ENABLED, extSpiCLK}, // PA17
        {EXT_CH_MODE_DISABLED, NULL}, // PA18
        {EXT_CH_MODE_DISABLED, NULL}, // PA19
        {EXT_CH_MODE_DISABLED, NULL}, // PA20
        {EXT_CH_MODE_DISABLED, NULL}, // PA21
        {EXT_CH_MODE_DISABLED, NULL}, // PA22
        {EXT_CH_MODE_DISABLED, NULL}, // PA23
        {EXT_CH_MODE_DISABLED, NULL}, // PA24
        {EXT_CH_MODE_DISABLED, NULL}, // PA25
        {EXT_CH_MODE_DISABLED, NULL}, // PA26
        {EXT_CH_MODE_DISABLED, NULL}, // PA27
        {EXT_CH_MODE_DISABLED, NULL}, // PA28
        {EXT_CH_MODE_DISABLED, NULL}, // PA29
        {EXT_CH_MODE_DISABLED, NULL}, // PA30
        {EXT_CH_MODE_DISABLED, NULL} // PA31
    },
    SAM7_EXT_MODE_RISING_EDGE,
    0
};


void sspiInit(void)
{
    _shift = 0;
    _shift_count = 7;
    extStart(&EXTDA, &extcfg);
}

#if 0
/* Receive byte buffer */
static uint8_t _shift;
/* Receive byte counter */
static uint8_t _shift_count;

void intCS(void)
{
    if(!palReadPad(IOPORT1, PIOA_SPI_NCS))
    {   /* Start SPI transfer */
        _shift = 0;
        _shift_count = 7; /* 7+1 = 8 bits */
    }
    else
    {   /* End SPI transfer */
    }
}

/* Hi to low clock */
void intSCK(void)
{
    if(!palReadPad(IOPORT1, PIOA_SPI_NCS))
    {
        _shift |= IN_MOSI_cur;
        if(!_shift_count) /* Byte done */
        {
        }
        else
        {
            _shift_count--;
        }
    }
}
#endif

