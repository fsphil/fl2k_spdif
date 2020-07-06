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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "fl2k.h"
#include "spdif.h"

static void print_usage(void)
{
	printf(
		"\n"
		"Usage: fl2k_spdif [options] <input> [<input> ...]\n"
		"\n"
		"  -s, --samplerate <value>       Set the audio sample rate in Hz.\n"
		"                                 Default: 44100 kHz\n"
		"  -i, --interpolate <value>      Interpolation factor for output sample rate.\n"
		"                                 Default: 2\n"
		"  -r, --repeat                   Repeat input files forever.\n"
		"\n"
	);
}

int main(int argc, char *argv[])
{
	FILE *fin;
	fl2k_t fl2k;
	int16_t pcm[192 * 2];
	uint8_t block[192 * 2 * 8];
	size_t out_len;
	int8_t *out;
	uint32_t audio_rate = 44100;
	int interp = 2;
	int repeat = 0;
	int i, c;
	int option_index;
	static struct option long_options[] = {
		{ "samplerate",  required_argument, 0, 's' },
		{ "interpolate", required_argument, 0, 'i' },
		{ "repeat",      no_argument,       0, 'r' },
		{ }
	};
	
	opterr = 0;
	while((c = getopt_long(argc, argv, "s:i:r", long_options, &option_index)) != -1)
	{
		switch(c)
		{
		case 's': /* -s, --samplerate <value> */
			audio_rate = atoi(optarg);
			break;
		
		case 'i': /* -i, --interpolate <value> */
			interp = atoi(optarg);
			break;
		
		case 'r': /* -r, --repeat */
			repeat = 1;
			break;
		
		case '?':
			print_usage();
			return(0);
		}
	}
	
	if(optind >= argc)
	{
		fprintf(stderr, "No input specified.\n");
		return(-1);
	}
	
	if(interp < 1)
	{
		fprintf(stderr, "Interpolate value can't be lower than 1.\n");
		return(-1);
	}
	
	fprintf(stderr, "Sample rate %d Hz, bitrate %d bit/s, output rate %d sample/s\n", audio_rate, audio_rate * 2 * 64, audio_rate * 2 * 64 * interp);
	
	rf_fl2k_open(&fl2k, NULL, audio_rate * 2 * 64 * interp);
	
	fprintf(stderr, "Effective sample rate: %.2f Hz\n", (double) fl2k.sample_rate / (2 * 64 * interp));
	
	out_len = 192 * 2 * 8 * 8 * interp;
	out = malloc(out_len);
	if(!out)
	{
		perror("malloc");
		return(-1);
	}
	
	do
	{
		for(c = optind; c < argc; c++)
		{
			fprintf(stderr, "Streaming '%s'...\n", argv[c]);
			
			if(strcmp(argv[c], "-") == 0)
			{
				fin = stdin;
			}
			else
			{
				fin = fopen(argv[c], "rb");
				if(!fin)
				{
					perror("fopen");
					continue;
				}
			}
			
			while(fread(pcm, 192 * 2 * sizeof(int16_t), 1, fin) == 1)
			{
				spdif_block(block, pcm);
				
				for(i = 0; i < out_len; i++)
				{
					int b = i / interp;
					out[i] = (block[b >> 3] >> (7 - (b & 7))) & 1 ? 150 : -150;
				}
				
				rf_fl2k_write(&fl2k, out, out_len);
			}
			
			if(fin != stdin)
			{
				fclose(fin);
			}
		}
	}
	while(repeat);
	
	rf_fl2k_close(&fl2k);
	
	free(out);
	
	fprintf(stderr, "Fin\n");
	
	return(0);
}

