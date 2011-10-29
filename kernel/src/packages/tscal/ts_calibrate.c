/*
 *  tslib/tests/ts_calibrate.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_calibrate.c,v 1.6 2002/11/08 23:28:55 dlowder Exp $
 *
 * Basic test program for touchscreen library.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
//#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>

#include "tslib.h"
#include "fbutils.h"
#include "ourhdr.h"

typedef struct {
	int x[5], xfb[5];
	int y[5], yfb[5];
	int a[7];
} calibration;

static void sig(int sig)
{
	close_framebuffer();
	fflush(stderr);
	printf("signal %d caught\n", sig);
	fflush(stdout);
	exit(1);
}

static int sort_by_x(const void* a, const void *b)
{
	return (((struct ts_sample *)a)->x - ((struct ts_sample *)b)->x);
}
static int sort_by_y(const void* a, const void *b)
{
	return (((struct ts_sample *)a)->y - ((struct ts_sample *)b)->y);
}
static int getxy(struct tsdev *ts, int *x, int *y)
{
#define MAX_SAMPLES 128
	struct ts_sample samp[MAX_SAMPLES];
	int index, middle;

	/* Read until we get a touch. */
	do {
		if (ts_read_raw(ts, &samp[0], 1) < 0) {
			perror("ts_read");
			close_framebuffer();
			exit(1);
		}
	} while (samp[0].pressure == 0);

	/* Now collect up to MAX_SAMPLES touches into the samp array. */
	index = 0;
	do {
		if (index < MAX_SAMPLES-1)
			index++;
		if (ts_read_raw(ts, &samp[index], 1) < 0) {
			perror("ts_read");
			close_framebuffer();
			exit(1);
		}
	} while (samp[index].pressure > 0);
	printf("Took %d samples...\n",index);

	/*
	 * At this point, we have samples in indices zero to (index-1)
	 * which means that we have (index) number of samples.  We want
	 * to calculate the median of the samples so that wild outliers
	 * don't skew the result.  First off, let's assume that arrays
	 * are one-based instead of zero-based.  If this were the case
	 * and index was odd, we would need sample number ((index+1)/2)
	 * of a sorted array; if index was even, we would need the
	 * average of sample number (index/2) and sample number
	 * ((index/2)+1).  To turn this into something useful for the
	 * real world, we just need to subtract one off of the sample
	 * numbers.  So for when index is odd, we need sample number
	 * (((index+1)/2)-1).  Due to integer division truncation, we
	 * can simplify this to just (index/2).  When index is even, we
	 * need the average of sample number ((index/2)-1) and sample
	 * number (index/2).  Calculate (index/2) now and we'll handle
	 * the even odd stuff after we sort.
	 */
	middle = index/2;
	if (x) {
		qsort(samp, index, sizeof(struct ts_sample), sort_by_x);
		if (index & 1)
			*x = samp[middle].x;
		else
			*x = (samp[middle-1].x + samp[middle].x) / 2;
	}
	if (x) {
		qsort(samp, index, sizeof(struct ts_sample), sort_by_y);
		if (index & 1)
			*y = samp[middle].y;
		else
			*y = (samp[middle-1].y + samp[middle].y) / 2;
	}
}

int perform_calibration(calibration *cal) {
	int j;
	float n, x, y, x2, y2, xy, z, zx, zy;
	float det, a, b, c, e, f, i;
	float scaling = 65536.0;

// Get sums for matrix
	n = x = y = x2 = y2 = xy = 0;
	for(j=0;j<5;j++) {
		n += 1.0;
		x += (float)cal->x[j];
		y += (float)cal->y[j];
		x2 += (float)(cal->x[j]*cal->x[j]);
		y2 += (float)(cal->y[j]*cal->y[j]);
		xy += (float)(cal->x[j]*cal->y[j]);
	}

// Get determinant of matrix -- check if determinant is too small
	det = n*(x2*y2 - xy*xy) + x*(xy*y - x*y2) + y*(x*xy - y*x2);
	if(det < 0.1 && det > -0.1) {
		printf("ts_calibrate: determinant is too small -- %f\n",det);
		return 0;
	}

// Get elements of inverse matrix
	a = (x2*y2 - xy*xy)/det;
	b = (xy*y - x*y2)/det;
	c = (x*xy - y*x2)/det;
	e = (n*y2 - y*y)/det;
	f = (x*y - n*xy)/det;
	i = (n*x2 - x*x)/det;

// Get sums for x calibration
	z = zx = zy = 0;
	for(j=0;j<5;j++) {
		z += (float)cal->xfb[j];
		zx += (float)(cal->xfb[j]*cal->x[j]);
		zy += (float)(cal->xfb[j]*cal->y[j]);
	}

// Now multiply out to get the calibration for framebuffer x coord
	cal->a[0] = (int)((a*z + b*zx + c*zy)*(scaling));
	cal->a[1] = (int)((b*z + e*zx + f*zy)*(scaling));
	cal->a[2] = (int)((c*z + f*zx + i*zy)*(scaling));

	printf("%f %f %f\n",(a*z + b*zx + c*zy),
				(b*z + e*zx + f*zy),
				(c*z + f*zx + i*zy));

// Get sums for y calibration
	z = zx = zy = 0;
	for(j=0;j<5;j++) {
		z += (float)cal->yfb[j];
		zx += (float)(cal->yfb[j]*cal->x[j]);
		zy += (float)(cal->yfb[j]*cal->y[j]);
	}

// Now multiply out to get the calibration for framebuffer y coord
	cal->a[3] = (int)((a*z + b*zx + c*zy)*(scaling));
	cal->a[4] = (int)((b*z + e*zx + f*zy)*(scaling));
	cal->a[5] = (int)((c*z + f*zx + i*zy)*(scaling));

	printf("%f %f %f\n",(a*z + b*zx + c*zy),
				(b*z + e*zx + f*zy),
				(c*z + f*zx + i*zy));

// If we got here, we're OK, so assign scaling to a[6] and return
	cal->a[6] = (int)scaling;
	return 1;
/*	
// This code was here originally to just insert default values
	for(j=0;j<7;j++) {
		c->a[j]=0;
	}
	c->a[1] = c->a[5] = c->a[6] = 1;
	return 1;
*/

}

int main()
{
	struct tsdev *ts;
	int fd;
	calibration cal;
	int cal_fd;
	char cal_buffer[256];
	char *tsdevice = NULL;
	char *calfile = NULL;
	int i;

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

	if( (tsdevice = getenv("TSLIB_TSDEVICE")) != NULL ) {
		ts = ts_open(tsdevice,0);
	} else {
#if 0
#ifdef USE_INPUT_API
		ts = ts_open("/dev/input/event0", 0);
#else
		ts = ts_open("/dev/touchscreen/ucb1x00", 0);
#endif /* USE_INPUT_API */
#else
	ts = ts_open("/dev/input/event2", 0);
#endif
	}

	if (!ts) {
		perror("ts_open");
		exit(1);
	}
#if 0
	if (ts_config(ts)) {
		perror("ts_config");
		exit(1);
	}
#endif

	if (open_framebuffer()) {
		close_framebuffer();
		exit(1);
	}
	close_framebuffer();
	if (open_framebuffer()) {
		close_framebuffer();
		exit(1);
	}

	setcolors(0x48ff48,0x880000);

	put_string(xres/2,yres/4,"TSLIB calibration utility",1);
	put_string(xres/2,yres/4 + 20,"Touch crosshair to calibrate",1);

	printf("xres = %d, yres = %d\n",xres,yres);

// Read a touchscreen event to clear the buffer
	//getxy(ts, 0, 0);

// Now paint a crosshair on the upper left and start taking calibration
// data
	put_cross(50,50,1);
	getxy(ts, &cal.x[0], &cal.y[0]);
	put_cross(50,50,0);

	cal.xfb[0] = 50;
	cal.yfb[0] = 50;

	printf("Top left : X = %4d Y = %4d\n", cal.x[0], cal.y[0]);

	put_cross(xres - 50, 50, 1);
	getxy(ts, &cal.x[1], &cal.y[1]);
	put_cross(xres - 50, 50, 0);

	cal.xfb[1] = xres-50;
	cal.yfb[1] = 50;

	printf("Top right: X = %4d Y = %4d\n", cal.x[1], cal.y[1]);

	put_cross(xres - 50, yres - 50, 1);
	getxy(ts, &cal.x[2], &cal.y[2]);
	put_cross(xres - 50, yres - 50, 0);

	cal.xfb[2] = xres-50;
	cal.yfb[2] = yres-50;

	printf("Bot right: X = %4d Y = %4d\n", cal.x[2], cal.y[2]);

	put_cross(50, yres - 50, 1);
	getxy(ts, &cal.x[3], &cal.y[3]);
	put_cross(50, yres - 50, 0);

	cal.xfb[3] = 50;
	cal.yfb[3] = yres-50;

	printf("Bot left : X = %4d Y = %4d\n", cal.x[3], cal.y[3]);

	put_cross(xres/2, yres/2, 1);
	getxy(ts, &cal.x[4], &cal.y[4]);
	put_cross(xres/2, yres/2, 0);

	cal.xfb[4] = xres/2;
	cal.yfb[4] = yres/2;

	printf("Middle: X = %4d Y = %4d\n", cal.x[4], cal.y[4]);

	if(perform_calibration(&cal)) {
		printf("Calibration constants: ");
		for(i=0;i<7;i++) printf("%d ",cal.a[i]);
		printf("\n");
		if( (calfile = getenv("TSLIB_CALIBFILE")) != NULL) {
			cal_fd = open(calfile,O_CREAT|O_RDWR);
		} else {
			cal_fd = open("/etc/pointercal",O_CREAT|O_RDWR);
		}
		sprintf(cal_buffer,"%d %d %d %d %d %d %d\n",cal.a[1],cal.a[2],cal.a[0],cal.a[4],cal.a[5],cal.a[3],cal.a[6]);
		write(cal_fd,cal_buffer,strlen(cal_buffer)+1);
		close(cal_fd);	
	} else {
		printf("Calibration failed.\n");
	}

//	while (1) {
//		struct ts_sample samp;
//
//		if (ts_read_raw(ts, &samp, 1) < 0) {
//			perror("ts_read");
//			exit(1);
//		}
//
//		printf("%ld.%06ld: %6d %6d %6d\n", samp.tv.tv_sec, samp.tv.tv_usec,
//			samp.x, samp.y, samp.pressure);
//	}
	close_framebuffer();
}
