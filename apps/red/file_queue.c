/*
 * file_queue.c
 *  Created on: 14.11.2010
 *      Author: Alexander Afanasiev <afalsoft[at]gmail.com>
 */
#include "file_queue.h"
#include "crc16_ccitt.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


#define MIN(a, b)	((a) <= (b) ? (a) : (b))
#define CRC_SIZE	2
#define RETURN_IF_ERROR(...)	if(queue->res != FQUEUE_OK){return __VA_ARGS__;}

static inline int table_size(Tfqueue *queue) {
	return (int)queue->inum * sizeof(Tfqueue_time) + CRC_SIZE;
}

static inline int array_offs(Tfqueue *queue) {
	return queue->offs + table_size(queue);
}

static inline int cell_offs(Tfqueue *queue, int index) {
	return queue->offs + index * sizeof(Tfqueue_time);
}

static inline int item_offs(Tfqueue *queue, int index) {
	return array_offs(queue) + index * queue->isize;
}

static inline void cell_reset(Tfqueue_time *cell) {
	*cell = 0;
}

static inline int cell_is_free(Tfqueue_time cell) {
	return !cell;
}

static inline int cell_in_time(Tfqueue_time cell, Tfqueue_time begin, Tfqueue_time end) {
	if(begin <= 0) begin = 1;
	if(end <= 0) end = 1;
	return (cell >= begin) && (cell <= end);
}

static inline int cell_distance(Tfqueue_time cell, Tfqueue_time tim) {
	//if(cell <= 0) cell = 1;
	//if(tim <= 0) tim = 1;
	return labs(cell - tim);
}

static inline void write_file(Tfqueue *queue, int offs, void const *src, int size) {
	int fd = queue->fd;
	int ret = lseek(fd, offs, SEEK_SET);
	if(ret != offs){
		queue->res = FQUEUE_EIO;
		return;
	}
	ret = write(fd, src, size);
	printf("Alert: write: %d bytes written fd=%d offs=%d\r\n", ret, fd, offs);
	if(ret != size)
		queue->res = FQUEUE_EIO;
}

static inline void read_file(Tfqueue *queue, int offs, void *dest, int size) {
	int fd = queue->fd;
	int ret = lseek(fd, offs, SEEK_SET);
	if(ret != offs){
		queue->res = FQUEUE_EIO;
		return;
	}
	ret = read(fd, dest, size);
	printf("Alert: read: %d bytes written size=%d fd=%d offs=%d\r\n", ret, size, fd, offs);
	if(ret < 0)
		queue->res = FQUEUE_EIO;
	else if(ret != size)
		queue->res = FQUEUE_ENODATA;
}

//static inline void write_cell(Tfqueue *queue, int index, Tfqueue_time cell) {
//	write_file(queue, cell_offs(queue, index), &cell, sizeof(Tfqueue_time));
//}

static inline void write_item(Tfqueue *queue, int index, void const *src, int size) {
	write_file(queue, item_offs(queue, index), src, MIN(queue->isize, size));
}

static inline void read_item(Tfqueue *queue, int index, void *dest, int *size) {
	int isize = MIN(queue->isize, *size);
	printf("Alert: isize = %d, queue->isize = %d, size = %d\n", isize, queue->isize, *size);
	read_file(queue, item_offs(queue, index), dest, isize);
	*size = isize;
}

//returns 1 if good, 0 if bad
static int table_crc_is_good(Tfqueue *queue) {
	uint16_t tcrc;
	memcpy(&tcrc, &queue->tbuff[queue->inum], sizeof(tcrc));
	return tcrc == crc16_ccitt((void*)queue->tbuff, table_size(queue) - CRC_SIZE);
}

static void table_crc_update(Tfqueue *queue) {
	uint16_t ccrc = crc16_ccitt((void*)queue->tbuff, table_size(queue) - CRC_SIZE);
	memcpy(&queue->tbuff[queue->inum], &ccrc, sizeof(ccrc));
}

static inline void save_table(Tfqueue *queue) {
	table_crc_update(queue);
	write_file(queue, queue->offs, queue->tbuff, table_size(queue));
	RETURN_IF_ERROR();
	queue->tfresh = 1;
}

//checks allocation table buffer freshness, and reload it if necessary
static void refresh_table(Tfqueue *queue) {
	if(queue->own && queue->tfresh)
		return;
	read_file(queue, queue->offs, queue->tbuff, table_size(queue));
	switch(queue->res) {
	case FQUEUE_ENODATA:
		//queue->res = FQUEUE_ENOTABLE;
		return;
		//queue->tfresh = 0;
		//queue->res = FQUEUE_OK;
		//save_table(queue);
		//RETURN_IF_ERROR();
	case FQUEUE_OK:
		if(table_crc_is_good(queue))
			queue->tfresh = 1;
		else
			queue->res = FQUEUE_ECRPT;
	break;
	default:
	break;
	}
}



void fqueue_add(Tfqueue *queue, Tfqueue_time itemtime, void const *src, int size) {
	queue->res = FQUEUE_OK;
	refresh_table(queue);
	RETURN_IF_ERROR();
	Tfqueue_time *table = queue->tbuff;
	int i, inum = queue->inum, ifree = -1;
	for(i = 0; i < inum; i++) {
		if(cell_is_free(*table)) {
			ifree = i;
		} else if(cell_in_time(*table, itemtime, itemtime)) {
			queue->res = FQUEUE_EEXIST;
			return;
		}
		table ++;
	}
	if(ifree >= 0) {
		write_item(queue, ifree, src, size);
		if(queue->res != FQUEUE_OK)
			printf("Alert: Couldn't write, error = %d\n",
				queue->res);
		RETURN_IF_ERROR();
		//write_cell(queue, ifree, itemtime);
		//for future: write table crc
		queue->tbuff[ifree] = itemtime;
		queue->tfresh = 0;
		save_table(queue);
		RETURN_IF_ERROR();
		//queue->tbuff[ifree] = itemtime;
		printf("Alert: added %d bytes successfully\r\n", size);
	} else
		queue->res = FQUEUE_ENOSPC;
}



//read item with time between begin & end, nearest to begin
//  dest - pointer to destination buffer
// *size - maximum size to read, returns: size of actually read
//returns time of item, or 0 if nothing to read
Tfqueue_time fqueue_read(Tfqueue *queue, Tfqueue_time begin, Tfqueue_time end, void *dest, int *size) {
	queue->res = FQUEUE_OK;
	refresh_table(queue);
	RETURN_IF_ERROR(0);
	Tfqueue_time *pcell = queue->tbuff;
	int i, item_i = -1, distance = 0x7FFFFFFF;
	Tfqueue_time item_t;
	for(i = 0; i < queue->inum; i++) {
		if(cell_in_time(*pcell, begin, end)) {
			int dist = cell_distance(*pcell, begin);
			if(dist < distance) {
				distance = dist;
				item_t = *pcell;
				item_i = i;
			}
		}
		pcell ++;
	}
	if(item_i >= 0) {
		read_item(queue, item_i, dest, size);
		item_t = queue->tbuff[item_i];
		if (*size == 0)
			printf("Alert: zero-sized item was read\r\n");
		else
			printf("Alert: %d sized item was read\r\n", *size);
	} else {
		printf("Alert: item is not found %d\n", item_i);
		*size = 0;
		cell_reset(&item_t);
	}
	return item_t;
}



//returns the number of items with times >= begin & <= end
int fqueue_status(Tfqueue *queue, Tfqueue_time *begin, Tfqueue_time *end) {
	Tfqueue_time *pcell, min, max;
	int i, count = 0;
	printf("bitch 0, queue = %p\r\n", queue);
	queue->res = FQUEUE_OK;
	printf("bitch 0.1\r\n");
	refresh_table(queue);
	printf("bitch 0.2\r\n");
	RETURN_IF_ERROR(0);
	printf("bitch 0.3\r\n");
	pcell = queue->tbuff;
	printf("bitch 0.4 end=%p\r\n", end);
	min = *end;
	printf("bitch 0.5\r\n");
	max = *begin;
	printf("bitch 1\r\n");
	for(i = 0; i < queue->inum; i++) {
		Tfqueue_time tim = *pcell;
		if(cell_in_time(tim, *begin, *end)) {
			count ++;
			if(min > tim) min = tim;
			if(max < tim) max = tim;
		}
		pcell ++;
	}
	printf("bitch 2\r\n");
	if(count) {
		*begin = min;
		*end = max;
	}
	printf("bitch 3\r\n");
	return count;
}



//clean items with times >= begin & <= end
//returns the number of deleted items
int fqueue_clean(Tfqueue *queue, Tfqueue_time begin, Tfqueue_time end) {
	queue->res = FQUEUE_OK;
	refresh_table(queue);
	RETURN_IF_ERROR(0);
	Tfqueue_time *pcell = queue->tbuff;
	int i, count = 0;
	for(i = 0; i < queue->inum; i++) {
		if(cell_in_time(*pcell, begin, end)) {
			cell_reset(pcell);
			count ++;
		}
		pcell ++;
	}
	queue->tfresh = 0;
	save_table(queue);
	RETURN_IF_ERROR(0);
	return count;
}




void fqueue_open(Tfqueue *queue){
	queue->res = FQUEUE_OK;
	refresh_table(queue);
}
void fqueue_reset(Tfqueue *queue){
	queue->res = FQUEUE_OK;
	memset(queue->tbuff, 0, table_size(queue));
	save_table(queue);
}




//     queue - pointer to a Tfqueue variable
//        fd - file descriptor
//       cfg - pointer to a Tfqueue_cfg constant with constant configuration parameters
//     tbuff - pointer to the buffer for allocation table
//tbuff_size - allocation table buffer size (to check it out)
//return: number of bytes occupied by the queue in the file
//int fqueue_init(Tfqueue *queue, int fd, Tfqueue_cfg const *cfg, void *tbuff, int tbuff_size) {
int fqueue_init(
		Tfqueue *queue, int fd,
		uint16_t offs, uint16_t inum, uint16_t isize, uint8_t own,
		void *tbuff, int tbuff_size)
{
	//queue->cfg = cfg;
	queue->inum = inum;
	int tsize = table_size(queue);
	if(tsize > tbuff_size)
		queue->res = FQUEUE_EBUFF;
	else {
		//TODO: parameters sanity check
		//memset(queue, 0, sizeof(Tfqueue));
		queue->fd = fd;
		//queue->cfg = cfg;
		queue->offs = offs;
		queue->isize = isize;
		queue->own = own;
		memset(tbuff, 0, tsize);
		queue->tbuff = tbuff;
		queue->tfresh = 0;
		queue->res = FQUEUE_OK;
	}
	return tsize + queue->inum * queue->isize;
}




const char * const fqueue_strerror_[] = {
	[FQUEUE_OK] = "NO ERROR",
	[FQUEUE_EBUFF] = "BUFFER",
	[FQUEUE_EIO] = "IO",
	[FQUEUE_ENODATA] = "NODATA",
	[FQUEUE_ECRPT] = "CORRUPT",
	[FQUEUE_ENOSPC] = "NOSPACE",
	[FQUEUE_EEXIST] = "EXIST",
};








