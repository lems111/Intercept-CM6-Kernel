/*
 *  tslib/src/ts_read_raw.c
 *
 *  Original version:
 *  Copyright (C) 2001 Russell King.
 *
 *  Rewritten for the Linux input device API:
 *  Copyright (C) 2002 Nicolas Pitre
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_read_raw.c,v 1.9 2003/03/05 22:56:37 dlowder Exp $
 *
 * Read raw pressure, x, y, and timestamp from a touchscreen device.
 */
#include "config.h"

#include <stdio.h>

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#ifdef USE_INPUT_API
#include <linux/input.h>
#else
struct ts_event  {   /* Used in UCB1x00 style touchscreens (the default) */
	unsigned short pressure;
	unsigned short x;
	unsigned short y;
	unsigned short pad;
	struct timeval stamp;
};
struct h3600_ts_event { /* Used in the Compaq IPAQ */
	unsigned short pressure;
	unsigned short x;
	unsigned short y;
	unsigned short pad;
};
struct mk712_ts_event { /* Used in the Hitachi Webpad */
	unsigned int header;
	unsigned int x;
	unsigned int y;
	unsigned int reserved;
};
struct arctic2_ts_event { /* Used in the IBM Arctic II */
	signed short pressure;
	signed int x;
	signed int y;
	int millisecs;
	int flags;
};
struct collie_ts_event { /* Used in the Sharp Zaurus SL-5000d and SL-5500 */
	long y;
	long x;
	long pressure;
	long long millisecs;
};
struct corgi_ts_event { /* Used in the Sharp Zaurus SL-C700 */
	short pressure;
	short x;
	short y;
	short millisecs;
};
#endif /* USE_INPUT_API */

#include "tslib-private.h"

#define USE_PRESSURE_HACK	1

#ifdef USE_PRESSURE_HACK
int hack_xxx = 0;
int hack_x = 0;
int hack_y = 0;
struct timeval hack_tv;
#endif

int ts_read_raw(struct tsdev *ts, struct ts_sample *samp, int nr)
{
#ifdef USE_INPUT_API
	struct input_event ev;
#else
	struct ts_event *evt;
	struct h3600_ts_event *hevt;
	struct mk712_ts_event *mevt;
	struct arctic2_ts_event *aevt;
	struct collie_ts_event *collie_evt;
	struct corgi_ts_event *corgi_evt;
#endif /* USE_INPUT_API */
	int ret;
	int total = 0;

	char *tseventtype=NULL;
	char *defaulttseventtype="UCB1x00";

#ifdef USE_INPUT_API
	/* warning: maybe those static vars should be part of the tsdev struct? */
	static int curr_x = 0, curr_y = 0, curr_p = 0;
	static int got_curr_x = 0, got_curr_y = 0;
	int got_curr_p = 0;
	int next_x, next_y;
	int got_next_x = 0, got_next_y = 0;
	int got_tstamp = 0;

	while (total < nr) {
		ret = read(ts->fd, &ev, sizeof(struct input_event));
		if (ret < sizeof(struct input_event)) break;

		/*
		 * We must filter events here.  We need to look for
		 * a set of input events that will correspond to a
		 * complete ts event.  Also need to be aware that
		 * repeated input events are filtered out by the kernel.
		 * 
		 * We assume the normal sequence is: 
		 * ABS_X -> ABS_Y -> ABS_PRESSURE
		 * If that sequence goes backward then we got a different
		 * ts event.  If some are missing then they didn't change.
		 */
//		printf("ev.code: %x, ev.type: %x\n", ev.code, ev.type);
		if (ev.type == EV_ABS) 
		{
			switch (ev.code) 
			{
			case ABS_X:
				if (!got_curr_x && !got_curr_y) 
				{
					got_curr_x = 1;
					curr_x = ev.value;
				} else 
				{
					got_next_x = 1;
					next_x = ev.value;
				}
	//			printf("=>[%s,%d] ABS_X: %d \n", __func__, __LINE__, ev.value);
				break;
			case ABS_Y:
				if (!got_curr_y) 
				{
					got_curr_y = 1;
					curr_y = ev.value;
				} else 
				{
					got_next_y = 1;
					next_y = ev.value;
				}
	//			printf("=>[%s,%d] ABS_Y: %d \n", __func__, __LINE__, ev.value);
				break;
#if 0 // [SEC_BSP.khLEE 2009.08.12 : Change to use BTN_TOUCH instead of ABS_PRESSURE
			case ABS_PRESSURE:
				got_curr_p = 1;
				curr_p = ev.value;
	//			printf("=>[%s,%d] ABS_PRESSURE: %d \n", __func__, __LINE__, ev.value);
				break;
#endif
			}
		}
// [SEC_BSP.khLEE 2009.08.12 : Change to use BTN_TOUCH instead of ABS_PRESSURE
		else if(ev.type == EV_KEY && ev.code == BTN_TOUCH)
		{
			got_curr_p = 1;
			curr_p = ev.value;
//			printf("=>[%s,%d] BTN_TOUCH: %d \n", __func__, __LINE__, ev.value);			
		}
// ]
#if 0
		else {
			switch (ev.code) {
			case 0x14a:
			got_curr_p = 1;
			curr_p = ev.value;
			printf("=>[%s,%d] BTN_TOUCH: %d \n", __func__, __LINE__, ev.value);
			break;
			}

		}
#endif

		/* go back if we just got irrelevant events so far */
		if (!got_curr_x && !got_curr_y && !got_curr_p) continue;

		/* time stamp with the first valid event only */
		if (!got_tstamp) {
			got_tstamp = 1;
			samp->tv = ev.time;
		}

		if ( (!got_curr_x || !got_curr_y) && !got_curr_p &&
		     !got_next_x && !got_next_y ) {
			/*
			 * The current event is not complete yet.
			 * Give the kernel a chance to feed us more.
			 */
			struct timeval tv = {0, 0};
			fd_set fdset;
			FD_ZERO(&fdset);
			FD_SET(ts->fd, &fdset);
			ret = select(ts->fd+1, &fdset, NULL, NULL, &tv);
		       	if (ret == 1) continue;
			if (ret == -1) break;
		}

		/* We consider having a complete ts event */
		samp->x = curr_x;
		samp->y = curr_y;
		samp->pressure = curr_p;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
		samp++;
		total++;
        
		/* get ready for next event */
		if (got_next_x) curr_x = next_x; else got_curr_x = 0;
		if (got_next_y) curr_y = next_y; else got_curr_y = 0;
		got_next_x = got_next_y = got_tstamp = 0;
	}

	if (ret) ret = -1;
	if (total) ret = total;
#else
	tseventtype = getenv("TSLIB_TSEVENTTYPE");
	if(tseventtype==NULL) tseventtype=defaulttseventtype;

	if( strcmp(tseventtype,"H3600") == 0) { /* iPAQ style h3600 touchscreen events */
		hevt = alloca(sizeof(*hevt) * nr);
		ret = read(ts->fd, hevt, sizeof(*hevt) * nr);
		if(ret > 0) {
			int nr = ret / sizeof(*hevt);
			while(ret >= sizeof(*hevt)) {
				samp->x = hevt->x;
				samp->y = hevt->y;
				samp->pressure = hevt->pressure;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
				gettimeofday(&samp->tv,NULL);
				samp++;
				hevt++;
				ret -= sizeof(*hevt);
			}
		} else {
			return -1;
		}
	} else if( strcmp(tseventtype,"MK712") == 0) { /* Hitachi Webpad events */
		mevt = alloca(sizeof(*mevt) * nr);
		ret = read(ts->fd, mevt, sizeof(*mevt) * nr);
		if(ret > 0) {
			int nr = ret / sizeof(*mevt);
			while(ret >= sizeof(*mevt)) {
				samp->x = (short)mevt->x;
				samp->y = (short)mevt->y;
				if(mevt->header==0)
					samp->pressure=1;
				else
					samp->pressure=0;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
				gettimeofday(&samp->tv,NULL);
				samp++;
				mevt++;
				ret -= sizeof(*mevt);
			}
		} else {
			return -1;
		}

	} else if( strcmp(tseventtype,"ARCTIC2") == 0) { /* IBM Arctic II events */
		aevt = alloca(sizeof(*aevt) * nr);
		ret = read(ts->fd, aevt, sizeof(*aevt) * nr);
		if(ret > 0) {
			int nr = ret / sizeof(*aevt);
			while(ret >= sizeof(*aevt)) {
				samp->x = (short)aevt->x;
				samp->y = (short)aevt->y;
				samp->pressure = aevt->pressure;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
				gettimeofday(&samp->tv,NULL);
				samp++;
				aevt++;
				ret -= sizeof(*aevt);
			}
		} else {
			return -1;
		}

	} else if( strcmp(tseventtype,"COLLIE") == 0) { /* Sharp Zaurus SL-5000d/5500 events */
	    collie_evt = alloca(sizeof(*collie_evt) * nr);
#ifdef USE_PRESSURE_HACK
	    if (hack_xxx) {
		samp->x = hack_x;
		samp->y = hack_y;
		samp->pressure = 0;
		samp->tv.tv_usec = hack_tv.tv_usec;
		samp->tv.tv_sec = hack_tv.tv_sec;
		nr = 1;
		hack_xxx = 0;
	    } else {
#endif
		ret = read(ts->fd, collie_evt, sizeof(*collie_evt) * nr);
		if(ret > 0) {
			int nr = ret / sizeof(*collie_evt);
			while(ret >= sizeof(*collie_evt)) {
				samp->x = collie_evt->x;
				samp->y = collie_evt->y;
				samp->pressure = collie_evt->pressure;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
				samp->tv.tv_usec = collie_evt->millisecs % 1000;
				samp->tv.tv_sec = collie_evt->millisecs / 1000;
#ifdef USE_PRESSURE_HACK
				if (samp->pressure == 0) {
				    hack_xxx = 1;
				    hack_x = samp->x;
				    hack_y = samp->y;
				    hack_tv.tv_usec = samp->tv.tv_usec;
				    hack_tv.tv_sec = samp->tv.tv_sec;
				}
#endif
				samp++;
				collie_evt++;
				ret -= sizeof(*collie_evt);
			}
		} else {
			return -1;
		}
#ifdef USE_PRESSURE_HACK
	    }
#endif
	} else if( strcmp(tseventtype,"CORGI") == 0) { /* Sharp Zaurus SL-C700 events */
	    corgi_evt = alloca(sizeof(*corgi_evt) * nr);
#ifdef USE_PRESSURE_HACK
	    if (hack_xxx) {
		samp->x = hack_x;
		samp->y = hack_y;
		samp->pressure = 0;
		samp->tv.tv_usec = hack_tv.tv_usec;
		samp->tv.tv_sec = hack_tv.tv_sec;
		nr = 1;
		hack_xxx = 0;
	    } else {
#endif
		ret = read(ts->fd, corgi_evt, sizeof(*corgi_evt) * nr);
		if(ret > 0) {
			int nr = ret / sizeof(*corgi_evt);
			while(ret >= sizeof(*corgi_evt)) {
				samp->x = corgi_evt->x;
				samp->y = corgi_evt->y;
				samp->pressure = corgi_evt->pressure;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
				samp->tv.tv_usec = corgi_evt->millisecs % 1000;
				samp->tv.tv_sec = corgi_evt->millisecs / 1000;
#ifdef USE_PRESSURE_HACK
				if (samp->pressure == 0) {
				    hack_xxx = 1;
				    hack_x = samp->x;
				    hack_y = samp->y;
				    hack_tv.tv_usec = samp->tv.tv_usec;
				    hack_tv.tv_sec = samp->tv.tv_sec;
				}
#endif
				samp++;
				corgi_evt++;
				ret -= sizeof(*corgi_evt);
			}
		} else {
			return -1;
		}
#ifdef USE_PRESSURE_HACK
	    }
#endif
	} else { /* Use normal UCB1x00 type events */
		evt = alloca(sizeof(*evt) * nr);
		ret = read(ts->fd, evt, sizeof(*evt) * nr);
		if(ret > 0) {
			int nr = ret / sizeof(*evt);
			while(ret >= sizeof(*evt)) {
				samp->x = evt->x;
				samp->y = evt->y;
				samp->pressure = evt->pressure;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
				samp->tv.tv_usec = evt->stamp.tv_usec;
				samp->tv.tv_sec = evt->stamp.tv_sec;
				samp++;
				evt++;
				ret -= sizeof(*evt);
			}
		} else {
			return -1;
		}
	}
	ret = nr;
#endif /* USE_INPUT_API */

	return ret;
}

static int __ts_read_raw(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	return ts_read_raw(inf->dev, samp, nr);
}

static const struct tslib_ops __ts_raw_ops =
{
	read:	__ts_read_raw,
};

struct tslib_module_info __ts_raw =
{
	next:	NULL,
	ops:	&__ts_raw_ops,
};
