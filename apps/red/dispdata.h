#ifndef DISPDATA_H
#define DISPDATA_H
#include <string.h>
#include <stdarg.h>
struct dispdata {
	int version;
	time_t gen_time;
	int src;
	int status;
	int route_code;
	char route_name[16];
	int route_suffix;
	int order;
	time_t time_begin;
	time_t time_end;
	int flight;
};

struct dispq_data {
	int d_size;
	unsigned char data[0];
};

static inline int getint(unsigned char **p, int size)
{
	int r;
	if (size > 4 || size < 0)
		return -1;
	memset(&r, 0, sizeof(r));
	memcpy(&r, *p, size);
	(*p) += size;
	return r; /* FIXME: Endianness */
}

static inline void getstr(unsigned char **p, char *s, int size)
{
	/* s should have size bytes allocated */
	memset(s, 0, size);
	memcpy(s, *p, size);
	(*p) += size;
}

static inline void setint(unsigned char **p, int val, int size)
{
	/* Endiannes?? */
	memcpy(*p, &val, size);
	(*p) += size;
}

static inline void setstr(unsigned char **p, char *s, int size)
{
	memset(*p, 0, size);
	strncpy((char *)*p, s, size);
	(*p) += size;
}

static inline void sdisp_fill(struct dispdata *dd,
			unsigned char *pdata)
{
	unsigned char *p = pdata;
	memset(dd, 0, sizeof(struct dispdata));
	/* FIXME: fix sizes to constants or make some other repacking
	 * algorythm */
	dd->version = getint(&p, 1);
	dd->gen_time = getint(&p, 4);
	dd->src = getint(&p, 1);
	dd->status = getint(&p, 1);
	dd->route_code = getint(&p, 2);
	getstr(&p, dd->route_name, 16);
	dd->order = getint(&p, 1);
	dd->time_begin = getint(&p, 4);
	dd->time_end = getint(&p, 4);
	dd->flight = getint(&p, 1);
}
static inline void sdisp_make(struct dispdata *dd, unsigned char *pdata)
{
	unsigned char d1;
	unsigned short d2;
	uint32_t t;
	setint(&pdata, dd->version, 1);
	setint(&pdata, dd->gen_time, 4);
	setint(&pdata, dd->src, 1);
	setint(&pdata, dd->status, 1);
	setint(&pdata, dd->route_code, 2);
	setstr(&pdata, dd->route_name, 16);
	setint(&pdata, dd->order, 1);
	setint(&pdata, dd->time_begin, 4);
	setint(&pdata, dd->time_end, 4);
	setint(&pdata, dd->flight, 1);
}
#endif
extern struct dispdata dispdata;

