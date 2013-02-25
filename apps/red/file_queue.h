/*
 * file_queue.h
 *  Created on: 14.11.2010
 *      Author: Alexander Afanasiev <afalsoft[at]gmail.com>
 */

#ifndef FILE_QUEUE_H_
#define FILE_QUEUE_H_

#include <stdint.h>
#include <time.h>


typedef time_t Tfqueue_time;

typedef enum _Tfqueue_res{
	FQUEUE_OK = 0,	//all ok
	FQUEUE_EBUFF,	//too small buffer for allocation table
	FQUEUE_EIO,		//media access error
	FQUEUE_ENODATA,	//no data in file for this offset
	FQUEUE_ECRPT,	//data corrupt
	FQUEUE_ENOSPC,	//no space in queue
	FQUEUE_EEXIST,	//item with the same time already exists
}Tfqueue_res;

/*typedef struct _Tfqueue_cfg{
	uint16_t	offs	:16;//allocation table offset within the file
	uint16_t	inum	:16;//max items number
	uint16_t	isize	:15;//item size
	uint8_t		cache	:1;	//1 = cache table in tbuff, 0 = release tbuff after each operation (allows sharing of tbuff, but works slower)
}Tfqueue_cfg;*/

typedef struct _Tfqueue{
	int					fd;			//file descriptor
	//Tfqueue_cfg const	*cfg;		//constant parameters
	Tfqueue_time		*tbuff;		//buffer for allocation table
	uint16_t	offs	:16;//allocation table offset within the file
	uint16_t	inum	:16;//max items number
	uint16_t	isize	:15;//item size
	uint8_t		own		:1;	//1 = cache table in tbuff, 0 = release tbuff after each operation (allows sharing of tbuff, but works slower)
	uint8_t		tfresh	:1;	//tbuff freshness: 0 -> need reread table
	Tfqueue_res	res		:4;	//last operation result
}Tfqueue;



// *** initialization of the queue ***
//	use case of queue initialization: (obsolete)
//const Tfqueue_cfg qcfg = {
//	.offs = 1024,	//address within the file
//	.inum = 128,	//max items number
//	.isize = 9,		//item size
//	.cache = 0		//1 = cache table in the buffer, 0 = release buffer after each operation (can share buffer, but slowly)
//};
//uint8_t qbuff[128 * sizeof(Tfqueue_time)];	//buffer for allocation table
//int fd = open("queues.bin", O_RDWR);			// file descriptor
//Tfqueue queue;
//int queue_total_size = fqueue_init(&queue, fd, &qcfg, qbuff, sizeof(qbuff));
int fqueue_init(
		Tfqueue *queue, int fd,
		uint16_t offs, uint16_t inum, uint16_t isize, uint8_t cache,
		void *tbuff, int tbuff_size);
//int fqueue_init(Tfqueue *queue, int fd, Tfqueue_cfg const *cfg, void *tbuff, int tbuff_size);

// read queue table & check it
void fqueue_open(Tfqueue *queue);
// reset queue table & rewrite it
void fqueue_reset(Tfqueue *queue);

// *** add to queue ***
//add item to queue
//will write MIN(queue->q.isize, size) bytes
void fqueue_add(Tfqueue *queue, Tfqueue_time time, void const *src, int size);

// *** reading queue ***
//read item with time between begin & end, nearest to begin
//  dest - pointer to destination buffer
// *size - maximum size to read, returns: size of actually read
//returns time of item, or 0 if nothing to read
Tfqueue_time fqueue_read(Tfqueue *queue, Tfqueue_time begin, Tfqueue_time end, void *dest, int *size);

// *** status of queue ***
//returns the number of items with times >= begin & <= end
int fqueue_status(Tfqueue *queue, Tfqueue_time *begin, Tfqueue_time *end);

// *** cleaning queue ***
//clean items with times >= begin & <= end
//returns the number of deleted items
int fqueue_clean(Tfqueue *queue, Tfqueue_time begin, Tfqueue_time end);






extern const char * const fqueue_strerror_[];

static inline const char *fqueue_strerror(Tfqueue *queue)
{
	return fqueue_strerror_[queue->res];
}


#endif /* FILE_QUEUE_H_ */
