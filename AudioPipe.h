#ifndef AudioMicH
#define AudioMicH

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>

typedef struct
{
	int front, rear;
	int buffersize, cqbuffersize;
	char *buffer, *cqbuffer;
}audiopipe;

void audioCQ_init(audiopipe *p, int buffersize);
void audioCQ_add(audiopipe *p, char *inbuffer, int inbuffersize);
int audioCQ_remove(audiopipe *p);
void audioCQ_close(audiopipe *p);
#endif
