//---------------------------------------------------------------------------------------
//                     --- EVENTS BUFFERS ---
//  iTETRA
//          URL: http://itetra.ru
//
//          Author: Afanasyev Alexander
//
//---------------------------------------------------------------------------------------

#include "ch.h"
#include "hal.h"
// #include "config.h"
// #include "global.h"
#include "fram.h"
#include "tml.h"
// #include "awc_link.h"
#include "queue_req.h"
#include "file_queue.h"
#include "dispdata.h"

// #include "debug.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <string.h>	// memcpy
#include <stdio.h>
#include "k2_serial.h"
#include "chprintf.h"

#define MIN_TIME 0x494ed880

#define DEBUG(a, b, ...) chprintf((BaseSequentialStream*)&SDDBG, __VA_ARGS__)
#define DEBUGF(a, b, ...) chprintf((BaseSequentialStream*)&SDDBG, __VA_ARGS__)
#define DBGPRINTF(a, b, ...) chprintf((BaseSequentialStream*)&SDDBG, __VA_ARGS__)
#define DEBUGCRLF(a, b) chprintf((BaseSequentialStream*)&SDDBG, "\r\n")
#define CRLF "\r\n"
#define NoYes(c) (c) ? 'Y':'N'
#define DBGCONDITION(x, y) 1
#define DEBUGDUMP_P(x, y, buf, len) dbg_hex_dump(buf, len)
#define DBGPRINTSTR(x, y, s) chprintf((BaseSequentialStream*)&SDDBG, s)

extern time_t sys_time;



//------------------------------Definitions----------------------------------------------

#pragma pack(push, _Struct, 1)		// Set the alignment of 1 byte	// далее идут упакованные структуры - элементы буферов FRAM

typedef struct _Tfram_data_info {	// информация о структуре данных FRAM и её версия	// 14 bytes max!
	unsigned char data_version;
	unsigned char data_sub_version;
	unsigned char reserved[6];
	// последние 2 байта - fram_datablock CRC -> 10 bytes
} Tfram_data_info;

typedef struct _Tfram_arm_uptime {	// информация о структуре данных FRAM и её версия	// 14 bytes max!
	uint32_t seconds;	//in seconds
	// последние 2 байта - CRC -> 6 bytes
} Tfram_arm_uptime;

/*
typedef struct _Tnext_frame_info{
	unsigned short num;
	unsigned short overflows;
}Tnext_frame_info;
*/

#pragma pack(pop, _Struct)		// Set the default alignment

/*
1.	Идентификационная информация (чтение). Включают данные о версии ПО устройства и ID устройства (AuthRep), поддерживаемой версии протокола TML
2.	Данные о местоположении (чтение). Включают данные о широте/долготе от ГЛОНАСС (GeoData) и данные от радиометок (RFID)
3.	Данные с датчиков. Включают состояние цифровых портов ввода/вывода, состояние аналоговых входов (SensorsData)
4.	Данные самодиагностики БО (чтение). Включают данные о наличии подключения GSM, GLONASS, СЭКОП, 433 МГц, время работы без перезагрузки. (DeviceDiag)
5.	Данные от СЭКОПа (чтение). Включают файлы транзакций (SekopTransaction), состояние БО СЭКОП (SekopDiag)
6.	Данные для СЭКОПа (запись). Включают текстовую строку (SekopText) для вывода водителю на дисплей
 alarm -???
*/

Tstatus_fram status_fram;			// ~~~ FRAM SATUS ~~~ - наличие/отсутствие микросхемы, и пр.

//#include "events_data_structures.h"

// -------------------------- описания блоков и буферов FRAM: -------------------------
static Tfile_datablock	datablock_data_info;// блок информации о структуре данных FRAM: (хранится в первых 16 байтах FRAM)
static Tfile_datablock	datablock_ARM_uptime;// блок содержащий последний аптайм
static Tfile_datablock	datablock_dispdata;
static Tfqueue			queue_alarm;	//
static Tfqueue			queue_rfid;		//
static Tfqueue			queue_diaggnss;	//
static Tfqueue			queue_diaggsm;	//
static Tfqueue			queue_diagsekop;	//
static Tfqueue			queue_sdispin, queue_sdispout;	//
       Tfsekop_transact		fram_sekop_transaction;		// транзакция от СЕКОП

#define RETURN_IF_ERROR(...)	if(queue->res != FQUEUE_OK){return __VA_ARGS__;}

static int queue_from_tag(TML_TAG tag, Tfqueue **queue)
{
	switch(tag)	{
		case ALARMDATA: *queue = &queue_alarm; break;
		case RFID: *queue = &queue_rfid; break;
		case DIAGGNSS: *queue = &queue_diaggnss; break;
		case DIAGGSM: *queue = &queue_diaggsm; break;
		case SEKOPSTATUSDATA: *queue = &queue_diagsekop; break;
		case SEKOPDATAIN: *queue = &queue_sdispin; break;
		case SEKOPDATAOUT: *queue = &queue_sdispout; break;
		default:{
			DEBUGF(FRAM, NORMAL, "error: no queue for tag %s"CRLF, tml_tag_str(tag));
			*queue = NULL;
			return 0;
		}
	}
	return 1;
}

#define SEKOPDIAG_SIZE	32
void queue_add_check(Tfqueue *queue, const void *data, int size, time_t tim)
{
	DEBUG(FRAM, DETAILED, "Queue add: 0x%08X, %dbytes" CRLF, tim, size);
	if (tim > MIN_TIME) {
		//ret = queue_add(queue, data, size, tim);
		fqueue_add(queue, tim, data, size);
		if (queue->res == FQUEUE_ENOSPC) {
			//queue_del(queue, 1);
			time_t begin = 1, end = 0x7FFFFFFF;
			pr_debug("Alert: out of space, removing old item\r\n");
			fqueue_status(queue, &begin, &end);//search the oldest item
			RETURN_IF_ERROR();
			fqueue_clean(queue, begin, begin);//delete oldest item
			RETURN_IF_ERROR();
			fqueue_add(queue, tim, data, size);
			if (queue->res == FQUEUE_ENOSPC)
				pr_debug("Alert: still out of space, removing old item\r\n");
			//return queue_add(queue, data, size, time);
		}
	} else {
		DEBUG(FRAM, NORMAL, "Bad time, not saving" CRLF);
	}
}

void fram_add_alarm(Tgeo_pos *pos, uint8_t src)
{
	Tqitem_alarm alarm;
	geo_pack(&alarm.pgeo, pos);
	alarm.src = src;
	queue_add_check(&queue_alarm, &alarm, sizeof(alarm), sys_time);
	if (queue_alarm.res != FQUEUE_OK)
		DEBUG(FRAM, NORMAL, "Failed to write to alarm queue. reason=%s time=%08x" CRLF, fqueue_strerror(&queue_alarm), sys_time);
}

/* XXX Chibios XXX */
#if 0
void fram_add_rfid(TRFIDdata rfid_data, uint8_t status)
{
	Tqitem_rfid rfid;
	rfid.data = *(uint32_t*)&rfid_data;
	/* FIXME for original RFID markers */
	rfid.data &= 0xffff;
	rfid.status = status;
	queue_add_check(&queue_rfid, &rfid, sizeof(rfid), sys_time);
	if (queue_rfid.res != FQUEUE_OK)
		DEBUG(FRAM, NORMAL, "Failed to write to rfid queue. reason=%s" CRLF, fqueue_strerror(&queue_rfid));
}
#endif

void fram_add_diaggnss(uint8_t status)
{
	queue_add_check(&queue_diaggnss, &status, sizeof(status), sys_time);
	if (queue_diaggnss.res != FQUEUE_OK)
		DEBUG(FRAM, NORMAL, "Failed to write to diaggnss queue. reason=%s" CRLF, fqueue_strerror(&queue_diaggnss));
}

static uint8_t gsm_status = 0;
void fram_add_diaggsm(uint8_t status)
{
	if (gsm_status == status)
		return;

	queue_add_check(&queue_diaggsm, &status, sizeof(status), sys_time);
	if (queue_diaggsm.res != FQUEUE_OK)
		DEBUG(FRAM, NORMAL, "Failed to write to diaggsm queue. reason=%s" CRLF, fqueue_strerror(&queue_diaggsm));
	gsm_status = status;
}

int fram_write_sdiag(uint8_t *p, int len)
{
	uint8_t tmp[SEKOPDIAG_SIZE];
	if (len > SEKOPDIAG_SIZE) {
		DEBUG(FRAM, NORMAL, "SEKOPDIAG buffer overflow" CRLF);
		return -1;
	}
	memset(tmp, 0, sizeof(tmp));
	memcpy(tmp, p, len);
	queue_add_check(&queue_diagsekop, tmp, SEKOPDIAG_SIZE, sys_time);
	if (queue_diagsekop.res != FQUEUE_OK) {
		DEBUG(FRAM, NORMAL, "Failed to write to diagsekop queue. reason=%s sys_time=%08x" CRLF, fqueue_strerror(&queue_diagsekop), sys_time);
		return -2;
	}
	return 0;
}

int fram_write_sdispin(void *data, int size)
{
	struct dispq_data *p = malloc(sizeof(struct dispq_data) + size);
	p->d_size = size;
	memcpy(p->data, data, size);
	queue_add_check(&queue_sdispin, p, sizeof(struct dispq_data) + size, sys_time);
	free(p);
	if (queue_sdispin.res != FQUEUE_OK) {
		DEBUG(FRAM, NORMAL, "Failed to write to sdispin queue. reason=%s sys_time=%08x" CRLF, fqueue_strerror(&queue_sdispin), sys_time);
		return -2;
	}
	return 0;
}

int fram_write_sdispout(void *data, int size)
{
	struct dispq_data *p = malloc(sizeof(struct dispq_data) + size);
	p->d_size = size;
	memcpy(p->data, data, size);
	pr_debug("Alert: adding to queue %d bytes\r\n", p->d_size);
	queue_add_check(&queue_sdispout, p, sizeof(struct dispq_data) + size, sys_time);
	free(p);
	if (queue_sdispout.res != FQUEUE_OK) {
		DEBUG(FRAM, NORMAL, "Failed to write to sdispout queue. reason=%s sys_time=%08x" CRLF, fqueue_strerror(&queue_sdispout), sys_time);
		return -2;
	}
	return 0;
}

/* XXX Chibios AWC */
#if 0
static void AWC_WGSMDIAG_handler(void * data, int size)
{
	extern void set_gsm_status(unsigned char u);
	int8_t status = *(int8_t *)data;
	fram_add_diaggsm(status);
	set_gsm_status(status);
}
#endif



#define QUEUE_ITEM_MAX_DATASIZE		256

static uint8_t reply_buff[QUEUE_ITEM_MAX_DATASIZE + sizeof(Tqueue_reply_read)];

/* XXX Chibios AWC */
#if 0
static void AWC_QCMD_handler(void *data, int size) {
	if(sizeof(Tqueue_req_params) > size) {
		DEBUGF(FRAM, NORMAL, "AWC_QCMD_handler wrong request size! (%d)"CRLF, size);
		return;
	}
	Tqueue_req_params *Prequest = data;
	TML_TAG qtag = (TML_TAG)Prequest->queue_tag;
	Tqueue_req_tag ctag = (Tqueue_req_tag)Prequest->cmd_tag;
	time_t begin = Prequest->begin;
	time_t end = Prequest->end;
	if(DBGCONDITION(FRAM, DETAILED)) {
		char const *s;
		switch(ctag) {
			case QUEUE_STATUS: s = "STATUS"; break;
			case QUEUE_CLEAN: s = "CLEAN"; break;
			case QUEUE_READ: s = "READ"; break;
			case QUEUE_WRITE: s = "WRITE"; break;
			default: s = "INVALID"; break;
		}
		DEBUGF(FRAM, 0, "%s REQ %s: beg=0x%08X, end=0x%08X"CRLF, s, tml_tag_str(qtag), begin, end);
	}
	int reply_size = 0;
	Tfqueue *queue;
	 ((Tqueue_reply_base*)reply_buff)->queue_tag = qtag;
	if(queue_from_tag(qtag, &queue)) {
		switch(ctag) {
			case QUEUE_STATUS: {
				int count = fqueue_status(queue, &begin, &end);
				DEBUGF(FRAM, DETAILED, "STATUS RESULT: count=%d, beg=0x%08X, end=0x%08X"CRLF, count, begin, end);
				Tqueue_reply_status *Preply = (void*)reply_buff;
				Preply->count = count;
				Preply->begin = begin;
				Preply->end = end;
				reply_size = sizeof(Tqueue_reply_status);
			break; }
			case QUEUE_CLEAN: {
				int count = fqueue_clean(queue, begin, end);
				DEBUGF(FRAM, DETAILED, "CLEAN RESULT: del count=%d"CRLF, count);
				Tqueue_reply_clean *Preply = (void*)reply_buff;
				Preply->count = count;
				reply_size = sizeof(Tqueue_reply_clean);
			break; }
			case QUEUE_READ: {
				int data_size = QUEUE_ITEM_MAX_DATASIZE;
				Tqueue_reply_read *Preply = (void*)reply_buff;
				time_t tim = fqueue_read(queue, begin, end, Preply->item_data, &data_size);
				DEBUGF(FRAM, DETAILED, "READ RESULT: itime=0x%08X, size=%d"CRLF, tim, data_size);
				Preply->item_time = tim;
				reply_size = sizeof(Tqueue_reply_read) + data_size;
			break; }
			case QUEUE_WRITE: {
				DEBUGF(FRAM, DETAILED, CRLF "WRITE!!! %d bytes" CRLF CRLF, size - sizeof(Tqueue_req_params));
				DEBUGDUMP_P(FRAM, DETAILED, Prequest->data, size - sizeof(Tqueue_req_params));
				if (queue == &queue_sdispout)
					fram_write_sdispout(Prequest->data, size - sizeof(Tqueue_req_params));
			/* FIXME: unify write */
			break; }
		}
		awc_send_msg(AWC_QUEUE_REPL, &reply_buff, reply_size);
	}
}
#endif

/* Sekop transaction status check timer */
static VirtualTimer sekopt_vt;
static WORKING_AREA(wa_sekopt, 128);

/* XXX Chibios timers XXX */
msg_t sekop_timer_thread(void *p)
{
#pragma pack(push, _Struct, 1)
	struct {
		time_t		comp;	//!0 = all packets collected
		uint16_t	size;	//current size of data in transaction buffer
		int8_t		pnum;	//number of last written packet
	} status;
#pragma pack(pop, _Struct)
	int last_pnum, size;
	time_t iscomplete;
	DEBUG(FRAM, NORMAL, "send SEKOP transaction status to WC"CRLF);
	file_sekop_transact_status(&fram_sekop_transaction, &last_pnum, &size, &iscomplete);
	status.pnum = last_pnum;
	status.size = size;
	status.comp = iscomplete;
#if 0
	awc_send_msg(AWC_STRANSACT_STATUS, &status, sizeof(status));
#endif
	return 0;
}

void sekop_transact_timer_handler(void *ctx)
{
	Thread *th;
	chSysLockFromIsr();
	if (chVTIsArmedI(&sekopt_vt))
		chVTResetI(&sekopt_vt);
	chVTSetI(&sekopt_vt, MS2ST(30 * 1000), sekop_transact_timer_handler, ctx);
	th = chThdCreateI(&wa_sekopt, sizeof(wa_sekopt),
		NORMALPRIO, sekop_timer_thread, NULL);
	chThdResumeI(th);
	chSysUnlockFromIsr();
}

/* XXX Chibios AWC XXX */
#if 0
static void AWC_STRANSACT_RRST_handler(void *data, int size)
{
	file_sekop_transact_read_reset(&fram_sekop_transaction);
	awc_send_msg(AWC_STRANSACT_RRST_REPL, NULL, 0);
}
static void AWC_STRANSACT_READ_handler(void *data, int size)
{
	uint8_t buff[32];
	int rsize = file_sekop_transact_read(&fram_sekop_transaction, buff, sizeof(buff));
	awc_send_msg(AWC_STRANSACT_READ_REPL, buff, rsize);
}
static void AWC_STRANSACT_CLEAN_handler(void *data, int size)
{
	file_sekop_transact_clean(&fram_sekop_transaction);
	uint8_t res = fram_sekop_transaction.res;
	awc_send_msg(AWC_STRANSACT_CLEAN_REPL, &res, 1);
}
#endif

static Tfqueue_time qtablebuff[100 + 1];//fqueue table buffer (length = max item number + 1)

#define DISP_SIZE		140
//returnz 0 if ok

int fram_init(void)
{
	/* XXX Chibios AWC XXX */
#if 0
	awc_msg_handler_subscribe(AWC_QUEUE_CMD, AWC_QCMD_handler);
	awc_msg_handler_subscribe(AWC_WGSMDIAG, AWC_WGSMDIAG_handler);
	awc_msg_handler_subscribe(AWC_STRANSACT_RRST, AWC_STRANSACT_RRST_handler);
	awc_msg_handler_subscribe(AWC_STRANSACT_READ, AWC_STRANSACT_READ_handler);
	awc_msg_handler_subscribe(AWC_STRANSACT_CLEAN, AWC_STRANSACT_CLEAN_handler);
#endif
	
	int fd = open("/dev/fram", O_RDWR);
	//header info blocks:
	int offset = file_datablock_init(&datablock_data_info, fd, 0, sizeof(Tfram_data_info));//FARM info (version of data structure)
	offset += file_datablock_init(&datablock_ARM_uptime, fd, offset, sizeof(Tfram_arm_uptime));//ARM last uptime
	if(offset > 16) {
		DEBUG(FRAM, NORMAL, "FRAM header blocks is oversized: %d"CRLF, offset);
		return 1;
	}
	
	//queues:
	offset = 16;
	offset += fqueue_init(&queue_alarm, fd, offset, 100/*inum*/, sizeof(Tqitem_alarm), 0/*own*/, qtablebuff, sizeof(qtablebuff));
	offset += fqueue_init(&queue_rfid, fd, offset, 30/*inum*/, sizeof(Tqitem_rfid), 0/*own*/, qtablebuff, sizeof(qtablebuff));
	offset += fqueue_init(&queue_diaggnss, fd, offset, 30/*inum*/, 1, 0/*own*/, qtablebuff, sizeof(qtablebuff));
	offset += fqueue_init(&queue_diaggsm, fd, offset, 30/*inum*/, 1, 0/*own*/, qtablebuff, sizeof(qtablebuff));
	offset += fqueue_init(&queue_diagsekop, fd, offset, 10/*inum*/, SEKOPDIAG_SIZE, 0/*own*/, qtablebuff, sizeof(qtablebuff));
	offset += fqueue_init(&queue_sdispout, fd, offset, 1/*inum*/, DISP_SIZE, 0/*own*/, qtablebuff, sizeof(qtablebuff));
	offset += fqueue_init(&queue_sdispin, fd, offset, 1/*inum*/, DISP_SIZE, 0/*own*/, qtablebuff, sizeof(qtablebuff));
	offset += file_sekop_transact_init(&fram_sekop_transaction, fd, offset, 1024/*transact buff size*/);
	offset += file_datablock_init(&datablock_dispdata, fd, offset, sizeof(struct dispdata));
	pr_debug("FRAM usage: %d\r\n", offset);
	return 0;
}



#define FRAM_DATA_VERSION		3
#define FRAM_DATA_SUB_VERSION	0


// returns 0 if ok
static char queue_open(Tfqueue *Pqueue, char *trace_str) {
	DEBUGF(FRAM, NORMAL, "open queue %s: err=", trace_str);
	fqueue_open(Pqueue);
	DBGPRINTSTR(FRAM, NORMAL, fqueue_strerror(Pqueue));
	switch(Pqueue->res) {
		case FQUEUE_EIO:
		case FQUEUE_ENODATA:
			DBGPRINTF(FRAM, NORMAL, " - fatal" CRLF);
			return 1;
		case FQUEUE_ECRPT:
			DBGPRINTF(FRAM, NORMAL, " - corrupted -> reset %s queue" CRLF, trace_str);
			fqueue_reset(Pqueue);
			if(Pqueue->res != FQUEUE_OK) {
				DEBUGF(FRAM, NORMAL, "fatal: can't save queue table (reason:%s)" CRLF, fqueue_strerror(Pqueue));
				return 1;
			} else
				return 0;
		default: {
			time_t begin = 0;
			time_t end = 0x7FFFFFFF;
			int count = fqueue_status(Pqueue, &begin, &end);
			DBGPRINTF(FRAM, NORMAL, " - ok, %d items"CRLF, count);
			return 0;
		}
	}
}


int sdispout_get_count(void)
{
	int ret;
	Tfqueue_time end = 0x7fffffff, beg = 0x0;
	pr_debug("damn running status\r\n");
	ret = fqueue_status(&queue_sdispout, &beg, &end);
	pr_debug("fqueue_status: %d\r\n", ret);
	return ret;
}

time_t sdispout_pass_next(void *data, int *size)
{
	int tsize = DISP_SIZE, ret;
	struct dispq_data *d = malloc(DISP_SIZE);
	if (!d)
		return -1;
	ret = fqueue_read(&queue_sdispout, 0, 0x7fffffff, d, &tsize);
	dbg_hex_dump(d, tsize);
	if (tsize > 0) {
		if (d->d_size > DISP_SIZE) {
			*size = 0;
			free(d);
			return ret;
		}
		memcpy(data, d->data, d->d_size);
		*size = d->d_size;
	} else
		*size = 0;
	free(d);
	return ret;
}

void sdispout_delete_item(time_t dtime)
{
	fqueue_clean(&queue_sdispout, dtime, dtime);
}

void fram_uptime_update(uint32_t seconds)
{
	DEBUG(FRAM, DETAILED, "uptime saving (%us)" CRLF, seconds);
	Tfram_arm_uptime uptime;
	uptime.seconds = seconds;//reset uptime
	file_datablock_write(&datablock_ARM_uptime, &uptime);
}


void fram_open(void)
{
	// read FRAM info
	Tfram_data_info *data_info = (void*)reply_buff;
	status_fram.is_present = 0;
	status_fram.ver = 0;
	status_fram.sver = 0;
	Tfile_datablock_res err;
	char attempts = 0;
	
	do {
		DEBUG(FRAM, NORMAL, "read info data block:" CRLF);
		err = file_datablock_read(&datablock_data_info, data_info);
		DEBUGDUMP_P(FRAM, DETAILED, data_info, sizeof(Tfram_data_info));
		DBGPRINTF(FRAM, NORMAL, "err=%s", file_datablock_strerr(err));
		if((err != FILE_DATABLOCK_OK) && (err != FILE_DATABLOCK_ECRPT)) {	// ошибка обращения к FRAM
			DBGPRINTF(FRAM, NORMAL, " - fatal" CRLF);
			break;
		}
		
		if((err != FILE_DATABLOCK_OK) || ((data_info->data_version != FRAM_DATA_VERSION) || (data_info->data_sub_version != FRAM_DATA_SUB_VERSION)))
		{	// ошибка чтения data info (crc) или несовпадение версии
			DBGPRINTF(FRAM, NORMAL, " - fail, reason: " );
			if(err != FILE_DATABLOCK_OK)
				DBGPRINTF(FRAM, NORMAL, "corrupted" CRLF);
			else
				DBGPRINTF(FRAM, NORMAL, "version does not match" CRLF);
			if(attempts > 1)
				break;
			attempts ++;
			memset(data_info, 0x00, sizeof(Tfram_data_info));
			data_info->data_version = FRAM_DATA_VERSION;
			data_info->data_sub_version = FRAM_DATA_SUB_VERSION;
			DEBUGF(FRAM, NORMAL, "write attempt %d info data block v%d.%d"CRLF, attempts, data_info->data_version, data_info->data_sub_version);
			Tfile_datablock_res res = file_datablock_write(&datablock_data_info, data_info);
			if(res != FILE_DATABLOCK_OK) {
				DEBUG(FRAM, NORMAL, "fatal: can't write info block" CRLF);
				break;
			}
		} else {
			DBGPRINTF(FRAM, NORMAL, " - ok" CRLF);
			status_fram.is_present = 1;
			break;
		}
	} while(1);
	
	
	// open FRAM queues & sekop transaction buffer
	if(status_fram.is_present) {
		queue_open(&queue_alarm, "ALARMs");
		queue_open(&queue_rfid, "RFIDs");
		queue_open(&queue_diaggnss, "DIAG GNSS");
		queue_open(&queue_diaggsm, "DIAG GSM");
		queue_open(&queue_diagsekop, "DIAG SEKOP");
		queue_open(&queue_sdispin, "DISPIN");
		queue_open(&queue_sdispout, "DISPOUT");
		
		DEBUGF(FRAM, NORMAL, "open SEKOP transaction buffer: err=");
		file_sekop_transact_open(&fram_sekop_transaction);
		DBGPRINTSTR(FRAM, NORMAL, file_sekop_transact_strerr(&fram_sekop_transaction));
		switch(fram_sekop_transaction.res) {
		case FSEKOP_TRANSACT_EIO:
			DBGPRINTF(FRAM, NORMAL, " - fatal" CRLF);
			break;
		case FSEKOP_TRANSACT_ECRPT:
			DBGPRINTF(FRAM, NORMAL, " - corrupted -> reset SEKOP transaction buffer" CRLF);
			//TODO: corrupt handling (add this event to diag?)
			file_sekop_transact_clean(&fram_sekop_transaction);
			if(fram_sekop_transaction.res != FSEKOP_TRANSACT_OK)
				DEBUGF(FRAM, NORMAL, "fatal: can't save header (reason:%s)" CRLF, file_sekop_transact_strerr(&fram_sekop_transaction));
			else {
				file_sekop_transact_open(&fram_sekop_transaction);
				if(fram_sekop_transaction.res != FSEKOP_TRANSACT_OK)
					DEBUGF(FRAM, NORMAL, "fatal: can't open after reset (reason:%s)" CRLF, file_sekop_transact_strerr(&fram_sekop_transaction));
			}
			break;
		case FSEKOP_TRANSACT_OK: {
			int last_pnum, size;
			time_t iscomplete;
			file_sekop_transact_status(&fram_sekop_transaction, &last_pnum, &size, &iscomplete);
			DBGPRINTF(FRAM, NORMAL, " - ok, last packet = %d, saved size = %d, completeness = %X "CRLF, last_pnum, size, iscomplete);
			break; }
		default:
			DBGPRINTF(FRAM, NORMAL, " - unknown condition" CRLF);
			break;
		}
		/* XXX Chibios XXX */
#if 0	
		timer_sekop_trasact_status = timer_open();
		timer_set(timer_sekop_trasact_status, 60*1000, sekop_transact_timer_handler, NULL);
		timer_start(timer_sekop_trasact_status);
		DEBUG(FRAM, NORMAL, "SEKOP transaction timer registered on id=%d"CRLF, timer_sekop_trasact_status);
#endif
		chSysLock();
		if (chVTIsArmedI(&sekopt_vt))
			chVTResetI((&sekopt_vt));
		chVTSetI(&sekopt_vt, MS2ST(60 * 1000),
			sekop_transact_timer_handler, NULL);
		chSysUnlock();
	} else
		DEBUGF(FRAM, NORMAL, "fatal: chip not present"CRLF);
	// read last uptime
	Tfram_arm_uptime *uptime = (void*)reply_buff;
	DEBUG(FRAM, NORMAL, "read uptime data block:" CRLF);
	err = file_datablock_read(&datablock_ARM_uptime, uptime);
	DEBUGDUMP_P(FRAM, DETAILED, uptime, sizeof(Tfram_arm_uptime));
	DBGPRINTF(FRAM, NORMAL, "err=%s", file_datablock_strerr(err));
	if(err != FILE_DATABLOCK_OK) {	// пока все ошибки обрабатываем как FILE_DATABLOCK_ECRPT
		DBGPRINTF(FRAM, NORMAL, " - corrupted" CRLF);
	} else {
		DBGPRINTF(FRAM, NORMAL, " - ok, last uptime=%us" CRLF, uptime->seconds);
		// здесь пишем в очередь диагностики последний аптайм
	}
	fram_uptime_update(0);//reset uptime
	
}


