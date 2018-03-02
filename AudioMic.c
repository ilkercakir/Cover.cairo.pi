/*
 * AudioMic.c
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
#include "AudioMic.h"

int init_audio_hw_mic(microphone *m)
{
	int err;
	snd_pcm_uframes_t size;

	snd_pcm_hw_params_alloca(&(m->hw_params));
	//snd_pcm_sw_params_alloca(&sw_params);

	if ((err = snd_pcm_open(&(m->capture_handle), m->device, SND_PCM_STREAM_CAPTURE, 0)) < 0)
	{
		printf("cannot open audio device %s (%s)\n", m->device, snd_strerror(err));
		return(err);
	}

	if ((err = snd_pcm_hw_params_any(m->capture_handle, m->hw_params)) < 0)
	{
		printf("cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
		return(err);
	}

	if ((err = snd_pcm_hw_params_set_access(m->capture_handle, m->hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		printf("cannot set access type (%s)\n", snd_strerror(err));
		return(err);
	}

	if ((err = snd_pcm_hw_params_set_format(m->capture_handle, m->hw_params, m->format)) < 0)
	{
		printf("cannot set sample format (%s)\n", snd_strerror(err));
		return(err);
	}

	if ((err = snd_pcm_hw_params_set_rate_near(m->capture_handle, m->hw_params, &(m->rate), 0)) < 0)
	{
		printf("cannot set sample rate (%s)\n", snd_strerror(err));
		return(err);
	}

	for (m->micchannels=1;m->micchannels<=2;m->micchannels++)
	{
		if (snd_pcm_hw_params_test_channels(m->capture_handle, m->hw_params, m->micchannels) == 0)
		{
			break;
printf("supported input channels: %d\n", m->micchannels);
		}
	}

	if ((err = snd_pcm_hw_params_set_channels(m->capture_handle, m->hw_params, m->micchannels)) < 0)
	{
		printf("cannot set channel count (%s)\n", snd_strerror(err));
		return(err);
	}

	m->micbufferframes = m->bufferframes;
	m->micbuffersamples = m->micbufferframes * m->micchannels;
	m->micbuffersize = m->micbuffersamples * ( snd_pcm_format_width(m->format) / 8 );
	m->micbuffer = malloc(m->micbuffersize);
	memset(m->micbuffer, 0, m->micbuffersize);
	//m->capturebuffersize = m->micbuffersize * 10; // 10 buffers
	m->capturebuffersize = m->micbuffersize * 6; // 6 buffers
	m->prescale = 1.0 / sqrt(m->micchannels);
//printf("buffersize %d\n", m->capturebuffersize);
	if ((err = snd_pcm_hw_params_set_buffer_size(m->capture_handle, m->hw_params, m->capturebuffersize)) < 0)
	{
		printf("Unable to set buffer size %d for capture: %s\n", m->capturebuffersize, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_get_buffer_size(m->hw_params, &size)) < 0)
	{
		printf("Unable to get buffer size for capture: %s\n", snd_strerror(err));
		return(err);
	}
	m->capturebuffersize = size;

	if ((err = snd_pcm_hw_params(m->capture_handle, m->hw_params)) < 0)
	{
		printf("cannot set parameters (%s)\n", snd_strerror(err));
		return(err);
	}

	if ((err = snd_pcm_prepare(m->capture_handle)) < 0)
	{
		printf("cannot prepare audio interface for use (%s)\n", snd_strerror(err));
		return(err);
	}

//printf("initialized format %d rate %d channels %d\n", m->format, m->rate, m->channels);
	return(err);
}

void close_audio_hw_mic(microphone *m)
{
	snd_pcm_close(m->capture_handle);
	free(m->micbuffer);
}

void haas_reinit(michaas *h, float millisec)
{
	free(h->delayed);
	h->millisec = millisec;
	int delayframes = h->millisec / 1000.0 * h->rate;
	h->delaybytes = delayframes * ( snd_pcm_format_width(h->format) / 8 );
	h->delayed = malloc((h->inbufferframes+delayframes) * ( snd_pcm_format_width(h->format) / 8 ));
	memset(h->delayed+h->inbuffersize, 0, h->delaybytes);
	h->delayedbuffer = (signed short *)h->delayed;
	//printf("%f, %d, %d\n", millisec, delayframes, delaybytes);
}

void haas_init(michaas *h, int enabled, float millisec, snd_pcm_format_t format, unsigned int rate, int inbuffersize, char *inbuffer, int modenabled, float modrate, float moddepth)
{
	int ret;

	h->haasenabled = enabled;
	h->millisec = millisec;
	h->format = format;
	h->rate = rate;

	h->haasmutex = malloc(sizeof(pthread_mutex_t));
	if((ret=pthread_mutex_init(h->haasmutex, NULL))!=0 )
		printf("mutex init failed, %d\n", ret);

	h->inbuffersize = inbuffersize;
	h->inbufferframes = h->inbuffersize / snd_pcm_format_width(h->format) * 8;
	int delayframes = h->millisec / 1000.0 * h->rate;
	h->delaybytes = delayframes * ( snd_pcm_format_width(h->format) / 8 );
	h->delayed = malloc((h->inbufferframes+delayframes) * ( snd_pcm_format_width(h->format) / 8 ));
	memset(h->delayed+h->inbuffersize, 0, h->delaybytes);
	h->delayedbuffer = (signed short *)h->delayed;
	//printf("%f, %d, %d\n", millisec, delayframes, delaybytes);

	h->sbuffersize = h->inbufferframes * stereo * ( snd_pcm_format_width(h->format) / 8 );
	h->sbuffer = malloc(h->sbuffersize);
	h->stereobuffer = (signed short *)h->sbuffer;

	h->mbuffer = inbuffer;
	h->monobuffer = (signed short *)h->mbuffer;

	h->sndmod.enabled = modenabled;
	soundmod_init(modrate, moddepth, format, rate, mono, &(h->sndmod));
}
/*
void haas_init(michaas *h, int enabled, float millisec, snd_pcm_format_t format, unsigned int rate, int inbuffersize, char *inbuffer, int modenabled, float modrate, float moddepth)
{
	int ret;

	h->haasenabled = enabled;
	h->millisec = millisec;
	h->format = format;
	h->rate = rate;

	h->haasmutex = malloc(sizeof(pthread_mutex_t));
	if((ret=pthread_mutex_init(h->haasmutex, NULL))!=0 )
		printf("mutex init failed, %d\n", ret);

	h->inbuffersize = inbuffersize;
	h->inbufferframes = h->inbuffersize / snd_pcm_format_width(format) * 8;
	int delayframes = millisec / 1000.0 * rate;
	h->delaybytes = delayframes * ( snd_pcm_format_width(format) / 8 );
	h->delayed = malloc((h->inbufferframes+delayframes) * ( snd_pcm_format_width(format) / 8 ));
	memset(h->delayed+inbuffersize, 0, h->delaybytes);
	h->delayedbuffer = (signed short *)h->delayed;
	//printf("%f, %d, %d\n", millisec, delayframes, delaybytes);

	h->sbuffersize = h->inbufferframes * stereo * ( snd_pcm_format_width(h->format) / 8 );
	h->sbuffer = malloc(h->sbuffersize);
	h->stereobuffer = (signed short *)h->sbuffer;

	h->mbuffer = inbuffer;
	h->monobuffer = (signed short *)h->mbuffer;

	h->sndmod.enabled = modenabled;
	soundmod_init(modrate, moddepth, format, rate, mono, &(h->sndmod));
}
*/

void haas_add(michaas *h)
{
	int i, j;

	pthread_mutex_lock(h->haasmutex);
	if (h->haasenabled)
	{
		//memcpy(h->delayed, h->delayed+h->inbuffersize, h->delaybytes); // Delay R
		for(i=0;i<h->delaybytes;i++)
			h->delayed[i]=h->delayed[h->inbuffersize+i];
		memcpy(h->delayed+h->delaybytes, h->mbuffer, h->inbuffersize);

		soundmod_add(h->delayed, h->inbuffersize, &(h->sndmod)); // Modulate R

		for(i=j=0;i<h->inbufferframes;i++)
		{
			// Haas
			h->stereobuffer[j++] = h->monobuffer[i] * 0.7; // L
			h->stereobuffer[j++] = h->delayedbuffer[i];
		}
	}
	else
	{
		for(i=j=0;i<h->inbufferframes;i++)
		{
			// Dry, mono -> mono on both sides of stereo
			h->stereobuffer[j++] = h->monobuffer[i]; // L
			h->stereobuffer[j++] = h->monobuffer[i]; // R
		}
	}
	pthread_mutex_unlock(h->haasmutex);
}

void haas_close(michaas *h)
{
	//soundmod_close(&sndmod);
	free(h->delayed);
	h->delayed = NULL;
	h->delaybytes = 0;
	h->delayedbuffer = NULL;
	free(h->sbuffer);
	h->sbuffer = NULL;
	h->stereobuffer = NULL;
	h->monobuffer = NULL;
	pthread_mutex_destroy(h->haasmutex);
	free(h->haasmutex);
	soundmod_close(&(h->sndmod));
}

static int xrun_recovery(snd_pcm_t *handle, int err) // Underrun and suspend recovery
{
	if (err == -EPIPE)	// under-run
	{
		err = snd_pcm_prepare(handle);
		if (err < 0)
			printf("Can't recover from underrun, prepare failed: %s\n", snd_strerror(err));
		return(0);
	}
	else if (err == -ESTRPIPE)
	{
		while ((err = snd_pcm_resume(handle)) == -EAGAIN)
			sleep(1);	/* wait until the suspend flag is released */
		if (err < 0)
		{
			err = snd_pcm_prepare(handle);
			if (err < 0)
				printf("Can't recover from suspend, prepare failed: %s\n", snd_strerror(err));
		}
		return(0);
	}
	return(err);
}

void init_mic(microphone *m, char* device, snd_pcm_format_t format, unsigned int rate, int outframes, int haasenabled, float haasms, int modenabled, float modrate, float moddepth)
{
	int ret;

	if (device)
	{
		m->device = malloc(32);
		strcpy(m->device, device);
	}
	else
		m->device = NULL;

	m->micmutex = malloc(sizeof(pthread_mutex_t));
	if((ret=pthread_mutex_init(m->micmutex, NULL))!=0 )
		printf("mic mutex init failed, %d\n", ret);

	m->format = format; // SND_PCM_FORMAT_S16_LE;
	m->rate = rate;
	m->channels = mono;
	m->status = MC_RUNNING;
	m->bufferframes = outframes;
	m->buffersamples = m->bufferframes * mono;
	m->buffersize = m->buffersamples * ( snd_pcm_format_width(m->format) / 8 );
	m->buffer = malloc(m->buffersize);
	memset(m->buffer, 0, m->buffersize);

	haas_init(&(m->mh), haasenabled, haasms, format, rate, m->buffersize, m->buffer, modenabled, modrate, moddepth);
}

int read_mic(microphone *m)
{
	pthread_mutex_lock(m->micmutex);
	int i, j, err, result = 1;

	err = snd_pcm_readi(m->capture_handle, m->micbuffer, m->micbufferframes);
	if (err == -EAGAIN) printf("EAGAIN mic\n");
	if (err < 0)
	{
		if (xrun_recovery(m->capture_handle, err) < 0)
		{
			printf("snd_pcm_readi error: %s\n", snd_strerror(err));
		}
	}
	err=snd_pcm_avail_delay(m->capture_handle, &(m->availp), &(m->delayp));
	//printf("\t%d\t%d\n", (int)m->availp, (int)m->delayp);

	signed short *src = (signed short *)m->micbuffer;
	signed short *dest = (signed short *)m->buffer;
	memset(m->buffer, 0, m->buffersize);
	if (!m->nullsamples)
	{
		for(i=0;i<m->micbufferframes;i++)
		{
			dest[i] = 0;
			for(j=0;j<m->micchannels;j++)
			{
				dest[i] += src[i*m->micchannels+j] * m->prescale;
			}
		}
	}

	pthread_mutex_unlock(m->micmutex);
	return(result);
}

snd_pcm_sframes_t discard_mic(microphone *m)
{
	int ret, err;
	char *buf;
	snd_pcm_sframes_t frames;

	buf = malloc(m->buffersize);

	pthread_mutex_lock(m->micmutex);
	for(ret=0;(frames = snd_pcm_avail(m->capture_handle))>m->bufferframes;ret+=frames) // skip
	{
		printf("available capture frames %ld\n", frames);

		err = snd_pcm_readi(m->capture_handle, m->buffer, m->bufferframes);
		if (err == -EAGAIN) printf("EAGAIN mic\n");
		if (err < 0)
		{
			if (xrun_recovery(m->capture_handle, err) < 0)
			{
				printf("snd_pcm_readi error: %s\n", snd_strerror(err));
			}
		}
	}
	pthread_mutex_unlock(m->micmutex);
	
	free(buf);
	return(ret);
}

void signalstop_mic(microphone *m)
{
	m->status = MC_STOPPED;
}

void close_mic(microphone *m)
{
	pthread_mutex_destroy(m->micmutex);
	free(m->micmutex);
	free(m->buffer);
	if (m->device)
		free(m->device);
	haas_close(&(m->mh));
}
