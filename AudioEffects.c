/*
 * AudioEffects.c
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

#include "AudioEffects.h"
#include "AudioEffectsDefaults.h"

// Delay Processor
void sounddelay_reinit(int N, dly_type delaytype, float millisec, float feedback, sounddelay *s)
{
	s->N = N;
	s->delaytype = delaytype;
	s->feedback = feedback;
	s->millisec = millisec;
	free(s->fbuffer);
	s->fbuffer = NULL;
}

void sounddelay_init(int N, dly_type delaytype, float millisec, float feedback, snd_pcm_format_t format, unsigned int rate, unsigned int channels, sounddelay *s)
{
	int ret;

	s->N = N;
	s->delaytype = delaytype;
	s->format = format;
	s->rate = rate;
	s->channels = channels;
	s->feedback = feedback;
	s->millisec = millisec;
	s->fbuffer = NULL;
	if((ret=pthread_mutex_init(&(s->delaymutex), NULL))!=0 )
		printf("delay mutex init failed, %d\n", ret);
	//printf("Delay initialized, type %d, %5.2f ms, %5.2f feedback, %d rate, %d channels\n", s->delaytype, s->millisec, s->feedback, s->rate, s->channels);
}

void sounddelay_add(char* inbuffer, int inbuffersize, sounddelay *s)
{
	int i;
	float prescale;
	signed short *inshort;

	pthread_mutex_lock(&(s->delaymutex));
	if (s->enabled)
	{
		if (!s->fbuffer)
		{
			s->physicalwidth = snd_pcm_format_width(s->format);
			s->insamples = inbuffersize * 8 / s->physicalwidth;
			s->delaysamples = ceil((s->millisec / 1000.0) * (float)s->rate) * s->channels;
			s->delaybytes = s->delaysamples * s->physicalwidth / 8;

			s->fbuffersize = s->delaybytes + inbuffersize;
			s->fbuffer = malloc(s->fbuffersize);
			memset(s->fbuffer, 0, s->fbuffersize);
			s->fbuffersamples = s->insamples + s->delaysamples;
			s->fshort = (signed short *)s->fbuffer;

			s->rear = s->delaysamples;
			s->front = 0;
			s->readfront = 0;
		}
		inshort = (signed short *)inbuffer;

		switch (s->delaytype)
		{
			case DLY_ECHO: // Repeating echo added to original
				prescale = sqrt(1 - s->feedback*s->feedback); // prescale=sqrt(sum(r^2n)), n=0..infinite
				for(i=0; i<s->insamples; i++)
				{
					inshort[i]*=prescale;
					s->fshort[s->rear++] = inshort[i] += s->fshort[s->front++]*s->feedback;
					s->front%=s->fbuffersamples;
					s->rear%=s->fbuffersamples;
				}
				break;
			case DLY_DELAY: // Single delayed signal added to original
				prescale = 1 / sqrt(1 + s->feedback*s->feedback); // prescale = 1/sqrt(1 + r^2)
				for(i=0;i<s->insamples; i++)
				{
					inshort[i]*=prescale;
					s->fshort[s->rear++] = inshort[i];
					inshort[i] += s->fshort[s->front++]*s->feedback;
					s->front%=s->fbuffersamples;
					s->rear%=s->fbuffersamples;
				}
				break;
			case DLY_REVERB: // Only repeating echo, no original
				//prescale = sqrt(1 - s->feedback*s->feedback); // prescale=sqrt(sum(r^2n)), n=0..infinite
				prescale = sqrt((1.0-s->feedback*s->feedback)/((s->N-1)*s->feedback*s->feedback+1.0)); // prescale=sqrt(sum(r^2n)-1), for all channels, n=0..infinite
				for(i=0; i<s->insamples; i++)
				{
					//s->fshort[s->rear++] = inshort[i]*prescale + s->fshort[s->front++]*s->feedback;
					s->fshort[s->rear++] = (inshort[i]*prescale + s->fshort[s->front++])*s->feedback;
					s->front%=s->fbuffersamples;
					s->rear%=s->fbuffersamples;
				}
				break;
			case DLY_LATE: // Single delayed signal, no original
				for(i=0;i<s->insamples; i++)
				{
					s->fshort[s->rear++] = inshort[i]*s->feedback;
					s->front++;
					s->front%=s->fbuffersamples;
					s->rear%=s->fbuffersamples;
				}
				break;
			default:
				break;
		}
	}
	pthread_mutex_unlock(&(s->delaymutex));
}

void sounddelay_close(sounddelay *s)
{
	pthread_mutex_destroy(&(s->delaymutex));
	if (s->fbuffer)
	{
		free(s->fbuffer);
		s->fbuffer = NULL;
	}
}

// VFO

void soundvfo_init(float vfofreq, float vfodepth, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundvfo *v)
{
	v->format = format;
	v->rate = rate;
	v->channels = channels;
	v->vfofreq = vfofreq;
	v->vfodepth = vfodepth;

	v->vfobuf = NULL;
	v->rear = v->front = 0;	
}

void soundvfo_add(char* inbuffer, int inbuffersize, soundvfo *v)
{
	int i, j, delta, index;
	float frac, ix;

	if (v->enabled)
	{
		if (!v->vfobuf)
		{
			v->physicalwidth = snd_pcm_format_width(v->format);
			v->framebytes = v->physicalwidth / 8 * v->channels;
			v->inbufferframes = inbuffersize / v->framebytes;
			v->inbuffersamples = v->inbufferframes * v->channels;
			v->framesinT = (int)((float)v->rate / v->vfofreq);
			v->periods = v->framesinT / v->inbufferframes;
			v->periods = (v->periods>>2)<<2; // Multiple of 4
			v->period = 0;

			v->lastframe = malloc(v->channels*v->physicalwidth);
			signed short *lf = (signed short *)v->lastframe;
			for(j=0;j<v->channels;j++)
				lf[j] = 0;
				
			v->peak = v->inbufferframes * v->vfodepth;
			v->vfobufframes = v->inbufferframes;
			for(i=0;i<v->periods>>1;i++) // simulate half modulation period
			{
				delta = (int)(((float)v->peak)*sin((float)i*2.0*M_PI/(float)v->periods));
				v->vfobufframes += delta;
			}
			//printf("peak %d periods %d vfobufframes %d\n", v->peak, v->periods, v->vfobufframes);
			v->vfobufsamples = v->vfobufframes * v->channels;
			v->vfobuf = malloc(v->vfobufsamples*v->physicalwidth);

			v->L = v->inbufferframes;
			//printf("channels %d\n", v->channels);
		}

		signed short *inshort, *vfshort;
		inshort = (signed short *)inbuffer;
		vfshort = (signed short *)v->vfobuf;

		if (v->period<v->periods>>1)
			delta = (int)(((float)v->peak)*sin((float)v->period*2.0*M_PI/(float)v->periods));
		else
			delta = -(int)(((float)v->peak)*sin((float)(v->period-(v->periods>>1))*2.0*M_PI/(float)v->periods));

		v->M = v->L + delta;
//printf("M %d L %d ", v->M, v->L);
		if (v->M>v->L) // interpolation
		{
			//printf("interpolating period %d\n", v->period);
			frac = (float)v->L/(float)v->M;
			for(j=0;j<v->channels;j++) // frame 1
			{
				vfshort[v->rear++] = ((float)inshort[j])*frac + ((float)v->lastframe[j])*(1.0-frac);
			}
			v->rear %= v->vfobufsamples;
			for(i=2;i<v->M;i++) // frame 2 .. M-1
			{
				ix = (float)(i*v->L)/(float)v->M;
				index = (int)ix; frac = ix - (float)index;
				for(j=0;j<v->channels;j++)
				{
					vfshort[v->rear++] = ((float)inshort[index*v->channels+j])*frac + ((float)inshort[(index-1)*v->channels+j])*(1.0-frac);
				}
				v->rear %= v->vfobufsamples;
			}
			for(j=0;j<v->channels;j++) // frame M
			{
				v->lastframe[j] = vfshort[v->rear++] = (float)inshort[(v->L-1)*v->channels+j];
			}
			v->rear %= v->vfobufsamples;
		}
		else if (v->M<v->L) // decimation
		{
			//printf("decimating period %d\n", v->period);
			for(i=1;i<v->M;i++) // frame 1 .. M-1
			{
				ix = (float)(i*v->L)/(float)v->M;
				index = (int)ix; frac = ix - (float)index;
				for(j=0;j<v->channels;j++)
				{
					vfshort[v->rear++] = ((float)inshort[(index-1)*v->channels+j])*(1.0-frac) + ((float)inshort[index*v->channels+j])*frac;
				}
				v->rear %= v->vfobufsamples;
			}
			for(j=0;j<v->channels;j++) // frame M
			{
				v->lastframe[j] = vfshort[v->rear++] = (float)inshort[(v->L-1)*v->channels+j];
			}
			v->rear %= v->vfobufsamples;
		}
		else // copy
		{
			//printf("copying period %d\n", v->period); 
			for(i=0;i<v->inbufferframes;i++)
			{
				for(j=0;j<v->channels;j++)
					vfshort[v->rear++] = inshort[i*v->channels+j];
				v->rear %= v->vfobufsamples;
			}
			for(j=0;j<v->channels;j++) // frame L
			{
				v->lastframe[j] = (float)inshort[(v->L-1)*v->channels+j];
			}
		}

		v->period++;
		v->period %= v->periods;
	}
}

void soundvfo_close(soundvfo *v)
{
	if (!v->vfobuf)
	{
		free(v->vfobuf);
		v->vfobuf = NULL;
		free(v->lastframe);
		v->lastframe = NULL;
	}
}

// Modulator

void soundmod_reinit(float modfreq, float moddepth, soundmod *m)
{
	soundvfo_close(&(m->v));
	m->modfreq = modfreq;
	m->moddepth = moddepth;
	soundvfo_init(modfreq, moddepth, m->format, m->rate, m->channels, &(m->v));
}

void soundmod_init(float modfreq, float moddepth, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundmod *m)
{
	int ret;

	m->format = format;
	m->rate = rate;
	m->channels = channels;
	m->modfreq = modfreq;
	m->moddepth = moddepth;
	m->v.enabled = 1;
	if((ret=pthread_mutex_init(&(m->modmutex), NULL))!=0 )
		printf("mod mutex init failed, %d\n", ret);

	soundvfo_init(modfreq, moddepth, format, rate, channels, &(m->v));
}

void soundmod_add(char* inbuffer, int inbuffersize, soundmod *m)
{
	int i, j;
	signed short *inshort, *vfshort;

	pthread_mutex_lock(&(m->modmutex));
	if (m->enabled)
	{
		soundvfo_add(inbuffer, inbuffersize, &(m->v));
		inshort = (signed short *)inbuffer;
		vfshort = (signed short *)m->v.vfobuf;
		for(i=0;i<m->v.inbufferframes;i++)
		{
			for(j=0;j<m->channels;j++)
				inshort[i*m->channels+j] = vfshort[m->v.front++];
			m->v.front %= m->v.vfobufsamples;
		}
	}
	pthread_mutex_unlock(&(m->modmutex));
}

void soundmod_close(soundmod *m)
{
	soundvfo_close(&(m->v));
	pthread_mutex_destroy(&(m->modmutex));
}

/*
void set_reverbeq(eqdefaults *d)
{
	float default_eqfreqs[2] = {500.0, 5000.0};
	char* default_eqlabels[2] = {"LSH", "LPF"};
	filtertype default_filtertypes[] = {LSH, LPF};
	float default_dbGains[2] = {-12.0, 12.0};

	int i;
	for(i=0;i<2;i++)
	{
		d->eqfreqs[i] = default_eqfreqs[i];
		strcpy(d->eqlabels[i], default_eqlabels[i]);
		d->filtertypes[i] = default_filtertypes[i];
		d->dbGains[i] = default_dbGains[i];
	}
}
*/

void set_reverbeq(eqdefaults *d)
{
	float default_eqfreqs[1] = {4000.0};
	char* default_eqlabels[1] = {"HSH"};
	filtertype default_filtertypes[] = {HSH};
	float default_dbGains[1] = {-12.0};

	int i;
	for(i=0;i<1;i++)
	{
		d->eqfreqs[i] = default_eqfreqs[i];
		strcpy(d->eqlabels[i], default_eqlabels[i]);
		d->filtertypes[i] = default_filtertypes[i];
		d->dbGains[i] = default_dbGains[i];
	}
}

/*
void RevBiQuad_process(uint8_t *buf, int bufsize, int bytesinsample, soundreverb *r)
{
	int a, b;
	signed short *intp;
	intp=(signed short *)buf;
	float preampfactor;

	preampfactor = 1.0;
	for (a=0,b=0;a<bufsize;a+=bytesinsample, b+=2) // process first 2 channels only
	{
		intp[b] *= preampfactor;
		intp[b+1] *= preampfactor;

		intp[b] = BiQuad(&(r->bl[0]), intp[b]);
		intp[b+1] = BiQuad(&(r->br[0]), intp[b+1]);

		intp[b] = BiQuad(&(r->bl[1]), intp[b]);
		intp[b+1] = BiQuad(&(r->br[1]), intp[b+1]);
	}
}
*/

void soundreverb_reinit(int delaylines, float feedback, float presence, eqdefaults *d, soundreverb *r)
{
	int i;

	AudioEqualizer_close(&(r->reveq));
	for(i=0; i<r->reverbdelaylines; i++)
	{
		if (r->snddlyrev[i].fbuffer)
		{
			free(r->snddlyrev[i].fbuffer);
			r->snddlyrev[i].fbuffer = NULL;
		}
	}
	free(r->bbuf);
	r->bbuf = NULL;

	r->reverbdelaylines = delaylines;
	r->feedback = feedback;
	r->presence = presence;
	for(i=0; i<r->reverbdelaylines; i++)
	{
		r->snddlyrev[i].enabled = 1;
		sounddelay_init(r->reverbdelaylines, DLY_REVERB, reverbprimes[i], r->feedback, r->format, r->rate, r->channels, &(r->snddlyrev[i]));
	}

	r->eqoctave = 1.0;

	//AudioEqualizer_init(&(r->reveq), 2, r->eqoctave, 1, 1, r->format, r->rate, r->channels, d);
	AudioEqualizer_init(&(r->reveq), 1, r->eqoctave, 1, 1, r->format, r->rate, r->channels, d);
}

void soundreverb_init(int delaylines, float feedback, float presence, eqdefaults *d, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundreverb *r)
{
	int i, ret;

	r->reverbdelaylines = delaylines;
	r->format = format;
	r->rate = rate;
	r->channels = channels;
	r->feedback = feedback;
	r->presence = presence;
	for(i=0; i<r->reverbdelaylines; i++)
	{
		r->snddlyrev[i].enabled = 1;
		sounddelay_init(r->reverbdelaylines, DLY_REVERB, reverbprimes[i], feedback, format, rate, channels, &(r->snddlyrev[i]));
	}

	r->bbuf = NULL;
	r->eqoctave = 1.0;

	//AudioEqualizer_init(&(r->reveq), 2, r->eqoctave, 1, 1, r->format, r->rate, r->channels, d);
	AudioEqualizer_init(&(r->reveq), 1, r->eqoctave, 1, 1, r->format, r->rate, r->channels, d);

	if((ret=pthread_mutex_init(&(r->reverbmutex), NULL))!=0 )
		printf("reverb mutex init failed, %d\n", ret);
}

void soundreverb_add(char* inbuffer, int inbuffersize, soundreverb *r)
{
	int i, j, *readfront, *fbsamples;
	signed short *dstbuf, *srcbuf;

	pthread_mutex_lock(&(r->reverbmutex));
	if (r->enabled)
	{
		for(i=0; i<r->reverbdelaylines; i++)
		{
			sounddelay_add(inbuffer, inbuffersize, &(r->snddlyrev[i]));
		}

		if (!r->bbuf)
		{
			r->bbuf = malloc(inbuffersize);
		}
		memset(r->bbuf, 0, inbuffersize);
		dstbuf = (signed short*)r->bbuf;
		for(i=0; i<r->reverbdelaylines; i++)
		{
			srcbuf = (signed short*)r->snddlyrev[i].fbuffer;
			readfront = &(r->snddlyrev[i].readfront);
			fbsamples = &(r->snddlyrev[i].fbuffersamples);
			for(j=0; j<r->snddlyrev[i].insamples; j++)
			{
				dstbuf[j] += srcbuf[(*readfront)++] * r->presence;
				(*readfront)%=(*fbsamples);
			}
		}

		//RevBiQuad_process((uint8_t*)r->bbuf, inbuffersize, snd_pcm_format_width(r->format)/8*r->channels, r);
		AudioEqualizer_BiQuadProcess(&(r->reveq), (uint8_t*)r->bbuf, inbuffersize);

		dstbuf = (signed short*)inbuffer;
		srcbuf = (signed short*)r->bbuf;
		for(j=0; j<r->snddlyrev[0].insamples; j++)
		{
			dstbuf[j] += srcbuf[j];
		}
	}
	pthread_mutex_unlock(&(r->reverbmutex));
}

void soundreverb_close(soundreverb *r)
{
	int i;

	pthread_mutex_destroy(&(r->reverbmutex));
	AudioEqualizer_close(&(r->reveq));
	for(i=0; i<r->reverbdelaylines; i++)
	{
		if (r->snddlyrev[i].fbuffer)
		{
			free(r->snddlyrev[i].fbuffer);
			r->snddlyrev[i].fbuffer = NULL;
		}
	}
	if (r->bbuf)
	{
		free(r->bbuf);
		r->bbuf = NULL;
	}
}

// Tremolo

void soundtremolo_init(float depth, float tremolorate, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundtremolo *t)
{
	int ret;

	t->format = format;
	t->rate = rate;
	t->channels = channels;
	t->depth = depth;
	t->tremolorate = tremolorate;
	t->framecount = 0;
	if((ret=pthread_mutex_init(&(t->tremolomutex), NULL))!=0 )
		printf("tremolo mutex init failed, %d\n", ret);
}

void soundtremolo_add(char* inbuffer, int inbuffersize, soundtremolo *t)
{
	pthread_mutex_lock(&(t->tremolomutex));
	if (t->enabled)
	{
		int physicalwidth = snd_pcm_format_width(t->format); // bits per sample
		int insamples = inbuffersize / physicalwidth * 8;
		int frames = insamples / t->channels;
		int framesinT = (int)((float)t->rate / (float)t->tremolorate);
		signed short *inshort = (signed short *)inbuffer;
		int i,j,k;
		for(i=0,j=0;i<=frames;i++,t->framecount++)
		{
			float time = ((float)t->framecount / (float)t->rate);
			float theta = 2.0*M_PI*t->tremolorate*time; // w*t = 2*pi*f*t
			for(k=0;k<t->channels;k++)
				inshort[j++] *= 1.0-((t->depth/2.0)*(1.0-sin(theta)));
		}
		t->framecount %= framesinT;
	}
	pthread_mutex_unlock(&(t->tremolomutex));
}

void soundtremolo_close(soundtremolo *t)
{
	pthread_mutex_destroy(&(t->tremolomutex));
}

// Folding Distortion

void soundfoldingdistort_init(float threshold, float gain, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundfoldingdistortion *d)
{
	int ret;

	d->format = format;
	d->rate = rate;
	d->channels = channels;
	d->threshold = threshold;
	d->gain = gain;

	d->initialized = 0;
	if((ret=pthread_mutex_init(&(d->fdmutex), NULL))!=0 )
		printf("fd mutex init failed, %d\n", ret);
}

void soundfoldingdistort_add(char* inbuffer, int inbuffersize, soundfoldingdistortion *d)
{
	pthread_mutex_lock(&(d->fdmutex));
	if (d->enabled)
	{
		if (!d->initialized)
		{
			d->physicalwidth = snd_pcm_format_width(d->format); // bits per sample
			d->insamples = inbuffersize / d->physicalwidth * 8;
			d->initialized = 1;
		}
		signed short *inshort = (signed short *)inbuffer;
		int i;
		for(i=0;i<d->insamples;i++)
			inshort[i]= (inshort[i]>d->threshold?2*d->threshold-inshort[i]:(inshort[i]<-d->threshold?-2*d->threshold-inshort[i]:inshort[i]))*d->gain;
			//inshort[i] = (inshort[i]>d->threshold?d->threshold:(inshort[i]<-d->threshold?-d->threshold:inshort[i]))*d->gain;
	}
	pthread_mutex_unlock(&(d->fdmutex));
}

void soundfoldingdistort_close(soundfoldingdistortion *d)
{
	pthread_mutex_destroy(&(d->fdmutex));
	d->initialized = 1;
}

// Chorus

void soundcho_init(int maxcho, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundcho *c)
{
	int i, ret;

	c->format = format;
	c->rate = rate;
	c->channels = channels;
	c->maxcho = maxcho;
	for(i=0;i<maxcho;i++)
	{
		c->v[i].enabled = 1;
		soundvfo_init(chofreq[i], chodepth[i], format, rate, channels, &(c->v[i]));
	}
	if((ret=pthread_mutex_init(&(c->chomutex), NULL))!=0 )
		printf("cho mutex init failed, %d\n", ret);
}

void soundcho_add(char* inbuffer, int inbuffersize, soundcho *c)
{
	int i, j, k;
	signed short *inshort, *vfshort;
	float prescale = 1.0 / sqrt(c->maxcho+1); // 1/sqrt(N+1);

	pthread_mutex_lock(&(c->chomutex));
	if (c->enabled)
	{
		for(j=0;j<c->maxcho;j++)
		{
			soundvfo_add(inbuffer, inbuffersize, &(c->v[j]));
		}
		inshort = (signed short *)inbuffer;
		for(i=0;i<c->v[0].inbuffersamples;)
			inshort[i++] *= prescale;
		for(j=0;j<c->maxcho;j++)
		{
			vfshort = (signed short *)c->v[j].vfobuf;
			for(i=0;i<c->v[j].inbuffersamples;)
			{
				for(k=0;k<c->channels;k++)
					inshort[i++] += vfshort[c->v[j].front++]*prescale;
				c->v[j].front %= c->v[j].vfobufsamples;
			}
		}
	}
	pthread_mutex_unlock(&(c->chomutex));
}

void soundcho_close(soundcho *c)
{
	int j;
	for(j=0;j<c->maxcho;j++)
	{
		soundvfo_close(&(c->v[j]));
	}
	pthread_mutex_destroy(&(c->chomutex));
}

// Mono

void soundmono_init(snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundmono *m)
{
	int ret;

	m->format = format;
	m->rate = rate;
	m->channels = channels;

	m->initialized = 0;
	m->prescale = 1.0/sqrt(m->channels);

	if((ret=pthread_mutex_init(&(m->monomutex), NULL))!=0 )
		printf("mono mutex init failed, %d\n", ret);
}

void soundmono_add(char* inbuffer, int inbuffersize, soundmono *m)
{
	pthread_mutex_lock(&(m->monomutex));
	if (m->enabled)
	{
		if (!m->initialized)
		{
			m->physicalwidth = snd_pcm_format_width(m->format); // bits per sample
			m->insamples = inbuffersize / m->physicalwidth * 8;
			m->initialized = 1;
			//printf("init %d %d %d %d\n", inbuffersize, m->format, m->physicalwidth, m->insamples);
		}
		signed short *inshort = (signed short *)inbuffer;
		int i,j;
		for(i=0;i<m->insamples;)
		{
			signed short value = 0;
			for(j=0;j<m->channels;j++) value+=inshort[i+j]*m->prescale;
			for(j=0;j<m->channels;j++) inshort[i++]=value;
		}
	}
	pthread_mutex_unlock(&(m->monomutex));
}

void soundmono_close(soundmono *m)
{
	pthread_mutex_destroy(&(m->monomutex));
}

// Haas

void soundhaas_init(float millisec, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundhaas *h)
{
	int ret;

	h->format = format;
	h->rate = rate;
	h->channels = channels;

	h->initialized = 0;
	h->haasdly.enabled = 1;
	sounddelay_init(1, DLY_LATE, millisec, 1.0, format, rate, channels, &(h->haasdly));

	if((ret=pthread_mutex_init(&(h->haasmutex), NULL))!=0 )
		printf("haas mutex init failed, %d\n", ret);
}

void soundhaas_add(char* inbuffer, int inbuffersize, soundhaas *h)
{
	pthread_mutex_lock(&(h->haasmutex));
	if (h->enabled)
	{
		if (!h->initialized)
		{
			h->physicalwidth = snd_pcm_format_width(h->format); // bits per sample
			h->insamples = inbuffersize / h->physicalwidth * 8;
			h->initialized = 1;
		}
		sounddelay_add(inbuffer, inbuffersize, &(h->haasdly));
		signed short *inshort = (signed short *)inbuffer;
		signed short *fbuffer = h->haasdly.fshort;
		int i;
		for(i=0;i<h->insamples;)
		{
			inshort[i++] *= 0.7; h->haasdly.readfront++; // rescale left channel
			inshort[i++] = fbuffer[h->haasdly.readfront++]; // Haas effect on right channel
			h->haasdly.readfront%=h->haasdly.fbuffersamples;
		}
	}
	pthread_mutex_unlock(&(h->haasmutex));
}

void soundhaas_close(soundhaas *h)
{
	pthread_mutex_destroy(&(h->haasmutex));
	sounddelay_close(&(h->haasdly));
}
