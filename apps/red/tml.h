/*
 * tml.h
 *  ________   __  _____  ____  _____  ____    ___
 *  \  /\  /  /_/ /_  _/ /___/ /_  _/ / /\ \  / | |
 *   \/__\/  __    / /  /___    / /  / /_/_/ / /| |
 *    \  /  / /   / /  /___    / /  / /\ \  / /_| |
 *     \/  /_/   /_/  /___/   /_/  /_/  \/ /_/  |_|
 *
 *  Created on: 09.08.2010
 *      Author: Alexander_Afanasiev
 */

#ifndef TML_H_
#define TML_H_


#ifdef __OAT_API_VERSION__	// compiler predefined symbol
#include "adl_global.h"
#else
#include <stdint.h>
typedef uint16_t u16;
typedef uint32_t u32;
#endif


#include "tml_tag.h"// <---[TML TAGs enumerator here]


extern const uint16_t TmlTagSize[256];



//int tml_init(void);

uint8_t *tag_unpack(uint8_t *ptr, TML_TAG *tag, int *data_size, int *tag_size);




// ================================ TML network methods =============================
u16 htotmls(u16 data);
u16 tmltohs(u16 ndata);
u32 tmltohl(u32 ndata);
u32 htotmll(u32 data);
void memcpy_htotml(void *dest, const void *src, int size);
#define memcpy_tmltoh(...) memcpy_htotml(__VA_ARGS__)




// ===================================== TML data types =============================
uint16_t tml_version(uint8_t ver, uint8_t subver);


// ========================================== TML debug =============================
char *tml_tag_str(TML_TAG tag);	// for teh debug output
//void tml_constr_test(void);
char *tml_tag_debug_str(TML_TAG tag, void *data);
//#define DBGPRINTF_TML_TAG(topic, level, ...) do{ if(DBGCONDITION(topic, level)) tml_print_tag(__VA_ARGS__); }while(0)


#endif /* TML_H_ */
