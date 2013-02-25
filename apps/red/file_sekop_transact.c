/*
 * file_sekop_transact.c - SEKOP transaction storage api
 * 
 *      Author: Alexander Afanasiev
 */


#include "file_sekop_transact.h"
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <crc16_ccitt.h>


static inline int buff_offset(Tfsekop_transact *sekop_transact) {
	return sekop_transact->offs + sizeof(Tfsekop_transact_hdr);
}
static inline void write_(Tfsekop_transact *sekop_transact, const void *data, int datasize) {
	int ret = write(sekop_transact->fd, data, datasize);
	if((ret < 0) || (ret != datasize))
		sekop_transact->res = FSEKOP_TRANSACT_EIO;
}
static inline void read_(Tfsekop_transact *sekop_transact, void *data, int datasize) {
	int ret = read(sekop_transact->fd, data, datasize);
	if((ret < 0) || (ret != datasize))
		sekop_transact->res = FSEKOP_TRANSACT_EIO;
}
static inline void lseek_(Tfsekop_transact *sekop_transact, off_t offset, int whence) {
	int ret = lseek(sekop_transact->fd, offset, whence);
	if(ret < 0)
		sekop_transact->res = FSEKOP_TRANSACT_EIO;
}


#define IF_ERROR_RETURN(value)	if(sekop_transact->res != FSEKOP_TRANSACT_OK) { return value; }



static inline uint16_t hdr_crc(Tfsekop_transact_hdr *hdr)
{
	uint16_t crc;
	do{	uint16_t size = hdr->size;
		crc = crc16_ccitt_upd(0xFFFF, (unsigned char *)&size, 2); }while(0);
	do{	uint8_t pnum = hdr->pnum;
		crc = crc16_ccitt_upd(crc, &pnum, 1); }while(0);
	do{	uint32_t tim = hdr->time;
		return crc16_ccitt_upd(crc, (unsigned char *)&tim, 4); }while(0);
}

static void save_hdr(Tfsekop_transact *sekop_transact)
{
	lseek_(sekop_transact, sekop_transact->offs, SEEK_SET);
	IF_ERROR_RETURN();
	sekop_transact->hdr.crc = hdr_crc(&sekop_transact->hdr);
	write_(sekop_transact, &sekop_transact->hdr, sizeof(Tfsekop_transact_hdr));
}
static void read_hdr(Tfsekop_transact *sekop_transact)
{
	lseek_(sekop_transact, sekop_transact->offs, SEEK_SET);
	IF_ERROR_RETURN();
	read_(sekop_transact, &sekop_transact->hdr, sizeof(Tfsekop_transact_hdr));
	IF_ERROR_RETURN();
	if(sekop_transact->hdr.crc != hdr_crc(&sekop_transact->hdr))
		sekop_transact->res = FSEKOP_TRANSACT_ECRPT;
}





void file_sekop_write_transact_block(Tfsekop_transact *sekop_transact, const void *packet, int packet_size, int packet_num, time_t fin_time)
{
	sekop_transact->res = FSEKOP_TRANSACT_OK;
	if(sekop_transact->opened != 1) {
		sekop_transact->res = FSEKOP_TRANSACT_ENOPEN;
		return;
	}
	if(sekop_transact->hdr.pnum >= packet_num) {
		sekop_transact->res = FSEKOP_TRANSACT_EEXIST;
		return;
	}
	int written_size = sekop_transact->hdr.size;
	if((written_size + packet_size) > sekop_transact->size) {
		sekop_transact->res = FSEKOP_TRANSACT_ENOSPC;
		return;
	}
	lseek_(sekop_transact, buff_offset(sekop_transact) + written_size, SEEK_SET);
	IF_ERROR_RETURN();
	write_(sekop_transact, packet, packet_size);
	IF_ERROR_RETURN();
	sekop_transact->hdr.size = written_size + packet_size;
	sekop_transact->hdr.pnum = packet_num;
	sekop_transact->hdr.time = fin_time;
	save_hdr(sekop_transact);
}



void file_sekop_transact_read_reset(Tfsekop_transact *sekop_transact)
{
	sekop_transact->roffs = 0;
}


#define MIN(a, b)		((a) < (b) ? (a) : (b))


int file_sekop_transact_read(Tfsekop_transact *sekop_transact, void *dest, int dest_size)
{
	sekop_transact->res = FSEKOP_TRANSACT_OK;
	if(sekop_transact->opened != 1) {
		sekop_transact->res = FSEKOP_TRANSACT_ENOPEN;
		return 0;
	}
	int read_offs = sekop_transact->roffs;
	int last_size = sekop_transact->hdr.size - read_offs;
	if(last_size <= 0) {
		if (last_size)	// < 0
			sekop_transact->res = FSEKOP_TRANSACT_EOFFS;
		return 0;
	}
	lseek_(sekop_transact, buff_offset(sekop_transact) + read_offs, SEEK_SET);
	IF_ERROR_RETURN(0);
	int read_size = MIN(last_size, dest_size);
	read_(sekop_transact, dest, read_size);
	IF_ERROR_RETURN(0);
	sekop_transact->roffs = read_offs + read_size;
	return read_size;
}







int file_sekop_transact_init(Tfsekop_transact *sekop_transact, int fd, int offs_file, int size_file_buf)
{
	memset(sekop_transact, 0, sizeof(Tfsekop_transact));
	sekop_transact->fd = fd;
	sekop_transact->offs = offs_file;
	sekop_transact->size = size_file_buf;
	sekop_transact->res = FSEKOP_TRANSACT_OK;
	sekop_transact->hdr.pnum = 0;//-1;
	return sizeof(Tfsekop_transact_hdr) + size_file_buf;
}

void file_sekop_transact_open(Tfsekop_transact *sekop_transact)
{
	sekop_transact->res = FSEKOP_TRANSACT_OK;
	read_hdr(sekop_transact);
	IF_ERROR_RETURN();
	sekop_transact->opened = 1;
}

void file_sekop_transact_status(Tfsekop_transact *sekop_transact, int *last_pnum, int *size, time_t *iscomplete)
{
	if(sekop_transact->opened != 1) {
		sekop_transact->res = FSEKOP_TRANSACT_ENOPEN;
		*last_pnum = -1;   *size = -1;   *iscomplete = 0;
	} else {
		*last_pnum = sekop_transact->hdr.pnum;
		*size = sekop_transact->hdr.size;
		*iscomplete = sekop_transact->hdr.time;
	}
}


void file_sekop_transact_clean(Tfsekop_transact *sekop_transact)
{
	sekop_transact->res = FSEKOP_TRANSACT_OK;
	memset(&sekop_transact->hdr, 0, sizeof(Tfsekop_transact_hdr));
	sekop_transact->hdr.pnum = 0;//-1;
	save_hdr(sekop_transact);
}








char const * const file_sekop_transact_strerr_[] = {
	[FSEKOP_TRANSACT_OK] = "NO ERROR",		//all ok
	[FSEKOP_TRANSACT_ENOPEN] = "NOTOPEN",	//sekop transaction not open
	[FSEKOP_TRANSACT_EIO] = "IO",		//media access error
	[FSEKOP_TRANSACT_EOFFS] = "OFFS",	//no data for this offset
	[FSEKOP_TRANSACT_ECRPT] = "CORRUPT",	//data corrupt
	[FSEKOP_TRANSACT_ENOSPC] = "NOSPACE",	//no space in packet sg buffer
	[FSEKOP_TRANSACT_EEXIST] = "EXIST",		//packet with the same number already exists
};

