/*
 * awc_transfer.c
 *  ________   __  _____  ____  _____  ____    ___
 *  \  /\  /  /_/ /_  _/ /___/ /_  _/ / /\ \  / | |
 *   \/__\/  __    / /  /___    / /  / /_/_/ / /| |
 *    \  /  / /   / /  /___    / /  / /\ \  / /_| |
 *     \/  /_/   /_/  /___/   /_/  /_/  \/ /_/  |_|
 *
 *  Created on: 02.09.2010
 *      Author: A.Afanasiev
 */

// #include "debug.h"
#include <string.h>
#include <stdint.h>


#include "awc_transfer.h"
// #include "config.h"
#include "buffer_circ_ram.h"



#define BURST 100	//bytes //512
typedef uint16_t	data_len;

static uint8_t	SPI_recv_buffer[BURST];
static uint8_t	SPI_trans_buffer[BURST];

static uint8_t	awc_trans_fifo_data[512];
Tbuff_cram	awc_trans_fifo = {
		.data = awc_trans_fifo_data,
		.size = sizeof(awc_trans_fifo_data)
};




int awc_space(void){
	return buff_cram_space(&awc_trans_fifo);
}
int awc_write(const void *data, int size){
	return buff_cram_write(&awc_trans_fifo, data, size);
}





//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//*                                    ARM PART                                         *
//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//#define BUILD_TARGET	ARM_PART

#if BUILD_TARGET == ARM_PART


static uint8_t	awc_recv_fifo_data[512];
Tbuff_cram	awc_recv_fifo = {
		.data = awc_recv_fifo_data,
		.size = sizeof(awc_recv_fifo_data)
};


#ifndef __GNUC__
#include <intrinsics.h>
#endif
#include <aic/aic.h>  // AIC_ConfigureIT(), AIC_EnableIT(), AIC_DisableIT()
#include <pio/pio.h>

//#define SPI_DEBUG

// ---------- ARM definitions ------------

// приоритет обработки прерывания о смене состояния NCS (начало сеанса передачи)
#define INTERRUPT_PRIORITY  5


// порт, на котором  будет эмулироваться SPI
#define PIO_BASE  AT91C_BASE_PIOA
#define PIO_ID    AT91C_ID_PIOA


// номера бит порта, на которых будет эмулироваться SPI
// software spi pin definitions
#define NCS_BIT_NUM	11	// NCS	(input)
#define MOSI_BIT_NUM	15	// MOSI	(input)
#define MISO_BIT_NUM	16	// MISO	(output)
#define SCK_BIT_NUM	17	// SCK	(input)


// команды ??
#define READ	0x03
#define WRITE	0x02

// ------------ ARM macros ---------------

#define MASK_NCS	(0x01 << NCS_BIT_NUM)
#define MASK_MOSI	(0x01 << MOSI_BIT_NUM)
#define MASK_MISO	(0x01 << MISO_BIT_NUM)
#define MASK_SCK	(0x01 << SCK_BIT_NUM)

#define PIN_NCS		{MASK_NCS,  PIO_BASE, PIO_ID, PIO_INPUT}	// input
#define PIN_MOSI	{MASK_MOSI, PIO_BASE, PIO_ID, PIO_INPUT}	// input
#define PIN_MISO	{MASK_MISO, PIO_BASE, PIO_ID, PIO_OUTPUT_0}	// output
#define PIN_SCK		{MASK_SCK,  PIO_BASE, PIO_ID, PIO_INPUT}	// input

// состояние всего порта с пинами эмулируемого SPI
#define IN_PINS		(PIO_BASE->PIO_PDSR)

// состояния пинов эмулируемого SPI
#define IN_NCS		(IN_PINS & MASK_NCS)
#define IN_MOSI		((IN_PINS & MASK_MOSI) >> MOSI_BIT_NUM)  // результирующий младший бит
#define IN_MOSI_cur		((cur_PINS & MASK_MOSI) >> MOSI_BIT_NUM)  // результирующий младший бит
#define SHIFT_MOSI(lshift)	(MOSI_BIT_NUM > (lshift)) ? ((IN_PINS & MASK_MOSI) >> (MOSI_BIT_NUM - (lshift))) : ((IN_PINS & MASK_MOSI) << ((lshift) - MOSI_BIT_NUM))
#define IN_SCK		(IN_PINS & MASK_SCK)

// установить состояние выхода данных (MISO) эмулируемого SPI
#define OUT_MISO(x)	do{ if ((x) & 1) PIO_BASE->PIO_SODR = MASK_MISO; else PIO_BASE->PIO_CODR = MASK_MISO; }while(0)
//#define OUT_MISO(x)	do{ PIO_BASE->PIO_SODR = ((x) << MISO_BIT_NUM) & MASK_MISO; PIO_BASE->PIO_CODR = ((~(x)) << MISO_BIT_NUM) & MASK_MISO; }while(0)
//static inline out_MISO(char x)
//{
//	PIO_BASE->PIO_SODR = (x << MISO_BIT_NUM) & MASK_MISO;
//	PIO_BASE->PIO_CODR = ((~x) << MISO_BIT_NUM) & MASK_MISO;
//}

// обновляет переменные текущего и предыдущего состояния пинов
#define UPDATE_PINS   do{prev_PINS = cur_PINS;	cur_PINS = IN_PINS;}while(0)

// текущее состояние чипселекта эмулируемого SPI
#define CUR_NCS		(cur_PINS & MASK_NCS)

//#define SOFT_SPI_TIMEOUT 7000	// оборотов цикла ожидания сигнала SCK
#define SOFT_SPI_TIMEOUT 100000	// оборотов цикла ожидания сигнала SCK




// ---------- ARM local variables --------

static const Pin SPI_pins[] = {
	PIN_NCS,
	PIN_MOSI,
	PIN_MISO,
	PIN_SCK
};

char SPI_access = 0;	// флаг, указывающий, что была попытка обращения
char SPI_timeout = 0;	// флаг, указывающий, что произошел выход по тайм-ауту


#ifdef SPI_DEBUG
#include <stdio.h>
typedef enum _Ttransfer
{
	NON,
	RECEIVE,
	TRANSMIT,
	WAIT
} Ttransfer;
void debug_proc(int data, const char *mask, Ttransfer state)
{
	char *s;
	switch(state)
	{
		case RECEIVE:	s = "RECV";	break;
		case TRANSMIT:	s = "TRAN";	break;
		case WAIT:	s = "WAIT";	break;
		default:	s = "NONE";	break;
	}
	printf(CRLF CRLF" !!! SPI timeout !!! %s %s %d" CRLF CRLF, mask, s, data);
}
#define timeout_debug_proc(__mask)	do{ mask_str = __mask; }while(0)
#else
#define timeout_debug_proc(__mask)
#endif


#define WAIT_FALL(_mask)		\
			do	\
			{	\
				prev_PINS = cur_PINS;	\
				cur_PINS = IN_PINS;	\
					\
				/*if(cur_PINS & MASK_NCS) break;*/	\
					\
				if (!(have_time --))	\
				{	\
					SPI_timeout = 1;	\
					timeout_debug_proc(#_mask);/* TIMEOUT!!! */	\
					goto exit;	/* 8-(o) */ \
				}	\
			}	\
			while ((~cur_PINS | prev_PINS) & _mask)

#define WAIT_FALL_SCK	WAIT_FALL(MASK_SCK)


#define WAIT_RISE(_mask)		\
			do	\
			{	\
				prev_PINS = cur_PINS;	\
				cur_PINS = IN_PINS;	\
					\
				/*if(cur_PINS & MASK_NCS) break;*/	\
					\
				if (!(have_time --))	\
				{	\
					SPI_timeout = 1;	\
					timeout_debug_proc(#_mask);/* TIMEOUT!!! */	\
					goto exit;	\
				}	\
			}	\
			while ((cur_PINS | ~prev_PINS) & _mask)




__ramfunc void ISR_SPI_SOFTWARE(void)
{
	extern void __disable_interrupt(void);
	extern void __enable_interrupt(void);
	__disable_interrupt();

	if(!(IN_PINS & MASK_NCS))
	{
		//OUT_MISO(1);
		register unsigned int cur_PINS = 0, prev_PINS, bytecnt;
		register unsigned int have_time = SOFT_SPI_TIMEOUT;

		int rcvd_len;
		bytecnt = BURST;

#ifdef SPI_DEBUG
		char *mask_str;
		Ttransfer state = RECEIVE;
#endif
		unsigned char *ptr = SPI_recv_buffer;
		register unsigned char byte;
		do {
			WAIT_FALL_SCK;
			byte = IN_MOSI_cur << 7;
			WAIT_FALL_SCK;
			byte |= IN_MOSI_cur << 6;
			WAIT_FALL_SCK;
			byte |= IN_MOSI_cur << 5;
			WAIT_FALL_SCK;
			byte |= IN_MOSI_cur << 4;
			WAIT_FALL_SCK;
			byte |= IN_MOSI_cur << 3;
			WAIT_FALL_SCK;
			byte |= IN_MOSI_cur << 2;
			WAIT_FALL_SCK;
			byte |= IN_MOSI_cur << 1;
			bytecnt --;
			WAIT_FALL_SCK;
			*(ptr++) = byte | IN_MOSI_cur;
		} while(bytecnt);
		bytecnt = BURST;
		ptr = SPI_trans_buffer;
		byte = *(ptr++);

#ifdef SPI_DEBUG
		state = WAIT;
#endif
		WAIT_FALL(MASK_NCS);
#ifdef SPI_DEBUG
		//printf(CRLF "have_time = %d" CRLF, have_time);
#endif
		//goto exit;
#ifdef SPI_DEBUG
		state = TRANSMIT;
#endif
		do {
			OUT_MISO(byte >> 7);
			WAIT_FALL_SCK;
			OUT_MISO(byte >> 6);
			WAIT_FALL_SCK;
			OUT_MISO(byte >> 5);
			WAIT_FALL_SCK;
			OUT_MISO(byte >> 4);
			WAIT_FALL_SCK;
			OUT_MISO(byte >> 3);
			WAIT_FALL_SCK;
			OUT_MISO(byte >> 2);
			WAIT_FALL_SCK;
			OUT_MISO(byte >> 1);
			bytecnt --;
			WAIT_FALL_SCK;
			OUT_MISO(byte >> 0);
			byte = *(ptr++);
			WAIT_FALL_SCK;
		} while(bytecnt);

		SPI_access = 1;

		rcvd_len = *(data_len*)SPI_recv_buffer;
		DEBUGF(AWC, DETAILED, "%d bytes rcvd"CRLF, rcvd_len);
		if(rcvd_len > (sizeof(SPI_trans_buffer) - sizeof(data_len)))
			rcvd_len = 0;

		//awc_transfer_handler(SPI_recv_buffer + sizeof(data_len), rcvd_len);

		buff_cram_write(&awc_recv_fifo, SPI_recv_buffer + sizeof(data_len), rcvd_len);
		//TODO: overflow handling

		*(data_len*)SPI_trans_buffer = buff_cram_read(&awc_trans_fifo, SPI_trans_buffer + sizeof(data_len), sizeof(SPI_trans_buffer) - sizeof(data_len));

exit:
		OUT_MISO(0);
		//debug_proc(bytecnt, mask_str, state);
	}
	AT91C_BASE_PIOA->PIO_ISR;					// dummy read PIO interrupt status register
	__enable_interrupt();
}

#endif




//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//*                                  WaveCom PART                                       *
//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#if BUILD_TARGET == WC_PART
static adl_tmr_t *spi_timer;

// SPI access configuration structure
adl_busAccess_t SPIAccessConfig = {
		.Address = 0,// No Opcode, No Address
		.Opcode = 0,
};
// SPI Subscription data
adl_busSPISettings_t SPIConfig = {
		.Clk_Speed = 64,							// 200kHz	13000/(1 + x ) = SPI SPEED (kHz)
		//.Clk_Speed = 48,							// 250kHz	13000/(1 + x ) = SPI SPEED (kHz)
		//.Clk_Speed = 9,							// 1.3MHz (13000/(1 + x ) = SPI SPEED (kHz))	// битый прием при low оптимизации ARM
		//.Clk_Speed = 12,							// 1MHz (13000/(1 + x ) = SPI SPEED (kHz))		// нормальный прием, битая передача при low оптимизации ARM
		//.Clk_Speed = 16,							// 765kHz (13000/(1 + x ) = SPI SPEED (kHz))		// нормальный прием, нормальная передача при low оптимизации ARM
		//.Clk_Speed = 24,							// 500kHz (13000/(1 + x ) = SPI SPEED (kHz))		// нормальное всё при любой оптимизации ARM
		.Clk_Mode = ADL_BUS_SPI_CLK_MODE_0, 	// Mode 0 clock
		.ChipSelect = ADL_BUS_SPI_ADDR_CS_GPIO,	// Use a GPIO to handle the Chip Select signal
		.ChipSelectPolarity = ADL_BUS_SPI_CS_POL_LOW,		// Chip Select active in low state
		.LsbFirst = ADL_BUS_SPI_MSB_FIRST,		// Data are sent MSB first
		.GpioChipSelect = ADL_IO_GPIO | 31,			// Use GPIO 31 to handle the Chip Select signal
		.LoadSignal = ADL_BUS_SPI_LOAD_UNUSED,	// LOAD signal not used
		.DataLinesConf = ADL_BUS_SPI_DATA_UNIDIR,	// 3 Wires configuration
		.MasterMode = ADL_BUS_SPI_MASTER_MODE,	// Master mode
		.BusySignal = ADL_BUS_SPI_BUSY_UNUSED		// BUSY signal not used
};

u32 SPIHandle = ERROR;


void SPI_transfer( u8 ID, void * Context ){

	*(data_len*)SPI_trans_buffer = buff_cram_read(&awc_trans_fifo, SPI_trans_buffer + sizeof(data_len), sizeof(SPI_trans_buffer) - sizeof(data_len));

	// transfer
	int res;
	res = adl_busWrite(SPIHandle, &SPIAccessConfig, BURST, SPI_trans_buffer);

	res = adl_busRead(SPIHandle, &SPIAccessConfig, sizeof(SPI_recv_buffer), SPI_recv_buffer);
	switch(res){
		case ERROR:
			DEBUGF(AWC, PARANOID, "read bus error"CRLF);
		break;
		case ADL_RET_ERR_UNKNOWN_HDL:
			DEBUGF(AWC, PARANOID, "read bus handle unknown"CRLF);
		break;
		case ADL_RET_ERR_PARAM:
			DEBUGF(AWC, PARANOID, "read bus bad parameter"CRLF);
		break;
		case ADL_RET_ERR_SERVICE_LOCKED:
			DEBUGF(AWC, PARANOID, "read bus forbidden"CRLF);
		break;
	}

	int rcvd_len = *(data_len*)SPI_recv_buffer;
	DEBUGF(AWC, DETAILED, "%d bytes rcvd"CRLF, rcvd_len);
	if(rcvd_len > (sizeof(SPI_trans_buffer) - sizeof(data_len)))
		rcvd_len = 0;

	awc_transfer_handler(SPI_recv_buffer + sizeof(data_len), rcvd_len);
}






#endif




//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//*                                     PC PART                                         *
//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#if BUILD_TARGET == PC_EMUL

void SPI_transfer_emulator(void)
{
	//WaveCom send, ARM read:


	//WaveCom read, ARM send:

}


#endif





//------------------------------------------------------------------------------
// returns 0 if successful or 1 if fail
//int awc_transfer_init(Tawc_transfer_handler handler)
int awc_transfer_init(void)
{
	DEBUGF(AWC, NORMAL, "transfer INIT"CRLF);
	//if(((unsigned long)SPI_trans_buffer) & 3)
	//{
	//	printf("FATAL: SPI_trans misaligned! (%08X)"CRLF, (unsigned long)SPI_trans_buffer);
	//	while(1);
	//}

	//awc_transfer_handler = handler;

	memset(SPI_trans_buffer, 0x00, sizeof(SPI_trans_buffer));	//transmit buffer

#if BUILD_TARGET == ARM_PART

	PIO_Configure(SPI_pins, PIO_LISTSIZE(SPI_pins));	// configure pins for software SPI emulation

	//AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOA;		// enable clock for PIOA controller

	PIO_BASE->PIO_IDR = 0xFFFFFFFF;					// disable all pin interrupts

	AIC_ConfigureIT(PIO_ID, INTERRUPT_PRIORITY | AT91C_AIC_SRCTYPE_EXT_NEGATIVE_EDGE, ISR_SPI_SOFTWARE);
	AIC_EnableIT(PIO_ID);						// enable interrupt routing from PIOA controller

	PIO_BASE->PIO_ISR;						// dummy read PIO pin interrupt status register
	PIO_BASE->PIO_IER = MASK_NCS;					// enable PIO pin interrupt from NCS pin

#elif BUILD_TARGET == WC_PART

	u32 AddSize=0;
	// Subscribe to the SPI1 BUS
	SPIHandle = adl_busSubscribe ( ADL_BUS_ID_SPI, 1, &SPIConfig );
	// Configure the Address length to 0 (rewrite the default value)
	adl_busIOCtl ( SPIHandle, ADL_BUS_CMD_SET_ADD_SIZE, &AddSize );

	spi_timer = adl_tmrSubscribe ( TRUE, 5, ADL_TMR_TYPE_TICK, SPI_transfer );


#elif BUILD_TARGET == PC_EMUL


#endif

	return 0;
}

void awc_transfer_deinit(void)
{
	DEBUGF(AWC, NORMAL, "transfer DEINIT"CRLF);
#if BUILD_TARGET == WC_PART
	adl_tmrUnSubscribe(spi_timer, SPI_transfer, ADL_TMR_TYPE_TICK);
	adl_busUnsubscribe(SPIHandle);
	SPIHandle = ERROR;
#endif
}

