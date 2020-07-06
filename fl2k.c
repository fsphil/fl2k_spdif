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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <osmo-fl2k.h>
#include <pthread.h>
#include "fl2k.h"

static void _callback(fl2k_data_info_t *data_info)
{
	fl2k_t *s = data_info->ctx;
	int i;
	
	if(data_info->device_error)
	{
		s->abort = 1;
		return;
	}
	
	/* Try to get a lock on the next output buffer */
	i = (s->out + 1) % _FL2K_BUFFERS;
	
	if(pthread_mutex_trylock(&s->mutex[i]) != 0)
	{
		/* No luck, the writer must have it */
		fprintf(stderr, "U");
		return;
	}
	
	/* Got a lock on the next buffer, clear and release the previous */
	pthread_mutex_unlock(&s->mutex[s->out]);
	
	s->out = i;
	
	data_info->sampletype_signed = 0;
	data_info->r_buf = (char *) s->buffer_r[s->out];
	data_info->g_buf = NULL;
	data_info->b_buf = NULL;
}

int rf_fl2k_write(fl2k_t *s, int8_t *data, size_t len)
{
	int i;
	
	if(s->abort)
	{
		return(-1);
	}
	
	while(len > 0)
	{
		for(; s->len < FL2K_BUF_LEN && len > 0; s->len++, len--)
		{
			s->buffer_r[s->in][s->len] = 128 + *(data++);
		}
		
		if(s->len == FL2K_BUF_LEN)
		{
			/* This buffer is full. Move on to the next */
			i = (s->in + 1) % _FL2K_BUFFERS;
			pthread_mutex_lock(&s->mutex[i]);
			pthread_mutex_unlock(&s->mutex[s->in]);
			s->in = i;
			s->len = 0;
		}
	}
	
	return(0);
}

int rf_fl2k_close(fl2k_t *s)
{
	int r;
	
	s->abort = 1;
	
	fl2k_stop_tx(s->d);
	fl2k_close(s->d);
	
	for(r = 0; r < _FL2K_BUFFERS; r++)
	{
		pthread_mutex_destroy(&s->mutex[r]);
	}
	
	memset(s, 0, sizeof(fl2k_t));
	
	return(0);
}

int rf_fl2k_open(fl2k_t *s, const char *device, uint32_t sample_rate)
{
	int r;
	
	memset(s, 0, sizeof(fl2k_t));
	
	s->abort = 0;
	
	r = device ? atoi(device) : 0;
	
	fl2k_open(&s->d, r);
	if(s->d == NULL)
	{
		fprintf(stderr, "fl2k_open() failed to open device #%d.\n", r);
		rf_fl2k_close(s);
		return(-1);
	}
	
	for(r = 0; r < _FL2K_BUFFERS; r++)
	{
		pthread_mutex_init(&s->mutex[r], NULL);
	}
	
	/* Lock the initial buffer for the provider */
	s->in = 0;
	pthread_mutex_lock(&s->mutex[s->in]);
	
	/* Lock the last empty buffer for the consumer */
	s->out = _FL2K_BUFFERS - 1;
	pthread_mutex_lock(&s->mutex[s->out]);
	
	s->len = 0;
	
	r = fl2k_start_tx(s->d, _callback, s, 0);
	if(r < 0)
	{
		fprintf(stderr, "fl2k_start_tx() failed: %d\n", r);
		rf_fl2k_close(s);
		return(-1);
	}
	
	r = fl2k_set_sample_rate(s->d, sample_rate);
	if(r < 0)
	{
		fprintf(stderr, "fl2k_set_sample_rate() failed: %d\n", r);
		rf_fl2k_close(s);
		return(-1);
	}
	
	/* Read back the actual frequency */
	s->sample_rate = fl2k_get_sample_rate(s->d);
	
	return(0);
};

