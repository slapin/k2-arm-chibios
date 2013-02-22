#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "k2_serial.h"
#include "geos1m.h"
msg_t gnss_thread(void *p) {

    (void)p;
    uint8_t buf[256];
    chRegSetThreadName("gnss");
    init_geos1m();
    while (TRUE) {
	    int t = sdGet(&SD1);
            // chprintf((BaseSequentialStream*)&SDDBG, "z %02x\r\n", t);
	    geos1m_parser_input(t);
    }
    return 0;
}

