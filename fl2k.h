/* fl2k_spdif                                                            */
/*=======================================================================*/
/* Copyright 2020 Philip Heron <phil@sanslogic.co.uk>                    */
/*                                                                       */
/* This program is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by  */
/* the Free Software Foundation, either version 3 of the License, or     */
/* (at your option) any later version.                                   */
/*                                                                       */
/* This program is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/* GNU General Public License for more details.                          */
/*                                                                       */
/* You should have received a copy of the GNU General Public License     */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _FL2K_H
#define _FL2K_H

#include <pthread.h>
#include <osmo-fl2k.h>

#define _FL2K_BUFFERS 4

typedef struct {
	
	fl2k_dev_t *d;
	int abort;
	uint32_t sample_rate;
	
	uint8_t buffer_r[_FL2K_BUFFERS][FL2K_BUF_LEN];
	pthread_mutex_t mutex[_FL2K_BUFFERS];
	int len;
	int in;
	int out;
	
} fl2k_t;

extern int rf_fl2k_write(fl2k_t *s, int8_t *data, size_t len);
extern int rf_fl2k_close(fl2k_t *s);
extern int rf_fl2k_open(fl2k_t *s, const char *device, uint32_t sample_rate);

#endif

