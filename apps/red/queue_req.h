/*
 * queue_req.h
 *  ________   __  _____  ____  _____  ____    ___
 *  \  /\  /  /_/ /_  _/ /___/ /_  _/ / /\ \  / | |
 *   \/__\/  __    / /  /___    / /  / /_/_/ / /| |
 *    \  /  / /   / /  /___    / /  / /\ \  / /_| |
 *     \/  /_/   /_/  /___/   /_/  /_/  \/ /_/  |_|
 *
 *  Created on: 16.11.2010
 *      Author: a.afanasiev
 */

#ifndef QUEUE_REQ_H_
#define QUEUE_REQ_H_

#include "tml_tag.h"
#include "awc_link.h"
#include <stdint.h>
#include <time.h>

#ifdef __OAT_API_VERSION__
#include "adl_global.h"
#endif

typedef enum _Tqueue_ret {
	QUEUE_OK = 0,	//request processed correctly
	QUEUE_EBUSY,	//queue is busy (previous request not complete)
	QUEUE_ESEND,	//error: queue request error
	QUEUE_EREPL,	//error: bad reply (reply size)
	QUEUE_ETOUT,	//error: queue request timeout
	QUEUE_EINV,		//error: invalid request tag
	// under construction:
	//QUEUE_EACCESS,//error: queue media error  (??? -> QUEUE_EREAD/QUEUE_EWRITE)
	//QUEUE_EDATA,//error: bad data read  (??? -> QUEUE_EACCESS)
	//QUEUE_ESTATE,//error: queue with insane state (state record is corrupted) (??? -> QUEUE_EDATA)
} Tqueue_ret;

typedef enum _Tqueue_req_tag {
	QUEUE_STATUS,
	QUEUE_CLEAN,
	QUEUE_READ,
	QUEUE_WRITE,
} Tqueue_req_tag;


#ifndef __IAR_SYSTEMS_ICC__
#define GCC_PACKED	__attribute__((__packed__))
#else
#define GCC_PACKED
#pragma pack(push, _Struct, 1)
#endif



// - - - - - - - - - - REQUESTS: - - - - - - - - - -

typedef struct _Tqueue_req_params {	// base parameters for status, clean & read requests
	uint8_t queue_tag;	//(TML_TAG)
	uint8_t cmd_tag;	//(Tqueue_req_tag) QUEUE_STATUS, QUEUE_CLEAN or QUEUE_READ
	//---------------
	time_t begin;
	time_t end;
	uint8_t data[0];
}
GCC_PACKED
Tqueue_req_params;



// - - - - - - - - - - REPLAYS: - - - - - - - - - -

typedef struct _Tqueue_reply_base {
	uint8_t queue_tag;	//(TML_TAG)
	//---------------
	//uint8_t params[0];
}
GCC_PACKED
Tqueue_reply_base;


typedef struct _Tqueue_reply_read {
	uint8_t queue_tag;	//(TML_TAG)
	//---------------
	time_t	item_time;	//
	uint8_t	item_data[0];
}
GCC_PACKED
Tqueue_reply_read;


typedef struct _Tqueue_reply_status {
	uint8_t queue_tag;	//(TML_TAG)
	//---------------
	uint16_t count;		//count of items
	time_t begin;		//actual begin
	time_t end;			//actual end
}
GCC_PACKED
Tqueue_reply_status;


typedef struct _Tqueue_reply_clean {
	uint8_t queue_tag;	//(TML_TAG)
	//---------------
	uint16_t count;		//count of deleted items
}
GCC_PACKED
Tqueue_reply_clean;



#ifdef __IAR_SYSTEMS_ICC__
#pragma pack(pop, _Struct)
#endif



#ifdef __OAT_API_VERSION__


struct _Tqueue_req;

typedef void (*Tqueues_req_handler) (struct _Tqueue_req *Prequest,
		const Tqueue_reply_base *Preply, int reply_size, Tqueue_ret status);

typedef struct _Tqueue_req {
	Tqueues_req_handler handler;
	adl_tmr_t* timer;
	TML_TAG qtag;
} Tqueue_req;


void	queue_request(Tqueue_req *Prequest, Tqueue_req_params *Pparams);
void	queue_req_init(void);

#endif


#endif /* QUEUE_REQ_H_ */

