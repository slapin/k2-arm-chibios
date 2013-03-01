#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
/* GNSS */
static const SerialConfig usart1_config_geos = {
	  115200 /* depends */,
	  AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK |
	  AT91C_US_CHRL_8_BITS | AT91C_US_PAR_NONE | AT91C_US_NBSTOP_1_BIT
};
static const SerialConfig usart1_config_1k161 = {
	  38400 /* depends */,
	  AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK |
	  AT91C_US_CHRL_8_BITS | AT91C_US_PAR_ODD | AT91C_US_NBSTOP_1_BIT
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
static struct Mutex debug_mutex;
void k2_init_serials(void)
{
    chMtxInit(&debug_mutex);
    /*
     * Activates the serial driver.
     */
    sdStart(&SD1, &usart1_config_geos); /* XXX */
    sdStart(&SD2, &usart2_config);
    sdStart(&SD3, &usart3_config);
    sdStart(&SDDBG, &dbgu_config);
}
void k2_usart1_geos(void)
{
    sdStop(&SD1);
    sdStart(&SD1, &usart1_config_geos);
}
void k2_usart1_1k161(void)
{
    sdStop(&SD1);
    sdStart(&SD1, &usart1_config_1k161);
}


void dbg_hex_dump(uint8_t *p, int len)
{
	char dump_buffer[66];
	int i, j;
	for (i = 0; i < len; i += 16) {
		memset(dump_buffer, ' ', 65);
		dump_buffer[65] = 0;
		for (j = 0; j < 16 && j + i < len; j++) {
			uint8_t c = *(p + i + j);
			snprintf(&dump_buffer[j * 3], 4, "%02x  ", c);
			if (c > ' ' && c < 127)
				dump_buffer[49 + j] = c;
			else
				dump_buffer[49 + j] = '.';
		}
		dump_buffer[48] = ' ';
        	chprintf((BaseSequentialStream*)&SDDBG, "hexdump: %s\r\n", dump_buffer);
	}
}

void pr_debug(const char *p, ...)
{
	va_list ap;
	char * buffer = chHeapAlloc(NULL, 128);
	chMtxLock(&debug_mutex);
	va_start(ap, p);
	vsnprintf(buffer, 127, p, ap);
#if 0
	chprintf((BaseSequentialStream*)&SDDBG, "%s", buffer);
#endif
	va_end(ap);
	chMtxUnlock();
	chHeapFree(buffer);
}

