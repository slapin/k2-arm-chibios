/*
 * awc_link.h  // ARM <-> WaveCom link
 *  ________   __  _____  ____  _____  ____    ___
 *  \  /\  /  /_/ /_  _/ /___/ /_  _/ / /\ \  / | |
 *   \/__\/  __    / /  /___    / /  / /_/_/ / /| |
 *    \  /  / /   / /  /___    / /  / /\ \  / /_| |
 *     \/  /_/   /_/  /___/   /_/  /_/  \/ /_/  |_|
 *
 *  Created on: 02.09.2010
 *      Author: Alexander Afanasiev <afalsoft[at]gmail.com>
 */

#ifndef AWC_LINK_H_
#define AWC_LINK_H_

#include <stdint.h>


typedef enum _Tawctag	// message types
{
	//--- GENERAL ---                               --+
	AWC_NOTHING = 0,    //no message                  |
	AWC_SOMETHING,      //message                     | do not change this
	AWC_TIMEOUT,        //no message, timeout code    | release-independent
	AWC_FRELEASE,       //firmware release            | values!!
	AWC_FRELANS,        //firmware release answer     |
	AWC_RESET,			//reset command               |
	AWC_RESETACK,		//reset acknowledge         --+

	AWC_ATRACE,			//trace stream from/to ARM
	AWC_WTRACE,			//trace stream from/to wavecom

	//--- EVENTS ---
	AWC_SYSTIME,		//system time change
	//current NAV & other
	AWC_NAV,			//current coordinates + time (time of receipt of coordinates)
	AWC_NAV_TIME,		//nav time ?
	AWC_NAV_DATE,		//nav date ?
	AWC_NAV_SAT,		//satellites ?number -TODO
	//current ALARM
	AWC_ALARM,			//

	//--- QUEUES ---
	//queue control: (alarm, ?rfid?, ...)	// event add -> queue <-> seek[], read, status[], delete[]	// <-[quetag]<attrs>   ->[quetag]<attrs><[size][data]>
	AWC_QUEUE_CMD,		//queue command	[qtag][params] params:[cmd][cmd params]
	AWC_QUEUE_REPL,		//queue reply [qtag][params]

	//--- STATUS & DIAGS ---
	AWC_LASTDIAGS,
	AWC_WGSMDIAG,		// GSM diagnostics from wavecom

	//--- FILES ---
	//AWC_FILE_CMD,		//[file_descr][params] params:[cmd][cmd params]
	//AWC_FILE_REPL,		//[file_descr] //resets file offset
	//AWC_F_CLEAN,		//[file_descr]
	//sekop
	AWC_TRANSACT,		//sekop transaction  TODO: remove in favor of AWC_FILE
	//PV
	AWC_PVFIRM,			//PV firmware download  TODO: remove in favor of AWC_FILE
	//files
	//AWC_FILE_WOPEN,		//?? [file_descr][file_size]
	
	AWC_STRANSACT_STATUS,
	AWC_STRANSACT_RRST,		//reset read offset
	AWC_STRANSACT_RRST_REPL,
	AWC_STRANSACT_READ,
	AWC_STRANSACT_READ_REPL,
	AWC_STRANSACT_CLEAN,
	AWC_STRANSACT_CLEAN_REPL,

AWC_LASTTAG
}
TAWCtag;


typedef void (*Tawc_msg_handler)(void *data, int size);
int awc_link_init(void);	//must be called before any other functions from this header
int awc_send_msg(TAWCtag tag, const void *src, int size);
void awc_msg_handler_subscribe(TAWCtag tag, Tawc_msg_handler handler);
int awc_link_input_data_parser(void *input_data, int input_data_size);

void awc_link_release(void);
void awc_link_back(void);



typedef void (*Tawc_frelease_handler)(uint16_t release);
int awc_frelease_req(Tawc_frelease_handler handler);


#endif /* AWC_LINK_H_ */
