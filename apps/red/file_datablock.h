
#ifndef FILE_DATABLOCK_H
#define FILE_DATABLOCK_H



typedef enum _Tfile_datablock_res {
	FILE_DATABLOCK_OK,		//all ok
	FILE_DATABLOCK_EREAD,	//read error
	FILE_DATABLOCK_EWRITE,	//write error
	FILE_DATABLOCK_ESEEK,	//seek error
	FILE_DATABLOCK_ECRPT,	//data corrupt (bad crc)
} Tfile_datablock_res;


typedef struct _Tfile_datablock {
	int fd;
	int addr;	// address of data block
	int size;	// data block size
} Tfile_datablock;



//returns result of operation
Tfile_datablock_res
	file_datablock_read(Tfile_datablock *datablock, void *dest);

//returns result of operation
Tfile_datablock_res
	file_datablock_write(Tfile_datablock *datablock, const void *src);


//returns actual size of datablock in the file
int file_datablock_init(Tfile_datablock *datablock, int fd, int addr, int size);




extern char const * const file_datablock_strerr_[];

static inline char const *file_datablock_strerr(Tfile_datablock_res err)
{
	return file_datablock_strerr_[err];
}




#endif //FILE_DATABLOCK_H