
#include "timer.h"
#include <string.h>

/*typedef enum _timer_unit_t {
	TIMER_MSEC,
	TIMER_SEC,
} timer_unit_t;

typedef enum _timer_mode_t {
	TIMER_ONCE,
	TIMER_REP,
} timer_mode_t;*/


#define		TIMER_MAX_TIMERS	20


#pragma pack(push, _Struct, 1)
typedef struct _timer_data_t {
	uint32_t left	:29;
	uint8_t inited	:1;
	uint8_t started	:1;
	uint8_t expired :1;
	//uint32_t period	:29;
	//uint8_t mode	:3;
	Ttimer_handler handler;
	void	*	ctx;	//user context
} timer_data_t;
#pragma pack(pop, _Struct)

static volatile timer_data_t timer_array_timers[TIMER_MAX_TIMERS];


void timer_init_timers(void)
{
	void * p = (void *) timer_array_timers;
	memset(p, 0x00, sizeof(timer_array_timers));
}



#ifdef TIMERS_DEC_RAMFUNC
__ramfunc
#endif
void timer_dec_timers(uint16_t msec)	// decrements left of inited & started timers
{
	int i;
	for(i = 0; i < TIMER_MAX_TIMERS; i ++) {
		if(timer_array_timers[i].inited && timer_array_timers[i].started) {
			uint32_t left = timer_array_timers[i].left;
			if(left <= msec) {
				left = 0;
				timer_array_timers[i].started = 0;
				timer_array_timers[i].expired = 1;
			} else
				left -= msec;
			timer_array_timers[i].left = left;
		}
	}
}


void timer_proc_timers(void)				// starts timers handlers, if expired
{
	int i;
	for(i = 0; i < TIMER_MAX_TIMERS; i ++) {
		if(timer_array_timers[i].expired) {
			if(timer_array_timers[i].handler != NULL)
				timer_array_timers[i].handler(timer_array_timers[i].ctx);
			timer_array_timers[i].expired = 0;
		}
	}
}


Ttimer	timer_open(void)
{
	int i;
	for(i = 0; i < TIMER_MAX_TIMERS; i ++) {
		if(!timer_array_timers[i].inited) {
			timer_clear(i);
			timer_array_timers[i].inited = 1;
			return i;
		}
	}
	return -1;
}



void timer_set(Ttimer timer, unsigned long left, Ttimer_handler handler, void *ctx)
{
	volatile timer_data_t *Ptimer = &timer_array_timers[timer];
	Ptimer->left = left;
	Ptimer->handler = handler;
	Ptimer->ctx = ctx;
}



void timer_start(Ttimer timer)
{
	timer_array_timers[timer].started = 1;
}


void timer_stop(Ttimer timer)
{
	timer_array_timers[timer].started = 0;
}


int timer_pending(Ttimer timer)
{
	return timer_array_timers[timer].left;
}


void timer_delay(Ttimer timer, unsigned long delay_time)
{
	timer_set(timer, delay_time, 0, 0);
	timer_start(timer);
	while(timer_pending(timer));
}


void timer_clear(Ttimer timer)
{
	timer_array_timers[timer].started = 0;
	timer_array_timers[timer].left = 0;
	timer_array_timers[timer].expired = 0;
}


void timer_close(Ttimer timer)
{
	timer_array_timers[timer].inited = 0;
	timer_clear(timer);
}



