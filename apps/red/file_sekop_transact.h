
#ifndef FILE_SEKOP_TRANSACT_H_
#define FILE_SEKOP_TRANSACT_H_

#include <stdint.h>
#include <time.h>



typedef enum _Tfsekop_transact_res {
	FSEKOP_TRANSACT_OK = 0,	//all ok
	FSEKOP_TRANSACT_ENOPEN,	//sekop transaction not open
	FSEKOP_TRANSACT_EIO,	//media access error
	FSEKOP_TRANSACT_EOFFS,	//no data for this offset
	FSEKOP_TRANSACT_ECRPT,	//data corrupt
	FSEKOP_TRANSACT_ENOSPC,	//no space in packet buffer
	FSEKOP_TRANSACT_EEXIST,	//packet with the same number already exists
} Tfsekop_transact_res;


#pragma pack(push, _Struct, 1)
typedef struct _Tfsekop_transact_hdr {
	uint32_t	time	:32;	//curent size of data in packet sg buffer
	uint16_t	size	:16;	//curent size of data in packet sg buffer
	int8_t		pnum	:8;		//number of last written packet
	//uint8_t	complete	:1;		//1 = all packets collected
	uint16_t	crc		:16;	//this header crc
} Tfsekop_transact_hdr;
#pragma pack(pop, _Struct)



typedef struct _Tfsekop_transact {
	int			fd;			//file descriptor
	uint16_t	offs	:16;//offset within the file
	uint16_t	size	:16;//size of file buffer for packets
	uint16_t	roffs	:16;//curent read offset
	uint8_t		opened	:1;	//1 = opened
	Tfsekop_transact_res	res	:4;	//last operation result
	Tfsekop_transact_hdr	hdr;	//Tfsekop_transact_hdr read & cache buffer
} Tfsekop_transact;



// writes transaction packet
// complete: !0 closes transaction
// fin_time: !0 finalize transaction & finish time
void file_sekop_write_transact_block(Tfsekop_transact *sekop_transact,
									 const void *packet, int packet_size, int packet_num, time_t fin_time);

//void file_sekop_transact_write_finish(Tfsekop_transact *sekop_transact, time_t fin_time);

// writes in last_pnum & size - number of last written packet & total size of written data respectively
void file_sekop_transact_status(Tfsekop_transact *sekop_transact, int *last_pnum, int *size, time_t *iscomplete);

// erase all data, reset status
void file_sekop_transact_clean(Tfsekop_transact *sekop_transact);

// resets read offset
void file_sekop_transact_read_reset(Tfsekop_transact *sekop_transact);

// read piece of transaction data, move read offset
// returns size of data actually read
int file_sekop_transact_read(Tfsekop_transact *sekop_transact, void *dest, int dest_size);



//
int  file_sekop_transact_init(Tfsekop_transact *sekop_transact, int fd, int offs_file, int size_file_buf);
//
void file_sekop_transact_open(Tfsekop_transact *sekop_transact);







extern char const * const file_sekop_transact_strerr_[];

static inline char const *file_sekop_transact_strerr(Tfsekop_transact *sekop_transact)
{
	return file_sekop_transact_strerr_[sekop_transact->res];
}


#endif //FILE_SEKOP_TRANSACT_H_

