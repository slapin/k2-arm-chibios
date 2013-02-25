//---------------------------------------------------------------------------------------
//                     --- EVENTS BUFFERS h ---
//  iTETRA
//          URL: http://itetra.ru
//
//          Author: Afanasyev Alexander
//
//---------------------------------------------------------------------------------------
#ifndef FRAM_H_
#define FRAM_H_

// #include "timer.h"
// #include <adc/rfid.h>

#include "file_datablock.h"
#include "file_queue.h"
#include "file_sekop_transact.h"
#include "queues_item_format.h"

typedef struct Tstatus_fram {		// ~~~ FRAM STATUS ~~~
	char is_present;
	unsigned char ver;
	unsigned char sver;
} Tstatus_fram;

extern Tstatus_fram status_fram;	// ~~~ FRAM SATUS ~~~

/* XXX Chibios XXX */
#if 0
extern Tfsekop_transact	fram_sekop_transaction;		// транзакция от СЕКОП

extern Ttimer timer_sekop_trasact_status;
#endif
void sekop_transact_timer_handler(void *ctx);


void fram_add_alarm(Tgeo_pos *pos, uint8_t src);
/* XXX Chibios XXX */
#if 0
void fram_add_rfid(TRFIDdata rfid_data, uint8_t status);
#endif
void fram_add_diaggnss(uint8_t status);
void fram_add_diaggsm(uint8_t status);
int fram_write_sdiag(uint8_t *p, int len);
struct dispdata;
int fram_read_sdisp(struct dispdata *d);
int fram_write_sdisp(struct dispdata *d);

void fram_uptime_update(uint32_t seconds);

int fram_init(void); //returnz 0 if ok

void fram_open(void);


#endif // FRAM_H_
