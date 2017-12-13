#ifndef VideoPlayerWidgetsH
#define VideoPlayerWidgetsH

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <wchar.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <sqlite3.h>

#include "AudioMic.h"
#include "AudioSpk.h"
#include "AudioMixer.h"
#include "VideoPlayer.h"

typedef enum
{
	text_html,
	string
}target_info;

typedef struct
{
	int sockfd, newsockfd, portno;
	unsigned int clilen;
	struct sockaddr_in serv_addr, cli_addr;
	pthread_t tid;
	int retval;
	pthread_mutex_t *simutex;
	pthread_cond_t *sicond;
	int single, binddone;
	int argc;
	char** argv;
	int num_pending_connections;
	int buffersize; // bytes, N*sizeof(wchar_t)
	int pathlength; // wchar_t
}instancesock;

typedef struct
{
GtkWidget *vpwindow;
GtkWidget *box1;
GtkWidget *horibox;
GtkWidget *drawingarea;
GtkWidget *button_box;
GtkWidget *button1;
GtkWidget *button2;
GtkWidget *button8;
GtkWidget *button9;
GtkWidget *button10;
GtkWidget *button11;
GtkWidget *buttonParameters;
GtkWidget *buttonTest;
GtkAdjustment *hadjustment;
GtkWidget *hscale;
GtkWidget *stackswitcher;
GtkWidget *stack;
GtkWidget *playerbox;
GtkWidget *box2;
GtkWidget *playlistbox;
GtkWidget *button_box2;
GtkWidget *button3;
GtkWidget *button4;
GtkWidget *button6;
GtkWidget *button7;
GtkWidget *listview;
GtkListStore *store;
GtkTreeIter iter;
GtkWidget *scrolled_window;
GtkWidget *statusbar;
gint context_id;
GtkWidget *windoweq;
GtkWidget *eqvbox;
GtkWidget *eqbox;
GtkAdjustment *vadj0;
GtkAdjustment *vadj1;
GtkAdjustment *vadj2;
GtkAdjustment *vadj3;
GtkAdjustment *vadj4;
GtkAdjustment *vadj5;
GtkAdjustment *vadj6;
GtkAdjustment *vadj7;
GtkAdjustment *vadj8;
GtkAdjustment *vadj9;
GtkAdjustment *vadjA;
GtkAdjustment *vadjV;
GtkWidget *vscaleeq0;
GtkWidget *vscaleeq1;
GtkWidget *vscaleeq2;
GtkWidget *vscaleeq3;
GtkWidget *vscaleeq4;
GtkWidget *vscaleeq5;
GtkWidget *vscaleeq6;
GtkWidget *vscaleeq7;
GtkWidget *vscaleeq8;
GtkWidget *vscaleeq9;
GtkWidget *vscaleeqA;
GtkWidget *vscaleeqV;
GtkWidget *eqbox0;
GtkWidget *eqbox1;
GtkWidget *eqbox2;
GtkWidget *eqbox3;
GtkWidget *eqbox4;
GtkWidget *eqbox5;
GtkWidget *eqbox6;
GtkWidget *eqbox7;
GtkWidget *eqbox8;
GtkWidget *eqbox9;
GtkWidget *eqboxA;
GtkWidget *eqboxV;
GtkWidget *eqlabel0;
GtkWidget *eqlabel1;
GtkWidget *eqlabel2;
GtkWidget *eqlabel3;
GtkWidget *eqlabel4;
GtkWidget *eqlabel5;
GtkWidget *eqlabel6;
GtkWidget *eqlabel7;
GtkWidget *eqlabel8;
GtkWidget *eqlabel9;
GtkWidget *eqlabelA;
GtkWidget *eqlabelV;
GtkWidget *eqenable;
GtkWidget *eqautolevel;
GtkWidget *combopreset;
/*
GtkWidget *windowparm;
GtkWidget *parmvbox;
GtkWidget *parmhbox1;
GtkWidget *parmlabel11;
GtkWidget *parmlabel12;
GtkWidget *spinbutton1;
GtkWidget *parmhbox2;
GtkWidget *parmlabel21;
GtkWidget *parmlabel22;
GtkWidget *spinbutton2;
GtkWidget *parmhbox3;
GtkWidget *parmlabel3;
GtkWidget *spinbutton3;
//GtkWidget *parmhbox4;
//GtkWidget *parmlabel4;
//GtkWidget *spinbutton4;
GtkWidget *parmhlevel1;
GtkWidget *levellabel1;
GtkWidget *parmhlevel2;
GtkWidget *levellabel2;
GtkWidget *parmhlevel3;
GtkWidget *levellabel3;
GtkWidget *parmhlevel4;
GtkWidget *levellabel4;
GtkWidget *parmhlevel5;
GtkWidget *levellabel5;
GtkWidget *parmhlevel6;
GtkWidget *levellabel6;
GtkWidget *parmhlevel7;
GtkWidget *levellabel7;
GtkWidget *levelbar1;
GtkWidget *levelbar2;
GtkWidget *levelbar3;
GtkWidget *levelbar4;
GtkWidget *levelbar5;
GtkWidget *levelbar6;
GtkWidget *levelbar7;
GtkWidget *label1;
GtkWidget *label2;
GtkWidget *label3;
GtkWidget *label4;
GtkWidget *label5;
GtkWidget *label6;
GtkWidget *label7;
*/
GtkWidget *mixvbox;
GtkWidget *mixhbox;
GtkWidget *micnull;
GtkWidget *notebook;
GtkWidget *nbpage1;
GtkWidget *nbpage2;
GtkWidget *nbpage3;
GtkWidget *nbpage4;

int vpvisible, hscaleupd, sliding, playerWidth, playerHeight;

videoplayer vp;
eqdefaults aedef;
audioequalizer ae;
audiomixer *ax;
int last_id;
pthread_t tid;
int retval0;
cpu_set_t cpu[4];
char msg[256];
GtkTargetEntry target_entries[2];
instancesock sisocket;
}vpwidgets;

typedef struct
{
vpwidgets *vpw;
microphone *mic;
speaker *spk;
int vqMaxLength, aqMaxLength, thread_count, spk_samplingrate;
}playlistparams;

void init_playlistparams(playlistparams *plparams, vpwidgets *vpw, microphone *mic, speaker *spk, int vqMaxLength, int aqMaxLength, int spk_samplingrate, int thread_count);
void close_playlistparams(playlistparams *plparams);
void toggle_vp(vpwidgets *vpw, GtkWidget *togglebutton);
void init_videoplayerwidgets(playlistparams *plp, int argc, char** argv, int playWidth, int playHeight, audiomixer *x);
void close_videoplayerwidgets(vpwidgets *vpw);
void press_vp_stop_button(playlistparams *plp);
void press_vp_resume_button(playlistparams *plp);
void vpw_commandline(playlistparams *plp, int argcount);
gpointer singleinstancethread(gpointer args);
int singleinstance(playlistparams *plp, int portno, int argc, char **argv);
#endif
