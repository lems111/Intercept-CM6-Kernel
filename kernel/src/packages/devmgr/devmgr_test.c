/****************************************************************************
**
** COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2005-2009 ALL RIGHTS RESERVED
**
** AUTHOR		: KyoungHOON Kim (khoonk)
**
*****************************************************************************
** 2006/07/18. khoonk	1.add self test program
** 2007/07/27. khoonk	1.add illuminate sensor code
*****************************************************************************/
#include <linux/apm_bios.h>
#include "devmgr_ext.h"

#include <linux/fb.h>
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "iniparser.h"
#include "RomanceTone.h"

/****************************************************************************
** defines  
*****************************************************************************/
#define SYSINFO_DB 			"/.info/system.info"
#if 0	
#define IOC_MZ_MAGIC				('h')
#define HN_DPRAM_PHONE_ON			_IO(IOC_MZ_MAGIC, 0xd0)
#define HN_DPRAM_PHONE_OFF			_IO(IOC_MZ_MAGIC, 0xd1)
#define HN_DPRAM_PHONE_GETSTATUS	_IOR(IOC_MZ_MAGIC, 0xd2, unsigned int)
#endif

#define IOC_SEC_MAGIC            		(0xf0)
#define DPRAM_PHONE_GETSTATUS			_IOR(IOC_SEC_MAGIC, 0xc1, unsigned int)

#define USB_UART_SWITCH    0
#define PDA_SWITCH    	   2
#define PHONE_SWITCH   	   4

#define UART_TEST_NAME  "UART TEST"
#define USB_TEST_NAME   "USB TEST"

#define LCD_OFF 		 	1
#define LCD_ON				255	

#define DEV_CAM_NAME  "/dev/video0"
#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define VIDIOC_S_ROTATE_ANGLE	_IOW('V', 86, void *)

#define SND_SRC_SIZE 		806448

#define IOC_SMB_GET_ACC_VALUE 0

/****************************************************************************
** global variables
*****************************************************************************/
#define USE_INVERTED_FONTS
#ifdef USE_INVERTED_FONTS
static int devmgr_current_item = 0;
#endif
static bool b_sensor_terminated=false;
static pthread_t t_acc_sensors;
static pthread_t t_mgn_sensors_cmd;
static pthread_t t_mgn_sensors_poll;
static pthread_t t_opt_sensors;
static pthread_t t_camera_preview;
static int bl_level = 0;
static int mgn_temperature = 0;
static int back_key_click = 0;
static int backlight_level = 255;
typedef enum _cam_func_mode {
	normal,
	back,
} cam_func_mode;

static int keyname[255];

	
typedef enum _devmgr_test_state {
	test_main = -1,
	test_key = 0,
	test_touch,
	test_lcd,
	test_lcd_backlight,
	test_lcd_power,
	test_led,
	test_camera,
	test_vibrator,
	test_sound,
	test_sensor,
	test_uart,
	test_usb,
	test_dpram,
	test_wifi,
	test_bt,
	test_sdcard,
	test_battery,
	test_version,
	test_reset,
#ifdef TOUCH_FW
	test_touch_fw,
#endif
	test_end,
} devmgr_test_state;

devmgr_test_state test_state = test_main;

#define MAX_LCD_STR	(sizeof(str_lcd)/sizeof(lcd_dev_t))
lcd_dev_t str_lcd[] =	/* str_lcd table */
{
	{ 10, 30, 0, 0, COLOR_BLACK, COLOR_WHITE, "1. KEYPAD.............<    >"},
	{ 10, 46, 0, 0, COLOR_BLACK, COLOR_WHITE, "2. TOUCH SCREEN.......<    >"},
	{ 10, 62, 0, 0, COLOR_BLACK, COLOR_WHITE, "3. LCD COLOR..........<    >"},
	{ 10, 78, 0, 0, COLOR_BLACK, COLOR_WHITE, "4. LCD BACKLIGHT......<    >"},
	{ 10, 94, 0, 0, COLOR_BLACK, COLOR_WHITE, "5. LCD POWER..........<    >"},
	{ 10,110, 0, 0, COLOR_BLACK, COLOR_WHITE, "6. LED ON/OFF.........<    >"},
	{ 10,126, 0, 0, COLOR_BLACK, COLOR_WHITE, "7. CAMERA CAPTURE.....<    >"},
	{ 10,142, 0, 0, COLOR_BLACK, COLOR_WHITE, "8. VIBRATOR ON/OFF....<    >"},
	{ 10,158, 0, 0, COLOR_BLACK, COLOR_WHITE, "9. SOUND PLAY.........<    >"},
	{ 10,174, 0, 0, COLOR_BLACK, COLOR_WHITE, "10.SENSORS............<    >"},
	{ 10,190, 0, 0, COLOR_BLACK, COLOR_WHITE, "11.UART...............<    >"},
	{ 10,206, 0, 0, COLOR_BLACK, COLOR_WHITE, "12.USB................<    >"},
	{ 10,222, 0, 0, COLOR_BLACK, COLOR_WHITE, "13.DPRAM TEST.........<    >"},
	{ 10,238, 0, 0, COLOR_BLACK, COLOR_WHITE, "14.WIFI TEST..........<    >"},
	{ 10,254, 0, 0, COLOR_BLACK, COLOR_WHITE, "15.BLUETOOTH TEST.....<    >"},
	{ 10,270, 0, 0, COLOR_BLACK, COLOR_WHITE, "16.SDCARD TEST........<    >"},
	{ 10,286, 0, 0, COLOR_BLACK, COLOR_WHITE, "17.BATTERY INFORMATION      "},
	{ 10,302, 0, 0, COLOR_BLACK, COLOR_WHITE, "18.SHOW VERSION INFORMATION "},
	{ 10,318, 0, 0, COLOR_BLACK, COLOR_WHITE, "19.RESET                    "},
#ifdef TOUCH_FW
	{ 10,334, 0, 0, COLOR_BLACK, COLOR_WHITE, "20.TOUCH FW UPDATE          "},
	{ 10,350, 0, 0, COLOR_BLACK, COLOR_WHITE, "21.SAVE & EXIT PROGRAM      "},
#else
	{ 10,334, 0, 0, COLOR_BLACK, COLOR_WHITE, "20.SAVE & EXIT PROGRAM      "},
#endif
};

typedef enum _switch_sel_state {
	MODEM = 0 ,
	PDA   = 1 ,
	USB,
	UART,
} switch_sel_state;

static int uart_current_switch_mode = -1;
static int usb_current_switch_mode = -1;

#define KEY_NONE  0x00
#define KEY_EXIST 0x01

#define MAX_KEY_NUM					(sizeof(key_info)/sizeof(tKEY_INFO))
#ifdef QWERTY_KEY
tKEY_INFO key_info[] = 
{
	{DM_KEY_SEARCH			,    95,     35,    20,     20,     "SEARCH    ",	KEY_NONE},
	{DM_KEY_1				,    95,     60,    20,     20,     "1	       ",	KEY_NONE},
	{DM_KEY_2				,    95,     85,    20,     20,     "2	       ",	KEY_NONE},
	{DM_KEY_3				,    95,    110,    20,     20,     "3	       ",	KEY_NONE},
	{DM_KEY_4				,    95,    135,    20,     20,     "4	       ",	KEY_NONE},
	{DM_KEY_5				,    95,    160,    20,     20,     "5	       ",	KEY_NONE},
	{DM_KEY_6				,    95,    185,    20,     20,     "6	       ",	KEY_NONE},
	{DM_KEY_7				,    95,    210,    20,     20,     "7	       ",	KEY_NONE},
	{DM_KEY_8				,    95,    235,    20,     20,     "8	       ",	KEY_NONE},
	{DM_KEY_9				,    95,    260,    20,     20,     "9	       ",	KEY_NONE},
	{DM_KEY_0				,    95,    285,    20,     20,     "0	       ",	KEY_NONE},
	{DM_KEY_DEL            	,    95,    310,    20,     20,     "BACK SPACE",	KEY_NONE},
	
	{DM_KEY_SHIFT_LEFT     	,    70,     35,    20,     20,     "SHIFT     ",	KEY_NONE},
	{DM_KEY_Q				,    70,     60,    20,     20,     "Q         ",	KEY_NONE},
	{DM_KEY_W	 	        ,    70,     85,    20,     20,     "W         ",	KEY_NONE},
	{DM_KEY_E     	        ,    70,    110,    20,     20,     "E         ",	KEY_NONE},
	{DM_KEY_R     	        ,    70,    135,    20,     20,     "R         ",	KEY_NONE},
	{DM_KEY_T     	        ,    70,    160,    20,     20,     "T         ",	KEY_NONE},
	{DM_KEY_Y              	,    70,    185,    20,     20,     "Y         ",	KEY_NONE},
	{DM_KEY_U              	,    70,    210,    20,     20,     "U         ",	KEY_NONE},
	{DM_KEY_I              	,    70,    235,    20,     20,     "I         ",	KEY_NONE},
	{DM_KEY_O              	,    70,    260,    20,     20,     "O         ",	KEY_NONE},
	{DM_KEY_P              	,    70,    285,    20,     20,     "P         ",	KEY_NONE},
	{DM_KEY_DPAD_RIGHT     	,    70,    310,    20,     20,     "RIGHT     ",	KEY_NONE},
	{DM_KEY_ENTER          	,    70,    335,    20,     20,     "ENTER     ",	KEY_NONE},
	
	{DM_KEY_ALT_LEFT       	,    45,     35,    20,     20,     "FUNCTION  ",	KEY_NONE},
	{DM_KEY_A              	,    45,     60,    20,     20,     "A         ",	KEY_NONE},
	{DM_KEY_S              	,    45,     85,    20,     20,     "S         ",	KEY_NONE},
	{DM_KEY_D              	,    45,    110,    20,     20,     "D         ",	KEY_NONE},
	{DM_KEY_F              	,    45,    135,    20,     20,     "F         ",	KEY_NONE},
	{DM_KEY_G              	,    45,    160,    20,     20,     "G         ",	KEY_NONE},
	{DM_KEY_H              	,    45,    185,    20,     20,     "H         ",	KEY_NONE},
	{DM_KEY_J              	,    45,    210,    20,     20,     "J         ",	KEY_NONE},
	{DM_KEY_K              	,    45,    235,    20,     20,     "K         ",	KEY_NONE},
	{DM_KEY_L              	,    45,    260,    20,     20,     "L         ",	KEY_NONE},
	{DM_KEY_DPAD_UP        	,    45,    285,    20,     20,     "UP        ",	KEY_NONE},
	{DM_KEY_DPAD_DOWN	    ,    45,    310,    20,     20,     "DOWN      ",	KEY_NONE},
	
	{DM_KEY_Z              	,    20,     35,    20,     20,     "Z         ",	KEY_NONE},
	{DM_KEY_X              	,    20,     60,    20,     20,     "X         ",	KEY_NONE},
	{DM_KEY_C              	,    20,     85,    20,     20,     "C         ",	KEY_NONE},
	{DM_KEY_V              	,    20,    110,    20,     20,     "V         ",	KEY_NONE},
	{DM_KEY_SPACE          	,    20,    135,    20,     45,     "SPACE     ",	KEY_NONE},
	{DM_KEY_B              	,    20,    185,    20,     20,     "B         ",	KEY_NONE},
	{DM_KEY_N              	,    20,    210,    20,     20,     "N         ",	KEY_NONE},
	{DM_KEY_M              	,    20,    235,    20,     20,     "M         ",	KEY_NONE},
	{DM_KEY_PERIOD	        ,    20,    260,    20,     20,     "DOT       ",	KEY_NONE},
	{DM_KEY_DPAD_LEFT      	,    20,    285,    20,     20,     "LEFT      ",	KEY_NONE},
	
	{DM_KEY_VOLUME_UP      	,   130,    260,    20,     20,     "VOL UP    ",	KEY_NONE},
	{DM_KEY_VOLUME_DOWN    	,   130,    285,    20,     20,     "VOL DOWN  ",	KEY_NONE},
	
	{DM_KEY_HOME           	,   130,    325,    30,     20,     "HOME      ",	KEY_NONE},
	{DM_KEY_MENU           	,   165,    325,    30,     20,     "MENU      ",	KEY_NONE},
	{DM_KEY_BACK           	,   200,    325,    30,     20,     "BACK      ",	KEY_NONE},
	
	{DM_KEY_CALL           	,   130,    350,    30,     20,     "SEND      ",	KEY_NONE},
	{DM_KEY_ENDCALL        	,   165,    350,    30,     20,     "END       ",	KEY_NONE},
	{DM_KEY_CAMERA		    ,   200,    350,    30,     20,     "CAM FULL  ",	KEY_NONE},
	{DM_KEY_FOCUS	        ,   205,    355,    20,     10,     "CAM HALF  ",	KEY_NONE},

	{DM_KEY_DPAD_CENTER    	,   165,    300,    30,     20,     "OK        ",	KEY_NONE},
	{DM_KEY_HOLD			,	200, 	300,	30,		20,		"HOLD	   ", 	KEY_NONE},

	{DM_KEY_ENVELOPE       	,    95,    335,    20,     20,     "MESSAGE   ",	KEY_NONE},
	{DM_KEY_COMMA		   	,	 20,	310,    20,	    20,	   	"COMMA     ",	KEY_NONE},
	//{DM_KEY_SYM            	,    20,    335,    20,     20,     "SYMBOL    ",	KEY_NONE},

};
#endif

#ifdef NON_QWERTY_KEY
tKEY_INFO key_info[] = 
{
	{DM_KEY_CAMERA			,   210,    280,    10,     25,     "CAMERA    ",	KEY_NONE},
	{DM_KEY_FOCUS			,   210,    285,    10,     15,     "FOCUS     ",	KEY_NONE},
	{DM_KEY_DPAD_DOWN		,   105,    340,    25,     25,     "DOWN      ",	KEY_NONE},
	{DM_KEY_CALL		    ,    30,    350,    50,     30,     "SEND      ",	KEY_NONE},
	{DM_KEY_DPAD_LEFT 	    ,    75,    310,    25,     25,     "LEFT      ",	KEY_NONE},
	{DM_KEY_ENDCALL	    	,   155,    350,    50,     30,     "END       ",	KEY_NONE},
	{DM_KEY_DPAD_RIGHT     	,   135,    310,    25,     25,     "RIGHT     ",	KEY_NONE},
	{DM_KEY_VOLUME_DOWN    	,    15,    280,    10,     25,     "VOL DOWN  ",	KEY_NONE},
	
	{DM_KEY_MENU		    ,    30,    250,    50,     30,     "MENU      ",	KEY_NONE},
	{DM_KEY_DPAD_UP        	,   105,    280,    25,     25,     "UP        ",	KEY_NONE},
	{DM_KEY_DPAD_CENTER    	,   105,    310,    25,     25,     "OK        ",	KEY_NONE},
	{DM_KEY_HOME		    ,    95,    250,    45,     25,     "HOME      ",	KEY_NONE},
	{DM_KEY_VOLUME_UP      	,    15,    250,    10,     25,     "VOL UP    ",	KEY_NONE},
	{DM_KEY_BACK		    ,   155,    250,    50,     30,     "BACK      ",	KEY_NONE},
	{DM_KEY_HOLD			,	210,	250,	10,		25,		"HOLD	   ",	KEY_NONE},
	{DM_KEY_SEARCH			,	 15,	310,	10,		25,		"SEARCH	   ",	KEY_NONE},
};
#endif

/************ for Camera Test ******************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/page.h> /* for PAGE_ALIGN */
struct buffer {
	void *start;
	size_t length;
};

static int cam_fd = -1;
struct buffer *cam_buffers = NULL;
static unsigned int cam_n_buffers = 0;
unsigned short *fb;
int fd_fb = -1;
static int finish = 0;
static pthread_t pth;

// s.lsi
#include "s3c_mem.h"
typedef struct _s3c_mem_t{
	int             dev_fd;   
	unsigned char * virt_addr;
	unsigned int    phys_addr;
	unsigned int    buf_size;
	struct s3c_mem_alloc mem_alloc_info;
}s3c_mem_t;

#define NUM_OF_MEMORY_OBJECT 4
s3c_mem_t           g_s3c_mem[NUM_OF_MEMORY_OBJECT];
/************************************/

/****************************************************************************
** functions
*****************************************************************************/

#undef __ASM_ARM_TYPES_H
#undef __ASSEMBLY_
#undef _I386_TYPES_H

void devmgr_set_overlay1_transparency(int trans, int xpos, int ypos, int width, int height, int bpp);

static int xioctl(int fd, int request, void * arg)
{
        int r;
        do r = ioctl (fd, request, arg);
        while (-1 == r && EINTR == errno);
        return r;
}
static void errno_exit(const char *s)
{
	fprintf (stderr, "%s error %d, %s\n", s, errno, strerror (errno));
	exit (EXIT_FAILURE);
}

																						  


int get_switch_sel(switch_sel_state mode)
{
	int param_fd,nwr,param_val;
	char value[20];
	
	param_fd = open("sys/class/sec/param/switch_sel",O_RDWR);
	if(param_fd <0)
	{
		perror("param_fd open error\n");
	}

	nwr = read(param_fd,value,2);
	param_val = atoi(value);
	close(param_fd);

	if(mode==USB)
		param_val &= 0x01;
	else if (mode==UART)
		param_val &= 0x02;
	else
	{
		perror("invalid mode error\n");
		
		return -1;
	}

	return param_val;
}
void set_switch_sel_for_usb(switch_sel_state mode)
{

	/* 0 value = modem , 1 value = pda   */
	
	int usb_fd;
	char value[20];
	
	usb_fd = open("/sys/class/sec/switch/usb_sel", O_RDWR);
	if(usb_fd < 0)
	{
		perror("usb_fd open error\n");
	}

	switch(mode)
	{
		case MODEM : 
			strcpy(value, "MODEM");
			write(usb_fd,value,6);
			usb_current_switch_mode = MODEM;
			break;
		case PDA   :
			strcpy(value,"PDA");
			write(usb_fd,value,4);
			usb_current_switch_mode = PDA;
			break;
		default:
			break;
	}

	close(usb_fd);

	return;
}

void set_switch_sel_for_uart(switch_sel_state mode)
{

	/* 0 value = modem , 1 value = pda   */
	
	int uart_fd;
	char value[20];
	int nwr;
	
	uart_fd = open("/sys/class/sec/switch/uart_sel",O_RDWR);
	if(uart_fd <0)
	{
		perror("uart_fd open error\n");
	}

	switch(mode)
	{
		case MODEM :
			nwr = sprintf(value,"%d\n",mode);
			write(uart_fd,value,nwr);
			uart_current_switch_mode = MODEM;
			break;
		case PDA   :
			nwr = sprintf(value,"%d\n",mode);
			write(uart_fd,value,nwr);
			uart_current_switch_mode = PDA;
			break;
		
		default:
			break;
	}
	
	close(uart_fd);
}


void devmgr_test_ShowDummyWindow(char * str)
{
	/* clear window */
	lcd_FilledRectangle(0, 0, PANEL_W-2, PANEL_H-2, COLOR_WHITE);
	
	/* draw title bar */
	lcd_FilledRectangle( 0, 0, TITLE_WIDTH, TITLE_HEIGHT, COLOR_BLUE );
	lcd_draw_font(76, 10, COLOR_WHITE, COLOR_BLUE, strlen(str), (unsigned char*)str);
}
void devmgr_test_DrawBatteryIndicator(unsigned int level)
{
	/* clear battery box */
	lcd_FilledRectangle(215,0,239,19,COLOR_BLUE);

	/* draw battery box */
	lcd_Rectangle(218,3,237,16,COLOR_WHITE);
	lcd_Rectangle(219,4,236,15,COLOR_WHITE);
	lcd_FilledRectangle(215,7,217,12,COLOR_WHITE);
	lcd_FilledRectangle(236,9,237,10,COLOR_BLUE);

	switch(level) 
	{
		case 3:
			lcd_FilledRectangle(221,6,224,13,COLOR_RED);
			lcd_FilledRectangle(226,6,229,13,COLOR_RED);
			lcd_FilledRectangle(231,6,234,13,COLOR_RED);
			break;
		case 2:
			lcd_FilledRectangle(226,6,229,13,COLOR_RED);
		case 1:
			lcd_FilledRectangle(231,6,234,13,COLOR_RED);
		default:
			break;
	}
}

void *t_acc_get_value_kxsd9(void *arg)
{
	int fd;
	input_event_t buf;
	struct pollfd poll_events;
	int poll_state;
	int x = 0, y = 0, z = 0;
	char acc_str[100];
	int aot_fd;
	char evnum[30];

	aot_fd = open ("/dev/kxsd9_aot", O_RDWR);
	if(aot_fd < 0)
	{
		perror("acc-sensor open error\n");
		lcd_draw_font(10,72,COLOR_BLACK,COLOR_WHITE,17,(unsigned char*)"  Not Available  ");
	}
	else	//open success
	{	
		sprintf(evnum,"/dev/input/%s",get_event_number("kxsd9"));
		fd = open(evnum, O_RDWR);
		if(fd < 0)
		{
			perror("Accelerometer-sensor open error\n");
			lcd_draw_font(10,72,COLOR_BLACK,COLOR_WHITE,17,(unsigned char*)"  Not Available  ");
		}
		else
		{
			if(ioctl(aot_fd, ACCS_IOCTL_OPEN, NULL)<0)
			{
				perror("ioctl open error");
			}
			poll_events.fd 		= fd;
			poll_events.events	= POLLIN;
			poll_events.revents = 0;

			while(!b_sensor_terminated)
			{
				poll_state = poll( (struct pollfd*)&poll_events, 1, -1 );
				if( poll_state > 0 )
				{
					if( poll_events.revents & POLLIN )
					{
						read(fd, &buf, sizeof(input_event_t));
						if(buf.type == EV_ABS){
							switch(buf.code)
							{
								case ABS_X :
									x = buf.value;
									break;
								case ABS_Y :
									y = buf.value;
									break;
								case ABS_Z :
									z = buf.value;
									break;
								default :
									break;
							}//switch
						}//if
						sprintf( acc_str, " x=%d y=%d z=%d ", x, y, z );
						lcd_draw_font(10,72,COLOR_BLACK,COLOR_WHITE,25,(unsigned char* )acc_str);
					}//if
				}//if
				else if(poll_state < 0)
				{
					printf("poll error!!\n");
				}
				else
				{
					//printf("poll time_out!!\n");
				}
			}//end while
			close(fd);
			if(ioctl(aot_fd, ACCS_IOCTL_CLOSE, NULL)<0)
			{
				perror("ioctl close error");
			}
		}//end else
		close(aot_fd);
	}//end else
	pthread_exit((void*)0);
	return NULL;
}//end funtion
		
void *t_acc_get_value_bma020(void *arg)
{
	typedef struct{
		short 	x,
				y,
				z;
	} bma020acc_t;
	
	int fd;
	char acc_str[100];
	bma020acc_t data;
	unsigned char mode = BMA020_MODE_NORMAL;

	fd = open("/dev/bma150", O_RDWR);
	if(fd < 0)
	{
		perror("acc open error");
		lcd_draw_font(10,72,COLOR_BLACK,COLOR_WHITE,17,(unsigned char*)"  Not Available  ");
		goto __exit__;
	}
	if( ioctl( fd, BMA150_SET_MODE, &mode ) < 0 )
	{
		perror( "set_mode ioctl error" );
		lcd_draw_font(10,72,COLOR_BLACK,COLOR_WHITE,17,(unsigned char*)"  Not Available  ");
		goto __exit__;
	}
	while(!b_sensor_terminated)
	{
		usleep(500000);
		if( ioctl( fd, BMA150_READ_ACCEL_XYZ, &data ) < 0 )
		{
			perror( "read xyz ioctl error" );
			lcd_draw_font(10,72,COLOR_BLACK,COLOR_WHITE,17,(unsigned char*)"  Not Available  ");
			goto __exit__;
		}
		sprintf( acc_str, " x=%d y=%d z=%d ", data.x, data.y, data.z );
		lcd_draw_font(10,72,COLOR_BLACK,COLOR_WHITE,25,(unsigned char* )acc_str);
		
	}
	close(fd);	
__exit__:	
	pthread_exit((void*)0);
	return NULL;
}//end funtion

void *t_mgn_cmd_thread(void *arg)
{
	input_event_t buf;
	int aot_fd;
	int akd_fd;
	short flag = 1;
	short mode = AKECS_MODE_MEASURE;

	
	aot_fd = open("dev/akm8973_aot", O_RDWR);
	if(aot_fd < 0)
	{
		perror("magnetic-sensor open failed\n");
		lcd_draw_font(10,279,COLOR_BLACK,COLOR_WHITE,17,(unsigned char*)"  Not Available  ");
	}
	akd_fd = open("dev/akm8973_daemon", O_RDWR);
	if(akd_fd < 0)
	{
		lcd_draw_font(10,279,COLOR_BLACK,COLOR_WHITE,17,(unsigned char*)"  Not Available  ");
		printf("ak daemon open failed\n");
	}
	if((aot_fd >= 0)&&(akd_fd >= 0))
	{
		if(ioctl(aot_fd, ECS_IOCTL_APP_SET_MVFLAG, &flag)<0)
		{
			printf("mvflag ioctl open error\n");
		}
		if(ioctl(aot_fd, ECS_IOCTL_APP_SET_TFLAG, &flag)<0)
		{
			printf("tflag ioctl open error\n");
		}
#ifdef OPTICAL_SENSOR	
		if(ioctl(aot_fd, ECS_IOCTL_APP_SET_PFLAG, &flag)<0)
		{
			printf("pflag ioctl open error\n");
		}

		lcd_draw_font(10,153,COLOR_BLACK,COLOR_WHITE,6,(unsigned char* )"Prx=0 ");
#endif	
		while(!b_sensor_terminated)
		{
			if(ioctl(akd_fd, ECS_IOCTL_SET_MODE, &mode)<0)
			{
				printf("set mode ioctl open error\n");
			}
			usleep(1000);

			if(ioctl(aot_fd, ECS_IOCTL_DEVMGR_GET_DATA, NULL)<0)
			{
				printf("get data ioctl open error\n");
			}
			usleep(100000);
		}//end while
		close(akd_fd);
		close(aot_fd);
	}//end else
	pthread_exit((void*)0);
	return NULL;
}//end funtion

void *t_mgn_poll_thread(void *arg)
{
	int fd;
	input_event_t buf;
	struct pollfd poll_events;
	int poll_state;
	int x = 0, y = 0, z = 0;
	char mgn_str_xyz[100];
	char mgn_str_temp[15];
	char evnum[30];

#ifdef OPTICAL_SENSOR
	int d = 0;
	char prx_str_temp[15];
#endif

	sprintf(evnum,"/dev/input/%s",get_event_number("compass"));
	fd = open(evnum, O_RDWR);
	if(fd < 0)
	{
		perror("Magnetic-sensor open error\n");
		lcd_draw_font(10,279,COLOR_BLACK,COLOR_WHITE,17,(unsigned char*)"  Not Available  ");
	}
	else	//open success
	{
		poll_events.fd		= fd;
		poll_events.events	= POLLIN;
		poll_events.revents = 0;

		while(!b_sensor_terminated)
		{
			poll_state = poll( (struct pollfd*)&poll_events, 1, 1 );
			if( poll_state > 0 )
			{
				if( poll_events.revents & POLLIN )
				{
					read(fd, &buf, sizeof(input_event_t));
					if(buf.type == EV_ABS){
						switch(buf.code)
						{
							case ABS_THROTTLE :
								mgn_temperature = (int)buf.value;
								break;
							case ABS_HAT0X :
								x = (int)buf.value;
								break;

							case ABS_HAT0Y :
								y = (int)buf.value;
								break;

							case ABS_BRAKE :
								z = (int)buf.value;
								break;
#ifdef OPTICAL_SENSOR
							case ABS_DISTANCE :
								d = (int)buf.value;
								break;
#endif
							default : 
								break;
						}//end switch
					}
					sprintf( mgn_str_temp," temperature=%d ", mgn_temperature );
					sprintf( mgn_str_xyz, " x=%d y=%d z=%d ", x, y, z );
					lcd_draw_font(10,264,COLOR_BLACK,COLOR_WHITE,16,(unsigned char* )mgn_str_temp);
					lcd_draw_font(10,294,COLOR_BLACK,COLOR_WHITE,25,(unsigned char* )mgn_str_xyz);
#ifdef OPTICAL_SENSOR
					sprintf( prx_str_temp,"Prx=%d ", d);
					lcd_draw_font(10,153,COLOR_BLACK,COLOR_WHITE,6,(unsigned char* )prx_str_temp);
#endif
				}//end if
			}//end if
			else if(poll_state < 0)
			{
				printf("poll error!!\n");
			}
			else
			{
				printf("mgn poll time_out! \n");
			}
		}//end while
		close(fd);
	}//end else
	pthread_exit((void*)0);
	return NULL;
}//end funtion

void *t_opt_get_state_value(void* arg)
{

	int light_fd,light_state_fd, nwr,val;
	char value[20];
	char light_state_value[20],light_string[20];
	int prox_fd;
	
	/* light sensor On */

	light_fd = open ("/sys/class/lightsensor/switch_cmd/lightsensor_file_cmd",O_RDWR);
	if(light_fd < 0)
	{
		perror("light-sensor open error\n");
		lcd_draw_font(50,153,COLOR_BLACK,COLOR_WHITE,19,(unsigned char*)" ,light sensor off ");
	}
	else	//open success
	{
		usleep(100000);
		val = 1;

		nwr = sprintf(value, "%d\n",val );
		write(light_fd, value, nwr);

	}

	/* proximity sensor On */
	prox_fd = open ("/dev/proximity", O_RDWR);
	if(prox_fd < 0)
	{
		perror("proximity sensor open error\n");
		lcd_draw_font(10,153,COLOR_BLACK,COLOR_WHITE,17,(unsigned char*)"  Not Available  ");
	}
	else
	{
		if(ioctl(prox_fd, SHARP_GP2AP_OPEN, 0) < 0)
		{
			printf("proximity ioctl error\n");
		}
	}
	
	
	/* light sensor state file */
	light_state_fd = open("/sys/class/lightsensor/switch_cmd/lightsensor_file_state",O_RDWR);
	if(light_state_fd <0)
	{
		perror("light_sensor_state open error\n");
		lcd_draw_font(50,153,COLOR_BLACK,COLOR_WHITE,18,(unsigned char *)" ,light sensor on ");

	}

	else
	{
		while(!b_sensor_terminated)
		{
			nwr = read(light_state_fd,light_state_value,2);
			val = atoi(light_state_value);
			sprintf(light_string," ,light = %d ",val);
			lcd_draw_font(50,153,COLOR_BLACK,COLOR_WHITE,18,(unsigned char *)light_string);
			lseek(light_state_fd,0,SEEK_SET);
				

		}//end while
	}

	close(light_state_fd);
	/* light sensor off */
	if(light_fd>=0)
	{
		val = 0;
		nwr = sprintf(value, "%d\n",val );
		write(light_fd, value, nwr);
		close(light_fd);
	}
	close(light_fd);

	/* proximity sensor off */

	if(prox_fd>=0)
	{
		if(ioctl(prox_fd, SHARP_GP2AP_CLOSE, 0) < 0)
		{
			printf("proximity ioctl error\n");
		}
	}
	close(prox_fd);

	pthread_exit((void*)0);
	return NULL;

}
void devmgr_test_sensor(void)
{
	int result;

	b_sensor_terminated = false;
#if defined(CONFIG_ACC_KXSD9)
	result = pthread_create( &t_acc_sensors, NULL, t_acc_get_value_kxsd9, (void *)0 );
	if(result != 0)
	{
		printf( "sensors thread create fail\n" );
		return;
	}
#elif defined(CONFIG_ACC_BMA020)
	result = pthread_create( &t_acc_sensors, NULL, t_acc_get_value_bma020, (void *)0 );
	if(result != 0)
	{
		printf( "sensors thread create fail\n" );
		return;
	}
#else
	lcd_draw_font(10,72,COLOR_BLACK,COLOR_WHITE,17,(unsigned char*)"  Not Available  ");	
#endif

#ifdef OPTICAL_SENSOR
	result = pthread_create( &t_opt_sensors, NULL, t_opt_get_state_value, (void *)0 );
	if(result != 0)
	{
		printf( "sensors thread create fail\n" );
		return;
	}
#endif

	result = pthread_create( &t_mgn_sensors_cmd, NULL, t_mgn_cmd_thread, (void *)0 );
	if(result != 0)
	{
		printf( "sensors thread create fail\n" );
		return;
	}
	result = pthread_create( &t_mgn_sensors_poll, NULL, t_mgn_poll_thread, (void *)0 );
	if(result != 0)
	{
		printf( "sensors thread create fail\n" );
		return;
	}
}


/********************************************
 *
 * Camera test function
 *
 * *****************************************/

#define S3C_MEM_DEV_NAME "/dev/s3c-mem"

static int createMem(unsigned int memory_size)
{
	int i = 0;
	s3c_mem_t  * s3c_mem = NULL;

	for(i = 0; i < NUM_OF_MEMORY_OBJECT; i++)
	{
		s3c_mem = &g_s3c_mem[i];
		s3c_mem->dev_fd = open(S3C_MEM_DEV_NAME, O_RDWR);
		if(s3c_mem->dev_fd < 0)
		{
			printf("%s::open(%s) fail(%s)\n", __func__, S3C_MEM_DEV_NAME, strerror(errno));
			s3c_mem->dev_fd = 0;
			return -1;
		}
		
		// kcoolsw
		s3c_mem->mem_alloc_info.size = memory_size;

		if(ioctl(s3c_mem->dev_fd, S3C_MEM_ALLOC, &s3c_mem->mem_alloc_info) < 0)
		{	
			printf("%s::S3C_MEM_ALLOC(size : %d)  fail\n", __func__, s3c_mem->mem_alloc_info.size);
			return -1;
		}

		s3c_mem->phys_addr =                 s3c_mem->mem_alloc_info.phy_addr;
		s3c_mem->virt_addr = (unsigned char*)s3c_mem->mem_alloc_info.vir_addr;
		s3c_mem->buf_size  =                 s3c_mem->mem_alloc_info.size;
	}
		
	return 0;
}

static int destroyMem(void)
{
	int i = 0;
	for(i = 0; i < NUM_OF_MEMORY_OBJECT; i++)
	{
		s3c_mem_t  * s3c_mem = &g_s3c_mem[i];

		if(0 < s3c_mem->dev_fd)
		{
			if (ioctl(s3c_mem->dev_fd, S3C_MEM_FREE, &s3c_mem->mem_alloc_info) < 0)
			{
				printf("%s::S3C_MEM_FREE fail\n", __func__);
				return -1;
			}
			
			close(s3c_mem->dev_fd);
			s3c_mem->dev_fd = 0;
		}
		s3c_mem->phys_addr = 0;
		s3c_mem->virt_addr = NULL;
		s3c_mem->buf_size  = 0;
	}

	return 0;
}


static void close_device()
{
	if (-1 == close (cam_fd))
		errno_exit ("close");

	if (-1 == close (fd_fb))
		errno_exit ("close");

	fd_fb = -1;
	cam_fd = -1;
	gprintf("complete\n");
}
static void stop_capturing(void)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl (cam_fd, VIDIOC_STREAMOFF, &type))
		errno_exit ("VIDIOC_STREAMOFF");
	gprintf("complete\n");
}


static void process_image(const void *p)
{
#ifdef CAMERA_TYPE_A
	unsigned short *temp;
	int i, j;

	temp = fb;

	for (i = 0; i < PANEL_H; i++)
		for (j = PANEL_W; j > 0; j--) {
			*((unsigned short *)temp) = *((unsigned short *)p + (PANEL_H * (j - 1) + i));
			temp++;
		}
#endif
#ifdef CAMERA_TYPE_B
	gprintf("\n");
	unsigned short *temp;
	int i, j;

	temp = fb;

	for (i = PANEL_H-1; i >= 0; i--)
		for (j = 0; j < PANEL_W; j++) {
			*((unsigned short *)temp) = *((unsigned short *)p + (PANEL_H * j + i));
			temp++;
		}
#endif
#ifdef CAMERA_TYPE_C
	gprintf("memcpy fb\n");
	memcpy(fb, p, PANEL_H * PANEL_W * 2);
#endif
}

static int read_frame()
{
	struct v4l2_buffer buf;
	unsigned int i;

	gprintf("start\n");
	CLEAR (buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl (cam_fd, VIDIOC_DQBUF, &buf)) {
		switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("VIDIOC_DQBUF");
		}
	}

	assert (buf.index < NUM_OF_MEMORY_OBJECT);	/* n_buffers = 4 */

	process_image (cam_buffers[buf.index].start);

	return 1;
}


static void mainloop(void)
{
	//pthread_create(&pth, NULL, (void *)&capture_thread, NULL);	
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_ORIGINAL;
	ctrl.value = 0;

	if (-1 == xioctl (cam_fd, VIDIOC_S_CTRL, &ctrl))
		errno_exit ("VIDIOC_S_CTRL");

	while (!finish) {
		for (;;) {
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO(&fds);
			FD_SET(cam_fd, &fds);

			/* Timeout. */
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			r = select(cam_fd + 1, &fds, NULL, NULL, &tv);
			if (-1 == r) {
				if (EINTR == errno)
					continue;

				errno_exit ("select");
			}

			if (0 == r) {
				fprintf (stderr, "select timeout\n");
				exit (EXIT_FAILURE);
			}
			
			if (read_frame ())
				break;

			/* EAGAIN - continue select loop. */
		}
	}

	//if(pthread_join(pth, NULL)==0)
		//gprintf("pthread_join : capture_thread\n");
}

static void start_capturing(void)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl (cam_fd, VIDIOC_STREAMON, &type))	/* STREAM ON */
		errno_exit ("VIDIOC_STREAMON");

}

static void uninit_device(void)
{
	unsigned int i;

	struct v4l2_requestbuffers req;

	CLEAR (req);

	req.count               = 0;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl (cam_fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support "
					"memory mapping\n", DEV_CAM_NAME);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}

	free (cam_buffers);
	cam_buffers = NULL;
	cam_n_buffers = 0;


	if(destroyMem() < 0)
		errno_exit ("destroyMem");
}

void fb_init() 
{
	int ret, i, j;
	unsigned short *temp;

	fd_fb = open("/dev/graphics/fb1", O_RDWR);

	s3c_win_info_t win;
	
	win.bpp = 16;
	win.left_x = 0;
	win.top_y = 0;
	win.width = PANEL_W;//target_x;
	win.height = PANEL_H;//target_y;
	
	ret = ioctl(fd_fb, S3C_FB_OSD_SET_INFO, &win);
	if (ret < 0) {
		perror("Ioctl S3C_FB_OSD_SET_INFO -> Error!\n");
		exit(1);
	}
	
	fb = (unsigned short *)mmap(0, (PANEL_W * PANEL_H * 2),
			PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
	if (fb == NULL) {
		errno_exit ("MMAP");
	}

	temp = fb;

	for (i = 0; i < PANEL_H; i++) {
		for (j = 0; j < PANEL_W; j++) {
			*temp = 0xF800; 
			temp++;
		}
	}
	
	ret = ioctl(fd_fb, S3C_FB_OSD_START, NULL);
	if (ret < 0) {
		perror("Ioctl S3C_FB_WIN_ON -> Error!\n");
		exit(1);
	}
}

static void init_mmap(void)
{
	struct v4l2_requestbuffers req;

	CLEAR (req);

	req.count               = NUM_OF_MEMORY_OBJECT;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl (cam_fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support "
					"memory mapping\n", DEV_CAM_NAME);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}
	gprintf("\n");
	unsigned int frame_size = (PANEL_H * PANEL_W * 2);
	if(createMem(frame_size) < 0)
		errno_exit("createMem fail");

	gprintf("\n");
	cam_buffers = calloc (req.count, sizeof (*cam_buffers));
	if (!cam_buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

		struct v4l2_buffer buf;
	gprintf("\n");
	for (cam_n_buffers = 0; cam_n_buffers < NUM_OF_MEMORY_OBJECT ; cam_n_buffers++)
	{
		gprintf("cam_n_buffers = %d\n",cam_n_buffers);
		s3c_mem_t  * s3c_mem = &g_s3c_mem[cam_n_buffers];

		CLEAR (buf);

		buf.memory      = V4L2_MEMORY_USERPTR;		
		buf.index       = cam_n_buffers;
		buf.length       = s3c_mem->buf_size;
		buf.m.offset   = (unsigned int)s3c_mem->phys_addr;

		if(ioctl(cam_fd, VIDIOC_QBUF, &buf) < 0)
			errno_exit ("VIDIOC_QBUF");

		cam_buffers[cam_n_buffers].length = buf.length;
		cam_buffers[cam_n_buffers].start  = s3c_mem->virt_addr;

		printf("buffers[%d].length = 0x%08x, buffers[%d].start = 0x%08x\n",
					cam_n_buffers, cam_buffers[cam_n_buffers].length,
					cam_n_buffers, (unsigned int)cam_buffers[cam_n_buffers].start);

	}
	gprintf("\n");
}


int m_getCropRect(unsigned int   src_width,  unsigned int   src_height,
                                 unsigned int   dst_width,  unsigned int   dst_height,
                                 unsigned int * crop_x,     unsigned int * crop_y,
                                 unsigned int * crop_width, unsigned int * crop_height)
{
	#define CAMERA_CROP_RESTRAIN_NUM (0x08)

	unsigned int cal_src_width   = 0;
	unsigned int cal_src_height  = 0;
	
	float src_ratio    = 1.0f;
	float dst_ratio    = 1.0f;
	
	*crop_x      = 0;
	*crop_y      = 0;
	*crop_width  = src_width;
	*crop_height = src_height;
	
	if(src_width == dst_width && src_height == dst_height)
		return 0;

	// ex : 1024 / 768
	src_ratio  = (float)src_width / (float)src_height ;

	// ex : 352  / 288
	dst_ratio =  (float)dst_width / (float)dst_height;

	if(src_ratio == dst_ratio)
		return 0;
	
	if(src_ratio <= dst_ratio)
	{
		// height 를 줄인다
		cal_src_width  = src_width;
		cal_src_height = src_width / dst_ratio;

		//if(cal_src_height & 0x01)
		//	cal_src_height++;
	}
	else //(src_ratio > dst_ratio)
	{
		// width 를 줄인다
		cal_src_width  = src_height * dst_ratio;
		cal_src_height = src_height;
		
		unsigned int width_align = (cal_src_width & (CAMERA_CROP_RESTRAIN_NUM-1));
		if(width_align != 0)
		{
			if(cal_src_width + (CAMERA_CROP_RESTRAIN_NUM - width_align) <= dst_width)
				cal_src_width += (CAMERA_CROP_RESTRAIN_NUM - width_align);
			else
				cal_src_width -= width_align;
	}
}

	// kcoolsw : this can be camera view weird..
	//           because dd calibrate x y once again
	//*crop_x      = (src_width  - cal_src_width ) >> 1;
	//*crop_y      = (src_height - cal_src_height) >> 1;
	*crop_width  = cal_src_width;
	*crop_height = cal_src_height;
	
	return 0;
}



static void init_device(void)
{
	init_mmap ();

	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_input input;
	struct v4l2_control ctrl;
	unsigned int min, in;

	gprintf("\n");
	if (-1 == xioctl (cam_fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s is no V4L2 device\n",
					DEV_CAM_NAME);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_QUERYCAP");
		}
	}

	gprintf("\n");
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {		/* Capabilities Check */
		fprintf (stderr, "%s is no video capture device\n",
				DEV_CAM_NAME);
		exit (EXIT_FAILURE);
	}
	gprintf("\n");

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf (stderr, "%s does not support streaming i/o\n",
				DEV_CAM_NAME);
		exit (EXIT_FAILURE);
	}
	gprintf("\n");


	/* Select video input, video standard and tune here. */
	
	CLEAR (input);
	
	printf("======== Input Device List ========\n");
	
	while (1) {
		if (-1 == xioctl (cam_fd, VIDIOC_ENUMINPUT, &input)) {
			if (EINVAL == errno) {
				break;
			} else {
				errno_exit ("VIDIOC_ENUMINPUT");
			}
		}
		else {
			printf("Index %d -> %s\n", input.index, input.name);
			input.index++;
		}
	}
	
	printf("===================================\n");
	
	if (-1 == xioctl (cam_fd, VIDIOC_G_INPUT, &in)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s is no V4L2 device\n",
					DEV_CAM_NAME);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_G_INPUT");
		}
	}

	printf("Current Input Device -> %d\n", in);

	printf("Select Input Device -> ");

	in = 1; //0: memory input, 1:camera input
	if (-1 == xioctl (cam_fd, VIDIOC_S_INPUT, &in)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s is no V4L2 device\n",
					DEV_CAM_NAME);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_S_INPUT");
		}
	}


	CLEAR (fmt);

	//fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.type                = V4L2_BUF_TYPE_VIDEO_OVERLAY;
	
	fmt.fmt.pix.width       = PANEL_H;//target_y; 
	fmt.fmt.pix.height      = PANEL_W;//target_x;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

	if (-1 == xioctl (cam_fd, VIDIOC_S_FMT, &fmt))
		errno_exit ("VIDIOC_S_FMT");

	/* Note VIDIOC_S_FMT may change width and height. */

	/*
	CLEAR (cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl (cam_fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		
		printf("cropcap.defrect.left   %d\n", cropcap.defrect.left);
		printf("cropcap.defrect.top    %d\n", cropcap.defrect.top);
		printf("cropcap.defrect.width  %d\n", cropcap.defrect.width);
		printf("cropcap.defrect.height %d\n", cropcap.defrect.height);
#if 0
		// Modify 
		if (in == 1) {
			// Mega Preview Mode
			cropcap.defrect.left = 0;
			cropcap.defrect.top = 0;
			cropcap.defrect.width = 1024;
			cropcap.defrect.height = 768;
		}
		else if (in == 2)
		{
			// CIF Preview Mode
			cropcap.defrect.left = 0;
			cropcap.defrect.top = 0;
			cropcap.defrect.width = 352;
			cropcap.defrect.height = 288;
		}
#endif
		printf("cropcap.defrect.left   %d\n", cropcap.defrect.left);
		printf("cropcap.defrect.top    %d\n", cropcap.defrect.top);
		printf("cropcap.defrect.width  %d\n", cropcap.defrect.width);
		printf("cropcap.defrect.height %d\n", cropcap.defrect.height);

		crop.c = cropcap.defrect; // reset to default

		if (-1 == xioctl (cam_fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
				case EINVAL:
					// Cropping not supported.
					break;
				default:
					// Errors ignored.
					break;
			}
		}
	} else {	
		// Errors ignored. 
	}
	*/
	
	// set crop
	//struct v4l2_cropcap cropcap;
	//struct v4l2_crop crop;
	unsigned int calib_x      = 0;
	unsigned int calib_y      = 0;
	unsigned int calib_width  = 0;
	unsigned int calib_height = 0;

	int     v4l_buf_type_video = V4L2_BUF_TYPE_VIDEO_OVERLAY;
//	int     v4l_buf_type_video = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	cropcap.type = v4l_buf_type_video;

	if(ioctl(cam_fd, VIDIOC_CROPCAP, &cropcap) >= 0)
	{
		m_getCropRect(cropcap.bounds.width, cropcap.bounds.height,
		              PANEL_W,              PANEL_H,
					  &calib_x,             &calib_y,
					  &calib_width,         &calib_height);

		//printf("####### kcoolsw : cropcap.bounds.width(%d), cropcap.bounds.height(%d), width(%d), height(%d) \n",
		//       cropcap.bounds.width, cropcap.bounds.height, width, height);

		//printf("####### kcoolsw : cropping calib_x(%d), calib_y(%d), calib_width(%d), calib_height(%d) \n",
		//       calib_x, calib_y, calib_width, calib_height);

		if(   (unsigned int)cropcap.bounds.width  != calib_width
		   || (unsigned int)cropcap.bounds.height != calib_height)
		{			
			cropcap.defrect.left   = calib_x;
			cropcap.defrect.top    = calib_y;
			cropcap.defrect.width  = calib_width;
			cropcap.defrect.height = calib_height;
			
			crop.type = v4l_buf_type_video;
			crop.c    = cropcap.defrect;
			
			if (ioctl(cam_fd, VIDIOC_S_CROP, &crop) < 0)
				printf("%s::VIDIOC_S_CROP fail (bug ignored..)\n", __func__);
		}
	}
	else
		printf("%s::VIDIOC_CROPCAP fail (bug ignored..)\n", __func__);
		// ignore errors


	// set sensor..
	//struct v4l2_control ctrl;

	ctrl.id = VIDIOC_S_SENSOR;
	ctrl.value = (1 << 0); //SENSOR_PREVIEW
	//ctrl.value = (1 << 1); //SENSOR_CAPTURE
	//ctrl.value = (1 << 2); //SENSOR_CAMCORDER

	ctrl.value = ctrl.value | (1 << 4); // SENSOR_NIGHTMODE
	
	if(ioctl(cam_fd, VIDIOC_S_SENSOR, &ctrl) < 0)
		errno_exit ("VIDIOC_S_SENSOR");

#if 0	
	CLEAR (ctrl);
	
	ctrl.value = 1;

	if (-1 == xioctl (cam_fd, VIDIOC_S_ROTATE_ANGLE, &ctrl))
		errno_exit ("VIDIOC_S_ROTATE_ANGLE");
#endif	

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

}

static void open_device(void)
{
	struct stat st; 

	gprintf("start\n");
	if (stat (DEV_CAM_NAME, &st)==-1) {
		fprintf (stderr, "Cannot identify '%s': %d, %s\n",
				DEV_CAM_NAME, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}

	if (!S_ISCHR (st.st_mode)) {
		fprintf (stderr, "%s is no device\n", DEV_CAM_NAME);
		exit (EXIT_FAILURE);
	}

	cam_fd = open (DEV_CAM_NAME, O_RDWR /* required */ | O_NONBLOCK, 0);
	if (-1 == cam_fd) {
		fprintf (stderr, "Cannot open '%s': %d, %s\n",
				DEV_CAM_NAME, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}
}

static void t_test_CameraPreview(void)
{
	open_device();

	gprintf("\n");
	init_device();

	fb_init();
	
	start_capturing();

	mainloop();
	
	stop_capturing();

	uninit_device();

	close_device();

	gprintf("pthread_exit(0)\n");
	pthread_exit(0);
}

void devmgr_test_CameraPreview(void)
{
	finish = 0;//finish false

	gprintf("start\n");
	pthread_create(&t_camera_preview, NULL, (void *)&t_test_CameraPreview, NULL);
}


void devmgr_test_VibratorOnOff(int onoff)
{
	int timeout_ms = 0;
	int fd, ret, nwr;
	char value[20];

	if(onoff)
	{
		timeout_ms = -1;
	}

	fd = open("/sys/class/timed_output/vibrator/enable", O_RDWR);
	if(fd < 0)
	{
		perror("vibrator open error\n");
	}

	nwr = sprintf(value, "%d\n", timeout_ms);
 	ret = write(fd, value, nwr);
    close(fd);

	return;
}


void devmgr_test_PlaySound(unsigned short input_path, unsigned short output_path)
{
	int fd_dsp;
	unsigned char buf[1024];
	int t,i;
	int ret;
	int loop_num = SND_SRC_SIZE / 1024;
	int change = SND_SRC_SIZE % 1024;
	unsigned char *ptr = (unsigned char*)Romance;

	fd_dsp = open("/dev/snd/dsp", O_RDWR);
	if(fd_dsp < 0)
	{	
		perror("dsp open error");
		goto __sound_err__;
	}
	lcd_draw_font(30,120,COLOR_WHITE,COLOR_RED,12,(unsigned char* )" Play Sound ");
	lcd_draw_font(10,152,COLOR_BLACK,COLOR_WHITE,20,(unsigned char* )"RomancingTheTone.wav");
	
	t = 16;
	gprintf("Set format %dBIT\n",t);
	ioctl(fd_dsp, SNDCTL_DSP_SETFMT, &t);

	t = 44100; // 22KHz = 22050, 44KHz = 44100
	gprintf("Set sample rate %dHz\n",t);
	ioctl(fd_dsp, SNDCTL_DSP_SPEED, &t);

	t = 1;
	if(t == 1)
		gprintf("Set stereo\n");
	else
		gprintf("Set mono\n");
	ioctl(fd_dsp, SNDCTL_DSP_STEREO, &t);

	for(i = 0; i < loop_num; i++)
	{
		memcpy(buf,ptr,sizeof(buf));
		write(fd_dsp, buf, sizeof(buf));
		ptr += 1024;
	}
	write(fd_dsp, ptr, change);
	
	close(fd_dsp);

__sound_err__:
	gprintf("end\n");
}


void devmgr_test_LedOnOff(unsigned int led_data)
{
	
	char value[20];
	int fd_kb,fd_bb,nwr;

	gprintf("LED VALUE = %d\n",led_data);
	if (led_data)
		led_data = 1;
#ifdef KEYPAD_LED
	fd_kb = open("/sys/class/leds/keyboard-backlight/brightness", O_RDWR);
	if(fd_kb < 0)
		gprintf("keybord-backlight open error\n");
	else
	{
		nwr = sprintf(value, "%d\n",led_data);
		write(fd_kb, value, nwr);
		close(fd_kb);
	}
#endif	

#ifdef BUTTON_LED
	fd_bb = open("/sys/class/leds/button-backlight/brightness", O_RDWR);
	if(fd_bb < 0)
		gprintf("button-backlight open error\n");
	else
	{
		nwr = sprintf(value, "%d\n",led_data);
		write(fd_bb, value, nwr);
		close(fd_bb);
	}
#endif
	
}
void devmgr_test_LcdBackLight(unsigned int level)
{
	char value[20];
	int fd,nwr;
	int fill;
	fd = open("/sys/devices/platform/s3c-lcd/backlight_level", O_RDWR);
	if(fd < 0)
		gprintf("backlight open error\n");
	else
	{
		
		gprintf("backlight level = %d\n",level);
		nwr = sprintf(value, "%d\n",level);
		write(fd, value, nwr);
		close(fd);
		lcd_draw_font(100,144,COLOR_BLACK,COLOR_WHITE,nwr,(unsigned char* )value);
		if(level > PANEL_W-2)
			fill = PANEL_W-2;
		else
			fill = level;
#if defined (CONFIG_FB_LCD_WQVGA)
		lcd_FilledRectangle(0, 198, PANEL_W-2, 207, COLOR_GRAY);
		lcd_FilledRectangle(0, 200, fill, 205, COLOR_BLACK);
#elif defined (CONFIG_FB_LCD_HVGA)
		lcd_FilledRectangle(30, 198, 285, 207, COLOR_GRAY);
		lcd_FilledRectangle(30, 200, fill+30, 205, COLOR_BLACK);
#elif defined (CONFIG_FB_LCD_WVGA)
		lcd_FilledRectangle(100, 198, 355, 207, COLOR_GRAY);
		lcd_FilledRectangle(100, 200, fill+100, 205, COLOR_BLACK);
#endif
		
	}
}

void devmgr_test_LcdPower(unsigned int level)
{
	#if 0
	char value[20];
	int fd,nwr;
	fd = open("/sys/devices/platform/s3c-lcd/lcd_power", O_RDWR);
	if(fd < 0)
		gprintf("lcd_power open error\n");
	else
	{
		nwr = sprintf(value, "%d\n",onoff);
		write(fd, value, nwr);
		close(fd);
	}
	#else
	char value[20];
	int fd,nwr;
	int fill;
	fd = open("/sys/devices/platform/s3c-lcd/backlight_level", O_RDWR);
	if(fd < 0)
		gprintf("backlight open error\n");
	else
	{
		gprintf("backlight level = %d\n",level);
		nwr = sprintf(value, "%d\n",level);
		write(fd, value, nwr);
		close(fd);
	}
	#endif
}


void devmgr_test_LcdColor(unsigned short color)
{
#ifdef CONFIG_FB_PXA_LCD_WVGA
	//lcd_FilledRectangle(0,0,239,399,color);
	lcd_FilledRectangle(0, 0, PANEL_W-2, PANEL_H-2, color);
#else
	//lcd_FilledRectangle(0,0,239,319,color);
	lcd_FilledRectangle(0, 0, PANEL_W-2, PANEL_H-2, color);
#endif
}

void devmgr_test_PowerOff(void)
{
	//lcd_FilledRectangle(0,0,239,399, COLOR_BLACK);
	lcd_FilledRectangle(0, 0, PANEL_W-2, PANEL_H-2, COLOR_BLACK);
	
	/* draw title bar */
	lcd_draw_font(64,2,COLOR_WHITE,COLOR_BLACK,14,(unsigned char* )"POWER OFF TEST");

	my_system("/sbin/poweroff");
}


void devmgr_test_ResetWindow(void)
{
	devmgr_test_ShowDummyWindow("RESET TEST");
	
	lcd_draw_font(10,72,COLOR_BLACK,COLOR_WHITE,12,(unsigned char* )"  UP : RESET");
	lcd_draw_font(10,88,COLOR_BLACK,COLOR_WHITE,19,(unsigned char* )"DOWN : SAVE & RESET");
	lcd_draw_font(10,104,COLOR_BLACK,COLOR_WHITE,13,(unsigned char* )"BACK : CANCEL");
}

void Show_TouchFirmwareVirsion(void)
{
	FILE *fptr = NULL;
	char *strptr = NULL;
	char *strptr2 = NULL;
	char linebuf[64];
	char linebuf2[64];
	system("cat /sys/class/sec/ts/firmware > /tmp/touch_fw");
	if((fptr = fopen("/tmp/touch_fw","rt+"))!=NULL)
	{
		if(fgets(linebuf, sizeof(linebuf), fptr) != NULL)
		{
			memcpy(linebuf2, linebuf, 64);
			if((strptr = strstr(linebuf, "H/W")) != NULL)
			{
				*(strptr+13) = 0;
				lcd_draw_font(10, 62, COLOR_GRAY, COLOR_WHITE, strlen(strptr) < 29 ? strlen(strptr) : 28, (unsigned char *)strptr);
				if((strptr2 = strstr(linebuf2, "F/W")) != NULL)
				{
					*(strptr2+13) = 0;
					lcd_draw_font(10, 78, COLOR_GRAY, COLOR_WHITE, strlen(strptr2) < 29 ? strlen(strptr2) : 28, (unsigned char *)strptr2);
				}
				fclose(fptr);
			}
		}
	}
}
void devmgr_test_TouchFirmwareWindow(void)
{
	devmgr_test_ShowDummyWindow("TOUCH FW UPDATE");

	Show_TouchFirmwareVirsion();
	lcd_draw_font(10,110, COLOR_BLACK,COLOR_WHITE,14,(unsigned char* )"OK : FW UPDATE");
	lcd_draw_font(10,126,COLOR_BLACK,COLOR_WHITE,13,(unsigned char* )"BACK : CANCEL");
}
void devmgr_save_parameters(void)
{
	int param_fd;
	int ret, nwr, param_val;
	char value[20];

	if( (uart_current_switch_mode == -1) &&
		(usb_current_switch_mode == -1) )
	{
	}
	else
	{
		param_fd = open("sys/class/sec/param/switch_sel",O_RDWR);
		if(param_fd <0)
		{
			perror("param_fd open error\n");

		}

		nwr = read(param_fd,value,2);
		param_val = atoi(value);
		
		switch(uart_current_switch_mode)
		{
			case MODEM :
				param_val &= ~(1<<1);
				break;
			case PDA : 
				param_val |= (1<<1);
				break;
			default :
				break;
		}
		switch(usb_current_switch_mode)
		{
			case MODEM :
				param_val &= ~ (1<<0);
				break;
			case PDA : 
				param_val |= (1<<0);
				break;
			default :
				break;
		}
		nwr = sprintf(value,"%d\n",param_val);
		ret = write(param_fd,value,nwr);

    	close(param_fd);
	}	
}


void devmgr_test_ShowVersionWindow(void)
{
#ifdef USE_CFL
	cflh_t cfg;
	cfl_value_type_t type;
#else
	dictionary *d;
#endif

	/* clear window */
	//lcd_FilledRectangle(0,0,239,319,COLOR_WHITE);
	lcd_FilledRectangle(0, 0, PANEL_W-2, PANEL_H-2, COLOR_WHITE);

	/* draw title bar */
	lcd_FilledRectangle(0,0,TITLE_WIDTH,TITLE_HEIGHT,COLOR_BLUE);
	lcd_draw_font(72,10,COLOR_WHITE,COLOR_BLUE,12,(unsigned char*)"VERSION INFO");

#ifdef USE_CFL
	if ((cfg = cfl_read(SYSINFO_DB))==NULL)
	{
		printf("can't open "SYSINFO_DB"\n");
		exit(EXIT_FAILURE);
	}

    {
	#define MAX_STR_SIZE	29
	int i;
	long position=0;
	
	long sections_count = cfl_get_sections_count (cfg);
	printf ("This file has %ld sections: \n", sections_count);
	for (i = 2; i <= sections_count; i++)
	    {
		int j;
		char section_buffer[64];
		char* section_name = cfl_section_get_name_by_position (cfg, i);
		sprintf (section_buffer,"[%s]",section_name);
		section_buffer[MAX_STR_SIZE]=0;
		long values_count = cfl_section_get_values_count (cfg, section_name);
		lcd_draw_font(	str_lcd[position].x1, 
						str_lcd[position].y1,
						COLOR_BLACK,
						COLOR_WHITE,
						strlen(section_buffer),
						section_buffer);
		position++;
		for (j = 1; j <= values_count; j++)
		    {
			char values_buffer[64];
			char* value_name;
			char* value;
			cfl_value_get_by_position (cfg, section_name, j, &value_name, NULL);
			value = cfl_value_get_by_name(cfg, section_name, value_name, &type);
			sprintf (values_buffer,"%s=%s",value_name,value);
			values_buffer[MAX_STR_SIZE]=0;
			lcd_draw_font(	str_lcd[position].x1,
							str_lcd[position].y1,
							COLOR_BLUE,
							COLOR_WHITE,
							strlen(values_buffer),
							values_buffer);
			position++;
		    }
		position++;
	    }
    }

    cfl_free (cfg);
#else
	d = iniparser_load(SYSINFO_DB);
	{
	#define ASCIILINESZ         1024
	int		i,j;
	char	keym[ASCIILINESZ+1];
    int		nsec;
    char	*secname;
    int		seclen;
	int 	position=0;

	nsec = iniparser_getnsec (d);
	printf ("This file has %d sections: \n", nsec);

	if (nsec<1) {
		printf ("This file has no sections \n");
		return ;
	}
	else
		printf ("This file has %d sections: \n", nsec);

    for (i=0 ; i<nsec ; i++) {
		char sbuf[64];
        secname = iniparser_getsecname(d, i) ;
        seclen  = (int)strlen(secname);
		sprintf(sbuf,"[%s]",secname);
        sprintf(keym, "%s:", secname);
		lcd_draw_font(	str_lcd[position].x1, 
						str_lcd[position].y1,
						COLOR_BLACK,
						COLOR_WHITE,
						strlen(sbuf),
						(unsigned char*)sbuf);
		position++;
        for (j=0 ; j<d->size ; j++) {
            if (d->key[j]==NULL)
                continue ;
            if (!strncmp(d->key[j], keym, seclen+1)) {
                sprintf(sbuf, "%s = %s", d->key[j]+seclen+1, d->val[j] ? d->val[j] : "");
				lcd_draw_font(	str_lcd[position].x1, 
								str_lcd[position].y1,
								COLOR_BLACK,
								COLOR_WHITE,
								strlen(sbuf),
								(unsigned char*)sbuf);
				position++;
            }
        }
		position++;
    }
	}
	iniparser_freedict(d);
#endif
}

void devmgr_test_ShowUartUsbWindow(char* device_name, unsigned int index)
{	
	
	
	lcd_dev_t lcd[] =	
	{
		{ 5, 30, 0, 0, COLOR_BLACK, COLOR_WHITE, "<UP KEY>   PDA..........<   >"},
		{ 5, 46, 0, 0, COLOR_BLACK, COLOR_WHITE, "<DOWN KEY> PHONE........<   >"},
		{ 5, 30, 0, 0, COLOR_RED, COLOR_WHITE,   "<UP KEY>   PDA..........< O >"},
		{ 5, 46, 0, 0, COLOR_BLACK, COLOR_WHITE, "<DOWN KEY> PHONE........<   >"},
		{ 5, 30, 0, 0, COLOR_BLACK, COLOR_WHITE, "<UP KEY>   PDA..........<   >"},
		{ 5, 46, 0, 0, COLOR_RED, COLOR_WHITE,   "<DOWN KEY> PHONE........< O >"},
	};

	if( index == USB_UART_SWITCH )
	{
		devmgr_test_ShowDummyWindow( device_name );
	}
	
	lcd_draw_font( lcd[index].x1, lcd[index].y1, lcd[index].fgc, 
				   lcd[index].bgc, strlen( (char* )lcd[index].data), lcd[index].data);
	index++;	
	
	lcd_draw_font( lcd[index].x1, lcd[index].y1, lcd[index].fgc, 
				   lcd[index].bgc, strlen( (char* )lcd[index].data), lcd[index].data);
	
}


void devmgr_test_ShowSensorWindow(void)
{
	/* clear window */
	lcd_FilledRectangle(0, 0, PANEL_W-2, PANEL_H-2, COLOR_WHITE);

	/* draw title bar */
	lcd_FilledRectangle(0,0,TITLE_WIDTH,TITLE_HEIGHT,COLOR_BLUE);
	lcd_draw_font( 74, 10, COLOR_WHITE, COLOR_BLUE,20, (unsigned char*)"SENSOR TEST         ");

	lcd_Rectangle(5,30,234,110,COLOR_RED);
	lcd_FilledRectangle(5,30,234,49,COLOR_RED);
	lcd_draw_font( 10, 32, COLOR_WHITE, COLOR_RED,20, (unsigned char*)"ACCELATION SENSOR   ");

	lcd_Rectangle(5,126,234,174,COLOR_RED);
	lcd_FilledRectangle(5,126,234,145,COLOR_RED);
	lcd_draw_font( 10, 128, COLOR_WHITE, COLOR_RED,20,(unsigned char*)"OPTICAL SENSOR   ");
	
	lcd_Rectangle(5,222,234,342,COLOR_RED);
	lcd_FilledRectangle(5,222,234,241,COLOR_RED);
	lcd_draw_font( 10, 224, COLOR_WHITE, COLOR_RED,20,(unsigned char*)"MAGNETIC SENSOR     ");
}


void devmgr_test_ShowKeyWindow(void)
{
	int i;
	int max_key_num = MAX_KEY_NUM;
	/* clear window */
	lcd_FilledRectangle(0, 0, PANEL_W-2, PANEL_H-2, COLOR_WHITE);

	/* draw title bar */
	lcd_FilledRectangle( 0, 0, TITLE_WIDTH, TITLE_HEIGHT, COLOR_BLUE );
	lcd_draw_font(76,10,COLOR_WHITE,COLOR_BLUE,11,(unsigned char*)"KEYPAD TEST");

#ifdef QWERTY_KEY
	lcd_FilledRectangle( 10,  32, 235, 375, COLOR_GRAY );
	//lcd_FilledRectangle(125, 335, 235, 375, COLOR_GRAY );
	lcd_FilledRectangle(130,  32, 235, 250, COLOR_BLACK );
#endif

#ifdef NON_QWERTY_KEY
	lcd_FilledRectangle( 0,  0, 237, 397, COLOR_GRAY );
	lcd_FilledRectangle( 10,  10, 220, 230, COLOR_BLACK );

#endif

	/* draw keypad array */
	for(i=0;i<max_key_num;i++) 
	{
		if(key_info[i].exist == KEY_EXIST) 
		{
			lcd_Rectangle(key_info[i].x,key_info[i].y,
				key_info[i].x + key_info[i].w,key_info[i].y + key_info[i].h,COLOR_BLACK);
		}
	}
}

void devmgr_test_ShowMainWindow(void)
{
	int i;
	int max_lcd_str = MAX_LCD_STR;
	int fd;
	char batt_buf[30];
	int batt_level;

	/* clear window */
	lcd_FilledRectangle(0, 0, PANEL_W-2, PANEL_H-2, COLOR_WHITE);
	
	/* draw title bar */
	lcd_FilledRectangle( 0, 0, TITLE_WIDTH, TITLE_HEIGHT, COLOR_BLUE );
	lcd_draw_font( 84, 10, COLOR_WHITE,COLOR_BLUE, 9, (unsigned char* )"SELF TEST" );
	
	
	/* draw list of test items */
	for(i=0;i<max_lcd_str;i++) {
		lcd_draw_font(str_lcd[i].x1, str_lcd[i].y1, str_lcd[i].fgc, str_lcd[i].bgc, strlen( (char* )str_lcd[i].data),str_lcd[i].data);
	}
#ifdef USE_INVERTED_FONTS
	lcd_draw_font(str_lcd[devmgr_current_item].x1, \
					str_lcd[devmgr_current_item].y1, \
					str_lcd[devmgr_current_item].bgc, \
					str_lcd[devmgr_current_item].fgc, \
					strlen( (char* )str_lcd[devmgr_current_item].data), \
					str_lcd[devmgr_current_item].data);
#endif
	fd = open ("/sys/class/power_supply/battery/capacity",O_RDONLY);
	if(fd < 0)
	{
		perror("batt-capacity open error\n");
		goto __batt_out__;
	}
	usleep(100000);
	read(fd,batt_buf,20);
	close(fd);
	//lseek(fd,0,SEEK_SET);
	batt_level = atoi(batt_buf);
	if((batt_level > 75) && (batt_level <= 100))
 		devmgr_test_DrawBatteryIndicator(3);
	else if((batt_level > 45) && (batt_level <= 75))
 		devmgr_test_DrawBatteryIndicator(2);
	else if((batt_level > 15) && (batt_level <= 45))
 		devmgr_test_DrawBatteryIndicator(1);
	else
 		devmgr_test_DrawBatteryIndicator(0);
	
__batt_out__:
	gprintf("end\n");
}


static void devmgr_test_ShowResult(int rv1,int rv2,int index)
{
	char *pos1;

#ifdef USE_INVERTED_FONTS
	devmgr_current_item = index;
#endif

	pos1 = strchr( (char*)str_lcd[index].data,'<');
	if(rv1==-1) {
		strncpy( pos1, (char* )str_fail,6 );
		str_lcd[index].fgc=COLOR_RED;
		str_lcd[index].bgc=COLOR_WHITE;
	}
	else {
		strncpy( pos1, (char*)str_pass, 6);
		str_lcd[index].fgc=COLOR_BLACK;
		str_lcd[index].bgc=COLOR_WHITE;
	}
#ifdef USE_INVERTED_FONTS
	lcd_draw_font(str_lcd[index].x1, str_lcd[index].y1, str_lcd[index].bgc, str_lcd[index].fgc, strlen( (char*)str_lcd[index].data),
	              str_lcd[index].data);
#else
	lcd_draw_font(str_lcd[index].x1, str_lcd[index].y1, str_lcd[index].fgc,str_lcd[index].bgc, strlen( (char*)str_lcd[index].data),
	              str_lcd[index].data);
#endif
}

static void devmgr_test_ShowDpramWithPhoneserver(unsigned short type)
{
	#if 0
	int dpram0_fd = 0;
	unsigned int p_ret = 0;
	char ret_str[30];
	memset(ret_str, 0x00, sizeof(ret_str));
	
	/* clear window */
	lcd_FilledRectangle(0, 0, PANEL_W-2, PANEL_H-2, COLOR_WHITE);


	dpram0_fd = open("/dev/dpram0", O_RDWR | O_NOCTTY | O_NONBLOCK);
	if(dpram0_fd<0)
	{
		gprintf("dpram open error!(%d)\n",dpram0_fd);
		lcd_draw_font( 10 , 70, COLOR_BLACK, COLOR_WHITE, 17, (unsigned char*)"dpram open error!");
	}
	else
	{
		switch(type)
		{
			case KEY_EXT_UP : //Phone on
				break;

			case KEY_EXT_DOWN : //Phone off 
				break;

			case KEY_EXT_SEND : //Phone status
				lcd_draw_font(50, 40, COLOR_WHITE, COLOR_RED, 12, (unsigned char*)" GET_STATUS ");
				if (ioctl(dpram0_fd, DPRAM_PHONE_GETSTATUS, &p_ret) < 0)
				{
					sprintf(ret_str, "ioctl err:DPRAM_PHONE_GETSTATUS");
				}
				else
				{
					sprintf(ret_str, "GETSTATUS:Phone is %s", p_ret? "active" : "inactive");
				}
				break;
			case KEY_EXT_CAM : //Phone IPC
				break;
			default :
				break;
		}
		lcd_draw_font( 8 , 100, COLOR_BLACK, COLOR_WHITE, strlen(ret_str), (unsigned char*)ret_str );
		close(dpram0_fd);
	}
	#endif
}

static void devmgr_test_ShowBatteryWindow(void)
{
	int fd, nwr;
	char batt_str[100];
	char batt_buf[30];
	
	memset(batt_str,0x00,sizeof(batt_str));
	memset(batt_buf,0x00,sizeof(batt_buf));
	
	devmgr_test_ShowDummyWindow("BATTERY TEST");
	lcd_draw_font(25,46,COLOR_WHITE,COLOR_RED,12,(unsigned char*)" Batt. INFO ");
	
	fd = open ("/sys/class/power_supply/battery/status",O_RDONLY);
	if(fd < 0)
	{
		perror("batt-status open error\n");
		goto __batt_out__;
	}
	usleep(100000);
	nwr = read(fd,batt_buf,20);
	lseek(fd,0,SEEK_SET);
	sprintf( batt_str, "Status : %s",batt_buf);
	lcd_draw_font(20,78,COLOR_BLACK,COLOR_WHITE,strlen(batt_str),(unsigned char *)batt_str);
	close(fd);

	memset(batt_str,0x00,sizeof(batt_str));
	memset(batt_buf,0x00,sizeof(batt_buf));
	
	fd = open ("/sys/class/power_supply/battery/capacity",O_RDONLY);
	if(fd < 0)
	{
		perror("batt-capacity open error\n");
		goto __batt_out__;
	}
	usleep(100000);
	nwr = read(fd,batt_buf,20);
	lseek(fd,0,SEEK_SET);
	sprintf( batt_str, "Capacity : %s%%",batt_buf);
	lcd_draw_font(20,110,COLOR_BLACK,COLOR_WHITE,strlen(batt_str),(unsigned char *)batt_str);
	close(fd);
__batt_out__:
	gprintf("end\n");
}
static void devmgr_test_ShowSDcardWindow(void)
{
	devmgr_test_ShowDummyWindow("SDCARD TEST");
	lcd_draw_font( 20, 46, COLOR_BLACK, COLOR_WHITE, 17, (unsigned char *)"Insert SDcard and");
	lcd_draw_font( 20, 78, COLOR_WHITE, COLOR_RED, 20, (unsigned char *)"Press OK button!");
}
static void	devmgr_test_sdcard(void)
{
	FILE *fp;
	char buf1[1024] = {0,};
	char buf2[1024] = {0,};
	char buf_size[12] = {0,};
	char *ptr_s, *ptr_e;
	int length = 0;
	int total_size = 0;
	char *strptr, retstr[50];

	devmgr_test_ShowDummyWindow("SDCARD TEST");
	lcd_draw_font( 20, 46, COLOR_WHITE, COLOR_RED, 15, (unsigned char *)" SDcard Memory ");
	gprintf("++\n");

	/* 1. Report SD card disk space usage */
	system("df /sdcard > /tmp/sdmmc_test"); 
#if 0	
	system("cat /tmp/sdmmc_test");
#endif
	
	if ((fp = fopen("/tmp/sdmmc_test","rt+"))==NULL) {
		gprintf("E: file open failed\n");
		goto __out__;
	}
	
	fread(buf1, sizeof(char), 1024, fp);
	fclose(fp);
	system("rm /tmp/sdmmc_test");


	/* 2. Write / Read test */

	/* Total */
	memset(retstr,0x00,sizeof(retstr));
	ptr_s = strchr(buf1,':') + 2;
	ptr_e = strchr(buf1,'K');
	length = ptr_e - ptr_s;
	if ( length > 11 ) {
		gprintf("E: invalid SD card size: %d\n",length);
		goto __out__;
	}
	strncpy(buf_size, ptr_s, length);
	buf_size[length] = 0;
	total_size = atoi(buf_size);
	if (!total_size) {
		sprintf(retstr,"SD card is not inseted");
		lcd_draw_font( 10, 78, COLOR_BLACK, COLOR_WHITE, strlen(retstr), (unsigned char *)retstr);
		goto __out__;
	}
	sprintf(retstr,"    Total : %dK",total_size);
	gprintf("%s\n",retstr);
	lcd_draw_font( 10, 78, COLOR_BLACK, COLOR_WHITE, strlen(retstr), (unsigned char *)retstr);
	memset(buf_size,0x00,sizeof(buf_size));
	memset(retstr,0x00,sizeof(retstr));
	gprintf("I: SD card size is %d\n", total_size);

	/* Used */
	strptr = ptr_e+1;
	ptr_s = strchr(strptr,',') + 2;
	ptr_e = strchr(strptr,'K');
	length = ptr_e - ptr_s;
	strncpy(buf_size, ptr_s, length);
	buf_size[length] = 0;
	sprintf(retstr,"     Used : %sK",buf_size);
	gprintf("%s\n",retstr);
	lcd_draw_font( 10, 94, COLOR_BLACK, COLOR_WHITE, strlen(retstr), (unsigned char *)retstr);
	memset(buf_size,0x00,sizeof(buf_size));
	memset(retstr,0x00,sizeof(retstr));
	

	/* Available */
	strptr = ptr_e+1;
	ptr_s = strchr(strptr,',') + 2;
	ptr_e = strchr(strptr,'K');
	length = ptr_e - ptr_s;
	strncpy(buf_size, ptr_s, length);
	buf_size[length] = 0;
	sprintf(retstr,"Available : %sK",buf_size);
	gprintf("%s\n",retstr);
	lcd_draw_font( 10, 110, COLOR_BLACK, COLOR_WHITE, strlen(retstr), (unsigned char *)retstr);
	
	if ((fp = fopen("/sdcard/rw_test","wt+"))==NULL) {
		gprintf("E: file open failed\n");
		goto __out__;
	}
	fwrite(buf1, sizeof(char), 1024, fp);
	fclose(fp);

	if ((fp = fopen("/sdcard/rw_test","rt"))==NULL) {
		gprintf("E: file open failed\n");
		goto __out__;
	}
	fread(buf2, sizeof(char), 1024, fp);
	fclose(fp);

	lcd_draw_font( 20, 142, COLOR_WHITE, COLOR_RED, 16, (unsigned char *)" SDcard RW TEST ");
	
	system("rm /sdcard/rw_test"); 

	if (strcmp(buf1, buf2))
	{
		lcd_draw_font( 15, 174, COLOR_BLACK, COLOR_WHITE, 21, (unsigned char *)"RW test.......<FAIL>");
		gprintf("E: read / write failed\n1: %s\n2: %s\n",
				buf1, buf2);
	}
	else
	{
		lcd_draw_font( 15, 174, COLOR_BLACK, COLOR_WHITE, 21, (unsigned char *)"RW test....<SUCCESS>");
		gprintf("I: read / write succeeded\n");
	}

__out__:
	gprintf(" --\n");

}

#if 0	//BT Scan Test
static void devmgr_test_bt(void)
{
	char buf[2048], errbuf[64];
	int i = 0;
	int max_line_val = 240;
	int status;
	FILE *fptr;
	char *ptr=NULL,*strptr=NULL,*cellstr=NULL;

	memset(buf,0x00,sizeof(buf));
	devmgr_test_ShowDummyWindow("BT TEST");
	lcd_draw_font( 20, 46, COLOR_BLACK, COLOR_WHITE, 15, (unsigned char *)"BT scanning ...");

	system("/system/bin/hciattach -n -p -s 115200 /dev/s3c2410_serial1 bcm2035 115200 flow &");
	system("sleep 1");

	system("/system/xbin/hciconfig hci0 up");
	if((status=system("/system/xbin/hcitool scan --numrsp=5 > /tmp/btscan"))<0)
		lcd_draw_font( 20, 46, COLOR_BLACK, COLOR_WHITE, 12 , (unsigned char *)"BT Scan fail");
#if 0	
	if((status=system("/system/xbin/hcitool inq > /tmp/btscan"))<0)
		lcd_draw_font( 20, 46, COLOR_BLACK, COLOR_WHITE, 12 , (unsigned char *)"BT Scan fail");
	if((status=system("/system/xbin/hciconfig > /tmp/btconfig"))<0)
		lcd_draw_font( 20, 46, COLOR_BLACK, COLOR_WHITE, 12 , (unsigned char *)"BT Scan fail");
#endif
	if((fptr=fopen("/tmp/btscan","rt+"))==NULL)
		lcd_draw_font( 20, 46, COLOR_BLACK, COLOR_WHITE, 12, (unsigned char *)"BT Scan fail");

	while(fgetc(fptr)==EOF)
	{
		lcd_draw_font( 20, 78, COLOR_BLACK, COLOR_WHITE, 19, (unsigned char *)"BT interface fail");
		goto btout;
	}
	lcd_draw_font( 20, 46, COLOR_WHITE, COLOR_RED, 16, (unsigned char *)" BT Scan Result ");

	if(status!=0){
		lcd_draw_font( 20, 78, COLOR_BLACK, COLOR_WHITE, 14, (unsigned char *)"BT device fail");
		goto btout;	
	}

	fread(buf,sizeof(char),2048,fptr);
	ptr=strchr(buf,'\n');
	strptr=ptr+1;

	if(strstr(strptr,"failed")!=NULL){
		lcd_draw_font( 20, 78, COLOR_BLACK, COLOR_WHITE, 14, (unsigned char *)"BT device fail");
		goto btout;
	}


	if((cellstr=strstr(strptr,"Scanning"))!=NULL)
	{
		cellstr=strchr(strptr,'\n');
		strptr = cellstr+1;
	}

	while((ptr=strchr(strptr,'\n'))!=NULL)
	{
		*ptr = 0;
		cellstr=strchr(strptr,':');
		strptr = cellstr-2;	
		cellstr = cellstr + 15;
		*cellstr = 0;

		lcd_draw_font( 15, 78+i, COLOR_GRAY, COLOR_WHITE, strlen(strptr) < 29 ? strlen(strptr) : 28, (unsigned char *)strptr);
		
		strptr=strptr+18;
		i+=16;

		lcd_draw_font( 15, 78+i, COLOR_BLACK, COLOR_WHITE, strlen(strptr) < 29 ? strlen(strptr) : 28, (unsigned char *)strptr);

		strptr = ptr + 1;
		i+=24;
		
		if(i>max_line_val)
			break;
	}
	if(i==0){
		lcd_draw_font( 0, 46, COLOR_BLACK, COLOR_WHITE, 25, (unsigned char *)"No Bluetooth device found");
	} 
btout:	
	fclose(fptr);	
	system("/system/xbin/hciconfig hci0 down");
#if 1 //to use bluez for test case	
	//system("sleep 1");
	//system("kill hciattach");
#endif	
	system("rm /tmp/btscan");
	//system("rm /tmp/btconfig");
}
#else	// Uart Attach Test
static void devmgr_test_bt(void)
{
	char buf[2048], errbuf[64];
	int i = 0;
	int count;
	FILE *fptr;
	char *ptr=NULL,*strptr=NULL,*cellstr=NULL;

	memset(buf,0x00,sizeof(buf));
	devmgr_test_ShowDummyWindow("BT TEST");
	//lcd_draw_font( 20, 46, COLOR_BLACK, COLOR_WHITE, 15, (unsigned char *)"BT scanning ...");


	system("echo 1 > /sys/class/rfkill/rfkill1/state");
	usleep(500000);
	system("echo 1 > /sys/class/rfkill/rfkill0/state");
	usleep(500000);
	system("/system/bin/hciattach -n -p -s 115200 /dev/s3c2410_serial1 bcm2035 115200 flow &");
	usleep(500000);
	system("/system/xbin/hciconfig hci0 up");
	usleep(500000);
	system("/system/xbin/hciconfig > /tmp/btconfig");
	usleep(500000);

	if((fptr=fopen("/tmp/btconfig","rt+"))==NULL)
		lcd_draw_font( 20, 46, COLOR_BLACK, COLOR_WHITE, 12, (unsigned char *)"BT Scan fail");

	while(fgetc(fptr)==EOF)
	{
		lcd_draw_font( 20, 78, COLOR_BLACK, COLOR_WHITE, 19, (unsigned char *)"BT interface fail");
		goto btout;
	}
	fseek(fptr, 0, SEEK_SET);

	count = fread(buf,sizeof(char),2048,fptr);
	if(count == 0)
		goto btout;
	ptr=strstr(buf,"BD");
	if(ptr != NULL)
	{
		lcd_draw_font( 20, 78, COLOR_WHITE, COLOR_RED, 20, (unsigned char *)"UART attach success!");
		strptr = buf;	
		*(ptr-1) = 0;
		lcd_draw_font( 20, 110, COLOR_GRAY, COLOR_WHITE, strlen(strptr) < 29 ? strlen(strptr) : 28, (unsigned char *)strptr);
		strptr = ptr;
		ptr = strstr(strptr,"Address:");
		if(ptr != NULL)
		{
			strptr = ptr+9;
			*(strptr+17) = 0;
			lcd_draw_font( 20, 142, COLOR_BLACK, COLOR_WHITE, 17, (unsigned char *)"BT Device Address");
			lcd_draw_font( 20, 158, COLOR_GRAY, COLOR_WHITE, strlen(strptr) < 29 ? strlen(strptr) : 28, (unsigned char *)strptr);
		}

	}

btout:	
	fclose(fptr);	
	system("/system/xbin/hciconfig hci0 down");
	usleep(500000);
	system("echo 0 > /sys/class/rfkill/rfkill0/state");
	usleep(500000);
	system("echo 0 > /sys/class/rfkill/rfkill1/state");
	
	system("rm /tmp/btconfig");
}
#endif


static void devmgr_test_wifi(void)
{
	char buf[2048], errbuf[64];
	int i = 0;
	int status;
	FILE *fptr;
	char *ptr,*strptr,*startstr,*endstr,*idstr, *cellstr;

	memset(buf,0x00,sizeof(buf));
	devmgr_test_ShowDummyWindow("WIFI TEST");
	lcd_draw_font( 20, 46, COLOR_BLACK, COLOR_WHITE, 20, (unsigned char *)"Wifi AP Scanning ...");

	if((status=system("insmod /lib/modules/dhd.ko \"firmware_path=/system/etc/rtecdc.bin nvram_path=/system/etc/nvram.txt\""))<0)
		lcd_draw_font( 10, 62, COLOR_BLACK, COLOR_WHITE, 14 , (unsigned char *)"Wifi init fail");
	system("sleep 1");
//	if((status=system("/system/bin/dhdarm_android -i eth0 download /system/etc/rtecdc.bin /system/etc/nvram.txt"))<0)
//		lcd_draw_font( 10, 62, COLOR_BLACK, COLOR_WHITE, 14 , (unsigned char *)"Wifi init fail");
//	system("sleep 1");
	if((status=system("ifconfig eth0 up"))<0)
		lcd_draw_font( 10, 62, COLOR_BLACK, COLOR_WHITE, 14 , (unsigned char *)"Wifi init fail");
	system("sleep 1");
	if((status=system("/system/bin/wlarm_android -i eth0 up"))<0)
		lcd_draw_font( 10, 62, COLOR_BLACK, COLOR_WHITE, 14 , (unsigned char *)"Wifi init fail");
	system("sleep 1");
	if((status=system("ifconfig eth0 107.108.218.92"))<0)
		lcd_draw_font( 10, 62, COLOR_BLACK, COLOR_WHITE, 14 , (unsigned char *)"Wifi init fail");
	system("sleep 1");
	if((status=system("/system/bin/wlarm_android scan"))<0)
		lcd_draw_font( 10, 62, COLOR_BLACK, COLOR_WHITE, 14 , (unsigned char *)"Wifi init fail");

	system("sleep 1");
	if((status=system("/system/bin/wlarm_android scanresults > /tmp/wlantest"))<0)
		lcd_draw_font( 10, 62, COLOR_BLACK, COLOR_WHITE, 14 , (unsigned char *)"Wifi Scan fail");
	system("sleep 1");
	if((fptr=fopen("/tmp/wlantest","rt+"))==NULL)
		lcd_draw_font( 10, 62, COLOR_BLACK, COLOR_WHITE, 14, (unsigned char *)"Wifi Scan fail");

	while(fgetc(fptr)==EOF)
	{
		lcd_draw_font( 10, 62, COLOR_BLACK, COLOR_WHITE, 19, (unsigned char *)"Wifi interface fail");
		goto wiout;
	}

	fseek(fptr,0,SEEK_SET);

	lcd_draw_font( 20, 46, COLOR_WHITE, COLOR_RED, 21, (unsigned char *)" Wifi AP Scan Result ");

	fread(buf,sizeof(char),2048,fptr);
	strptr=buf;

	while((ptr=strchr(strptr,'\n'))!=NULL)
	{
		*ptr = 0;
		if((startstr=strchr(strptr,'\"'))!=NULL)
		{
			strptr = (startstr+1);
			if((endstr=strchr(strptr,'\"'))!=NULL)
			{
				*endstr=0;
				lcd_draw_font( 10, 78+i, COLOR_BLACK, COLOR_WHITE, strlen(strptr) < 29 ? strlen(strptr) : 28, (unsigned char *)strptr);
				i+=16;
			}
		}
		strptr = (ptr+1);
	}
	if(i==0){
		lcd_draw_font( 10, 62, COLOR_BLACK, COLOR_WHITE, 16, (unsigned char *)"No Wifi AP found");
	} 
wiout:		
	fclose(fptr);	
	system("rm /tmp/wlantest");
	if((status=system("rmmod dhd"))<0)
		lcd_draw_font( 10, 62, COLOR_BLACK, COLOR_WHITE, 14 , (unsigned char *)"Wifi close fail");
}


void devmgr_status_main(unsigned short scan_code)
{
	int value;
	int max_lcd_str = MAX_LCD_STR;

	if((scan_code == DM_KEY_DPAD_CENTER) || (scan_code == DM_KEY_ENTER))
	{
		test_state = devmgr_current_item;
		switch(test_state) {
			case test_key:
				back_key_click = 0;
				devmgr_test_ShowKeyWindow();
				break;
			case test_touch:
				devmgr_test_ShowDummyWindow("TOUCH TEST");
				break;
			case test_lcd:
				devmgr_test_LcdColor(COLOR_RED);
				break;
			case test_lcd_backlight:
				devmgr_test_ShowDummyWindow("LCD BL TEST");
				lcd_draw_font(50,72,COLOR_BLACK,COLOR_WHITE,20,(unsigned char* )"Press OK & UP & DOWN");
				bl_level = 0;	//initialize bl_level 
				devmgr_test_LcdBackLight(20);
				break;
			case test_lcd_power:
				devmgr_test_ShowDummyWindow("LCD POWER TEST");
				lcd_draw_font(10,72,COLOR_BLACK,COLOR_WHITE,20,(unsigned char* )"  UP : LCD PWR ON ");
				lcd_draw_font(10,88,COLOR_BLACK,COLOR_WHITE,20,(unsigned char* )"DOWN : LCD PWR OFF");
				break;
			case test_led:
				devmgr_test_ShowDummyWindow("LED TEST");
#if defined (KEYPAD_LED) || defined (BUTTON_LED)
				lcd_draw_font(10,72,COLOR_BLACK,COLOR_WHITE,14,(unsigned char* )"  UP : LED ON ");
				lcd_draw_font(10,88,COLOR_BLACK,COLOR_WHITE,14,(unsigned char* )"DOWN : LED OFF");
				devmgr_test_LedOnOff( LED_ON );
#endif
				break;
			case test_camera:
				devmgr_test_CameraPreview();
				break;
			case test_vibrator:
				devmgr_test_ShowDummyWindow("VIBRATOR TEST");
				devmgr_test_VibratorOnOff(1);
				break;
			case test_sound:
				devmgr_test_ShowDummyWindow("SOUND TEST");
				devmgr_test_PlaySound(AUDIO_IN_PDA,AUDIO_OUT_SPEAKER);
				break;
			case test_uart:
				devmgr_test_ShowUartUsbWindow( UART_TEST_NAME, USB_UART_SWITCH );
				value = get_switch_sel(UART);
				if (value) 
					devmgr_test_ShowUartUsbWindow( UART_TEST_NAME, PDA_SWITCH );
				else
					devmgr_test_ShowUartUsbWindow( UART_TEST_NAME, PHONE_SWITCH );
				break;
			case test_usb:
				devmgr_test_ShowUartUsbWindow( USB_TEST_NAME, USB_UART_SWITCH );
				value = get_switch_sel(USB);
				if (value) 
					devmgr_test_ShowUartUsbWindow( USB_TEST_NAME, PDA_SWITCH );
				else
					devmgr_test_ShowUartUsbWindow( USB_TEST_NAME, PHONE_SWITCH );
				break;
			case test_sensor:
				devmgr_test_ShowSensorWindow();
				devmgr_test_sensor();
				break;
			case test_dpram:
				devmgr_test_ShowDummyWindow("DPRAM TEST");
				//devmgr_test_ShowDpramWithPhoneserver(KEY_EXT_SEND);
				//default : Get Status
				break;
			case test_wifi:
				devmgr_test_wifi();
				break;
			case test_bt:
				devmgr_test_bt();
				break;
			case test_sdcard:
				devmgr_test_ShowSDcardWindow();
				break;
			case test_battery:
				devmgr_test_ShowBatteryWindow();
				break;
			case test_version:
				sys_main();
				devmgr_test_ShowVersionWindow();
				break;
			case test_reset:
				devmgr_test_ResetWindow();
				break;
#ifdef TOUCH_FW
			case test_touch_fw:
				devmgr_test_TouchFirmwareWindow();
				break;
#endif
			case test_end:
				devmgr_save_parameters();
				devmgr_test_LcdColor(COLOR_BLACK);
				sleep(1);
				exit(1);
				break;
			default:
				break;
		}//switch
	}//if
	else if(scan_code == DM_KEY_DPAD_UP || scan_code == DM_KEY_VOLUME_UP)
	{
#ifdef USE_INVERTED_FONTS
		if(devmgr_current_item > 0 && devmgr_current_item <= (max_lcd_str-1)) {
			devmgr_current_item--;
			lcd_draw_font(str_lcd[devmgr_current_item+1].x1, \
					str_lcd[devmgr_current_item+1].y1, \
					str_lcd[devmgr_current_item+1].fgc, \
					str_lcd[devmgr_current_item+1].bgc, \
					strlen( (char* )str_lcd[devmgr_current_item+1].data), \
					str_lcd[devmgr_current_item+1].data);
			lcd_draw_font(str_lcd[devmgr_current_item].x1, \
					str_lcd[devmgr_current_item].y1, \
					str_lcd[devmgr_current_item].bgc, \
					str_lcd[devmgr_current_item].fgc, \
					strlen( (char* )str_lcd[devmgr_current_item].data), \
					str_lcd[devmgr_current_item].data);
		}
#endif
	}
	else if(scan_code == DM_KEY_DPAD_DOWN || scan_code == DM_KEY_VOLUME_DOWN)
	{
#ifdef USE_INVERTED_FONTS
		if(devmgr_current_item >= 0 && devmgr_current_item < (max_lcd_str-1))
			devmgr_current_item++;
		lcd_draw_font(str_lcd[devmgr_current_item-1].x1, \
				str_lcd[devmgr_current_item-1].y1, \
				str_lcd[devmgr_current_item-1].fgc, \
				str_lcd[devmgr_current_item-1].bgc, \
				strlen( (char* )str_lcd[devmgr_current_item-1].data), \
				str_lcd[devmgr_current_item-1].data);
		lcd_draw_font(str_lcd[devmgr_current_item].x1, \
				str_lcd[devmgr_current_item].y1, \
				str_lcd[devmgr_current_item].bgc, \
				str_lcd[devmgr_current_item].fgc, \
				strlen( (char* )str_lcd[devmgr_current_item].data), \
				str_lcd[devmgr_current_item].data);
#endif
	}
}


void devmgr_status_key(unsigned int scan_code)
{
	int i,key_color;
	char str_key[15];
	int max_key_num = MAX_KEY_NUM;
	memset(str_key, 0x00, sizeof(str_key));

	if((back_key_click == 1) && (scan_code == DM_KEY_BACK))
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_key);
	}
	else
	{
		for(i=0;i<max_key_num;i++){
			if( scan_code==key_info[i].index) {
				key_color=COLOR_BLACK;
				if(scan_code == DM_KEY_BACK)
					back_key_click = 1;
#ifdef QWERTY_KEY
				if(scan_code == DM_KEY_FOCUS) 
					key_color=COLOR_RED;
				lcd_FilledRectangle(key_info[i].x, key_info[i].y, key_info[i].x + key_info[i].w, key_info[i].y + key_info[i].h,key_color);
				lcd_draw_font(140,100,COLOR_WHITE,COLOR_BLACK,10,key_info[i].data);
#endif
#ifdef NON_QWERTY_KEY
				if(scan_code == DM_KEY_FOCUS) 
					key_color=COLOR_RED;
				lcd_FilledRectangle(key_info[i].x, key_info[i].y, key_info[i].x + key_info[i].w, key_info[i].y + key_info[i].h,key_color);
				lcd_draw_font(100,100,COLOR_WHITE,COLOR_BLACK,10,key_info[i].data);
#endif
			}
		}
	}
}

void devmgr_status_touch(unsigned int scan_code)
{
	if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_touch);
	}
}

void devmgr_status_lcd(unsigned int scan_code)
{
	if((scan_code == DM_KEY_DPAD_CENTER)||(scan_code == DM_KEY_ENTER))
	{
		devmgr_test_LcdColor(COLOR_RED);
	}
	else if((scan_code == DM_KEY_DPAD_UP) || (scan_code == DM_KEY_VOLUME_UP))
	{
		devmgr_test_LcdColor(COLOR_GREEN);
	}
	else if((scan_code == DM_KEY_DPAD_DOWN)|| (scan_code == DM_KEY_VOLUME_DOWN))
	{
		devmgr_test_LcdColor(COLOR_BLUE);
	}
	else if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_lcd);
	}
}


void devmgr_status_lcd_backlight(unsigned int scan_code)
{
	if((scan_code == DM_KEY_DPAD_CENTER)||(scan_code == DM_KEY_ENTER))
	{
		bl_level=(bl_level+1)%8; 
		switch(bl_level)
		{
			case 0:
				backlight_level = 20;
				break;
			case 1:
				backlight_level = 40;
				break;
			case 2:
				backlight_level = 70;
				break;
			case 3:
				backlight_level = 100;
				break;
			case 4:
				backlight_level = 130;
				break;
			case 5:
				backlight_level = 170;
				break;
			case 6:
				backlight_level = 200;
				break;
			case 7:
				backlight_level = 230;
				break;
			default:
				backlight_level = 20;
				break;
		}
		devmgr_test_LcdBackLight(backlight_level);
	}
	else if(scan_code == DM_KEY_DPAD_UP || scan_code == DM_KEY_VOLUME_UP)
	{
		if(backlight_level != 255)
			backlight_level++;
		devmgr_test_LcdBackLight(backlight_level);
	}
	else if(scan_code == DM_KEY_DPAD_DOWN || scan_code == DM_KEY_VOLUME_DOWN)
	{
		if(backlight_level != 1)
			backlight_level--;
		devmgr_test_LcdBackLight(backlight_level);
	}
	else if(scan_code == DM_KEY_BACK)
	{
		devmgr_test_LcdPower(LCD_ON);
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_lcd_backlight);
	}
}


void devmgr_status_lcd_power(unsigned int scan_code)
{
	if(scan_code == DM_KEY_DPAD_UP || scan_code == DM_KEY_VOLUME_UP)
	{
		devmgr_test_LcdPower(LCD_ON);
	}
	else if(scan_code == DM_KEY_DPAD_DOWN || scan_code == DM_KEY_VOLUME_DOWN)
	{
		devmgr_test_LcdPower(LCD_OFF);
	}
	else if(scan_code == DM_KEY_BACK)
	{
		devmgr_test_LcdPower(LCD_ON);
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_lcd_power);
	}
}

void devmgr_status_led(unsigned int scan_code)
{
	if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_LedOnOff( LED_OFF );
		devmgr_test_ShowResult(1,0,test_led);
	}
	else if(scan_code == DM_KEY_DPAD_UP || scan_code == DM_KEY_VOLUME_UP)
	{
		devmgr_test_LedOnOff( LED_ON );
	}
	else if(scan_code == DM_KEY_DPAD_DOWN || scan_code == DM_KEY_VOLUME_DOWN)
	{
		devmgr_test_LedOnOff( LED_OFF );
	}
}

void devmgr_status_camera(unsigned int scan_code)
{
	if(scan_code == DM_KEY_BACK)
	{
		finish = 1;
		test_state = test_main;
		if(pthread_join(t_camera_preview, NULL)==0)
			gprintf("pthread_join : t_camera_previce\n");
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_camera);

	}
}

void devmgr_status_vibrator(unsigned int scan_code)
{
	if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_VibratorOnOff(0);
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_vibrator);
	}
}

void devmgr_status_sound(unsigned int scan_code)
{
	if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_sound);
	}
	else if((scan_code == DM_KEY_DPAD_CENTER)||(scan_code == DM_KEY_ENTER))
	{
		devmgr_test_PlaySound(AUDIO_IN_PDA,AUDIO_OUT_SPEAKER);
	}
}

void devmgr_status_sensor(unsigned int scan_code)
{
	if(scan_code == DM_KEY_BACK)
	{
		b_sensor_terminated = true;
		if(!pthread_join(t_acc_sensors, NULL))
			gprintf("pthread_join acc\n");

		if(!pthread_join(t_opt_sensors, NULL))
			gprintf("pthread_join opt\n");

		if(!pthread_join(t_mgn_sensors_cmd, NULL))
			gprintf("pthread_join mgn_cmd\n");
		
		if(!pthread_join(t_mgn_sensors_poll, NULL))
			gprintf("pthread_join mgn_poll\n");
		

		devmgr_test_LcdBackLight(LCD_ON);
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_sensor);
	}
}


void devmgr_status_uart(unsigned int scan_code)
{
	if(scan_code == DM_KEY_DPAD_UP || scan_code == DM_KEY_VOLUME_UP)
	{
		devmgr_test_ShowUartUsbWindow( UART_TEST_NAME, PDA_SWITCH );
		set_switch_sel_for_uart(PDA);
	}
	else if(scan_code == DM_KEY_DPAD_DOWN || scan_code == DM_KEY_VOLUME_DOWN)
	{
		devmgr_test_ShowUartUsbWindow( UART_TEST_NAME, PHONE_SWITCH );
		set_switch_sel_for_uart(MODEM);
	}
	else if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_uart);
	}
}

void devmgr_status_usb(unsigned int scan_code)
{
	if(scan_code == DM_KEY_DPAD_UP || scan_code == DM_KEY_VOLUME_UP)
	{
			devmgr_test_ShowUartUsbWindow( USB_TEST_NAME, PDA_SWITCH );
			set_switch_sel_for_usb(PDA);
	}
	else if(scan_code == DM_KEY_DPAD_DOWN || scan_code == DM_KEY_VOLUME_DOWN)
	{
			devmgr_test_ShowUartUsbWindow( USB_TEST_NAME, PHONE_SWITCH );
			set_switch_sel_for_usb(MODEM);
	}
	else if(scan_code == DM_KEY_BACK)
	{
			test_state = test_main;
			devmgr_test_ShowMainWindow();
			devmgr_test_ShowResult(1,0,test_usb);
	}
}

void devmgr_status_dpram(unsigned int scan_code)
{
	if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_dpram);
	}
}

void devmgr_status_reset(unsigned int scan_code)
{
	if(scan_code == DM_KEY_DPAD_UP || scan_code == DM_KEY_VOLUME_UP)
	{
		system("reboot");
	}
	else if(scan_code == DM_KEY_DPAD_DOWN || scan_code == DM_KEY_VOLUME_DOWN)
	{
		devmgr_save_parameters();
		sleep(1);
		system("reboot");
	}
	else if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
	}
}

void devmgr_status_touch_fw(unsigned int scan_code)
{
	if((scan_code == DM_KEY_DPAD_CENTER)||(scan_code == DM_KEY_ENTER))
	{
		lcd_draw_font(10, 158, COLOR_WHITE, COLOR_GRAY, 18, (unsigned char *)"FW update ........");
		system("echo \"update\" > /sys/class/sec/ts/firmware");
		Show_TouchFirmwareVirsion();
		lcd_draw_font(10, 158, COLOR_WHITE, COLOR_RED, 18, (unsigned char *)"FW update complete");
	}
	else if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
	}
}

void devmgr_status_version(unsigned int scan_code)
{
	if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
	}
}


void devmgr_status_battery(unsigned int scan_code)
{
	if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
	}
}

void devmgr_status_sdcard(unsigned int scan_code)
{
	if((scan_code == DM_KEY_DPAD_CENTER)||(scan_code == DM_KEY_ENTER))
	{
		devmgr_test_sdcard();
	}
	else if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_sdcard);
	}
}

void devmgr_status_bt(unsigned int scan_code)
{
	if((scan_code == DM_KEY_DPAD_CENTER)||(scan_code == DM_KEY_ENTER))
	{
		devmgr_test_bt();
	}
	else if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_bt);
	}
	
}
void devmgr_status_wifi(unsigned int scan_code)
{
	if((scan_code == DM_KEY_DPAD_CENTER)||(scan_code == DM_KEY_ENTER))
	{
		devmgr_test_wifi();
	}
	else if(scan_code == DM_KEY_BACK)
	{
		test_state = test_main;
		devmgr_test_ShowMainWindow();
		devmgr_test_ShowResult(1,0,test_wifi);
	}
}
void devmgr_test_proc_keypad_event(char* buf)
{
	input_event_t rcv_msg;
	unsigned short scancode;

	memcpy( &rcv_msg, buf, sizeof(input_event_t) );	
	printf("scan_code : %x\n", rcv_msg.code);
	scancode = keyname[rcv_msg.code];

	if ( rcv_msg.value == 1 ) { /*when key is pressed,..*/

		switch(test_state) {
			case test_main: 			return devmgr_status_main(scancode);
			case test_key: 				return devmgr_status_key(scancode);
			case test_touch: 			return devmgr_status_touch(scancode);
			case test_lcd: 				return devmgr_status_lcd(scancode);
			case test_lcd_backlight: 	return devmgr_status_lcd_backlight(scancode);
			case test_lcd_power: 		return devmgr_status_lcd_power(scancode);
			case test_led: 				return devmgr_status_led(scancode);
			case test_camera:			return devmgr_status_camera(scancode);
			case test_vibrator: 		return devmgr_status_vibrator(scancode);
			case test_sound: 			return devmgr_status_sound(scancode);
			case test_sensor:		    return devmgr_status_sensor(scancode);
			case test_uart: 			return devmgr_status_uart(scancode);
			case test_usb: 				return devmgr_status_usb(scancode);
			case test_dpram:			return devmgr_status_dpram(scancode);
			case test_wifi:				return devmgr_status_wifi(scancode);
			case test_bt:				return devmgr_status_bt(scancode);
			case test_sdcard:			return devmgr_status_sdcard(scancode);
			case test_battery:			return devmgr_status_battery(scancode);
			case test_version:			return devmgr_status_version(scancode);
			case test_reset:			return devmgr_status_reset(scancode);
#ifdef TOUCH_FW
			case test_touch_fw:			return devmgr_status_touch_fw(scancode);
#endif
			case test_end: 				
			default:					
				break;
		}			
	}
}


void devmgr_test_proc_touch_event(char* buf, int read_count)
{
	static unsigned int old_press = 0;

	static unsigned int cur_x = 0;
	static unsigned int cur_y = 0;
	static unsigned int cur_press = 0;

	static unsigned int save_x = 0;
	static unsigned int save_y = 0;

	input_event_t rcv_msg;
	int data_num, i;

	data_num = read_count / sizeof( input_event_t );

	i=0;	
	for( i = 0; i < data_num; i++ )
	{
		memcpy(&rcv_msg, &buf[( i*sizeof( input_event_t ) )], sizeof( input_event_t ) );
		switch(test_state) {
			case test_touch:
				switch (rcv_msg.type) 
				{
					case EV_SYN:
					#if 0
						if(!old_press && cur_press)
						{
							gprintf("old 0 cur 1\n");
							save_x = cur_x;
							save_y = cur_y;
						}
						else if(old_press && !cur_press)
						{
							gprintf("old 1 cur 0\n");
							lcd_Line(save_x,save_y,cur_x,cur_y,COLOR_RED);

						}
						else if(cur_press)
						{
							gprintf("cur 1\n");
							if(abs(cur_x-save_x)>2 || abs(cur_y-save_y)>2)
							{
								gprintf("\n");
								lcd_Line(save_x,save_y,cur_x,cur_y,COLOR_RED);
								save_x = cur_x;
								save_y = cur_y;
							}
						}
					#endif
						if(cur_press)
						{
							if(abs(cur_x-save_x)>2 || abs(cur_y-save_y)>2)
							{
								lcd_Line(save_x,save_y,cur_x,cur_y,COLOR_RED);
								save_x = cur_x;
								save_y = cur_y;
							}
						}
						break;
					case EV_ABS:
						switch(rcv_msg.code) {
							case ABS_X:
								cur_x = rcv_msg.value;
								if(cur_x > PANEL_W-2)
									cur_x = PANEL_W-2; 
								break;
							case ABS_Y:
								cur_y = rcv_msg.value;
								if(cur_y > PANEL_H-2)
									cur_y = PANEL_H-2; 
								break;
							#if 0
							case ABS_PRESURE:
								gprintf("\n");
								old_press = cur_press;
								cur_press = rcv_msg.value;
								break;
							#endif
						}//switch
						break;
					case EV_KEY:
						switch(rcv_msg.code){
							case BTN_TOUCH:
								old_press = cur_press;
								cur_press = rcv_msg.value;
								if(cur_press)
								{
									save_x = cur_x;
									save_y = cur_y;
								}
								else
								{
									if(abs(cur_x-save_x)>2 || abs(cur_y-save_y)>2)
									{
										lcd_Line(save_x,save_y,cur_x,cur_y,COLOR_RED);
									}
								}
								break;
						}
						break;
					default:
						break;
				}//switch
				break;
			default:
				break;
		}//switch
	}
}

static void mark_keyExist_using_keyName(char keyName)
{
	int max_key_num = MAX_KEY_NUM;
	int i;

	for(i=0;i<max_key_num;i++) 
	{
		if(key_info[i].index == keyName) 
		{
			key_info[i].exist = KEY_EXIST;
		}
	}
}

static void set_key_value(char *name, int value)
{
		 if(!strcmp(name,"DPAD_UP"		)) {keyname[value]=DM_KEY_DPAD_UP		;mark_keyExist_using_keyName(DM_KEY_DPAD_UP		);}		 
	else if(!strcmp(name,"DPAD_DOWN"	)) {keyname[value]=DM_KEY_DPAD_DOWN		;mark_keyExist_using_keyName(DM_KEY_DPAD_DOWN	);}
	else if(!strcmp(name,"DPAD_LEFT"	)) {keyname[value]=DM_KEY_DPAD_LEFT		;mark_keyExist_using_keyName(DM_KEY_DPAD_LEFT	);}	
	else if(!strcmp(name,"DPAD_RIGHT"	)) {keyname[value]=DM_KEY_DPAD_RIGHT	;mark_keyExist_using_keyName(DM_KEY_DPAD_RIGHT	);}	
	else if(!strcmp(name,"DPAD_CENTER"	)) {keyname[value]=DM_KEY_DPAD_CENTER	;mark_keyExist_using_keyName(DM_KEY_DPAD_CENTER	);}	
	else if(!strcmp(name,"VOLUME_UP"	)) {keyname[value]=DM_KEY_VOLUME_UP		;mark_keyExist_using_keyName(DM_KEY_VOLUME_UP	);}	
	else if(!strcmp(name,"VOLUME_DOWN"	)) {keyname[value]=DM_KEY_VOLUME_DOWN	;mark_keyExist_using_keyName(DM_KEY_VOLUME_DOWN	);}
	else if(!strcmp(name,"MENU"			)) {keyname[value]=DM_KEY_MENU			;mark_keyExist_using_keyName(DM_KEY_MENU		);}
	else if(!strcmp(name,"HOME"			)) {keyname[value]=DM_KEY_HOME			;mark_keyExist_using_keyName(DM_KEY_HOME		);}
	else if(!strcmp(name,"BACK"			)) {keyname[value]=DM_KEY_BACK			;mark_keyExist_using_keyName(DM_KEY_BACK		);}
	else if(!strcmp(name,"CALL"			)) {keyname[value]=DM_KEY_CALL			;mark_keyExist_using_keyName(DM_KEY_CALL		);}
	else if(!strcmp(name,"CAMERA"		)) {keyname[value]=DM_KEY_CAMERA		;mark_keyExist_using_keyName(DM_KEY_CAMERA		);}
	else if(!strcmp(name,"FOCUS"		)) {keyname[value]=DM_KEY_FOCUS			;mark_keyExist_using_keyName(DM_KEY_FOCUS		);}
	else if(!strcmp(name,"ENDCALL"		)) {keyname[value]=DM_KEY_ENDCALL		;mark_keyExist_using_keyName(DM_KEY_ENDCALL		);}  
	else if(!strcmp(name,"Q"			)) {keyname[value]=DM_KEY_Q				;mark_keyExist_using_keyName(DM_KEY_Q			);}
	else if(!strcmp(name,"W"			)) {keyname[value]=DM_KEY_W				;mark_keyExist_using_keyName(DM_KEY_W			);}
	else if(!strcmp(name,"E"			)) {keyname[value]=DM_KEY_E				;mark_keyExist_using_keyName(DM_KEY_E			);}
	else if(!strcmp(name,"R"			)) {keyname[value]=DM_KEY_R				;mark_keyExist_using_keyName(DM_KEY_R			);}
	else if(!strcmp(name,"T"			)) {keyname[value]=DM_KEY_T				;mark_keyExist_using_keyName(DM_KEY_T			);}
	else if(!strcmp(name,"Y"			)) {keyname[value]=DM_KEY_Y				;mark_keyExist_using_keyName(DM_KEY_Y			);}
	else if(!strcmp(name,"U"			)) {keyname[value]=DM_KEY_U				;mark_keyExist_using_keyName(DM_KEY_U			);}
	else if(!strcmp(name,"I"			)) {keyname[value]=DM_KEY_I				;mark_keyExist_using_keyName(DM_KEY_I			);}
	else if(!strcmp(name,"O"			)) {keyname[value]=DM_KEY_O				;mark_keyExist_using_keyName(DM_KEY_O			);}
	else if(!strcmp(name,"P"			)) {keyname[value]=DM_KEY_P				;mark_keyExist_using_keyName(DM_KEY_P			);}
	else if(!strcmp(name,"A"			)) {keyname[value]=DM_KEY_A				;mark_keyExist_using_keyName(DM_KEY_A			);}
	else if(!strcmp(name,"S"			)) {keyname[value]=DM_KEY_S				;mark_keyExist_using_keyName(DM_KEY_S			);}
	else if(!strcmp(name,"D"			)) {keyname[value]=DM_KEY_D				;mark_keyExist_using_keyName(DM_KEY_D			);}
	else if(!strcmp(name,"F"			)) {keyname[value]=DM_KEY_F				;mark_keyExist_using_keyName(DM_KEY_F			);}
	else if(!strcmp(name,"G"			)) {keyname[value]=DM_KEY_G				;mark_keyExist_using_keyName(DM_KEY_G			);}
	else if(!strcmp(name,"H"			)) {keyname[value]=DM_KEY_H				;mark_keyExist_using_keyName(DM_KEY_H			);}
	else if(!strcmp(name,"J"			)) {keyname[value]=DM_KEY_J				;mark_keyExist_using_keyName(DM_KEY_J			);}
	else if(!strcmp(name,"K"			)) {keyname[value]=DM_KEY_K				;mark_keyExist_using_keyName(DM_KEY_K			);}
	else if(!strcmp(name,"L"			)) {keyname[value]=DM_KEY_L				;mark_keyExist_using_keyName(DM_KEY_L			);}
	else if(!strcmp(name,"Z"			)) {keyname[value]=DM_KEY_Z				;mark_keyExist_using_keyName(DM_KEY_Z			);}
	else if(!strcmp(name,"X"			)) {keyname[value]=DM_KEY_X				;mark_keyExist_using_keyName(DM_KEY_X			);}
	else if(!strcmp(name,"C"			)) {keyname[value]=DM_KEY_C				;mark_keyExist_using_keyName(DM_KEY_C			);}
	else if(!strcmp(name,"V"			)) {keyname[value]=DM_KEY_V				;mark_keyExist_using_keyName(DM_KEY_V			);}
	else if(!strcmp(name,"B"			)) {keyname[value]=DM_KEY_B				;mark_keyExist_using_keyName(DM_KEY_B			);}
	else if(!strcmp(name,"N"			)) {keyname[value]=DM_KEY_N				;mark_keyExist_using_keyName(DM_KEY_N			);}
	else if(!strcmp(name,"M"			)) {keyname[value]=DM_KEY_M				;mark_keyExist_using_keyName(DM_KEY_M			);}
	else if(!strcmp(name,"ALT_LEFT"		)) {keyname[value]=DM_KEY_ALT_LEFT		;mark_keyExist_using_keyName(DM_KEY_ALT_LEFT	);}
	else if(!strcmp(name,"ENVELOPE"		)) {keyname[value]=DM_KEY_ENVELOPE		;mark_keyExist_using_keyName(DM_KEY_ENVELOPE	);}
	else if(!strcmp(name,"SHIFT_LEFT"	)) {keyname[value]=DM_KEY_SHIFT_LEFT	;mark_keyExist_using_keyName(DM_KEY_SHIFT_LEFT	);}
	else if(!strcmp(name,"SYM"			)) {keyname[value]=DM_KEY_SYM			;mark_keyExist_using_keyName(DM_KEY_SYM			);}  
	else if(!strcmp(name,"1"			)) {keyname[value]=DM_KEY_1				;mark_keyExist_using_keyName(DM_KEY_1			);}
	else if(!strcmp(name,"2"			)) {keyname[value]=DM_KEY_2				;mark_keyExist_using_keyName(DM_KEY_2			);}
	else if(!strcmp(name,"3"			)) {keyname[value]=DM_KEY_3				;mark_keyExist_using_keyName(DM_KEY_3			);}
	else if(!strcmp(name,"4"			)) {keyname[value]=DM_KEY_4				;mark_keyExist_using_keyName(DM_KEY_4			);}
	else if(!strcmp(name,"5"			)) {keyname[value]=DM_KEY_5				;mark_keyExist_using_keyName(DM_KEY_5			);}
	else if(!strcmp(name,"6"			)) {keyname[value]=DM_KEY_6				;mark_keyExist_using_keyName(DM_KEY_6			);}
	else if(!strcmp(name,"7"			)) {keyname[value]=DM_KEY_7				;mark_keyExist_using_keyName(DM_KEY_7			);}
	else if(!strcmp(name,"8"			)) {keyname[value]=DM_KEY_8				;mark_keyExist_using_keyName(DM_KEY_8			);}
	else if(!strcmp(name,"9"			)) {keyname[value]=DM_KEY_9				;mark_keyExist_using_keyName(DM_KEY_9			);}
	else if(!strcmp(name,"0"			)) {keyname[value]=DM_KEY_0				;mark_keyExist_using_keyName(DM_KEY_0			);}
	else if(!strcmp(name,"SPACE"		)) {keyname[value]=DM_KEY_SPACE			;mark_keyExist_using_keyName(DM_KEY_SPACE		);}
	else if(!strcmp(name,"PERIOD"		)) {keyname[value]=DM_KEY_PERIOD		;mark_keyExist_using_keyName(DM_KEY_PERIOD		);}
	else if(!strcmp(name,"DEL"			)) {keyname[value]=DM_KEY_DEL			;mark_keyExist_using_keyName(DM_KEY_DEL			);}
	else if(!strcmp(name,"ENTER"		)) {keyname[value]=DM_KEY_ENTER			;mark_keyExist_using_keyName(DM_KEY_ENTER		);}
	else if(!strcmp(name,"COMMA"		)) {keyname[value]=DM_KEY_COMMA			;mark_keyExist_using_keyName(DM_KEY_COMMA		);}
	else if(!strcmp(name,"HOLD"			)) {keyname[value]=DM_KEY_HOLD			;mark_keyExist_using_keyName(DM_KEY_HOLD		);}
	else if(!strcmp(name,"SEARCH"		)) {keyname[value]=DM_KEY_SEARCH		;mark_keyExist_using_keyName(DM_KEY_SEARCH		);}
	else
		gprintf("wrong key value! %s, %d\n",name,value);
}
																																																					   

static int devmgr_load_keymap(void)
{
	FILE *fptr 		= NULL;
	char *ptr 		= NULL;
	char *strptr 	= NULL;
	char *endptr 	= NULL;
	char rev[64];
	char linebuf[64];
	int index = 0;
	int pos;

	system("cat /proc/cpuinfo > /tmp/cpuinfo");
	usleep(500000);
	if( (fptr = fopen("/tmp/cpuinfo", "rt+")) == NULL )
	{
		gprintf("/temp/cpuinfo open failed\n");
		return -1;
	}
	while(1)
	{
		
		if (NULL == fgets(linebuf, sizeof(linebuf), fptr))
			break;
		if ((strptr = strstr(linebuf, "Revision")) != NULL)
		{
			if((ptr = strchr(strptr, '0')) != NULL)
			{
				endptr = ptr + 4;
				*endptr = 0;
				strptr = ptr;
				//sprintf(rev, "/system/usr/keylayout/s3c-keypad-rev%s.kl",strptr);
				sprintf(rev, "/tmp/s3c-keypad-rev%s.kl",strptr);
			}
		}
	}
	fclose(fptr);

	fptr	= NULL;
	ptr		= NULL;
	strptr 	= NULL;
	endptr 	= NULL;
	
	if( (fptr = fopen(rev, "rt+")) == NULL )
	{
		gprintf("%s open failed\n", rev);
		return -1;
	}
	printf("\n======%s=======\n",rev);
	while(1)
	{
		if (NULL == fgets(linebuf, sizeof(linebuf), fptr))
			break;		

		if( ( ptr = strchr(linebuf, '#') ) == NULL )
		{
			if( (strptr = strstr(linebuf, "key")) != NULL )
			{
				strptr += 4;

				endptr = strchr(strptr, ' ');
				*endptr = 0;
				index = atoi(strptr);
				strptr = endptr + 1;
			
				pos = 0;
				while( strptr[pos]==' ')
					pos++;
				strptr += pos;

				pos = 0;	
				while( strptr[pos]>='0' && strptr[pos]<='_')
					pos++;
				endptr = strptr + pos;
				*endptr = 0;

				//strcpy(keyname[index], strptr);
				printf("%s(%d) : %d\n",strptr,strlen(strptr),index);

				set_key_value(strptr, index);
			}
		}
	}

	printf("======================================================\n");
	fclose(fptr);
	return 0;
}


void devmgr_test_main(void)
{
	int n, i, count;
	struct pollfd pfds[5];
	unsigned char buf[2048];
	input_event_t rcv_msg; //for TOUCH_KEY
	char evnum[30];	//for OPTICAL_JOYSTICK

	devmgr_load_keymap();

	pfds[0].fd = fd_key;
	pfds[0].events = POLLIN|POLLPRI;
	pfds[1].fd = fd_touch;
	pfds[1].events = POLLIN|POLLPRI;

	#if defined (OPTICAL_JOYSTICK)
	sprintf(evnum,"/dev/input/%s",get_event_number("optjoy_device"));
	gprintf("%s optjoy_device\n",evnum);
	pfds[2].fd = open(evnum, O_RDWR);
	pfds[2].events = POLLIN|POLLPRI;
	#elif defined (HALL_MOUSE)
	sprintf(evnum,"/dev/input/%s",get_event_number("hallmouse"));
	gprintf("%s hallmouse\n",evnum);
	pfds[2].fd = open(evnum, O_RDWR);
	pfds[2].events = POLLIN|POLLPRI;
	#endif

	/* temp code */
	system("mount -t vfat /dev/block/mmcblk0p1 /sdcard");
	
	devmgr_test_ShowMainWindow();

	for(;;) {
		memset(buf,0x00,1024);
		
		#if (defined (OPTICAL_JOYSTICK) || defined (HALL_MOUSE))
		n = poll(pfds,MAX_DEVICE_NUM+1,-1);			/* blocking mode */
		switch(n) 
		{
			case -1:								/*error*/
				perror("poll(keytest)");
				break;
			case 0:									/*time out*/
				gprintf("time out\n");
				break;
			default:								/* valid input */
				for(i=0;i<MAX_DEVICE_NUM+1;i++) 
				{
					if(pfds[i].revents & (POLLIN|POLLPRI)) 
					{
						count = read( pfds[i].fd, (char*) buf, sizeof(input_event_t));
						memcpy(&rcv_msg, &buf, sizeof( input_event_t ) );
						//gprintf("i= %d, type = %d, code = %d, value =%d\n", i, rcv_msg.type, rcv_msg.code, rcv_msg.value);

						if(i==0)
						{
							devmgr_test_proc_keypad_event( (char* )buf);	
						}
						if(i==1)
						{
							#ifdef TOUCH_KEY
							if((rcv_msg.code == KEY_BACK)||(rcv_msg.code == KEY_HOME)||(rcv_msg.code == KEY_MENU))
							{
							//	gprintf("TOUCH_KEY code = %d\n", rcv_msg.code);
								devmgr_test_proc_keypad_event( (char* )buf);
							}
							else
							#endif
								devmgr_test_proc_touch_event( (char*) buf, count);
						}
						if(i==2)
						{
							//gprintf("NAVI_KEY code = %d\n", rcv_msg.code);
							devmgr_test_proc_keypad_event( (char* )buf);	
						}
					}
				}
				break;
		}
		#else
		n = poll(pfds,MAX_DEVICE_NUM,-1);			/* blocking mode */
		switch(n) 
		{
			case -1:								/*error*/
				perror("poll(keytest)");
				break;
			case 0:									/*time out*/
				gprintf("time out\n");
				break;
			default:								/* valid input */
				for(i=0;i<MAX_DEVICE_NUM;i++) 
				{
					if(pfds[i].revents & (POLLIN|POLLPRI)) 
					{
						if(i==0)
						{
							count = read( pfds[i].fd, (char*) buf, sizeof(input_event_t)*64 );
							devmgr_test_proc_keypad_event( (char* )buf);	
						}
						if(i==1){
							count = read(pfds[i].fd,(char *)buf,sizeof(input_event_t)*64 );
							#ifdef TOUCH_KEY
							memcpy(&rcv_msg, &buf[( i*sizeof( input_event_t ) )], sizeof( input_event_t ) );
							if((rcv_msg.code == KEY_BACK)||(rcv_msg.code == KEY_HOME)||(rcv_msg.code == KEY_MENU))
							{
								gprintf("TOUCH_KEY code = %d\n", rcv_msg.code);
								devmgr_test_proc_keypad_event( (char* )buf);
							}
							else
							#endif
								devmgr_test_proc_touch_event( (char*) buf, count);
						}
					}
				}
				break;
		}
		#endif
	}
}

void ov1_set_var(int fd, int width, int height, int bpp)
{

	int ret;
	struct fb_var_screeninfo fbvar;

	// base setting
	fbvar.xoffset = 0;
	fbvar.yoffset = 0;
	fbvar.bits_per_pixel = bpp;
	fbvar.grayscale = 0;
	fbvar.red.offset = 10;
	fbvar.red.length = 5;
	fbvar.red.msb_right = 0;
	fbvar.green.offset = 5;
	fbvar.green.length = 6;
	fbvar.green.msb_right = 0;
	fbvar.blue.offset = 0;
	fbvar.blue.length = 5;
	fbvar.blue.msb_right = 0;
	fbvar.transp.offset = 15;
	fbvar.transp.length = 1;
	fbvar.transp.msb_right = 0;
	fbvar.xoffset = 0;
	fbvar.nonstd = 0;
	fbvar.activate = 0;
	fbvar.height = -1;
	fbvar.width = -1;
	fbvar.accel_flags = 0;
	fbvar.pixclock = 110000;
	fbvar.left_margin = 12;
	fbvar.right_margin = 14;
	fbvar.upper_margin = 5;
	fbvar.lower_margin = 6;
	fbvar.hsync_len = 4;
	fbvar.vsync_len = 4;
	fbvar.sync = 3;
	fbvar.vmode = 0;
	fbvar.rotate = 0;
	fbvar.reserved[0] = 0;
	fbvar.reserved[1] = 0;
	fbvar.reserved[2] = 0;
	fbvar.reserved[3] = 0;
	fbvar.reserved[4] = 0;

	// resoultion 
	fbvar.xres = width;
	fbvar.xres_virtual = width;
	fbvar.yres_virtual = height;
	fbvar.nonstd = 0;

	ret = ioctl(fd, FBIOPUT_VSCREENINFO, &fbvar);
	if(ret < 0)
		perror("fbdev ioctl(FB SET_VAR)");
}

void devmgr_set_overlay1_transparency(int trans, int xpos, int ypos, int width, int height, int bpp)
{
	int fd, i, j;
	unsigned char *fbdata;

	if(!(bpp == 19 || bpp == 25)) {
		printf("not supported bpp values.\n");
		return;
	}

	fd = open("/dev/fb1", O_RDWR);
	if(fd < 0)
		perror("open error");

	ov1_set_var(fd, PANEL_W, PANEL_H, bpp);
	fbdata = (unsigned char *)mmap(0, PANEL_W*PANEL_H*((bpp + 7) / 8), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

//	memset(fbdata, 0, PANEL_W*PANEL_H*((bpp + 7) / 8));

	for(i = xpos; i < xpos+width; i++)
		for(j = ypos; j < ypos+height; j++) {
			*((int*)fbdata + (j*PANEL_W + i)) = 0xffffffff;
		}
	for(i = xpos; i < xpos+width; i++)
		for(j = ypos; j < ypos+height; j++) {
			if(bpp == 19) {
				*(fbdata + (j*PANEL_W + i) * 3 + 2) = trans << 2;
			//*(fbdata + i * 3) = 0xff;
			} else if(bpp == 25) {
				*(fbdata + (j*PANEL_W + i) * 4 + 3) = trans;
			}
	}

	close(fd);
}



