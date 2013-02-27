#ifndef SERIAL_H
#define SERIAL_H
void k2_init_serials(void);
void dbg_hex_dump(uint8_t *p, int len);
void pr_debug(char *fmt, ...);
void k2_usart1_geos(void);
void k2_usart1_1k161(void);
#endif
