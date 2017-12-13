/*
 * AudioPipe.c
 * 
 * Copyright 2017  <pi@raspberrypi>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include "AudioPipe.h"

void audioCQ_init(audiopipe *p, int buffersize)
{
	p->front = p->rear = 0;
	p->buffersize = buffersize; // out buffer size is quantized
	p->buffer = malloc(p->buffersize);
	p->cqbuffersize = 10 * p->buffersize;
	p->cqbuffer = malloc(p->cqbuffersize);
}

void audioCQ_add(audiopipe *p, char *inbuffer, int inbuffersize)
{
	int offset, bytesleft, bytestocopy;

	for(offset=0,bytesleft=inbuffersize;bytesleft;bytesleft-=bytestocopy)
	{
		bytestocopy = (p->rear+bytesleft>p->cqbuffersize?p->cqbuffersize-p->rear:bytesleft);
		memcpy(p->cqbuffer+p->rear, inbuffer+offset, bytestocopy);
		offset += bytestocopy;
		p->rear += bytestocopy; p->rear %= p->cqbuffersize;
	}
}

int audioCQ_remove(audiopipe *p)
{
	int offset, bytesleft, bytestocopy;

	int length = (p->rear - p->front + p->cqbuffersize) % p->cqbuffersize;
	if (length<p->buffersize)
	{
		return(0);
	}
	else
	{
		for(offset=0,bytesleft=p->buffersize;bytesleft;bytesleft-=bytestocopy)
		{
			bytestocopy = (p->front+bytesleft>p->cqbuffersize?p->cqbuffersize-p->front:bytesleft);
			memcpy(p->buffer+offset, p->cqbuffer+p->front, bytestocopy);
			offset += bytestocopy;
			p->front += bytestocopy; p->front %= p->cqbuffersize;
		}
		return(p->buffersize);
	}
}

void audioCQ_close(audiopipe *p)
{
	free(p->cqbuffer);
	free(p->buffer);
}
