#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

#define TIMERS_DEC_RAMFUNC


typedef int				Ttimer;
typedef void			(*Ttimer_handler)(void *ctx);


void	timer_init_timers(void);			// initialize software timers manager, must be called before

#ifdef TIMERS_DEC_RAMFUNC
__ramfunc
#endif
void	timer_dec_timers(uint16_t msec);	// decrements expires of inited & started timers

void	timer_proc_timers(void);			// starts timers callbacks, if expired




Ttimer	timer_open(void);		// if error returns -1, otherwise - ok

void	timer_set(Ttimer timer, unsigned long left, Ttimer_handler handler, void *ctx);
void	timer_start(Ttimer timer);//resume
void	timer_stop(Ttimer timer);
int		timer_pending(Ttimer timer);
void	timer_delay(Ttimer timer, unsigned long delay_time);
void	timer_clear(Ttimer timer);

void	timer_close(Ttimer timer);




// for compatibility:
typedef unsigned long	func_data_t;
typedef void			(*timer_func_ptr_t)(func_data_t);
typedef Ttimer	itetra_timer_t;
#define itetra_timer_open(...)		timer_open(__VA_ARGS__)
static inline void itetra_timer_set(itetra_timer_t timer, unsigned long expires, timer_func_ptr_t func_ptr, func_data_t func_data)
{	timer_set(timer, expires, (Ttimer_handler)func_ptr, (void *)func_data);}
#define itetra_timer_start(...)		timer_start(__VA_ARGS__)
#define itetra_timer_pending(...)	timer_pending(__VA_ARGS__)	//returns msec to call handler
#define itetra_timer_clear(...)		timer_clear(__VA_ARGS__)	//stops & clears the timer

// ---------------------------------------------------------------------------


#endif // __TIMER_H__
