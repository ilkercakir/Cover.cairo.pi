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

/* 
 * First 1000 Prime Numbers
 * 
      2      3      5      7     11     13     17     19     23     29 
     31     37     41     43     47     53     59     61     67     71 
     73     79     83     89     97    101    103    107    109    113 
    127    131    137    139    149    151    157    163    167    173 
    179    181    191    193    197    199    211    223    227    229 
    233    239    241    251    257    263    269    271    277    281 
    283    293    307    311    313    317    331    337    347    349 
    353    359    367    373    379    383    389    397    401    409 
    419    421    431    433    439    443    449    457    461    463 
    467    479    487    491    499    503    509    521    523    541 
    547    557    563    569    571    577    587    593    599    601 
    607    613    617    619    631    641    643    647    653    659 
    661    673    677    683    691    701    709    719    727    733 
    739    743    751    757    761    769    773    787    797    809 
    811    821    823    827    829    839    853    857    859    863 
    877    881    883    887    907    911    919    929    937    941 
    947    953    967    971    977    983    991    997   1009   1013 
   1019   1021   1031   1033   1039   1049   1051   1061   1063   1069 
   1087   1091   1093   1097   1103   1109   1117   1123   1129   1151 
   1153   1163   1171   1181   1187   1193   1201   1213   1217   1223 
   1229   1231   1237   1249   1259   1277   1279   1283   1289   1291 
   1297   1301   1303   1307   1319   1321   1327   1361   1367   1373 
   1381   1399   1409   1423   1427   1429   1433   1439   1447   1451 
   1453   1459   1471   1481   1483   1487   1489   1493   1499   1511 
   1523   1531   1543   1549   1553   1559   1567   1571   1579   1583 
   1597   1601   1607   1609   1613   1619   1621   1627   1637   1657 
   1663   1667   1669   1693   1697   1699   1709   1721   1723   1733 
   1741   1747   1753   1759   1777   1783   1787   1789   1801   1811 
   1823   1831   1847   1861   1867   1871   1873   1877   1879   1889 
   1901   1907   1913   1931   1933   1949   1951   1973   1979   1987 
   1993   1997   1999   2003   2011   2017   2027   2029   2039   2053 
   2063   2069   2081   2083   2087   2089   2099   2111   2113   2129 
   2131   2137   2141   2143   2153   2161   2179   2203   2207   2213 
   2221   2237   2239   2243   2251   2267   2269   2273   2281   2287 
   2293   2297   2309   2311   2333   2339   2341   2347   2351   2357 
   2371   2377   2381   2383   2389   2393   2399   2411   2417   2423 
   2437   2441   2447   2459   2467   2473   2477   2503   2521   2531 
   2539   2543   2549   2551   2557   2579   2591   2593   2609   2617 
   2621   2633   2647   2657   2659   2663   2671   2677   2683   2687 
   2689   2693   2699   2707   2711   2713   2719   2729   2731   2741 
   2749   2753   2767   2777   2789   2791   2797   2801   2803   2819 
   2833   2837   2843   2851   2857   2861   2879   2887   2897   2903 
   2909   2917   2927   2939   2953   2957   2963   2969   2971   2999 
   3001   3011   3019   3023   3037   3041   3049   3061   3067   3079 
   3083   3089   3109   3119   3121   3137   3163   3167   3169   3181 
   3187   3191   3203   3209   3217   3221   3229   3251   3253   3257 
   3259   3271   3299   3301   3307   3313   3319   3323   3329   3331 
   3343   3347   3359   3361   3371   3373   3389   3391   3407   3413 
   3433   3449   3457   3461   3463   3467   3469   3491   3499   3511 
   3517   3527   3529   3533   3539   3541   3547   3557   3559   3571 
   3581   3583   3593   3607   3613   3617   3623   3631   3637   3643 
   3659   3671   3673   3677   3691   3697   3701   3709   3719   3727 
   3733   3739   3761   3767   3769   3779   3793   3797   3803   3821 
   3823   3833   3847   3851   3853   3863   3877   3881   3889   3907 
   3911   3917   3919   3923   3929   3931   3943   3947   3967   3989 
   4001   4003   4007   4013   4019   4021   4027   4049   4051   4057 
   4073   4079   4091   4093   4099   4111   4127   4129   4133   4139 
   4153   4157   4159   4177   4201   4211   4217   4219   4229   4231 
   4241   4243   4253   4259   4261   4271   4273   4283   4289   4297 
   4327   4337   4339   4349   4357   4363   4373   4391   4397   4409 
   4421   4423   4441   4447   4451   4457   4463   4481   4483   4493 
   4507   4513   4517   4519   4523   4547   4549   4561   4567   4583 
   4591   4597   4603   4621   4637   4639   4643   4649   4651   4657 
   4663   4673   4679   4691   4703   4721   4723   4729   4733   4751 
   4759   4783   4787   4789   4793   4799   4801   4813   4817   4831 
   4861   4871   4877   4889   4903   4909   4919   4931   4933   4937 
   4943   4951   4957   4967   4969   4973   4987   4993   4999   5003 
   5009   5011   5021   5023   5039   5051   5059   5077   5081   5087 
   5099   5101   5107   5113   5119   5147   5153   5167   5171   5179 
   5189   5197   5209   5227   5231   5233   5237   5261   5273   5279 
   5281   5297   5303   5309   5323   5333   5347   5351   5381   5387 
   5393   5399   5407   5413   5417   5419   5431   5437   5441   5443 
   5449   5471   5477   5479   5483   5501   5503   5507   5519   5521 
   5527   5531   5557   5563   5569   5573   5581   5591   5623   5639 
   5641   5647   5651   5653   5657   5659   5669   5683   5689   5693 
   5701   5711   5717   5737   5741   5743   5749   5779   5783   5791 
   5801   5807   5813   5821   5827   5839   5843   5849   5851   5857 
   5861   5867   5869   5879   5881   5897   5903   5923   5927   5939 
   5953   5981   5987   6007   6011   6029   6037   6043   6047   6053 
   6067   6073   6079   6089   6091   6101   6113   6121   6131   6133 
   6143   6151   6163   6173   6197   6199   6203   6211   6217   6221 
   6229   6247   6257   6263   6269   6271   6277   6287   6299   6301 
   6311   6317   6323   6329   6337   6343   6353   6359   6361   6367 
   6373   6379   6389   6397   6421   6427   6449   6451   6469   6473 
   6481   6491   6521   6529   6547   6551   6553   6563   6569   6571 
   6577   6581   6599   6607   6619   6637   6653   6659   6661   6673 
   6679   6689   6691   6701   6703   6709   6719   6733   6737   6761 
   6763   6779   6781   6791   6793   6803   6823   6827   6829   6833 
   6841   6857   6863   6869   6871   6883   6899   6907   6911   6917 
   6947   6949   6959   6961   6967   6971   6977   6983   6991   6997 
   7001   7013   7019   7027   7039   7043   7057   7069   7079   7103 
   7109   7121   7127   7129   7151   7159   7177   7187   7193   7207
   7211   7213   7219   7229   7237   7243   7247   7253   7283   7297 
   7307   7309   7321   7331   7333   7349   7351   7369   7393   7411 
   7417   7433   7451   7457   7459   7477   7481   7487   7489   7499 
   7507   7517   7523   7529   7537   7541   7547   7549   7559   7561 
   7573   7577   7583   7589   7591   7603   7607   7621   7639   7643 
   7649   7669   7673   7681   7687   7691   7699   7703   7717   7723 
   7727   7741   7753   7757   7759   7789   7793   7817   7823   7829 
   7841   7853   7867   7873   7877   7879   7883   7901   7907   7919 

*/

int soundreverb_initprimes_callback(void *data, int argc, char **argv, char **azColName) 
{
	soundreverb *r = (soundreverb *)data;

//    for (int i = 0; i < argc; i++)
//    {
//     printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
//    }
	r->reverbprimes[r->i++] = atof(argv[1]);

	return 0;
}

void soundreverb_initprimes(soundreverb *r)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char sql[200];
	int rc;
	int i;

	//float reverbprimes[] = {97.0, 101.0, 103.0, 107.0, 109.0, 113.0, 127.0, 131.0, 137.0, 139.0, 149.0, 151.0, 157.0, 163.0, 167.0, 173.0, 179.0, 181.0};

	r->reverbprimes = malloc(r->reverbdelaylines*sizeof(float));
	r->feedbacks = malloc(r->reverbdelaylines*sizeof(float));
	r->snddlyrev = malloc(r->reverbdelaylines*sizeof(sounddelay));

	r->i = 0;

	if ((rc = sqlite3_open("/var/sqlite3DATA/mediaplayer.db", &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		sprintf(sql, "select * from primes where prime >= %d order by id limit %d;", r->reflect, r->reverbdelaylines);
		if ((rc = sqlite3_exec(db, sql, soundreverb_initprimes_callback, (void*)r, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to select data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);

//printf("%d initialized\n", r->i);
//	for(i=0;i<r->i;i++)
//		printf("%5.2f\n", r->reverbprimes[i]);

	r->alpha = -log(r->feedback) / (r->reverbprimes[0]/1000.0);
	for(i=0;i<r->reverbdelaylines;i++)
	{
		r->feedbacks[i] = exp(-r->alpha*r->reverbprimes[i]/1000.0);
		//printf("%d\t%f\n", i, r->feedbacks[i]);
	}
}

void soundreverb_reinit(int reflect, int delaylines, float feedback, float presence, eqdefaults *d, soundreverb *r)
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
	free(r->reverbprimes);
	r->reverbprimes = NULL;
	free(r->feedbacks);
	r->feedbacks = NULL;
	free(r->snddlyrev);	
	r->snddlyrev = NULL;

	r->reflect = reflect;
	r->reverbdelaylines = delaylines;
	r->feedback = feedback;
	r->presence = presence;
	soundreverb_initprimes(r);
	for(i=0;i<r->reverbdelaylines;i++)
	{
		r->snddlyrev[i].enabled = 1;
		//sounddelay_init(r->reverbdelaylines, DLY_REVERB, r->reverbprimes[i], r->feedback, r->format, r->rate, r->channels, &(r->snddlyrev[i]));
		sounddelay_init(r->reverbdelaylines, DLY_REVERB, r->reverbprimes[i], r->feedbacks[i], r->format, r->rate, r->channels, &(r->snddlyrev[i]));
	}

	r->eqoctave = 1.0;

	//AudioEqualizer_init(&(r->reveq), 2, r->eqoctave, 1, 1, r->format, r->rate, r->channels, d);
	AudioEqualizer_init(&(r->reveq), 1, r->eqoctave, 1, 1, r->format, r->rate, r->channels, d);
}

void soundreverb_init(int reflect, int delaylines, float feedback, float presence, eqdefaults *d, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundreverb *r)
{
	int i, ret;

	r->format = format;
	r->rate = rate;
	r->channels = channels;
	r->reflect = reflect;
	r->reverbdelaylines = delaylines;
	r->feedback = feedback;
	r->presence = presence;
	soundreverb_initprimes(r);
	for(i=0;i<r->reverbdelaylines;i++)
	{
		r->snddlyrev[i].enabled = 1;
		//sounddelay_init(r->reverbdelaylines, DLY_REVERB, r->reverbprimes[i], feedback, format, rate, channels, &(r->snddlyrev[i]));
		sounddelay_init(r->reverbdelaylines, DLY_REVERB, r->reverbprimes[i], r->feedbacks[i], format, rate, channels, &(r->snddlyrev[i]));
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
	r->bbuf = NULL;
	free(r->reverbprimes);
	r->reverbprimes = NULL;
	free(r->feedbacks);
	r->feedbacks = NULL;
	free(r->snddlyrev);	
	r->snddlyrev = NULL;
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

// Overdrive

void soundoverdrive_set(float clip, soundoverdrive *o)
{
	o->a = clip;
	o->denom = 1.0 - exp(-o->a); // 1-e^-a
	o->dnf = (float)((0x1<<(snd_pcm_format_width(o->format)-1))-1); // maxint/2
	o->nf = 1.0 / o->dnf;
}

void soundoverdrive_init(float clip, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundoverdrive *o)
{
	int ret;

	o->format = format;
	o->rate = rate;
	o->channels = channels;
	soundoverdrive_set(clip, o);

	o->initialized = 0;
	if((ret=pthread_mutex_init(&(o->odmutex), NULL))!=0 )
		printf("od mutex init failed, %d\n", ret);

	//printf("\t%f\t%f\t%f\t%f\n", o->a, o->denom, o->nf, o->dnf);
}

void soundoverdrive_add(char* inbuffer, int inbuffersize, soundoverdrive *o)
{
	pthread_mutex_lock(&(o->odmutex));
	if (o->enabled)
	{
		if (!o->initialized)
		{
			o->physicalwidth = snd_pcm_format_width(o->format); // bits per sample
			o->insamples = inbuffersize / o->physicalwidth * 8;
			o->initialized = 1;
		}
		signed short *inshort = (signed short *)inbuffer;
		int i;
		for(i=0;i<o->insamples;i++)
		{
			float x = ((float)inshort[i]) * o->nf;
			float y = (inshort[i]>=0?1.0-exp(-o->a*x):exp(o->a*x)-1.0) / o->denom;
			inshort[i] = (signed short)(y * o->dnf);
		}
	}
	pthread_mutex_unlock(&(o->odmutex));
}

void soundoverdrive_close(soundoverdrive *o)
{
	pthread_mutex_destroy(&(o->odmutex));
	o->initialized = 1;
}

// Chorus

void soundcho_initfd(soundcho *c)
{
	float chofreq[MAXCHORUS] = {0.307, 0.409, 0.509}; // modulation frequencies
	float chodepth[MAXCHORUS] = {0.02, 0.02, 0.02}; // modulation depths in percent 0 .. 1.0

	int i;
	for(i=0;i<MAXCHORUS;i++)
	{
		c->chofreq[i] = chofreq[i];
		c->chodepth[i] = chodepth[i];
	}
}

void soundcho_init(int maxcho, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundcho *c)
{
	int i, ret;

	c->format = format;
	c->rate = rate;
	c->channels = channels;
	c->maxcho = maxcho;
	soundcho_initfd(c);
	for(i=0;i<maxcho;i++)
	{
		c->v[i].enabled = 1;
		soundvfo_init(c->chofreq[i], c->chodepth[i], format, rate, channels, &(c->v[i]));
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
