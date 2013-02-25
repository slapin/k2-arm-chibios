/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
* **** This file incorporates work covered by the following copyright and ****
* **** permission notice:                                                 ****
*
*  Copyright (c) 2009 by Michael Fischer. All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*  1. Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*  2. Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*  3. Neither the name of the author nor the names of its contributors may
*     be used to endorse or promote products derived from this software
*     without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
*  THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
*  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
*  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
*  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
*  SUCH DAMAGE.
*
****************************************************************************
*  History:
*
*  28.03.09  mifi       First Version, based on the original syscall.c from
*                       newlib version 1.17.0
*  17.08.09  gdisirio   Modified the file for use under ChibiOS/RT
*  15.11.09  gdisirio   Added read and write handling
****************************************************************************/

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ch.h"
#if defined(STDOUT_SD) || defined(STDIN_SD)
#include "hal.h"
#endif
#include "eeprom.h"

/***************************************************************************/

static EepromFileStream fram;
#define EEPROM_PAGE_SIZE        128         /* page size in bytes. Consult datasheet. */
#define EEPROM_SIZE             8192       /* total amount of memory in bytes */
#define EEPROM_I2CD             I2CD1       /* ChibiOS I2C driver used to communicate with EEPROM */
#define EEPROM_I2C_ADDR         0x50   /* EEPROM address on bus */
#define EEPROM_WRITE_TIME_MS    20          /* time to write one page in mS. Consult datasheet! */
#define EEPROM_TX_DEPTH         (EEPROM_PAGE_SIZE + 2)/* temporal transmit buffer depth for eeprom driver */

static uint8_t i_buf[EEPROM_TX_DEPTH];
static I2CEepromFileConfig icfg = {
  &EEPROM_I2CD,
  0,
  EEPROM_SIZE, // barrier_hi
  EEPROM_SIZE,
  EEPROM_PAGE_SIZE,
  EEPROM_I2C_ADDR,
  MS2ST(EEPROM_WRITE_TIME_MS),
  i_buf,
};

int _read_r(struct _reent *r, int file, void * ptr, int len)
{
  (void)r;
  int status;
  switch(file) {
  case 0:
#if defined(STDIN_SD)
  if (!len)
	  return 0;
  if (!len || (file != 0)) {
    __errno_r(r) = EINVAL;
    return -1;
  }
  len = sdRead(&STDIN_SD, (uint8_t *)ptr, (size_t)len);
  return len;
#else
  (void)file;
  (void)ptr;
  (void)len;
  __errno_r(r) = EINVAL;
  return -1;
#endif
  break;
  case 4:
  	/* Read FRAM here */
        status = chFileStreamRead(&fram, (uint8_t *)ptr, len);
	if (status == len)
		return status;
	else {
  		__errno_r(r) = EINVAL;
		return -1;
	}
  	break;
  default:
  	__errno_r(r) = EINVAL;
	return -1;
  }
}

/***************************************************************************/

int _lseek_r(struct _reent *r, int file, int ptr, int dir)
{
  (void)r;
  if (file == 4 && dir == SEEK_SET) {
	/* FIXME add margins check */
	chFileStreamSeek(&fram, ptr);
	return ptr;
  } else {
	__errno_r(r) = EINVAL;
	return -1;
  }

  return 0;
}

/***************************************************************************/

int _write_r(struct _reent *r, int file, void * ptr, int len)
{
  int status;
  (void)r;
  (void)file;
  (void)ptr;
  if (len == 0)
	  return 0;
  switch(file) {
  case 0:
  case 1:
  case 2:
#if defined(STDOUT_SD)
  	sdWrite(&STDOUT_SD, (uint8_t *)ptr, (size_t)len);
  	return len;
#endif
  	break;
  case 4:
  	status = chFileStreamWrite(&fram, (uint8_t *)ptr, len);
	if (status == len)
		return status;
	else {
		__errno_r(r) = EINVAL;
		return -1;
	}
  	break;
  default:
	__errno_r(r) = EINVAL;
	return -1;
  	break;
  }
  return len;
}

/***************************************************************************/
int _open_r(struct _reent *r, const char *file, int flags, int mode)
{
	if (!strcmp(file, "/dev/fram")) {
    		EepromFileOpen(&fram, &icfg);
		return 4;
	} else {
		__errno_r(r) = ENOENT;
		return -1;
	}
}
/***************************************************************************/
int _close_r(struct _reent *r, int file)
{
  (void)r;
  if (file == 4)
	  chFileStreamClose(&fram);
  return 0;
}

/***************************************************************************/

caddr_t _sbrk_r(struct _reent *r, int incr)
{
#if CH_USE_MEMCORE
  void *p;

  chDbgCheck(incr > 0, "_sbrk_r");

  (void)r;
  p = chCoreAlloc((size_t)incr);
  if (p == NULL) {
    __errno_r(r) = ENOMEM;
    return (caddr_t)-1;
  }
  return (caddr_t)p;
#else
  __errno_r(r) = ENOMEM;
  return (caddr_t)-1;
#endif
}

/***************************************************************************/

int _fstat_r(struct _reent *r, int file, struct stat * st)
{
  (void)r;
  (void)file;

  memset(st, 0, sizeof(*st));
  st->st_mode = S_IFCHR;
  return 0;
}

/***************************************************************************/

int _isatty_r(struct _reent *r, int fd)
{
  (void)r;
  switch(fd) {
  case 0:
  case 1:
  case 2:
  	return 1;
  default:
  	return 0;
  }
}

/*** EOF ***/
