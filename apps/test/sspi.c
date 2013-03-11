#include "sspi.h"

#include <hal.h>

// SEMAPHORE_DECL(semSspi, 0);

#define BURST 100	//bytes //512

static uint8_t	SPI_recv_buffer[BURST];
static uint8_t	SPI_trans_buffer[BURST];

static uint8_t *pRecv;
static uint8_t *pTrans;

static uint8_t state;

/* Receive byte buffer */
static uint8_t _shift;
/* Receive byte counter */
static uint8_t _shift_count;
static int _exchange_cnt;

static void extSpiNCS(EXTDriver *extp, expchannel_t channel)
{
    (void)extp;
    (void)channel;
    if(palReadPad(IOPORT1, PIOA_SPI_NCS)) /* Transmission complete */
    {
        switch(state)
        {
            case 1:
                pTrans = SPI_trans_buffer;
                _shift = *pTrans++;
                _shift_count = 8;
                _exchange_cnt = 0;
                state = 2;
                break;
            case 2:
                state = 0;
                break;
        }
    }
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
        switch(state)
        {
            default: // Prepare receive
                _shift = 0;
                _shift_count = 7;
                pRecv = SPI_recv_buffer;
                _exchange_cnt = 0;
                state = 1;
            case 1: // Receive
                _shift |= palReadPad(IOPORT1, PIOA_SPI_MOSI);
                if(_shift_count == 0) /* Byte done */
                {
                    if(_exchange_cnt < BURST) *pRecv++ = _shift;
                    _exchange_cnt++;
                    _shift = 0;
                    _shift_count = 7;
                } else {
                    _shift_count--;
                    _shift <<= 1;
                }
                break;
            case 2:
                if(_shift & 0x80)
                    palSetPad(IOPORT1, PIOA_SPI_MISO);
                else
                    palClearPad(IOPORT1, PIOA_SPI_MISO);
                _shift <<= 1;
                _shift_count--;
                if(_shift_count == 0)
                    _shift = *pTrans++;
                    _shift_count = 8;
                    _exchange_cnt++;
                }
                break;
        }

            /*
            chSysLockFromIsr();
            chSemSignalI(&semSspi);
            chSysUnlockFromIsr();
            */
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
    state = 0;
    _shift = 0;
    _shift_count = 7;
    pRecv = NULL;
    pTrans = NULL;
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

