/****************************************************************************
**
** COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2006-2010 ALL RIGHTS RESERVED
**
** AUTHOR		: KyoungHOON Kim (khoonk)
**
*****************************************************************************
**
**
*****************************************************************************/
#ifndef _DEVMGR_MAIN_H_
#define _DEVMGR_MAIN_H_

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
//#include <stropts.h>
//#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <poll.h>
#include <limits.h>
#include <pthread.h>

#include <time.h>
#include <linux/fb.h>
#include <sys/time.h>
#include <linux/input.h>
#include <linux/soundcard.h>

#include "message.h"

/****************************************************************************
** structures
*****************************************************************************/
typedef struct lcd_dev {
	unsigned short x1;
	unsigned short y1;
	unsigned short x2;
	unsigned short y2;	
	unsigned int   fgc;
	unsigned int   bgc;
	unsigned char data[41];
} lcd_dev_t;


typedef struct cam_dev {
	unsigned short x1;
	unsigned short y1;
	unsigned short x2;
	unsigned short y2;	
	unsigned int   fgc;
	unsigned int   bgc;
	unsigned char data[41];
} cam_dev_t;

typedef struct lcd_rec_dev {
	unsigned short x1;
	unsigned short y1;
	unsigned short x2;
	unsigned short y2;	
	unsigned int  color;
} lcd_dev_rect_t;

typedef struct _key_info {
	unsigned int index;
	unsigned short x;
	unsigned short y;
	unsigned short w;
	unsigned short h;	
	unsigned char data[11];
	unsigned char exist;
} tKEY_INFO;

typedef struct input_event input_event_t;

/**************for LCD setting************************************/
#define S3C_FB_OSD_START	_IO('F', 201)
typedef struct {
	int bpp;
	int left_x;
	int top_y;
	int width;
	int height;
} s3c_win_info_t;
#define S3C_FB_OSD_SET_INFO 	_IOW('F', 209, s3c_win_info_t)
/****************************************************************/


/****************************************************************************
** device definition
*****************************************************************************/

#define REV00 	0x00
#define REV01 	0x01
#define REV02	0x02
#define REV03	0x03
#define REV04	0x04
#define REV05	0x05
#define REV06	0x06

#define KEY_DEV		"s3c-keypad"

#ifdef CONFIG_MACH_CAPELA  //TEMP
#define CONFIG_FB_LCD_HVGA
#define TOUCH_DEV	"melfas-tsi-touchscreen"
#define NON_QWERTY_KEY
#define CAMERA_TYPE_A
#endif

#ifdef CONFIG_MACH_CYGNUS
#define CONFIG_FB_LCD_HVGA
#define TOUCH_DEV 	"S3C TouchScreen"
#define NON_QWERTY_KEY
#define CAMERA_TYPE_B
#define CONFIG_ACC_BMA020
#define OPTICAL_SENSOR
#endif

#ifdef CONFIG_MACH_BONANZA
#define CONFIG_FB_LCD_WVGA
#define TOUCH_DEV 	"S3C TouchScreen"
#define NON_QWERTY_KEY
#define CAMERA_TYPE_B
#define CONFIG_ACC_KXSD9
#endif

#ifdef CONFIG_MACH_NAOS
#define CONFIG_FB_LCD_WQVGA
#define TOUCH_DEV	"melfas-tsi-touchscreen"
#define QWERTY_KEY
#define CAMERA_TYPE_A
#define HALL_MOUSE
#if (CONFIG_NAOS_REV == REV00)
#define CONFIG_ACC_KXSD9
#else
#define TOUCH_KEY
#define CONFIG_ACC_BMA020
#define KEYPAD_LED
#define BUTTON_LED	//TOUCH_BUTTON_BACKLIGHT_LED
#endif
#endif

#ifdef CONFIG_MACH_SPICA
#if (CONFIG_SPICA_REV >= REV01)
#define CONFIG_FB_LCD_HVGA
#else
#define CONFIG_FB_LCD_WQVGA
#endif
#define TOUCH_DEV	"qt5480_ts_input"
#define NON_QWERTY_KEY
#define CAMERA_TYPE_A
#define CONFIG_ACC_BMA020
#endif

#ifdef CONFIG_MACH_ORION_6410
#define CONFIG_FB_LCD_WQVGA
#define TOUCH_DEV	"melfas-tsi-touchscreen"
#define QWERTY_KEY
#define CAMERA_TYPE_A
#define CONFIG_ACC_BMA020
#endif

#ifdef CONFIG_MACH_INSTINCTQ 
#define CONFIG_FB_LCD_HVGA
#define TOUCH_DEV	"melfas_ts_input"
#define QWERTY_KEY
#define TOUCH_KEY
#define TOUCH_FW
#define OPTICAL_JOYSTICK
#define CAMERA_TYPE_A
#define KEYPAD_LED
#define BUTTON_LED	//TOUCH_BUTTON_BACKLIGHT_LED
#define CONFIG_ACC_BMA020
#if defined (CONFIG_MACH_MAX)
#define OPTICAL_SENSOR
#else
#if (CONFIG_INSTINCTQ_REV >= REV01)
#define OPTICAL_SENSOR
#endif
#endif
#endif

#ifdef CONFIG_FB_LCD_HVGA
#define PANEL_W						(320)
#define PANEL_H						(480)
#endif

#ifdef CONFIG_FB_LCD_WQVGA
#define PANEL_W						(240)
#define PANEL_H						(400)
#endif

#ifdef CONFIG_FB_LCD_WVGA
#define PANEL_W						(480)
#define PANEL_H						(800)
#endif

#define CONFIG_LCD_COLOR_DEPTH_16

#define TITLE_WIDTH		PANEL_W-2
#define TITLE_HEIGHT	26
#define LINE_H			16

#define LCD_SIZE					(PANEL_W*PANEL_H)

#define COLOR_RED                   0xf800
#define COLOR_GREEN                 0x03e0
#define COLOR_BLUE                  0x001f
#define COLOR_WHITE                 0xffff
#define COLOR_BLACK                 0x0000
#define COLOR_GRAY                  0x8410
#define COLOR_LIGHTGRAY             0xDE39

#define PRESS		    	  1
#define RELEASE				  0

#define MAX_DEVICE_NUM	       2	
#define TOTAL_DISP_LEN		  70


/****************************************************************************
** ioctl 
*****************************************************************************/
enum {
	PM_SUSPEND_ON = 0,
	PM_SUSPEND_STANDBY,
	PM_SUSPEND_MEM = 3,
	PM_SUSPEND_DISK,
	PM_SUSPEND_LCDREFRESH,
	PM_SUSPEND_DEEPSLEEP,
	PM_SUSPEND_MAX,
};
#define ACCS_IOCTL_OPEN		101
#define ACCS_IOCTL_CLOSE	102
#define BMA020_MODE_NORMAL	0
#define BMA150_IOC_MAGIC                'B'
#define BMA150_SET_MODE                 _IOWR(BMA150_IOC_MAGIC,6, unsigned char)
#define BMA150_READ_ACCEL_XYZ           _IOWR(BMA150_IOC_MAGIC,46,short)

#define AKMIO               0xA1 
#define EMCS_IOCTL_OPEN 	201
#define EMCS_IOCTL_CLOSE	202
#define ECS_IOCTL_DEVMGR_GET_DATA 203
#define AKECS_MODE_MEASURE  0x00    /* Starts measurement. Please use AKECS_MODE_MEASURE_SNG */
#define ECS_IOCTL_SET_MODE          _IOW(AKMIO, 0x07, short)
#define ECS_IOCTL_APP_SET_MVFLAG	_IOW(AKMIO, 0x19, short)
#define ECS_IOCTL_APP_SET_TFLAG		_IOR(AKMIO, 0x15, short)
#define ECS_IOCTL_APP_SET_PFLAG     _IOW(AKMIO, 0x1B, short)

#define VIDIOC_S_SENSOR             _IOW  ('V', 84, void *)
#define V4L2_CID_ORIGINAL		(V4L2_CID_PRIVATE_BASE + 0)
/* IOCTL for proximity sensor */                                    
#define SHARP_GP2AP_IOC_MAGIC   'C'                                 
#define SHARP_GP2AP_OPEN    _IO(SHARP_GP2AP_IOC_MAGIC,1)            
#define SHARP_GP2AP_CLOSE   _IO(SHARP_GP2AP_IOC_MAGIC,2)  

#endif

