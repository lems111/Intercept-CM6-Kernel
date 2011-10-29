/****************************************************************************
**
** COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2005-2009 ALL RIGHTS RESERVED
**
** AUTHOR		: KyoungHOON Kim (khoonk)
**
*****************************************************************************
** 2006/07/18. khoonk	1.add IOC_DCM_START for usding apm_bios event sync 
** 2006/07/20. jhmin	2.add BATTERY_EVENT for supporting battery and
** 						  charger events.
** 2006/10/19. jhmin    3.add suspend/resume function for power management
** 2006/11/10. jhmin	4.add suspend/resume and suspend timeout setting
*****************************************************************************/
#include <unistd.h>
#include <linux/apm_bios.h>
//#include <linux/hrtimer.h>
#include <linux/delay.h>

#include "devmgr_main.h"
#include "devmgr_ext.h"



/****************************************************************************
** global variables
*****************************************************************************/
typedef enum	_sys_state {
	sys_full_woken_up,
	sys_half_woken_up,
	sys_suspended,
} sys_state;

sys_state prev_ss = sys_full_woken_up;
sys_state curr_ss = sys_full_woken_up;

typedef enum	_call_state {
	call_idle_idle,
	call_idle_rx,
	call_conv_idle,
	call_conv_rx,
} call_state;

call_state prev_cs = call_idle_idle;
call_state curr_cs = call_idle_idle;


char *wakeup_source_list[] = {
	"UNKNOWN_WAKEUP",
	"POWER_BUTTON_WAKEUP",
	"DPRAM_WAKEUP",
	"KEYPAD_WAKEUP",
	"GPIO_RESET_WAKEUP",
	"FOLDER_WAKEUP",
	"JACK_INT_WAKEUP",
	"JACK_KEY_WAKEUP",
};

int wakeup_source = UNKNOWN_WAKEUP;

//#define TIME_LOG 

#ifdef TIME_LOG 
#define TIME_LOG 	struct timeval tv;\
				   	gettimeofday(&tv, NULL);\
				   	fprintf( stderr,"[%s][%d] time=[%f]\n", __FUNCTION__, __LINE__,\
					(float)tv.tv_sec + (float)tv.tv_usec/1000000L ); 
#else 
#define TIME_LOG  
#endif




/****************************************************************************
** functions
*****************************************************************************/
static void devmgr_insert_char(size_t l_margin,size_t r_margin,size_t len,char c)
{
	int i, num, data;
	data = TOTAL_DISP_LEN - r_margin - l_margin;
	num = data - len;
	if(data < 0 || num < 0 )
		return;
	
	for(i=0;i<num;i++)
	{
		printf("%c",c);
	}
}

static void devmgr_close(void)
{
	close(fd_key);
	close(fd_touch);
}

static void devmgr_open(void)
{
	char key_evnum_str[30];
	char touch_evnum_str[30];
	char *temp = NULL;

	temp = get_event_number(KEY_DEV);
	if(temp != NULL)
	{
		sprintf(key_evnum_str,"/dev/input/%s",temp);
		fd_key = open(key_evnum_str, O_RDWR);
		if(fd_key < 0)
		{
			gprintf("open failed : keypad\n");
		}
	}
	temp = NULL;
	
	temp = get_event_number(TOUCH_DEV);
	if(temp != NULL)
	{
		sprintf(touch_evnum_str,"/dev/input/%s",temp);
		fd_touch = open(touch_evnum_str, O_RDWR);
		if(fd_touch < 0)
		{
			gprintf("open failed : touchscreen\n");
		}
	}
}

void devmgr_proc_keypad_event( char* buf, int read_count )
{
	int data_num, i;
	input_event_t rcv_msg;
	t_callMsg* p_snd_msg, snd_msg;

	data_num = read_count / sizeof( input_event_t );

	for( i=0; i<data_num; i++ )
	{
		TIME_LOG
		
		memcpy(&rcv_msg, &buf[( i*sizeof( input_event_t ) )], sizeof( input_event_t ) );


		memset( &snd_msg, 0, sizeof(t_callMsg) );

		p_snd_msg = &snd_msg;
		CALL_MSG_MSGID(p_snd_msg) = KEYPAD_EVENT;
		gettimeofday(&snd_msg.hdr.time,NULL);

		S_PARA1 = rcv_msg.type;
		S_PARA2 = rcv_msg.code;
		S_PARA3 = rcv_msg.value;
	}
}

void devmgr_proc_touch_event( char* buf, int read_count )
{
	input_event_t rcv_msg;
	t_callMsg* p_snd_msg, snd_msg;
	int data_num, i;

	data_num = read_count / sizeof( input_event_t );


	for( i = 0; i < data_num; i++ )
	{
		TIME_LOG
		
		memcpy(&rcv_msg, &buf[( i*sizeof( input_event_t ) )], sizeof( input_event_t ) );

		memset(&snd_msg,0,sizeof(t_callMsg));

		p_snd_msg = &snd_msg;

		CALL_MSG_MSGID(p_snd_msg) = TOUCH_EVENT;
		gettimeofday(&snd_msg.hdr.time,NULL);

		S_PARA1 = rcv_msg.type;
		S_PARA2 = rcv_msg.code;
		S_PARA3 = rcv_msg.value;
	}
}



void devmgr_proc_full_woken_up_state(void)
{
	prev_cs = curr_cs;
	
	switch(curr_cs) {
		case call_conv_idle:
			/* turn lcd backlight on */
			//x_device_manager_set_lcd_backlight(on);
			/* turn sub lcd backlight on */
			//x_device_manager_set_sub_lcd_backlight(on);
			break;
		default:
			/* play wake up tone */
			my_system("/usr/local/bin/aplay /data/sound/wakeup_tone.wav &");	
			/* turn lcd backlight on */
			//x_device_manager_set_lcd_backlight(on);
			/* turn sub lcd backlight on */ 
			//x_device_manager_set_sub_lcd_backlight(on);
			break;
	}
}

void devmgr_proc_half_woken_up_state(void)
{
	prev_cs = curr_cs;

	if (wakeup_source & DPRAM_WAKEUP) {
		switch(curr_cs) {
			case call_idle_rx:	/* sms receiving */
				prev_ss = sys_suspended;
				break;
			case call_conv_idle:	/* sms not receving */	
			case call_idle_idle:
				curr_ss = sys_full_woken_up;
				prev_ss = sys_full_woken_up;
				break;
			default:
				break;
		}
	}
	else {	/* KEYPAD_WAKEUP | POWER_BUTTON_WAKEUP */
		curr_ss = sys_full_woken_up;
		prev_ss = sys_full_woken_up;
	}
}

#if 0
void devmgr_proc_suspended_state(void)
{
	t_callMsg* p_snd_msg, snd_msg;

	alarm(0);

	x_device_manager_start_device_control_module(0);

	x_device_manager_set_cpu_mode(PM_SUSPEND_MEM);

	x_device_manager_start_device_control_module(1);

	x_device_manager_get_wakeup_source(&wakeup_source);

	x_printf(MSG_LVL_DEBUG,MSG_MOD_CORE,
		"[DEVMGR] WAKEUP SOURCE (%s)\n", wakeup_source_list[wakeup_source]);
	
	memset(&snd_msg, 0, sizeof(t_callMsg));

	p_snd_msg = &snd_msg;

	CALL_MSG_MSGID(p_snd_msg) = CALL_EVENT;
	S_PARA1 = REQUEST_STATUS;
	
	//x_device_manager_sendto(TASKID_PHONESVR, p_snd_msg, sizeof(t_callMsg)); 

	if (wakeup_source & (POWER_BUTTON_WAKEUP | KEYPAD_WAKEUP | DPRAM_WAKEUP)) { 
		curr_ss = sys_half_woken_up;
	}
	else {
		curr_ss = sys_full_woken_up;
	}
}

void devmgr_proc_state(void)
{
	if(curr_ss == prev_ss)
		return;

	prev_ss = curr_ss;
	
	switch(curr_ss) {
		case sys_half_woken_up:
			devmgr_proc_half_woken_up_state();
			if (prev_ss == sys_suspended) {
				break;
			}
		case sys_full_woken_up:
			devmgr_proc_full_woken_up_state();
			break;

		case sys_suspended:
			devmgr_proc_suspended_state();
			break;
		default:
			break;
	}
}
#endif

void sigterm_handler(int signo) 
{
	devmgr_close();
    exit(0);
}

int main(int argc, char** argv)
{
	int ret;
	s3c_win_info_t win;
	int i,j;

	gprintf("start!\n" );

	signal(SIGTERM, sigterm_handler);
	signal(SIGINT,  sigterm_handler);
	signal(SIGQUIT, sigterm_handler);

	devmgr_open();

	//devmgr_set_overlay1_transparency(0, 240, 400, 19);
#if defined(CONFIG_MACH_BONANZA)
	if((fd_lcd = open("/dev/graphics/fb0",O_RDWR))<0)
#else
	if((fd_lcd = open("/dev/graphics/fb1",O_RDWR))<0)
#endif
	{
		gprintf("fb1 open fail\n");
	}
	else
	{	
		gprintf("fb1 open success\n");
	}

	win.bpp = 16;
	win.left_x = 0;
	win.top_y = 0;
	win.width = PANEL_W;
	win.height = PANEL_H;

#if defined(CONFIG_MACH_CAPELA)
	gprintf("CONFIG_MACH_CAPELA\n");
#elif defined(CONFIG_MACH_NAOS)
	gprintf("CONFIG_MACH_NAOS\n");
#elif defined(CONFIG_MACH_SPICA)
	gprintf("CONFIG_MACH_SPICA\n");
#elif defined(CONFIG_MACH_ORION_6410)
	gprintf("CONFIG_MACH_ORION_6410\n");
#elif defined(CONFIG_MACH_CYGNUS)
	gprintf("CONFIG_MACH_CYGNUS\n");
#elif defined(CONFIG_MACH_INSTINCTQ)
	gprintf("CONFIG_MACH_INSTINCTQ\n");
#elif defined(CONFIG_MACH_BONANZA)
	gprintf("CONFIG_MACH_BONANZA\n");
#endif

#if defined(CONFIG_LCD_COLOR_DEPTH_16)
	gprintf("CONFIG_LCD_COLOR_DEPTH_16 TRUE\n");
#else
	gprintf("CONFIG_LCD_COLOR_DEPTH_16 FALSE\n");
#endif

	gprintf("width = %d, height = %d \n",PANEL_W,PANEL_H);

	ret = ioctl(fd_lcd, S3C_FB_OSD_SET_INFO, &win);
	if(ret < 0)
	{
		gprintf("ioctl set info fail\n");
	}


#if defined(CONFIG_LCD_COLOR_DEPTH_16) 
	fbpDevice = (unsigned short*)mmap(0,LCD_SIZE*2, PROT_READ | PROT_WRITE, MAP_SHARED, fd_lcd, 0);
#else
	fbpDevice = (unsigned int*)mmap(0,LCD_SIZE*4, PROT_READ | PROT_WRITE, MAP_SHARED, fd_lcd, 0);
#endif

	if ((int)fbpDevice == -1) 
	{ 
		gprintf("Error: failed to map framebuffer device to memory.\n"); 
		perror("");
		exit(4);
	}

	fbpMem = fbpDevice;

	ret = ioctl(fd_lcd, S3C_FB_OSD_START, NULL);
	if(ret < 0)
	{
		gprintf("ioctl start fail\n");
	}

	devmgr_test_main();
	
	return 0;
}	
