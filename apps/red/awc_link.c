/*
 * awc_link.c  // ARM <-> WaveCom link
 *  ________   __  _____  ____  _____  ____    ___
 *  \  /\  /  /_/ /_  _/ /___/ /_  _/ / /\ \  / | |
 *   \/__\/  __    / /  /___    / /  / /_/_/ / /| |
 *    \  /  / /   / /  /___    / /  / /\ \  / /_| |
 *     \/  /_/   /_/  /___/   /_/  /_/  \/ /_/  |_|
 *
 *  Created on: 02.09.2010
 *      Author: A.Afanasiev
 */


#include "debug.h"
#include "crc16_ccitt.h"

#include "awc_link.h"
#include "awc_transfer.h"

#include <string.h>


// work only with a stuffing!
#define STUFFING



#ifdef STUFFING
#define ESC				0xDB	//byte stuffing esc symbol
#define FRAME			0xC0	//frame end flag symbol
#define REPLACE_ESC		0xDD
#define REPLACE_FRAME	0xDC
static const uint8_t replace_esc[] = {ESC, REPLACE_ESC};//esc symbol replacement
static const uint8_t replace_frame[] = {ESC, REPLACE_FRAME};//flag symbol replacement
static const uint8_t frame_flag = FRAME;
#define FRAME_FLAG_SIZE		1
#else
#define FRAME_FLAG_SIZE		0
#endif


#define AWC_MSG_CRC_TYPE	uint16_t
#define AWC_MSG_CRC_SIZE	sizeof(AWC_MSG_CRC_TYPE)
#define AWC_CRC_INIT		0xFFFF


#define	AWCTAG_CASE_STR(tag)	case tag: return #tag
char *awctag_2_str(TAWCtag tag)
{
	switch(tag){
	AWCTAG_CASE_STR(AWC_NOTHING);
	AWCTAG_CASE_STR(AWC_SOMETHING);
	AWCTAG_CASE_STR(AWC_TIMEOUT);
	AWCTAG_CASE_STR(AWC_FRELEASE);
	AWCTAG_CASE_STR(AWC_FRELANS);
	AWCTAG_CASE_STR(AWC_RESET);
	AWCTAG_CASE_STR(AWC_RESETACK);

	AWCTAG_CASE_STR(AWC_ATRACE);
	AWCTAG_CASE_STR(AWC_WTRACE);

	AWCTAG_CASE_STR(AWC_SYSTIME);

	AWCTAG_CASE_STR(AWC_NAV);
	AWCTAG_CASE_STR(AWC_NAV_TIME);
	AWCTAG_CASE_STR(AWC_NAV_DATE);
	AWCTAG_CASE_STR(AWC_NAV_SAT);

	AWCTAG_CASE_STR(AWC_ALARM);

	AWCTAG_CASE_STR(AWC_QUEUE_CMD);
	AWCTAG_CASE_STR(AWC_QUEUE_REPL);

	AWCTAG_CASE_STR(AWC_LASTDIAGS);
	AWCTAG_CASE_STR(AWC_WGSMDIAG);

	AWCTAG_CASE_STR(AWC_TRANSACT);
	AWCTAG_CASE_STR(AWC_PVFIRM);
	
	AWCTAG_CASE_STR(AWC_STRANSACT_STATUS);

	default: return "AWC_INVALID";
	}
}


//------------------------- MSG ----------------------------
//AWC message format:	[TAG][LENGTH][...data...][CS]

#ifdef __IAR_SYSTEMS_ICC__
#pragma pack(push, _Struct, 1)
#endif
typedef struct _Tawcmsg_header{
	uint8_t		tag;
	uint16_t	length;
}
#ifdef __IAR_SYSTEMS_ICC__
#pragma pack(pop, _Struct)
#else
__attribute__((__packed__))
#endif
Tawcmsg_header;
//------------------------- /MSG ---------------------------







// * * * * * * * * * * * * * * * * * * SEND MSG * * * * * * * * * * * * * * * * * * * *
#ifdef STUFFING
static int stuffing_size(const void *srcv, int size)
{
	const uint8_t *src = srcv;
	int left = size;
	while(left--)
		switch(*(src++)){
			case ESC:
			case FRAME:
				size++;
			break;
		}
	return size;
}
static void stuffing_write(const void *srcv, int size)
{
	const uint8_t *block = srcv, *src = srcv;
	int block_size = 0;
	while(size--){
		uint8_t c = *(src++);
		if((c == ESC) || (c == FRAME)){
			if(block_size)
				awc_write(block, block_size);
			block = src;
			block_size = 0;
			switch(c){
				case ESC:
					awc_write(replace_esc, sizeof(replace_esc));
				break;
				case FRAME:
					awc_write(replace_frame, sizeof(replace_frame));
				break;
			}
		}else{
			block_size ++;
		}
	}
	if(block_size)
		awc_write(block, block_size);
}
#else
#define stuffing_size(ptr, size)	(size)
#define stuffing_write(ptr, size)	awc_write(ptr, size)
#endif


// returns number of bytes actually send, -1 if error
int awc_send_msg(TAWCtag tag, const void *src, int size)
{
	Tawcmsg_header header;
	header.tag = tag;
	header.length = size;
	AWC_MSG_CRC_TYPE crc;
	crc = crc16_ccitt_upd(AWC_CRC_INIT, (void*)&header, sizeof(header));
	crc = crc16_ccitt_upd(crc, src, size);
	int total_size = stuffing_size(&header, sizeof(Tawcmsg_header)) +
			stuffing_size(src, size) + stuffing_size(&crc, AWC_MSG_CRC_SIZE) + FRAME_FLAG_SIZE;
	if(awc_space() >= total_size){
			DEBUG_DUMP(AWCLINK, PARANOID, &header, sizeof(header));
			DEBUGCRLF(AWCLINK, PARANOID);
		stuffing_write(&header, sizeof(header));
			DEBUG_DUMP(AWCLINK, PARANOID, src, size);
			DEBUGCRLF(AWCLINK, PARANOID);
		stuffing_write(src, size);
			DEBUG_DUMP(AWCLINK, PARANOID, &crc, AWC_MSG_CRC_SIZE);
			DEBUGCRLF(AWCLINK, PARANOID);
		stuffing_write(&crc, AWC_MSG_CRC_SIZE);
		if(FRAME_FLAG_SIZE){
				DEBUG_DUMP(AWCLINK, PARANOID, &frame_flag, FRAME_FLAG_SIZE);
				DEBUGCRLF(AWCLINK, PARANOID);
			awc_write(&frame_flag, FRAME_FLAG_SIZE);
		}
		DEBUGF(AWCLINK, DETAILED, "msg sent tag=%s %d -> %d"CRLF, awctag_2_str(tag), size, total_size);
		return size;
	}else{
		DEBUGF(AWCLINK, NORMAL, "msg %s %d>%d, can't send - channel is busy"CRLF, awctag_2_str(tag), size, total_size);
		return -1;
	}
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))



// * * * * * * * * * * * * * * * * * * HANDLERS * * * * * * * * * * * * * * * * * * * *


static Tawc_msg_handler awc_msg_handler_table[AWC_LASTTAG];


void awc_msg_handler_subscribe(TAWCtag tag, Tawc_msg_handler handler)
{
	if(tag < AWC_LASTTAG){
		awc_msg_handler_table[tag] = handler;
	}else{
		DEBUGF(AWCLINK, NORMAL, "set handler: invalid tag! =%d"CRLF, tag);
	}
}


static void awc_msg_default_handler(void *data, int size)
{
	DEBUGF(AWCLINK, NORMAL, "No handler for data[%d]:"CRLF, size);
	DEBUG_DUMP(AWCLINK, DETAILED, data, size);
	DEBUGCRLF(AWCLINK, DETAILED);
}


static void awc_frelease_req_handler(void *data, int size);

static int awc_link_initialized = 0;

//must call before awc_parse_input_data() & awc_set_msg_handler()
//returns 0 if successful
int awc_link_init(void)
{
	TAWCtag tag;
	for(tag = 0; tag < AWC_LASTTAG; tag ++)
		awc_msg_handler_table[tag] = awc_msg_default_handler;
	awc_msg_handler_subscribe(AWC_FRELANS, awc_frelease_req_handler);
	awc_link_initialized = 1;
	return awc_transfer_init();
}


void awc_link_release(void)
{
	if(awc_link_initialized)
		awc_transfer_deinit();
}

void awc_link_back(void)
{
	if(awc_link_initialized)
		awc_transfer_init();
}



#ifndef STUFFING

static void awc_msg_handler(TAWCtag tag, void *recv_data, int recv_size)
{
	DEBUGF(AWCLINK, PARANOID, "message: tag=%d, size=%d"CRLF, tag, recv_size);
	if(tag < AWC_LASTTAG){
		Tawc_msg_handler handler = awc_msg_handler_table[tag];
		if(handler != NULL)
			handler(recv_data, recv_size);
	}else{
		DEBUGF(AWCLINK, NORMAL, "call: invalid tag! =%d"CRLF, tag);
	}

}
#endif



// * * * * * * * * * * * * * * * * * * * PARSER * * * * * * * * * * * * * * * * * * * *
// TODO: get rid of global variables
static uint8_t	awc_link_recv_buffer[1024];	//буфер принимаемого сообщения
static uint8_t	*awc_link_recv_ptr = awc_link_recv_buffer;
static int	awc_link_recv_size = 0;





#ifdef STUFFING

//int awc_link_input_data_parser(void *input_data, int input_data_size)
void awc_transfer_handler(void *input_data, int input_data_size)
{
	static uint8_t	esc = 0, frame = 0, overflow;

	uint8_t *ptr = input_data;

	if(input_data_size)
	{
		DEBUGF(AWCLINK, PARANOID, "%d bytes arrived"CRLF, input_data_size);
		DEBUG_DUMP(AWCLINK, PARANOID, input_data, input_data_size);
		DEBUGCRLF(AWCLINK, PARANOID);

		while(input_data_size)
		{
			if(frame){// collect data frame
				uint8_t c = *(ptr++);
				input_data_size --;

				switch(c){
					case ESC:
						esc = 1;
					break;
					case FRAME:{// end collect
						if(overflow){
							// bad_frame_handler();
							DEBUG(AWCLINK, NORMAL, "!frame too big");
						}else{// check message in frame, call handler
							//now message(header+data+crc) in awc_link_recv_buffer[], message size(header+data+crc) in awc_link_recv_size, awc_link_recv_ptr points the end+1 of the message(end+1 of crc)
							AWC_MSG_CRC_TYPE crc, calc_crc;
							Tawcmsg_header *p = (Tawcmsg_header *)awc_link_recv_buffer;
							memcpy(&crc, awc_link_recv_ptr - AWC_MSG_CRC_SIZE, AWC_MSG_CRC_SIZE);
							awc_link_recv_size -= AWC_MSG_CRC_SIZE;
							calc_crc = crc16_ccitt_upd(AWC_CRC_INIT, awc_link_recv_buffer, awc_link_recv_size);
							int msg_data_len = p->length;
							TAWCtag tag = p->tag;
							if((crc == calc_crc) && ((awc_link_recv_size - sizeof(Tawcmsg_header)) == msg_data_len)){
								DEBUGF(AWCLINK, DETAILED, "message: tag=%s, size=%d"CRLF, awctag_2_str(tag), msg_data_len);
								DEBUG_DUMP(AWCLINK, PARANOID, awc_link_recv_buffer, awc_link_recv_size);
								DEBUGCRLF(AWCLINK, PARANOID);
								if(tag < AWC_LASTTAG){
									if(tag != AWC_NOTHING) {
										Tawc_msg_handler handler = awc_msg_handler_table[tag];
										if(handler != NULL)
											handler(awc_link_recv_buffer + sizeof(Tawcmsg_header), msg_data_len);
									}
								}else{
									DEBUGF(AWCLINK, NORMAL, "message: invalid tag! =%d"CRLF, tag);
								}
							}else{
								// !!bad message!!
								// bad_message_handler(SPI_rcv_tag, SPIemu_recv_data, SPI_rcv_size, crc, calc_crc);
								DEBUGF(AWCLINK, NORMAL, "!bad message: tag=%s, size=%d, crc=%04X, crccalc=%04X"CRLF, awctag_2_str(tag), awc_link_recv_size, crc, calc_crc);
								DEBUG_DUMP(AWCLINK, PARANOID, awc_link_recv_buffer, awc_link_recv_size);
								DEBUGCRLF(AWCLINK, PARANOID);
							}
						}
						frame = 0;// next input byte begins collect new frame
					}break;
					default:// collect
						if(esc){
							esc = 0;
							switch(c){
								case REPLACE_ESC:
									c = ESC;
								break;
								case REPLACE_FRAME:
									c = FRAME;
								break;
								default:
									DEBUGF(AWCLINK, NORMAL, "!bad ESC seq. 0x%02X", c);
								break;
							}
						}

						if(awc_link_recv_size < sizeof(awc_link_recv_buffer)){
							*awc_link_recv_ptr = c;
							awc_link_recv_ptr ++;
							awc_link_recv_size ++;
						}else{
							overflow = 1;
							// !!frame too big!!
						}
					break;
				}

			}else{// frame start
				awc_link_recv_size = 0;
				awc_link_recv_ptr = awc_link_recv_buffer;
				overflow = 0;
				frame = 1;
			}
		}
	}
}

#else

//Tfifo	awc_link_recv_fifo = {awc_link_recv_fifo_data, sizeof(awc_link_recv_fifo_data), 0, 0};

// calls a handler if a message is parsed
// returns 0 if success
int awc_link_input_data_parser(void *input_data, int input_data_size)
{
	static TAWCtag	awc_link_recv_tag = AWC_NOTHING;
	static int	awc_link_recv_size_left = 0;
	static uint8_t	awc_link_recv_bypass = 0;


	uint8_t *ptr = input_data;

	if(input_data_size)
	{
		DEBUGF(AWCLINK, DETAILED, "%d bytes arrived"CRLF, input_data_size);

		while(input_data_size)
		{
			int proceed = 0;

			if(awc_link_recv_tag)	// collect data from the message
			{

				if(input_data_size)
				{
					if(0)//SPI_rcv_bypass)
					{

						proceed = MIN(input_data_size, awc_link_recv_size_left);
						awc_link_recv_size_left -= proceed;

						if(!awc_link_recv_size_left)
						{
							awc_link_recv_bypass = 0;
							DEBUGF(AWCLINK, NORMAL, "message bypassed"CRLF);
							awc_link_recv_tag = AWC_NOTHING;
						}
					}
					else
					{
						DEBUGF(AWCLINK, NORMAL, "data size=%d"CRLF, input_data_size);

						if(awc_link_recv_size_left > 0)
						{
							int cur_data_size = MIN(input_data_size, awc_link_recv_size_left);

							DEBUGF(AWCLINK, NORMAL, "add msg data size=%d"CRLF, cur_data_size);

							memcpy(awc_link_recv_ptr, ptr, cur_data_size);
							proceed = cur_data_size;

							awc_link_recv_size_left -= proceed;
							awc_link_recv_ptr += proceed;

							if(!awc_link_recv_size_left)
							{
								uint16_t crc, calc_crc;
								memcpy(&crc, &awc_link_recv_buffer[awc_link_recv_size], sizeof(crc));
								calc_crc = crc16_ccitt(awc_link_recv_buffer, awc_link_recv_size);

								if(crc == calc_crc)
								{
									awc_msg_handler(awc_link_recv_tag, awc_link_recv_buffer, awc_link_recv_size);
								}
								else
								{
									// !!bad message!!
									// bad_message_handler(SPI_rcv_tag, SPIemu_recv_data, SPI_rcv_size, crc, calc_crc);
									DEBUGF(AWCLINK, NORMAL, "!bad message: tag=%s, size=%d, crc=%04X, crccalc=%04X"CRLF, awctag_2_str(awc_link_recv_tag), awc_link_recv_size, crc, calc_crc);
								}
								awc_link_recv_tag = AWC_NOTHING;
							}
						}
					}
				}
			}
			else	// search a message
			{
				Tawcmsg_header *msg_header = (void*)ptr;
				if(msg_header->tag && (msg_header->tag < AWC_LASTTAG) /*!*/&& (msg_header->length < 500))
				{
					awc_link_recv_ptr = awc_link_recv_buffer;
					awc_link_recv_tag = (TAWCtag)msg_header->tag;
					awc_link_recv_size = msg_header->length;
					awc_link_recv_size_left = awc_link_recv_size + AWC_MSG_CRC_SIZE;

					DEBUGF(AWCLINK, NORMAL, "msg found tag=%s len=%d"CRLF, awctag_2_str(awc_link_recv_tag), awc_link_recv_size);

					proceed = sizeof(Tawcmsg_header);

					if(0)//SPI_rcv_size_left > sizeof(SPIemu_recv_data))
					{
						DEBUGF(AWCLINK, NORMAL, "!too long message (max length=%d) -> bypass message"CRLF, sizeof(awc_link_recv_buffer) - AWC_MSG_CRC_SIZE);
						awc_link_recv_bypass = 1;
					}
				}
				else
					proceed = 1;
			}

			ptr += proceed;
			input_data_size -= proceed;
		}
	}

	return input_data_size;//now input_data_size contains the number of unprocessed bytes
}



void awc_transfer_handler(void *input_data, int input_data_size)
{
	awc_link_input_data_parser(input_data, input_data_size);
}

#endif






static Tawc_frelease_handler frelease_handler;

static void awc_frelease_req_handler(void *data, int size){
	if(size != 2)
		DEBUG(AWCLINK, NORMAL, "frelease message size is wrong"CRLF);
	else{
		uint16_t rls = *((uint16_t*)data);
		frelease_handler(rls);
		DEBUGF(AWCLINK, NORMAL, "frelease = %hu"CRLF, rls);
	}
}

int awc_frelease_req(Tawc_frelease_handler handler){
	DEBUG(AWCLINK, NORMAL, "send firmware release request"CRLF);
	frelease_handler = handler;
	return awc_send_msg(AWC_FRELEASE, NULL, 0);
}





