
/*
 * Cover.c
 * 
 * Copyright 2017  İlker Çakır   ilker.cakir13@gmail.com
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

/*
compile with gcc -Wall -c "%f" -DUSE_OPENGL -DUSE_EGL -DIS_RPI -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -g -ftree-vectorize -pipe -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -mneon-for-64bits $(pkg-config --cflags gtk+-3.0) -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -Wno-deprecated-declarations
link with gcc -Wall -o "%e" "%f" AudioDev.o AudioEffects.o AudioMic.o AudioMixer.o AudioPipe.o AudioSpk.o BiQuad.o VideoPlayerWidgets.o VideoPlayer.o VideoQueue.o YUV420RGBgl.o $(pkg-config --cflags gtk+-3.0) -Wl,--whole-archive -I/opt/vc/include -L/opt/vc/lib/ -lGLESv2 -lEGL -lbcm_host -lvchiq_arm -lpthread -lrt -ldl -lm -Wl,--no-whole-archive -rdynamic $(pkg-config --libs gtk+-3.0) $(pkg-config --libs libavcodec libavformat libavutil libswscale) -lasound $(pkg-config --libs gtk+-3.0) $(pkg-config --libs sqlite3)
 */

#define _GNU_SOURCE

#include "AudioDev.h"
#include "AudioEffects.h"
#include "AudioMic.h"
#include "AudioMixer.h"
#include "AudioSpk.h"
#include "BiQuad.h"
#include "YUV420RGBgl.h"
#include "VideoPlayer.h"
#include "VideoPlayerWidgets.h"

#include <pthread.h>
#include <math.h>
#include <gtk/gtk.h>

GtkWidget *window;
GtkWidget *fxbox;
GtkWidget *confbox;
GtkWidget *frameconf;
GtkWidget *inputdevlabel;
GtkWidget *comboinputdev;
GtkWidget *frameslabel;
GtkWidget *spinbutton1;
GtkWidget *outputdevlabel;
GtkWidget *combooutputdev;
GtkWidget *vpenable;

typedef struct
{
GtkWidget *framehaas1;
GtkWidget *haasbox1;
GtkWidget *haasenable;
GtkWidget *haaslabel1;
GtkWidget *spinbutton2;
GtkWidget *modenable;
GtkWidget *modlabel1;
GtkWidget *spinbutton14;
GtkWidget *modlabel2;
GtkWidget *spinbutton15;
}haaseffect;

typedef struct
{
GtkWidget *framedelay1;
GtkWidget *dlybox1;
GtkWidget *dlyenable;
GtkWidget *delaylabel1;
GtkWidget *spinbutton5;
GtkWidget *delaytypelabel;
GtkWidget *combodelaytype;
GtkWidget *feedbacklabel1;
GtkWidget *spinbutton6;
sounddelay snddly;
}delayeffect;

typedef struct
{
GtkWidget *framervrb1;
GtkWidget *rvrbboxv;
GtkWidget *rvrbbox1;
GtkWidget *rvrbenable;
GtkWidget *rvrblabel1;
GtkWidget *spinbutton7;
GtkWidget *rvrblabel2;
GtkWidget *spinbutton8;
GtkWidget *rvrbbox2;
GtkWidget *rvrbdelays;
GtkWidget *comborvrbdelays;
GtkWidget *rvrblabel3;
GtkWidget *spinbutton11;
GtkWidget *rvrblabel4;
GtkWidget *spinbutton18;
eqdefaults reverbeqdef;
soundreverb sndreverb;
}reverbeffect;

haaseffect haas1;
delayeffect delay1;
reverbeffect reverb1;

GtkWidget *statusbar;
gint context_id;

pthread_t tid[4];
cpu_set_t cpu[4];
int retval_thread0, retval_thread1, retval_thread2, retval_thread3;

snd_pcm_format_t samplingformat = SND_PCM_FORMAT_S16;
int samplingrate = 48000;
int queueLength = 2;
int frames_default = 128;

int mixerChannels = 2;

microphone mic;
audiomixer mx;
audiojack jack1;
speaker spk;

vpwidgets vpw1;
playlistparams plparams;

static void push_message(GtkWidget *widget, gint cid, char *msg)
{
  gchar *buff = g_strdup_printf("%s", msg);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), cid, buff);
  g_free(buff);
}

static void pop_message(GtkWidget *widget, gint cid)
{
  gtk_statusbar_pop(GTK_STATUSBAR(widget), cid);
}

void Delay_initAll(delayeffect *d, snd_pcm_format_t format, float rate, unsigned int channels)
{
	gchar *strval;

	pthread_mutex_lock(&(d->snddly.delaymutex));
	g_object_get((gpointer)d->combodelaytype, "active-id", &strval, NULL);
	//printf("Selected id %s\n", strval);
	d->snddly.delaytype = atoi((const char *)strval);
	sounddelay_init(1, atoi((const char *)strval), (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(d->spinbutton5)), 
					(float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(d->spinbutton6)), format, rate, channels, &(d->snddly));
	g_free(strval);
	pthread_mutex_unlock(&(d->snddly.delaymutex));
}

void Delay_closeAll(delayeffect *d)
{
	pthread_mutex_lock(&(d->snddly.delaymutex));
	sounddelay_close(&(d->snddly));
	pthread_mutex_unlock(&(d->snddly.delaymutex));
}

void Reverb_initAll(reverbeffect *r, snd_pcm_format_t format, float rate, unsigned int channels)
{
	int count;
	gchar *strval;

	pthread_mutex_lock(&(r->sndreverb.reverbmutex));
	g_object_get((gpointer)(r->comborvrbdelays), "active-id", &strval, NULL);
	//printf("Selected id %s\n", strval);
	count = atoi((const char *)strval);
	soundreverb_init(count, (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(r->spinbutton7)), (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(r->spinbutton8)), &(r->reverbeqdef), format, rate, channels, &(r->sndreverb));
	g_free(strval);
	pthread_mutex_unlock(&(r->sndreverb.reverbmutex));
}

void Reverb_closeAll(reverbeffect *r)
{
	pthread_mutex_lock(&(r->sndreverb.reverbmutex));
	soundreverb_close(&(r->sndreverb));
	pthread_mutex_unlock(&(r->sndreverb.reverbmutex));
}

gboolean close_windows_idle(gpointer data)
{
	gtk_main_quit();
	return(FALSE);
}

void close_windows()
{
	gdk_threads_add_idle(close_windows_idle, NULL);
}

static gpointer recorderthread(gpointer args)
{
	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	int err;

	//mic.device = "hw:1,0";
	gchar *strval;
	g_object_get((gpointer)comboinputdev, "active-id", &strval, NULL);
	if (strval)
	{
		init_mic(&mic, strval, SND_PCM_FORMAT_S16_LE, samplingrate, mx.outbufferframes, mic.mh.haasenabled, mic.mh.millisec, mic.mh.sndmod.enabled, mic.mh.sndmod.modfreq, mic.mh.sndmod.moddepth);
		g_free(strval);
		connect_audiojack(queueLength, &jack1, &mx);

		// kick initialize mixer channel
		memset(mic.mh.sbuffer, 0, mic.mh.sbuffersize);
		writetojack(mic.mh.sbuffer, mic.mh.sbuffersize, &jack1);
		pthread_mutex_lock(jack1.x->xmutex);
		*(jack1.rear) = *(jack1.front) = 0;
		pthread_mutex_unlock(jack1.x->xmutex);

		Delay_initAll(&delay1, mx.format, mx.rate, mx.channels);
		Reverb_initAll(&reverb1, mx.format, mx.rate, mx.channels);

		if ((err=init_audio_hw_mic(&mic)))
		{
			printf("Init mic error %d\n", err);
		}
		else
		{
			while (mic.status==MC_RUNNING)
			{
				if (read_mic(&mic))
				{
					haas_add(&(mic.mh));
					// Process input frames here
					sounddelay_add(mic.mh.sbuffer, mic.mh.sbuffersize, &(delay1.snddly));
					soundreverb_add(mic.mh.sbuffer, mic.mh.sbuffersize, &(reverb1.sndreverb));
//printf("writetojack\n");
					writetojack(mic.mh.sbuffer, mic.mh.sbuffersize, &jack1);
				}
				else
				{
					printf("stopping mic\n");
					mic.status=MC_STOPPED;
				}
			}
			close_audio_hw_mic(&mic);
		}
		Reverb_closeAll(&reverb1);
		Delay_closeAll(&delay1);

		close_audiojack(&jack1);
		close_mic(&mic);
	}
	else
	{
		printf("No input devices found, exiting\n");
		close_windows();
	}

//printf("exiting 0\n");
	retval_thread1 = 0;
	pthread_exit(&retval_thread1);
}

static gpointer mixerthread(gpointer args)
{
	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	gchar *strval;
	g_object_get((gpointer)combooutputdev, "active-id", &strval, NULL);
	init_spk(&spk, strval, samplingformat, samplingrate, stereo);
	if (strval)
		g_free(strval);

	int err;
	if ((err=init_audio_hw_spk(&spk)))
	{
		printf("Init spk error %d\n", err);
	}
	else
	{
		while (readfrommixer(&mx) != MX_STOPPED)
		{
			// process mixed stereo frames here

			write_spk(&spk, mx.outbuffer, mx.outbufferframes);
		}
	}
	close_audio_hw_spk(&spk);
	close_spk(&spk);

//printf("exiting 2\n");
	retval_thread2 = 0;
	pthread_exit(&retval_thread2);
}

/*
static gpointer filereaderthread(gpointer args) // test using file
{
	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	int bytesread, period_size=1024;
	char read_buffer[4096]; // period_size*4

	connect_audiojack(queueLength, &jack2, &mx);
	FILE *fp = fopen("piano2.wav", "rb");
	bytesread=fread(read_buffer, sizeof(char), 44, fp); // skip header
	while ((bytesread=fread(read_buffer, sizeof(char), period_size*4, fp))>0)
	{
		writetojack(read_buffer, 4096, &jack2);
	}

	close_audiojack(&jack2);

	fclose(fp);

//printf("exiting 3\n");
	retval_thread3 = 0;
	pthread_exit(&retval_thread3);
}
*/

static gpointer thread0(gpointer args)
{
	char delayms[50];

	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	int frames = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinbutton1));
	init_audiomixer(mixerChannels, MX_BLOCKING, samplingformat, samplingrate, frames, stereo, &mx);
	//init_audiomixer(mixerChannels, MX_NONBLOCKING, samplingformat, samplingrate, frames, stereo, &mx);

	sprintf(delayms, "%5.2f ms delay, rate %d fps, %d buffers", getdelay_audiomixer(&mx), samplingrate, queueLength);
	push_message(statusbar, context_id, delayms);

	int err;
	err = pthread_create(&(tid[1]), NULL, &recorderthread, NULL);
	if (err)
	{}
//printf("recorderthread->%d\n", 1);
	CPU_ZERO(&(cpu[1]));
	CPU_SET(1, &(cpu[1]));
	if ((err=pthread_setaffinity_np(tid[1], sizeof(cpu_set_t), &(cpu[1]))))
		printf("pthread_setaffinity_np error %d\n", err);

	err = pthread_create(&(tid[2]), NULL, &mixerthread, NULL);
	if (err)
	{}
//printf("mixerthread->%d\n", 1);
	CPU_ZERO(&(cpu[1]));
	CPU_SET(1, &(cpu[1]));
	if ((err=pthread_setaffinity_np(tid[2], sizeof(cpu_set_t), &(cpu[1]))))
		printf("pthread_setaffinity_np error %d\n", err);

	int i;
	if ((i=pthread_join(tid[1], NULL)))
		printf("pthread_join error, tid[1], %d\n", i);

	signalstop_audiomixer(&mx);
	if ((i=pthread_join(tid[2], NULL)))
		printf("pthread_join error, tid[2], %d\n", i);

	close_audiomixer(&mx);

//printf("exiting 0\n");
	retval_thread0 = 0;
	pthread_exit(&retval_thread0);
}

int create_thread0(void)
{
	int err;

	err = pthread_create(&(tid[0]), NULL, &thread0, NULL);
	if (err)
	{}
//printf("thread0->%d\n", 1);
	CPU_ZERO(&(cpu[1]));
	CPU_SET(1, &(cpu[1]));
	if ((err=pthread_setaffinity_np(tid[0], sizeof(cpu_set_t), &(cpu[1]))))
	{
		//printf("pthread_setaffinity_np error %d\n", err);
	}

	return(0);
}

void terminate_thread0(gpointer data)
{
	playlistparams *plp = (playlistparams *)data;

	press_vp_stop_button(plp);

	mic.status = MC_STOPPED;
	int i;
	if ((i=pthread_join(tid[0], NULL)))
		printf("pthread_join error, tid[0], %d\n", i);

	pop_message(statusbar, context_id);
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	playlistparams *plp = (playlistparams *)data;

	press_vp_resume_button(plp); // Press resume if paused
	press_vp_stop_button(plp); // Press stop if playing
	terminate_thread0(data);
	return FALSE; // return FALSE to emit destroy signal
}

static void destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

static void realize_cb(GtkWidget *widget, gpointer data)
{
	g_object_set((gpointer)delay1.combodelaytype, "active-id", "0", NULL);
	g_object_set((gpointer)reverb1.comborvrbdelays, "active-id", "12", NULL);
}

static void inputdev_changed(GtkWidget *combo, gpointer data)
{
	playlistparams *plp = (playlistparams *)data;
	//gchar *strval;

	press_vp_stop_button(plp);

	//g_object_get((gpointer)combo, "active-id", &strval, NULL);
	//printf("Selected id %s\n", strval);
	//g_free(strval);
	terminate_thread0(data);
	create_thread0();
}

static void frames_changed(GtkWidget *widget, gpointer data)
{
	playlistparams *plp = (playlistparams *)data;

	press_vp_stop_button(plp);

	terminate_thread0(data);
	create_thread0();
}

static void outputdev_changed(GtkWidget *combo, gpointer data)
{
	playlistparams *plp = (playlistparams *)data;
	//gchar *strval;

	press_vp_stop_button(plp);

	//g_object_get((gpointer)combo, "active-id", &strval, NULL);
	//printf("Selected id %s\n", strval);
	//g_free(strval);
	terminate_thread0(data);
	create_thread0();
}

static void haas_toggled(GtkWidget *togglebutton, gpointer data)
{
	microphone *m = (microphone*)data;

	pthread_mutex_lock(m->mh.haasmutex);
	m->mh.haasenabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
	pthread_mutex_unlock(m->mh.haasmutex);
	//printf("toggle state %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modenable)));
}

static void haasdly_changed(GtkWidget *widget, gpointer data)
{
	microphone *m = (microphone*)data;

	pthread_mutex_lock(m->mh.haasmutex);
	float millisec = (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	m->mh.millisec = millisec;
	if (m->mh.haasenabled)
	{
		haas_reinit(&(m->mh), m->mh.millisec);
	}
	pthread_mutex_unlock(m->mh.haasmutex);
}

static void mod_toggled(GtkWidget *togglebutton, gpointer data)
{
	microphone *m = (microphone*)data;

	pthread_mutex_lock(&(m->mh.sndmod.modmutex));
	m->mh.sndmod.enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
	pthread_mutex_unlock(&(m->mh.sndmod.modmutex));
	//printf("toggle state %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modenable)));
}

static void modfreq_changed(GtkWidget *widget, gpointer data)
{
	microphone *m = (microphone*)data;

	pthread_mutex_lock(&(m->mh.sndmod.modmutex));
	float modfreq = (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	soundmod_reinit(modfreq, m->mh.sndmod.moddepth, &(m->mh.sndmod));
	pthread_mutex_unlock(&(m->mh.sndmod.modmutex));
}

static void moddepth_changed(GtkWidget *widget, gpointer data)
{
	microphone *m = (microphone*)data;

	pthread_mutex_lock(&(m->mh.sndmod.modmutex));
	float moddepth = (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	soundmod_reinit(m->mh.sndmod.modfreq, moddepth, &(m->mh.sndmod));
	pthread_mutex_unlock(&(m->mh.sndmod.modmutex));
}

static void dly_toggled(GtkWidget *togglebutton, gpointer data)
{
	delayeffect *d = (delayeffect*)data;

	gchar *strval;

	pthread_mutex_lock(&(d->snddly.delaymutex));

	g_object_get((gpointer)d->combodelaytype, "active-id", &strval, NULL);
	//printf("Selected id %s\n", strval);
	d->snddly.delaytype = atoi((const char *)strval);
	g_free(strval);
	d->snddly.millisec = (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(d->spinbutton5));
	d->snddly.feedback = (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(d->spinbutton6));

	d->snddly.enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));

	pthread_mutex_unlock(&(d->snddly.delaymutex));
	//printf("toggle state %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dlyenable)));
}

void select_delay_types(delayeffect *d)
{
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(d->combodelaytype), "0", "Echo");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(d->combodelaytype), "1", "Delay");
	//gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(d->combodelaytype), "2", "Reverb");
	//gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(d->combodelaytype), "3", "Late");
}

static void delaytype_changed(GtkWidget *combo, gpointer data)
{
	delayeffect *d = (delayeffect*)data;

	gchar *strval;

	pthread_mutex_lock(&(d->snddly.delaymutex));
	g_object_get((gpointer)combo, "active-id", &strval, NULL);
	//printf("Selected id %s\n", strval);
	d->snddly.delaytype = atoi((const char *)strval);
	g_free(strval);
	if (d->snddly.enabled)
	{
		sounddelay_reinit(1, d->snddly.delaytype, d->snddly.millisec, d->snddly.feedback, &(d->snddly));
	}
	pthread_mutex_unlock(&(d->snddly.delaymutex));
}

static void delay_changed(GtkWidget *widget, gpointer data)
{
	delayeffect *d = (delayeffect*)data;

	pthread_mutex_lock(&(d->snddly.delaymutex));
	float newvalue = (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	d->snddly.millisec = newvalue;
	if (d->snddly.enabled)
	{
		sounddelay_reinit(1, d->snddly.delaytype, d->snddly.millisec, d->snddly.feedback, &(d->snddly));
	}
	pthread_mutex_unlock(&(d->snddly.delaymutex));
}

static void feedback_changed(GtkWidget *widget, gpointer data)
{
	delayeffect *d = (delayeffect*)data;

	pthread_mutex_lock(&(d->snddly.delaymutex));
	float newvalue = (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	d->snddly.feedback = newvalue;
	if (d->snddly.enabled)
	{
		sounddelay_reinit(1, d->snddly.delaytype, d->snddly.millisec, d->snddly.feedback, &(d->snddly));
	}
	pthread_mutex_unlock(&(d->snddly.delaymutex));
}

void select_delay_lines(reverbeffect *r)
{
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "1", "1 Delay");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "2", "2 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "3", "3 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "4", "4 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "5", "5 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "6", "6 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "7", "7 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "8", "8 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "9", "9 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "10", "10 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "11", "11 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "12", "12 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "13", "13 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "14", "14 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "15", "15 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "16", "16 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "17", "17 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "18", "18 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "19", "19 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "20", "20 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "21", "21 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "22", "22 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "23", "23 Delays");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r->comborvrbdelays), "24", "24 Delays");
}

static void rvrb_toggled(GtkWidget *togglebutton, gpointer data)
{
	reverbeffect *r = (reverbeffect*)data;

	pthread_mutex_lock(&(r->sndreverb.reverbmutex));
	r->sndreverb.enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
	pthread_mutex_unlock(&(r->sndreverb.reverbmutex));
	//printf("toggle state %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rvrbenable)));
}

static void delaylines_changed(GtkWidget *combo, gpointer data)
{
	reverbeffect *r = (reverbeffect*)data;
	gchar *strval;

	pthread_mutex_lock(&(r->sndreverb.reverbmutex));
	g_object_get((gpointer)combo, "active-id", &strval, NULL);
	//printf("Selected id %s\n", strval);
	r->sndreverb.reverbdelaylines = atoi((const char *)strval);
	g_free(strval);
	if (r->sndreverb.enabled)
	{
		soundreverb_reinit(r->sndreverb.reverbdelaylines, r->sndreverb.feedback, r->sndreverb.presence, &(r->reverbeqdef), &(r->sndreverb));
	}
	pthread_mutex_unlock(&(r->sndreverb.reverbmutex));
}

static void rvrbfeedback_changed(GtkWidget *widget, gpointer data)
{
	reverbeffect *r = (reverbeffect*)data;

	pthread_mutex_lock(&(r->sndreverb.reverbmutex));
	float newvalue = (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	r->sndreverb.feedback = newvalue;
	if (r->sndreverb.enabled)
	{
		soundreverb_reinit(r->sndreverb.reverbdelaylines, r->sndreverb.feedback, r->sndreverb.presence, &(r->reverbeqdef), &(r->sndreverb));
	}
	pthread_mutex_unlock(&(r->sndreverb.reverbmutex));
}

static void rvrbpresence_changed(GtkWidget *widget, gpointer data)
{
	reverbeffect *r = (reverbeffect*)data;

	pthread_mutex_lock(&(r->sndreverb.reverbmutex));
	float newvalue = (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	r->sndreverb.presence = newvalue;
	if (r->sndreverb.enabled)
	{
		soundreverb_reinit(r->sndreverb.reverbdelaylines, r->sndreverb.feedback, r->sndreverb.presence, &(r->reverbeqdef), &(r->sndreverb));
	}
	pthread_mutex_unlock(&(r->sndreverb.reverbmutex));
}

static void rvrbLSH_changed(GtkWidget *widget, gpointer data)
{
	reverbeffect *r = (reverbeffect*)data;

	pthread_mutex_lock(&(r->sndreverb.reverbmutex));
	float newvalue = (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	r->reverbeqdef.eqfreqs[0] = newvalue;
	if (r->sndreverb.enabled)
	{
		soundreverb_reinit(r->sndreverb.reverbdelaylines, r->sndreverb.feedback, r->sndreverb.presence, &(r->reverbeqdef), &(r->sndreverb));
	}
	pthread_mutex_unlock(&(r->sndreverb.reverbmutex));
}

static void rvrbLPF_changed(GtkWidget *widget, gpointer data)
{
	reverbeffect *r = (reverbeffect*)data;

	pthread_mutex_lock(&(r->sndreverb.reverbmutex));
	float newvalue = (float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	r->reverbeqdef.eqfreqs[1] = newvalue;
	if (r->sndreverb.enabled)
	{
		soundreverb_reinit(r->sndreverb.reverbdelaylines, r->sndreverb.feedback, r->sndreverb.presence, &(r->reverbeqdef), &(r->sndreverb));
	}
	pthread_mutex_unlock(&(r->sndreverb.reverbmutex));
}

static void vp_toggled(GtkWidget *togglebutton, gpointer data)
{
	vpwidgets* vpw = (vpwidgets*)data;

	toggle_vp(vpw, togglebutton);
}

void setup_default_icon(char *filename)
{
	GdkPixbuf *pixbuf;
	GError *err;

	err = NULL;
	pixbuf = gdk_pixbuf_new_from_file(filename, &err);

	if (pixbuf)
	{
		GList *list;      

		list = NULL;
		list = g_list_append(list, pixbuf);
		gtk_window_set_default_icon_list(list);
		g_list_free(list);
		g_object_unref(pixbuf);
    }
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "tr_TR.UTF-8");

	setup_default_icon("./Cover.png");

	init_playlistparams(&plparams, &vpw1, &mic, &spk, 20, 20, samplingrate, 4); // video, audio, spk_samplingrate, thread_count

	if (!singleinstance(&plparams, 2017, argc, argv))
	{
		//wprintf(L"multiple instance\n");
		exit(0);
	}

	/* This is called in all GTK applications. Arguments are parsed
	 * from the command line and are returned to the application. */
	gtk_init(&argc, &argv);

	/* create a new window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER (window), 2);
	//gtk_widget_set_size_request(window, 100, 100);
	gtk_window_set_title(GTK_WINDOW(window), "Cover");
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	/* When the window is given the "delete-event" signal (this is given
	* by the window manager, usually by the "close" option, or on the
	* titlebar), we ask it to call the delete_event () function
	* as defined above. The data passed to the callback
	* function is NULL and is ignored in the callback function. */
	g_signal_connect (window, "delete-event", G_CALLBACK (delete_event), (void*)&plparams);
    
	/* Here we connect the "destroy" event to a signal handler.  
	* This event occurs when we call gtk_widget_destroy() on the window,
	* or if we return FALSE in the "delete-event" callback. */
	g_signal_connect(window, "destroy", G_CALLBACK (destroy), NULL);

	g_signal_connect(window, "realize", G_CALLBACK (realize_cb), NULL);

// vertical box
	fxbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(window), fxbox);

// config frame
	frameconf = gtk_frame_new("Configuration");
	//gtk_widget_set_size_request(frameconf, 300, 50);
	gtk_container_add(GTK_CONTAINER(fxbox), frameconf);

// horizontal box
	confbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(frameconf), confbox);

// input devices combobox
	inputdevlabel = gtk_label_new("Input Device");
	gtk_widget_set_size_request(inputdevlabel, 100, 30);
	gtk_container_add(GTK_CONTAINER(confbox), inputdevlabel);
	comboinputdev = gtk_combo_box_text_new();
	gtk_container_add(GTK_CONTAINER(confbox), comboinputdev);

// frames
	frameslabel = gtk_label_new("Frames");
	gtk_widget_set_size_request(frameslabel, 100, 30);
	gtk_container_add(GTK_CONTAINER(confbox), frameslabel);
	spinbutton1 = gtk_spin_button_new_with_range(32.0, 1024.0 , 1.0);
	gtk_widget_set_size_request(spinbutton1, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton1), (float)frames_default);
	g_signal_connect(GTK_SPIN_BUTTON(spinbutton1), "value-changed", G_CALLBACK(frames_changed), (void*)&plparams);
	gtk_container_add(GTK_CONTAINER(confbox), spinbutton1);

// output devices combobox
	outputdevlabel = gtk_label_new("Output Device");
	gtk_widget_set_size_request(outputdevlabel, 100, 30);
	gtk_container_add(GTK_CONTAINER(confbox), outputdevlabel);
	combooutputdev = gtk_combo_box_text_new();
	gtk_container_add(GTK_CONTAINER(confbox), combooutputdev);

	populate_card_list(comboinputdev, combooutputdev); // Fill input/output devices combo boxes
	g_signal_connect(GTK_COMBO_BOX(comboinputdev), "changed", G_CALLBACK(inputdev_changed), (void*)&plparams);
	g_signal_connect(GTK_COMBO_BOX(combooutputdev), "changed", G_CALLBACK(outputdev_changed), (void*)&plparams);

// checkbox
	vpw1.vpvisible = TRUE;
	vpenable = gtk_check_button_new_with_label("VP");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vpenable), vpw1.vpvisible);
	g_signal_connect(GTK_TOGGLE_BUTTON(vpenable), "toggled", G_CALLBACK(vp_toggled), (void*)&vpw1);
	gtk_container_add(GTK_CONTAINER(confbox), vpenable);


// haas frame
	haas1.framehaas1 = gtk_frame_new("Haas");
	gtk_container_add(GTK_CONTAINER(fxbox), haas1.framehaas1);

// horizontal box
	haas1.haasbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(haas1.framehaas1), haas1.haasbox1);

	mic.mh.haasenabled = TRUE;
	haas1.haasenable = gtk_check_button_new_with_label("Haas");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(haas1.haasenable), mic.mh.haasenabled);
	g_signal_connect(GTK_TOGGLE_BUTTON(haas1.haasenable), "toggled", G_CALLBACK(haas_toggled), &mic);
	gtk_container_add(GTK_CONTAINER(haas1.haasbox1), haas1.haasenable);

// haas delay
	mic.mh.millisec = 15.0;
	haas1.haaslabel1 = gtk_label_new("Haas Delay (ms)");
	gtk_widget_set_size_request(haas1.haaslabel1, 100, 30);
	gtk_container_add(GTK_CONTAINER(haas1.haasbox1), haas1.haaslabel1);
	haas1.spinbutton2 = gtk_spin_button_new_with_range(1.0, 30.0 , 0.1);
	gtk_widget_set_size_request(haas1.spinbutton2, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(haas1.spinbutton2), mic.mh.millisec);
	g_signal_connect(GTK_SPIN_BUTTON(haas1.spinbutton2), "value-changed", G_CALLBACK(haasdly_changed), &mic);
	gtk_container_add(GTK_CONTAINER(haas1.haasbox1), haas1.spinbutton2);

// checkbox
	mic.mh.sndmod.enabled = FALSE;
	haas1.modenable = gtk_check_button_new_with_label("Modulation");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(haas1.modenable), mic.mh.sndmod.enabled);
	g_signal_connect(GTK_TOGGLE_BUTTON(haas1.modenable), "toggled", G_CALLBACK(mod_toggled), &mic);
	gtk_container_add(GTK_CONTAINER(haas1.haasbox1), haas1.modenable);

// rate
	mic.mh.sndmod.modfreq = 0.3;
	haas1.modlabel1 = gtk_label_new("Rate (Hz)");
	gtk_widget_set_size_request(haas1.modlabel1, 100, 30);
	gtk_container_add(GTK_CONTAINER(haas1.haasbox1), haas1.modlabel1);
	haas1.spinbutton14 = gtk_spin_button_new_with_range(0.1, 20.0, 0.1);
	gtk_widget_set_size_request(haas1.spinbutton14, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(haas1.spinbutton14), mic.mh.sndmod.modfreq);
	g_signal_connect(GTK_SPIN_BUTTON(haas1.spinbutton14), "value-changed", G_CALLBACK(modfreq_changed), &mic);
	gtk_container_add(GTK_CONTAINER(haas1.haasbox1), haas1.spinbutton14);

// depth
	mic.mh.sndmod.moddepth = 0.02;
	haas1.modlabel2 = gtk_label_new("Depth");
	gtk_widget_set_size_request(haas1.modlabel2, 100, 30);
	gtk_container_add(GTK_CONTAINER(haas1.haasbox1), haas1.modlabel2);
	haas1.spinbutton15 = gtk_spin_button_new_with_range(0.001, 0.900, 0.001);
	gtk_widget_set_size_request(haas1.spinbutton15, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(haas1.spinbutton15), mic.mh.sndmod.moddepth);
	g_signal_connect(GTK_SPIN_BUTTON(haas1.spinbutton15), "value-changed", G_CALLBACK(moddepth_changed),&mic);
	gtk_container_add(GTK_CONTAINER(haas1.haasbox1), haas1.spinbutton15);


// delay frame
	delay1.framedelay1 = gtk_frame_new("Delay");
	gtk_container_add(GTK_CONTAINER(fxbox), delay1.framedelay1);

// horizontal box
    delay1.dlybox1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(delay1.framedelay1), delay1.dlybox1);

// checkbox
	delay1.snddly.enabled = FALSE;
	delay1.dlyenable = gtk_check_button_new_with_label("Enable");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(delay1.dlyenable), delay1.snddly.enabled);
	g_signal_connect(GTK_TOGGLE_BUTTON(delay1.dlyenable), "toggled", G_CALLBACK(dly_toggled), &delay1);
	gtk_container_add(GTK_CONTAINER(delay1.dlybox1), delay1.dlyenable);

// combobox
	delay1.delaytypelabel = gtk_label_new("Delay Type");
	gtk_widget_set_size_request(delay1.delaytypelabel, 100, 30);
	gtk_container_add(GTK_CONTAINER(delay1.dlybox1), delay1.delaytypelabel);
	delay1.combodelaytype = gtk_combo_box_text_new();
	select_delay_types(&delay1);
	g_signal_connect(GTK_COMBO_BOX(delay1.combodelaytype), "changed", G_CALLBACK(delaytype_changed), &delay1);
	gtk_container_add(GTK_CONTAINER(delay1.dlybox1), delay1.combodelaytype);

// delay
	delay1.delaylabel1 = gtk_label_new("Delay (ms)");
	gtk_widget_set_size_request(delay1.delaylabel1, 100, 30);
	gtk_container_add(GTK_CONTAINER(delay1.dlybox1), delay1.delaylabel1);
	delay1.spinbutton5 = gtk_spin_button_new_with_range(0.0, 2000.0, 10.0);
	gtk_widget_set_size_request(delay1.spinbutton5, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(delay1.spinbutton5), 500.0);
	g_signal_connect(GTK_SPIN_BUTTON(delay1.spinbutton5), "value-changed", G_CALLBACK(delay_changed), &delay1);
	gtk_container_add(GTK_CONTAINER(delay1.dlybox1), delay1.spinbutton5);

// feedback
	delay1.feedbacklabel1 = gtk_label_new("Feedback");
	gtk_widget_set_size_request(delay1.feedbacklabel1, 100, 30);
	gtk_container_add(GTK_CONTAINER(delay1.dlybox1), delay1.feedbacklabel1);
	delay1.spinbutton6 = gtk_spin_button_new_with_range(0.0, 0.95, 0.01);
	gtk_widget_set_size_request(delay1.spinbutton6, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(delay1.spinbutton6), 0.7);
	g_signal_connect(GTK_SPIN_BUTTON(delay1.spinbutton6), "value-changed", G_CALLBACK(feedback_changed), &delay1);
	gtk_container_add(GTK_CONTAINER(delay1.dlybox1), delay1.spinbutton6);


	set_reverbeq(&(reverb1.reverbeqdef));
// reverb frame
	reverb1.framervrb1 = gtk_frame_new("Reverb");
	gtk_container_add(GTK_CONTAINER(fxbox), reverb1.framervrb1);

// vertical box
	reverb1.rvrbboxv = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(reverb1.framervrb1), reverb1.rvrbboxv);

// horizontal box
	reverb1.rvrbbox1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(reverb1.rvrbboxv), reverb1.rvrbbox1);

// checkbox
	reverb1.sndreverb.enabled = FALSE;
	reverb1.rvrbenable = gtk_check_button_new_with_label("Enable");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reverb1.rvrbenable), reverb1.sndreverb.enabled);
	g_signal_connect(GTK_TOGGLE_BUTTON(reverb1.rvrbenable), "toggled", G_CALLBACK(rvrb_toggled), &reverb1);
	gtk_container_add(GTK_CONTAINER(reverb1.rvrbbox1), reverb1.rvrbenable);

// presence
	reverb1.rvrblabel2 = gtk_label_new("Presence");
	gtk_widget_set_size_request(reverb1.rvrblabel2, 100, 30);
	gtk_container_add(GTK_CONTAINER(reverb1.rvrbbox1), reverb1.rvrblabel2);
	reverb1.spinbutton8 = gtk_spin_button_new_with_range(0.1, 2.00, 0.1);
	gtk_widget_set_size_request(reverb1.spinbutton8, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(reverb1.spinbutton8), 0.6);
	g_signal_connect(GTK_SPIN_BUTTON(reverb1.spinbutton8), "value-changed", G_CALLBACK(rvrbpresence_changed), &reverb1);
	gtk_container_add(GTK_CONTAINER(reverb1.rvrbbox1), reverb1.spinbutton8);

// feedback
	reverb1.rvrblabel1 = gtk_label_new("Feedback");
	gtk_widget_set_size_request(reverb1.rvrblabel1, 100, 30);
	gtk_container_add(GTK_CONTAINER(reverb1.rvrbbox1), reverb1.rvrblabel1);
	reverb1.spinbutton7 = gtk_spin_button_new_with_range(0.0, 0.95, 0.01);
	gtk_widget_set_size_request(reverb1.spinbutton7, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(reverb1.spinbutton7), 0.65);
	g_signal_connect(GTK_SPIN_BUTTON(reverb1.spinbutton7), "value-changed", G_CALLBACK(rvrbfeedback_changed), &reverb1);
	gtk_container_add(GTK_CONTAINER(reverb1.rvrbbox1), reverb1.spinbutton7);

// horizontal box
	reverb1.rvrbbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(reverb1.rvrbboxv), reverb1.rvrbbox2);

// combobox
	reverb1.rvrbdelays = gtk_label_new("Delay Lines");
	gtk_widget_set_size_request(reverb1.rvrbdelays, 100, 30);
	gtk_container_add(GTK_CONTAINER(reverb1.rvrbbox2), reverb1.rvrbdelays);
    reverb1.comborvrbdelays = gtk_combo_box_text_new();
    select_delay_lines(&reverb1);
    g_signal_connect(GTK_COMBO_BOX(reverb1.comborvrbdelays), "changed", G_CALLBACK(delaylines_changed), &reverb1);
    gtk_container_add(GTK_CONTAINER(reverb1.rvrbbox2), reverb1.comborvrbdelays);

// LSH
	reverb1.rvrblabel4 = gtk_label_new("LSH (Hz)");
	gtk_widget_set_size_request(reverb1.rvrblabel4, 100, 30);
	gtk_container_add(GTK_CONTAINER(reverb1.rvrbbox2), reverb1.rvrblabel4);
	reverb1.spinbutton18 = gtk_spin_button_new_with_range(1.0, 1000.0, 100.0);
	gtk_widget_set_size_request(reverb1.spinbutton18, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(reverb1.spinbutton18), reverb1.reverbeqdef.eqfreqs[0]);
	g_signal_connect(GTK_SPIN_BUTTON(reverb1.spinbutton18), "value-changed", G_CALLBACK(rvrbLSH_changed), &reverb1);
	gtk_container_add(GTK_CONTAINER(reverb1.rvrbbox2), reverb1.spinbutton18);

// LPF
	reverb1.rvrblabel3 = gtk_label_new("LPF (Hz)");
	gtk_widget_set_size_request(reverb1.rvrblabel3, 100, 30);
	gtk_container_add(GTK_CONTAINER(reverb1.rvrbbox2), reverb1.rvrblabel3);
	reverb1.spinbutton11 = gtk_spin_button_new_with_range(32.0, 20000.0, 100.0);
	gtk_widget_set_size_request(reverb1.spinbutton11, 120, 30);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(reverb1.spinbutton11), reverb1.reverbeqdef.eqfreqs[1]);
	g_signal_connect(GTK_SPIN_BUTTON(reverb1.spinbutton11), "value-changed", G_CALLBACK(rvrbLPF_changed), &reverb1);
	gtk_container_add(GTK_CONTAINER(reverb1.rvrbbox2), reverb1.spinbutton11);


// statusbar
	statusbar = gtk_statusbar_new();
	gtk_container_add(GTK_CONTAINER(fxbox), statusbar);

	//init_videoplayerwidgets(&plparams, argc, argv, 800, 450, &mx);
	init_videoplayerwidgets(&plparams, argc, argv, 640, 360, &mx);

	gtk_widget_show_all(window);

	create_thread0();
	vpw_commandline(&plparams, argc);
	gtk_main();

	close_videoplayerwidgets(&vpw1);
	close_playlistparams(&plparams);

	exit(0);
}
