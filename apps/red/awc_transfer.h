/*
 * awc_transfer.h
 *  ________   __  _____  ____  _____  ____    ___
 *  \  /\  /  /_/ /_  _/ /___/ /_  _/ / /\ \  / | |
 *   \/__\/  __    / /  /___    / /  / /_/_/ / /| |
 *    \  /  / /   / /  /___    / /  / /\ \  / /_| |
 *     \/  /_/   /_/  /___/   /_/  /_/  \/ /_/  |_|
 *
 *  Created on: 02.09.2010
 *      Author: A.Afanasiev
 */

#ifndef AWC_TRANSFER_H_
#define AWC_TRANSFER_H_





int awc_space(void);
int awc_write(const void *data, int size);


//typedef void (*Tawc_transfer_handler)(void *input_data, int input_data_size);
//int awc_transfer_init(Tawc_transfer_handler handler);

int awc_transfer_init(void);
void awc_transfer_deinit(void);


//----------------user-functions-------------------
extern void awc_transfer_handler(void *input_data, int input_data_size);



//---------------for-async-parsing-----------------
#include "buffer_circ_ram.h"
extern Tbuff_cram	awc_recv_fifo;

#endif /* AWC_TRANSFER_H_ */
