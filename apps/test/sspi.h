#ifndef _SSPI_H_
#define _SSPI_H_

#include <ch.h>

extern Semaphore semSspi;
extern uint8_t sspi_byte;

// команды ??
#define READ	0x03
#define WRITE	0x02


void sspiInit(void);

#endif /* _SSPI_H_ */

