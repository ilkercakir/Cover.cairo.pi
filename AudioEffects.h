#ifndef AudioEffectsH
#define AudioEffectsH

#define _GNU_SOURCE

#include <math.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include "BiQuad.h"

typedef enum
{
	DLY_ECHO,
	DLY_DELAY,
	DLY_REVERB,
	DLY_LATE,
}dly_type; /* Delay types */

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float feedback; // feedback level 0.0 .. 1.0
	float millisec; // delay in milliseconds
	int N; // parallel delays
	int enabled;
	pthread_mutex_t delaymutex; // = PTHREAD_MUTEX_INITIALIZER;

	int physicalwidth; // bits per sample
	char *fbuffer;
	int fbuffersize;
	int fbuffersamples;
	int delaysamples;
	int delaybytes;
	int insamples;

	int front, rear, readfront;
	signed short *fshort;

	dly_type delaytype;
}sounddelay;

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float vfofreq; // modulation frequency
	float vfodepth; // modulation depth in percent 0..1.0
	int invertphase; // invert phase of modulation
	int enabled;

	int N; // extra frames
	char *vfobuf;
	int physicalwidth;
	int vfobufframes;
	int vfobufsamples;
	int framebytes;
	int inbuffersamples;
	int inbufferframes;
	int front, rear;
	int framesinT, period, periods, peak, L, M;
	char *lastframe;
}soundvfo;

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float modfreq; // modulation frequency
	float moddepth; // modulation depth in percent 0..1.0
	int enabled;
	pthread_mutex_t modmutex; // = PTHREAD_MUTEX_INITIALIZER;

	soundvfo v;
}soundmod;

#define REVERBDLINES 24

typedef struct
{
	int reverbdelaylines;
	sounddelay snddlyrev[REVERBDLINES];
	char* bbuf;

	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float feedback; // feedback level 0.0 .. 1.0
	float presence; // reverbation presence
	int enabled;
	pthread_mutex_t reverbmutex; // = PTHREAD_MUTEX_INITIALIZER;

	float eqoctave;
	audioequalizer reveq;
}soundreverb;

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float depth; // depth 0.0 .. 1.0
	float tremolorate; // tremolo rate  0.1 .. 10
	unsigned int framecount; // time=framecount/rate % rate; instantaneous volume=1-depth/2*(1-sin(2*pi*tremolorate*time))

	int enabled;
	pthread_mutex_t tremolomutex; // = PTHREAD_MUTEX_INITIALIZER;
}soundtremolo;

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float threshold;
	float gain;
	int enabled;
	pthread_mutex_t fdmutex; // = PTHREAD_MUTEX_INITIALIZER;

	int initialized;
	int physicalwidth;
	int insamples;
}soundfoldingdistortion;

#define MAXCHORUS 3

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	int enabled;
	pthread_mutex_t chomutex; // = PTHREAD_MUTEX_INITIALIZER;

	int maxcho;
	soundvfo v[MAXCHORUS];
}soundcho;

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float threshold;
	float gain;

	pthread_mutex_t monomutex; // = PTHREAD_MUTEX_INITIALIZER;
	int initialized;
	int enabled;
	int physicalwidth;
	int insamples;
	float prescale;
}soundmono;

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float threshold;
	float gain;

	pthread_mutex_t haasmutex; // = PTHREAD_MUTEX_INITIALIZER;
	int initialized;
	int enabled;
	int physicalwidth;
	int insamples;

	sounddelay haasdly;
}soundhaas;

void sounddelay_reinit(int N, dly_type delaytype, float millisec, float feedback, sounddelay *s);
void sounddelay_init(int N, dly_type delaytype, float millisec, float feedback, snd_pcm_format_t format, unsigned int rate, unsigned int channels, sounddelay *s);
void sounddelay_add(char* inbuffer, int inbuffersize, sounddelay *s);
void sounddelay_close(sounddelay *s);
void soundvfo_init(float vfofreq, float vfodepth, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundvfo *v);
void soundvfo_add(char* inbuffer, int inbuffersize, soundvfo *v);
void soundvfo_close(soundvfo *v);
void soundmod_reinit(float modfreq, float moddepth, soundmod *m);
void soundmod_init(float modfreq, float moddepth, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundmod *m);
void soundmod_add(char* inbuffer, int inbuffersize, soundmod *m);
void soundmod_close(soundmod *m);
void set_reverbeq(eqdefaults *d);
void soundreverb_reinit(int delaylines, float feedback, float presence, eqdefaults *d, soundreverb *r);
void soundreverb_init(int delaylines, float feedback, float presence, eqdefaults *d, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundreverb *r);
void soundreverb_add(char* inbuffer, int inbuffersize, soundreverb *r);
void soundreverb_close(soundreverb *r);
void soundtremolo_init(float depth, float tremolorate, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundtremolo *t);
void soundtremolo_add(char* inbuffer, int inbuffersize, soundtremolo *t);
void soundtremolo_close(soundtremolo *t);
void soundfoldingdistort_init(float threshold, float gain, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundfoldingdistortion *d);
void soundfoldingdistort_add(char* inbuffer, int inbuffersize, soundfoldingdistortion *d);
void soundfoldingdistort_close(soundfoldingdistortion *d);
void soundcho_init(int maxcho, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundcho *c);
void soundcho_add(char* inbuffer, int inbuffersize, soundcho *c);
void soundcho_close(soundcho *c);
void soundmono_init(snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundmono *m);
void soundmono_add(char* inbuffer, int inbuffersize, soundmono *m);
void soundmono_close(soundmono *m);
void soundhaas_init(float millisec, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundhaas *h);
void soundhaas_add(char* inbuffer, int inbuffersize, soundhaas *h);
void soundhaas_close(soundhaas *h);
#endif
