


#include "file_datablock.h"
#include <unistd.h>
#include <crc16_ccitt.h>


#define SIZE_OF_CRC	2



Tfile_datablock_res
	file_datablock_read(Tfile_datablock *datablock, void *dest)
{
	int fd = datablock->fd;
	if(lseek(fd, datablock->addr, SEEK_SET) != datablock->addr)
		return FILE_DATABLOCK_ESEEK;
	if(read(fd, dest, datablock->size) != datablock->size)
		return FILE_DATABLOCK_EREAD;
	unsigned short crc;
	if(read(fd, &crc, SIZE_OF_CRC) != SIZE_OF_CRC)
		return FILE_DATABLOCK_EREAD;
	if(crc != crc16_ccitt((unsigned char*)dest, datablock->size - 2))
		return FILE_DATABLOCK_ECRPT;
	return FILE_DATABLOCK_OK;
}


Tfile_datablock_res
	file_datablock_write(Tfile_datablock *datablock, const void *src)
{
	int fd = datablock->fd;
	if(lseek(fd, datablock->addr, SEEK_SET) != datablock->addr)
		return FILE_DATABLOCK_ESEEK;
	if(write(fd, src, datablock->size) != datablock->size)
		return FILE_DATABLOCK_EWRITE;
	unsigned short crc = crc16_ccitt((unsigned char*)src, datablock->size - 2);
	if(write(fd, &crc, SIZE_OF_CRC) != SIZE_OF_CRC)
		return FILE_DATABLOCK_EWRITE;
	return FILE_DATABLOCK_OK;
}


int
	file_datablock_init(Tfile_datablock *datablock, int fd, int addr, int size) 
{
	datablock->fd = fd;
	datablock->addr = addr;
	datablock->size = size;
	return size + SIZE_OF_CRC;
}



char const * const file_datablock_strerr_[] = {
	[FILE_DATABLOCK_OK] = "NO ERROR",	//all ok
	[FILE_DATABLOCK_EREAD] = "READ",	//read error
	[FILE_DATABLOCK_EWRITE] = "WRITE",	//write error
	[FILE_DATABLOCK_ESEEK] = "SEEK",	//seek error
	[FILE_DATABLOCK_ECRPT] = "CORRUPT",	//data corrupt
};


