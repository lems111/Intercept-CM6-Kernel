/****************************************************************************
**
** COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2005-2009 ALL RIGHTS RESERVED
**
** AUTHOR		: KyoungHOON Kim (khoonk)
**
*****************************************************************************
** 2006/07/18. khoonk	1. create file for extern variables & functions
** 2006/07/19. khoonk	1. add touchscreen calibration convert function
*****************************************************************************/
#ifndef _DEVMGR_EXT_H_
#define _DEVMGR_EXT_H_

#include "devmgr_main.h"

#if 1 
#define gprintf(fmt, x... ) printf( "%s(%d): " fmt, __func__ ,__LINE__, ## x)
#else
#define gprintf(x...) do { } while (0)
#endif


/****************************************************************************
** file descriptors
*****************************************************************************/
extern int				fd_key;
extern int				fd_touch;
extern int 				fd_lcd;

/****************************************************************************
** variables
*****************************************************************************/
#if defined(CONFIG_LCD_COLOR_DEPTH_16)
extern unsigned short* 	fbpDevice;
extern unsigned short* 	fbpMem;
#else
extern unsigned int* 	fbpDevice;
extern unsigned int* 	fbpMem;
#endif

extern	unsigned char str_pass[];
extern	unsigned char str_fail[];


/****************************************************************************
** strings
*****************************************************************************/
/****************************************************************************
** functions
*****************************************************************************/
/* devmgr_util.c */
extern int	my_system(const char *command);
extern char* get_event_number(char* devicename);
#if 1
/* [LINUSYS] get by khoonk from DirectFB ts driver on 060719 */
extern int	x_device_manager_get_ts_value(int axis, int x_val, int y_val);
/* [LINUSYS] get by khoonk from DirectFB ts driver on 060719 */
#endif
extern bool x_device_manager_start_device_control_module(int mode);
extern bool x_device_manager_set_set_system_suspend(void);
extern bool x_device_manager_set_cpu_mode(unsigned int mode);
extern bool x_device_manager_get_wakeup_source(int *wakeup_source);
extern bool x_device_manager_power_off_system(void);

/* devmgr_lcd.c */
extern unsigned short lcd_GetPixel(int x, int y);
extern void lcd_PutPixel(int x, int y,unsigned short color );
extern void lcd_Rectangle(int x1, int y1, int x2, int y2, unsigned short color);
extern void lcd_FilledRectangle(int x1, int y1, int x2, int y2, unsigned short color);
extern void lcd_Line(int x1, int y1, int x2, int y2, unsigned short color);
extern void lcd_ClearScr(unsigned short color);
extern void lcd_draw_font(int x,int y,int fgc,int bgc,unsigned char len, unsigned char * data);
extern void lcd_draw_font2(int x,int y,int fgc,int bgc,unsigned char len, unsigned char * data);
extern void lcd_PutPixel2(int x, int y,unsigned short color );
extern void lcd_Line2(int x1, int y1, int x2, int y2, unsigned short color);


/* devmgr_test.c */
extern void devmgr_test_main(void);
extern void devmgr_set_overlay1_transparency(int trans, int xpos, int ypos, int width, int height, int bpp);

/* sysinfo.c */
extern int sys_main(void);

#endif
