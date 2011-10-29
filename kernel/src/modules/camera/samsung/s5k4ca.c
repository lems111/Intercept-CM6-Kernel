/*
 *  Copyright (C) 2004 Samsung Electronics
 *             SW.LEE <hitchcar@samsung.com>
 *            - based on Russell King : pcf8583.c
 * 	      - added  smdk24a0, smdk2440
 *            - added  poseidon (s3c24a0+wavecom)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Driver for FIMC2.x Camera Decoder
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-id.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <mach/hardware.h>

#include <plat/gpio-cfg.h>
#include <plat/egpio.h>

#include "../s3c_camif.h"

#include "s5k4ca.h"

// function define
//#define CONFIG_LOAD_FILE
#define I2C_BURST_MODE //dha23 100325 ī�޶� �⵿ �ð� ���̱� ���� I2C Burst mode ���.
// Purpose of verifying I2C operaion. must be ignored later.
//#define LOCAL_CONFIG_S5K4CA_I2C_TEST

static struct i2c_driver s5k4ca_driver;

static void s5k4ca_sensor_gpio_init(void);
void s5k4ca_sensor_enable(void);
static void s5k4ca_sensor_disable(void);

static int s5k4ca_sensor_init(void);
static void s5k4ca_sensor_exit(void);

static int s5k4ca_sensor_change_size(struct i2c_client *client, int size);

#ifdef CONFIG_FLASH_AAT1271A
	extern int aat1271a_flash_init(void);
	extern void aat1271a_flash_exit(void);
	extern void aat1271a_falsh_camera_control(int ctrl);
	extern void aat1271a_falsh_movie_control(int ctrl);
#endif

#ifdef CONFIG_LOAD_FILE
	static int s5k4ca_regs_table_write(char *name);
#endif

/* 
 * MCLK: 24MHz, PCLK: 54MHz
 * 
 * In case of PCLK 54MHz
 *
 * Preview Mode (1024 * 768)  
 * 
 * Capture Mode (2048 * 1536)
 * 
 * Camcorder Mode
 */
static camif_cis_t s5k4ca_data = {
	itu_fmt:       	CAMIF_ITU601,
	order422:      	CAMIF_CRYCBY,
	camclk:        	24000000,		
	source_x:      	1024,		
	source_y:      	768,
	win_hor_ofst:  	0,
	win_ver_ofst:  	0,
	win_hor_ofst2: 	0,
	win_ver_ofst2: 	0,
	polarity_pclk: 	0,
	polarity_vsync:	1,
	polarity_href: 	0,
	reset_type:		CAMIF_RESET,
	reset_udelay: 	5000,
};

/* #define S5K4CA_ID	0x78 */

static unsigned short s5k4ca_normal_i2c[] = { (S5K4CA_ID >> 1), I2C_CLIENT_END };
static unsigned short s5k4ca_ignore[] = { I2C_CLIENT_END };
static unsigned short s5k4ca_probe[] = { I2C_CLIENT_END };

static int previous_scene_mode = -1;
static int previous_WB_mode = 0;
static int af_mode = -1;
static unsigned short lux_value = 0;

static unsigned short AFPosition = 0x00FF; 
static unsigned short DummyAFPosition = 0x00FE; 


static struct i2c_client_address_data s5k4ca_addr_data = {
	.normal_i2c = s5k4ca_normal_i2c,
	.ignore		= s5k4ca_ignore,
	.probe		= s5k4ca_probe,
};


static int s5k4ca_sensor_read(struct i2c_client *client,
		unsigned short subaddr, unsigned short *data)
{
	int ret;
	unsigned char buf[2];
	struct i2c_msg msg = { client->addr, 0, 2, buf };
	
	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);

	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO) 
		goto error;

	msg.flags = I2C_M_RD;
	
	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO) 
		goto error;

	*data = ((buf[0] << 8) | buf[1]);

error:
	return ret;
}

static int s5k4ca_sensor_write(struct i2c_client *client,unsigned short subaddr, unsigned short val)
{
	if(subaddr == 0xdddd)
	{
/*	
			if (val == 0x0010)
				msleep(10);
			else if (val == 0x0020)
				msleep(20);
			else if (val == 0x0030)
				msleep(30);
			else if (val == 0x0040)
				msleep(40);
			else if (val == 0x0050)
				msleep(50);
			else if (val == 0x0100)
*/			
			msleep(val);
			printk("delay time(%d msec)\n", val);
	}	
	else
	{					
		unsigned char buf[4];
		struct i2c_msg msg = { client->addr, 0, 4, buf };

		buf[0] = (subaddr >> 8);
		buf[1] = (subaddr & 0xFF);
		buf[2] = (val >> 8);
		buf[3] = (val & 0xFF);

		return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	}
}

static int s5k4ca_sensor_write_list(struct i2c_client *client, struct samsung_short_t *list,char *name)
{
	int i, ret;
	ret = 0;
#ifdef CONFIG_LOAD_FILE 
	s5k4ca_regs_table_write(name);	
#else
	for (i = 0; list[i].subaddr != 0xffff; i++)
	{
		if(s5k4ca_sensor_write(client, list[i].subaddr, list[i].value) < 0)
			return -1;
	}
#endif
	return ret;
}

#ifdef I2C_BURST_MODE //dha23 100325

#define BURST_MODE_SET			1
#define BURST_MODE_END			2
#define NORMAL_MODE_SET			3
#define MAX_INDEX				1000
static int s5k4ca_sensor_burst_write_list(struct i2c_client *client, struct samsung_short_t *list,char *name)
{
	__u8 temp_buf[MAX_INDEX];
	int index_overflow = 1;
	int new_addr_start = 0;
	int burst_mode = NORMAL_MODE_SET;
	unsigned short pre_subaddr = 0;
	struct i2c_msg msg = { client->addr, 0, 4, temp_buf };
	int i=0, ret=0;
	unsigned int index = 0;
	
	printk("s5k4ca_sensor_burst_write_list( %s ) \n", name); 
#ifdef CONFIG_LOAD_FILE 
	s5k4ca_regs_table_write(name);	
#else
	for (i = 0; list[i].subaddr != 0xffff; i++)
	{
		if(list[i].subaddr == 0xdddd)
		{
		    /*
			if (list[i].value == 0x0010)
				msleep(10);
			else if (list[i].value == 0x0020)
				msleep(20);
			else if (list[i].value == 0x0030)
				msleep(30);
			else if (list[i].value == 0x0040)
				msleep(40);
			else if (list[i].value == 0x0050)
				msleep(50);
			else if (list[i].value == 0x0100)
				msleep(100);
		    */
		    msleep(list[i].value);
			printk("delay 0x%04x, value 0x%04x\n", list[i].subaddr, list[i].value);
		}	
		else
		{					
			if( list[i].subaddr == list[i+1].subaddr )
			{
				burst_mode = BURST_MODE_SET;
				if((list[i].subaddr != pre_subaddr) || (index_overflow == 1))
				{
					new_addr_start = 1;
					index_overflow = 0;
				}
			}
			else
			{
				if(burst_mode == BURST_MODE_SET)
				{
					burst_mode = BURST_MODE_END;
					if(index_overflow == 1)
					{
						new_addr_start = 1;
						index_overflow = 0;
					}
				}
				else
				{
					burst_mode = NORMAL_MODE_SET;
				}
			}

			if((burst_mode == BURST_MODE_SET) || (burst_mode == BURST_MODE_END))
			{
				if(new_addr_start == 1)
				{
					index = 0;
					memset(temp_buf, 0x00 ,1000);
					index_overflow = 0;

					temp_buf[index] = (list[i].subaddr >> 8);
					temp_buf[++index] = (list[i].subaddr & 0xFF);

					new_addr_start = 0;
				}
				
				temp_buf[++index] = (list[i].value >> 8);
				temp_buf[++index] = (list[i].value & 0xFF);
				
				if(burst_mode == BURST_MODE_END)
				{
					msg.len = ++index;

					ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
					if( ret < 0)
					{
						printk("i2c_transfer fail ! \n");
						return -1;
					}
				}
				else if( index >= MAX_INDEX-1 )
				{
					index_overflow = 1;
					msg.len = ++index;
					
					ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
					if( ret < 0)
					{
						printk("I2C_transfer Fail ! \n");
						return -1;
					}
				}
				
			}
			else
			{
				memset(temp_buf, 0x00 ,4);
			
				temp_buf[0] = (list[i].subaddr >> 8);
				temp_buf[1] = (list[i].subaddr & 0xFF);
				temp_buf[2] = (list[i].value >> 8);
				temp_buf[3] = (list[i].value & 0xFF);

				msg.len = 4;
				ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
				if( ret < 0)
				{
					printk("I2C_transfer Fail ! \n");
					return -1;
				}
			}
		}
		
		pre_subaddr = list[i].subaddr;
	}
#endif
	return ret;
}

#endif


static int s5k4ca_sensor_ae_awb_lock(struct i2c_client *client)
{
    //s5k4ca_sensor_write_list(client,s5k4ca_ae_awb_lock,"s5k4ca_ae_awb_lock");
    s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
    s5k4ca_sensor_write(client, 0x0028, 0x7000);
    s5k4ca_sensor_write(client, 0x002A, 0x0578);
    s5k4ca_sensor_write(client, 0x0F12, 0x0075);
}

static int s5k4ca_sensor_ae_awb_unlock(struct i2c_client *client)
{

    if(previous_WB_mode == 0 && previous_scene_mode != 4)
	{
        //s5k4ca_sensor_write_list(client,s5k4ca_ae_awb_unlock,"s5k4ca_ae_awb_unlock");
        s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
        s5k4ca_sensor_write(client, 0x0028, 0x7000);
        s5k4ca_sensor_write(client, 0x002A, 0x0578);
        s5k4ca_sensor_write(client, 0x0F12, 0x007F);
	}
	else
	{
        //s5k4ca_sensor_write_list(client,s5k4ca_ae_mwb_unlock,"s5k4ca_ae_mwb_unlock");
        s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
        s5k4ca_sensor_write(client, 0x0028, 0x7000);
        s5k4ca_sensor_write(client, 0x002A, 0x0578);
        s5k4ca_sensor_write(client, 0x0F12, 0x0077);
    
	}				

}

static void s5k4ca_sensor_get_id(struct i2c_client *client)
{
	unsigned short id = 0;
	
	s5k4ca_sensor_write(client, 0x002C, 0x7000);
	s5k4ca_sensor_write(client, 0x002E, 0x01FA);
	s5k4ca_sensor_read(client, 0x0F12, &id);

	printk("Sensor ID(0x%04x) is %s!\n", id, (id == 0x4CA4) ? "Valid" : "Invalid"); 
}

static void s5k4ca_sensor_gpio_init(void)
{
	I2C_CAM_DIS;
	MCAM_RST_DIS;
	VCAM_RST_DIS;
	CAM_PWR_DIS;
	AF_PWR_DIS;
	MCAM_STB_DIS;
	VCAM_STB_DIS;
}

#if defined(CONFIG_LDO_LP8720)
extern void	s5k4ca_sensor_power_init(void);	
#endif

void s5k4ca_sensor_enable(void)
{
	s5k4ca_sensor_gpio_init();

	MCAM_STB_EN;

	/* > 0 ms */
	msleep(1);

	AF_PWR_EN;	

#if defined(CONFIG_LDO_LP8720)
	s5k4ca_sensor_power_init();	
#endif

	CAM_PWR_EN;

	/* > 0 ms */
	msleep(1);

	/* MCLK Set */
	clk_set_rate(cam_clock, s5k4ca_data.camclk);

	/* MCLK Enable */
	clk_enable(cam_clock);
	clk_enable(cam_hclk);
	
	msleep(1);

	MCAM_RST_EN;
	
	msleep(40);
	
	I2C_CAM_EN;
}

static void s5k4ca_sensor_disable(void)
{
	I2C_CAM_DIS;
	
	MCAM_STB_DIS;

	/* > 20 cycles */
	msleep(1);

	/* MCLK Disable */
	clk_disable(cam_clock);
	clk_disable(cam_hclk);

	/* > 0 ms */
	msleep(1);

	MCAM_RST_DIS;

	/* > 0 ms */
	msleep(1);

	AF_PWR_DIS;

	CAM_PWR_DIS;
}

static int sensor_init(struct i2c_client *client)
{
	int i, size;
	int ret = 0;

	if (system_rev >= 0x40)
	{
#ifdef I2C_BURST_MODE //dha23 100325	
		if(s5k4ca_sensor_burst_write_list(client,s5k4ca_init0_04,"s5k4ca_init0_04") < 0)
			return -1;
#else
		if(s5k4ca_sensor_write_list(client,s5k4ca_init0_04,"s5k4ca_init0_04") < 0)
			return -1;
#endif
		msleep(10);	
		af_mode = -1;

		/* Check Sensor ID */
		s5k4ca_sensor_get_id(client);
#ifdef I2C_BURST_MODE //dha23 100325	
		ret = s5k4ca_sensor_burst_write_list(client,s5k4ca_init1_04,"s5k4ca_init1_04");
#else
		ret = s5k4ca_sensor_write_list(client,s5k4ca_init1_04,"s5k4ca_init1_04");
#endif
	
	}
	else
	{
		if(s5k4ca_sensor_write_list(client,s5k4ca_init0,"s5k4ca_init0") < 0)
			return -1;

		msleep(10);	

		/* Check Sensor ID */
		s5k4ca_sensor_get_id(client);

		ret = s5k4ca_sensor_write_list(client,s5k4ca_init1,"s5k4ca_init1");
	}
	//s5k4ca_sensor_change_size(client, SENSOR_XGA);
	return 0;
}

static int s5k4cagx_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	
	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));	

	strcpy(c->name, "s5k4ca");
	c->addr = addr;
	c->adapter = adap;
	c->driver = &s5k4ca_driver;
	s5k4ca_data.sensor = c;

#ifdef LOCAL_CONFIG_S5K4CA_I2C_TEST
	i2c_attach_client(c);
	msleep(10);
	sensor_init(c);

	return 0;
#else
	return i2c_attach_client(c);
#endif
}

static int s5k4ca_sensor_attach_adapter(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &s5k4ca_addr_data, s5k4cagx_attach);
}

static int s5k4ca_sensor_detach(struct i2c_client *client)
{
	i2c_detach_client(client);
	return 0;
}

static int s5k4ca_sensor_mode_set(struct i2c_client *client, int type)
{
	int i, size;
	unsigned short light;
	int delay = 0;

	printk("Sensor Mode ");

	if (system_rev >= 0x40)
	{

		if (type & SENSOR_PREVIEW)
		{	
			printk("-> Preview ");
#ifdef I2C_BURST_MODE //dha23 100325				
			s5k4ca_sensor_burst_write_list(client,s5k4ca_preview_04,"s5k4ca_preview_04");
#else
			s5k4ca_sensor_write_list(client,s5k4ca_preview_04,"s5k4ca_preview_04");
#endif

		
		}

		else if (type & SENSOR_CAPTURE)
		{	
			printk("-> Capture ");

			s5k4ca_sensor_ae_awb_unlock(client);

			s5k4ca_sensor_write(client, 0x002C, 0x7000);	
			s5k4ca_sensor_write(client, 0x002E, 0x12FE);
			s5k4ca_sensor_read(client, 0x0F12, &light);
			lux_value = light;

            if (previous_scene_mode == 6) /* fireworks use own capture routine */
            {
                printk("Snapshot : firework capture\n");
//                delay = s5k4ca_delay_firework_capture;

                s5k4ca_sensor_write_list(client,s5k4ca_snapshot_fireworks_04,"s5k4ca_snapshot_fireworks_04");

            }
            else
            {
                if (light <= 0x18) /* Low light */
                {	
                
                    printk("Snapshot : Low Light\n");
//                    delay = s5k4ca_delay_normal_low_capture;
                    
                    if(previous_scene_mode == 3) 
                    {
                        printk("Snapshot : Night Mode\n");
                    
                        s5k4ca_sensor_write_list(client,s5k4ca_snapshot_nightmode_04,"s5k4ca_snapshot_nightmode_04");
                    }
                    else
                    {
                        printk("Snapshot : Normal mode \n");
                        
                        s5k4ca_sensor_write_list(client,s5k4ca_snapshot_normal_low_04,"s5k4ca_snapshot_normal_low_04");
                    }
                
                }
                
                else
                {
#if 0                
                    if (light <= 0x40)
                    {
                        printk("Snapshot : Normal Middle Lignt\n");
//                        delay = s5k4ca_delay_normal_middle_capture;//600;
                        s5k4ca_sensor_write_list(client,s5k4ca_capture_normal_middle_04,"s5k4ca_capture_normal_middle_04");
                    }
                    else
#endif                    
                    {	
                        printk("Snapshot : Normal Normal Light\n");
//                        delay = s5k4ca_delay_normal_normal_capture;//300;
                        s5k4ca_sensor_write_list(client,s5k4ca_capture_normal_normal_04,"s5k4ca_capture_normal_normal_04");
                    }
                    
                }
			}
		}
		else if (type & SENSOR_FLASH_CAP_LOW)
		{	
  			printk("flash Normal Low Light Capture\n");
	  		delay = 300;

			s5k4ca_sensor_write_list(client,s5k4ca_flashcapture_low_04,"s5k4ca_flashcapture_low_04");
			printk("delay time(%d msec)\n", delay);
		}
		else if (type & SENSOR_FLASH_CAPTURE)
		{	
			printk("flash Normal Normal Light Capture\n");
			delay = 300;

			s5k4ca_sensor_write_list(client,s5k4ca_flashcapture_04,"s5k4ca_flashcapture_04");
			printk("delay time(%d msec)\n", delay);
		}
		else if (type & SENSOR_CAMCORDER )
		{
			printk("Record\n");
						
			s5k4ca_sensor_write(client, 0xFCFC, 0xD000); 
            s5k4ca_sensor_write(client, 0x0028, 0x7000); 

            s5k4ca_sensor_write(client, 0x002A, 0x030E);  
            s5k4ca_sensor_write(client, 0x0F12, 0x00DF);  //030E = 00FF �Է����� �ٸ��� �ӽ��Է�

            s5k4ca_sensor_write(client, 0x002A, 0x030C); 
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual 

            msleep(130); //AF Manual ��� �ν����� 1frame delay (������ 7.5fps 133ms ���)
                         //������� lens ������ ����
            s5k4ca_sensor_write(client, 0x002A, 0x030E);  
            s5k4ca_sensor_write(client, 0x0F12, 0x00E0);  // 030E = 00FF �Է�. lens ������ 
             
            msleep(50);  //lens�� ��ǥ�������� �����ϱ� ���� �ʿ��� delay
            
			delay = 300;
			s5k4ca_sensor_write_list(client,s5k4ca_fps_15fix_04,"s5k4ca_fps_15fix_04");
			printk("delay time(%d msec)\n", delay);
		}

		msleep(delay);
	
		return 0;
	}
	else  
	{
		if (type & SENSOR_PREVIEW)
		{	
			printk("-> Preview ");
            s5k4ca_sensor_write_list(client,s5k4ca_preview,"s5k4ca_preview");
		}
		else if (type & SENSOR_CAPTURE)
		{	
			printk("-> Capture ");
						
			s5k4ca_sensor_write(client, 0x002C, 0x7000);	
			s5k4ca_sensor_write(client, 0x002E, 0x12FE);
			s5k4ca_sensor_read(client, 0x0F12, &light);
				
			if (light <= 0x20) /* Low light */
			{	
				printk("Normal Low Light\n");
//				delay = 1200;
				
				s5k4ca_sensor_write_list(client,s5k4ca_snapshot_low,"s5k4ca_snapshot_low");
			}
			else
			{
				if (light <= 0x40)
				{
					printk("Normal Middle Lignt\n");
//					delay = 600;
				}
				else
				{	
					printk("Normal Normal Light\n");
//					delay = 300;
				}
				
				s5k4ca_sensor_write_list(client,s5k4ca_capture,"s5k4ca_capture");
			}
		}
		else if (type & SENSOR_FLASH_CAP_LOW)
		{	
  			printk("flash Normal Low Light Capture\n");
//			delay = 1200;
		
			s5k4ca_sensor_write_list(client,s5k4ca_flashcapture_low,"s5k4ca_flashcapture_low");
		}
		else if (type & SENSOR_FLASH_CAPTURE)
		{	
	  		printk("flash Normal Normal Light Capture\n");
//			delay = 300;
					
			s5k4ca_sensor_write_list(client,s5k4ca_flashcapture,"s5k4ca_flashcapture");
		}
		else if (type & SENSOR_CAMCORDER )
		{
			printk("Record\n");
			delay = 300;
			s5k4ca_sensor_write_list(client,s5k4ca_fps_15fix,"s5k4ca_fps_15fix");
		}
		
		msleep(delay);

		printk("delay time(%d msec)\n", delay);
			
		return 0;
	}
}

static int s5k4ca_sensor_change_size(struct i2c_client *client, int size)
{
	switch (size) {
		case SENSOR_XGA:
			s5k4ca_sensor_mode_set(client, SENSOR_PREVIEW);
			break;

		case SENSOR_QXGA:
			s5k4ca_sensor_mode_set(client, SENSOR_CAPTURE);
			break;		
	
		default:
			printk("Unknown Size! (Only XGA & QXGA)\n");
			break;
	}

	return 0;
}

static int s5k4ca_sensor_af_control(struct i2c_client *client, int type)
{

    int count = 50;
    int tmpVal = 0;
    int ret = 0;
    int size = 0;
    int i = 0;
    unsigned short light = 0;


    switch (type)
    {
        case 0: // release
            printk("[CAM-SENSOR] Focus Mode -> release\n"); 
			
            s5k4ca_sensor_ae_awb_unlock(client); // unlock AWB/AE

            //af move to initial position.

            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);    
            s5k4ca_sensor_write(client, 0x0028, 0x7000);

            s5k4ca_sensor_write(client, 0x002A, 0x030E);  
            s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition); // dummy lens position
			
            s5k4ca_sensor_write(client, 0x002A, 0x030C);    
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF manual
            
            msleep(130);

            s5k4ca_sensor_write(client, 0x002A, 0x030E);
            s5k4ca_sensor_write(client, 0x0F12, AFPosition); // move lens to initial position

            msleep(50);

            break;

        case 1: // AF start
			printk("Focus Mode -> Single\n");

            s5k4ca_sensor_ae_awb_lock(client); // lock AWB/AE

            s5k4ca_sensor_write(client, 0xFCFC, 0xD000); 
            s5k4ca_sensor_write(client, 0x0028, 0x7000); 

#if 0 // move to af success case
            s5k4ca_sensor_write(client, 0x002A, 0x030C); 
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual 

            msleep(130); //AF Manual ��� �ν����� 1frame delay (������ 7.5fps 133ms ���)
                         //������� lens ������ ����
#endif                         
           
            //s5k4ca_sensor_write(client, 0x002A, 0x030C); 
            //s5k4ca_sensor_write(client, 0x0F12, 0x0003); // AF Freeze 
            //msleep(50);  

            //AF freeze �� �ϸ� AF power off�� �ϰ� �Ǿ� �ణ�� power �Ҹ� ������ �ֽ��ϴ�.
            //�ʿ信���� ����Ͻñ� �ٶ��ϴ�.

#if 0 // move lens to searching position (??)
            s5k4ca_sensor_write(client, 0x002A, 0x030E); 
            s5k4ca_sensor_write(client, 0x0F12, AFPosition);
#endif 

            s5k4ca_sensor_write(client, 0x002A, 0x030C);
            s5k4ca_sensor_write(client, 0x0F12, 0x0002); // AF Single 
            
            msleep(260); // delay 2 frames before af status check
            
            do // remove low light check
            {
                if( count == 0)
                    break;

                s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
                s5k4ca_sensor_write(client, 0x002C, 0x7000);    
                s5k4ca_sensor_write(client, 0x002E, 0x130E);
                msleep(100);
                s5k4ca_sensor_read(client, 0x0F12, &tmpVal); 

                count--;

                printk("CAM 3M AF Status Value = %x \n", tmpVal); 

            }
            while( (tmpVal & 0x3) != 0x3 && (tmpVal & 0x3) != 0x2 );

            if(count == 0) // af timeout : move lens to initial position
            {
                s5k4ca_sensor_write(client, 0xFCFC, 0xD000); 
                s5k4ca_sensor_write(client, 0x0028, 0x7000); 
                
                s5k4ca_sensor_write(client, 0x002A, 0x030E);  
                s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition);  //030E = 00FF �Է����� �ٸ��� �ӽ��Է�
                
                s5k4ca_sensor_write(client, 0x002A, 0x030C); 
                s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual 
                
                msleep(130); //AF Manual ��� �ν����� 1frame delay (������ 7.5fps 133ms ���)
                             //������� lens ������ ����
                s5k4ca_sensor_write(client, 0x002A, 0x030E);  
                s5k4ca_sensor_write(client, 0x0F12, AFPosition);  // 030E = 00FF �Է�. lens ������ 
                
                msleep(50);  //lens�� ��ǥ�������� �����ϱ� ���� �ʿ��� delay
                	
                ret = 0;
                printk("CAM 3M AF_Single Mode Fail.==> TIMEOUT \n");
                
            }

            if((tmpVal & 0x3) == 0x02) // af fail : move lens to initial position
            {
                s5k4ca_sensor_write(client, 0xFCFC, 0xD000); 
                s5k4ca_sensor_write(client, 0x0028, 0x7000); 
                
                s5k4ca_sensor_write(client, 0x002A, 0x030E);  
                s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition);  //030E = 00FF �Է����� �ٸ��� �ӽ��Է�
                
                s5k4ca_sensor_write(client, 0x002A, 0x030C); 
                s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual 
                
                msleep(130); //AF Manual ��� �ν����� 1frame delay (������ 7.5fps 133ms ���)
                             //������� lens ������ ����
                s5k4ca_sensor_write(client, 0x002A, 0x030E);  
                s5k4ca_sensor_write(client, 0x0F12, AFPosition);  // 030E = 00FF �Է�. lens ������ 
                
                msleep(50);  //lens�� ��ǥ�������� �����ϱ� ���� �ʿ��� delay
                
                ret = 0;
                
                printk("CAM 3M AF_Single Mode Fail.==> FAIL \n");
            }

            if(tmpVal & 0x3 == 0x3) // af success
            {
                ret = 1;
                
                s5k4ca_sensor_write(client, 0x002A, 0x030C); // set to Manual for next snapshot and Single AF
                s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual     
                
                printk("CAM 3M AF_Single Mode SUCCESS. \r\n");
            }
            
            printk("CAM:3M AF_SINGLE SET \r\n");
            break;
            
        case 2: // auto
            printk("[CAM-SENSOR] =Focus Mode -> auto\n");
            
            DummyAFPosition = 0x00FE;
            AFPosition = 0x00FF;
            
            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);    
            s5k4ca_sensor_write(client, 0x0028, 0x7000);
            
            s5k4ca_sensor_write(client, 0x002A, 0x161C);    
            s5k4ca_sensor_write(client, 0x0F12, 0x82A8); // Set Normal AF Mode
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition); // dummy lens position
            
            s5k4ca_sensor_write(client, 0x002A, 0x030C);    
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual
            
            msleep(130);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, AFPosition); // move lens to initial position
            
            msleep(50); // Lens�� �ʱ� ��ġ�� ���ư��� �̵��ð�.           
            
            af_mode = 2;
            break;

        case 3: // infinity
            printk("[CAM-SENSOR] =Focus Mode -> infinity\n");
            
            DummyAFPosition = 0x00FE;
            AFPosition = 0x00FF;
            
            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
            s5k4ca_sensor_write(client, 0x0028, 0x7000);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030C);
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual
            
            msleep(130);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, AFPosition); // move lens to initial position
            
            msleep(50);
            
            af_mode = 3;
            break;

        case 4: // macro
            printk("[CAM-SENSOR] =Focus Mode -> Macro\n");
            
            DummyAFPosition = 0x0048;
            AFPosition = 0x0040;
            
            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	
            s5k4ca_sensor_write(client, 0x0028, 0x7000);
            
            s5k4ca_sensor_write(client, 0x002A, 0x161C);
            s5k4ca_sensor_write(client, 0x0F12, 0xA2A8);  // Set Macro AF mode
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);	
            s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition); // set dummy position
            
            
            s5k4ca_sensor_write(client, 0x002A, 0x030C);	
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual
            
            msleep(130);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);	
            s5k4ca_sensor_write(client, 0x0F12, AFPosition); // move lens to initial position
            
            msleep(50);
            
            af_mode = 4;
            break;

        case 5: // fixed, same as infinity
            printk("[CAM-SENSOR] =Focus Mode -> fixed\n");	
            
            DummyAFPosition = 0x00FE;
            AFPosition = 0x00FF;
            
            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
            s5k4ca_sensor_write(client, 0x0028, 0x7000);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition); // set dummy position
            
            s5k4ca_sensor_write(client, 0x002A, 0x030C);
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual
            
            msleep(130);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, AFPosition); // move lens to initial position
            
            msleep(50);
            
            af_mode = 5;
            break;
            
        default:
            break;
    }
               
    return ret;
}

static int s5k4ca_sensor_change_effect(struct i2c_client *client, int type)
{
	int i, size;	
	
	printk("Effects Mode ");

	if (system_rev >= 0x40)
	{

		switch (type)
		{
			case 0:
			default:
				printk("-> Mode None\n");
				s5k4ca_sensor_write_list(client,s5k4ca_effect_off_04,"s5k4ca_effect_off_04");
			break;

			case 1:
				printk("-> Mode Gray\n");
				s5k4ca_sensor_write_list(client,s5k4ca_effect_gray_04,"s5k4ca_effect_gray_04");
			break;

			case 2:
				printk("-> Mode Sepia\n");
				s5k4ca_sensor_write_list(client,s5k4ca_effect_sepia_04,"s5k4ca_effect_sepia_04");
			break;

			case 3:
				printk("-> Mode Negative\n");
				s5k4ca_sensor_write_list(client,s5k4ca_effect_negative_04,"s5k4ca_effect_negative_04");
			break;
			
			case 4:
				printk("-> Mode Aqua\n");
				s5k4ca_sensor_write_list(client,s5k4ca_effect_aqua_04,"s5k4ca_effect_aqua_04");
			break;

			case 5:
				printk("-> Mode Sketch\n");
				s5k4ca_sensor_write_list(client,s5k4ca_effect_sketch_04,"s5k4ca_effect_sketch_04");
			break;
		}

	return 0;

	}
	
	else
	{
		switch (type)
		{
			case 0:
			default:
				printk("-> Mode None\n");
				s5k4ca_sensor_write_list(client,s5k4ca_effect_off,"s5k4ca_effect_off");
			break;

			case 1:
				printk("-> Mode Gray\n");
				s5k4ca_sensor_write_list(client,s5k4ca_effect_gray,"s5k4ca_effect_gray");
			break;
			
			case 2:
				printk("-> Mode Sepia\n");
				s5k4ca_sensor_write_list(client,s5k4ca_effect_sepia,"s5k4ca_effect_sepia");
			break;

			case 3:
				printk("-> Mode Negative\n");
				s5k4ca_sensor_write_list(client,s5k4ca_effect_negative,"s5k4ca_effect_negative");
			break;

			case 4:
				printk("-> Mode Aqua\n");
				s5k4ca_sensor_write_list(client,s5k4ca_effect_aqua,"s5k4ca_effect_aqua");
			break;

			case 5:
				printk("-> Mode Sketch\n");
				s5k4ca_sensor_write_list(client,s5k4ca_effect_sketch,"s5k4ca_effect_sketch");
			break;
		}

		return 0;
	}
}

static int s5k4ca_sensor_change_br(struct i2c_client *client, int type)
{
	int i, size;

	printk("Brightness Mode \n");

	if (system_rev >= 0x40)
	{

		switch (type)
		{
			case 0: 
			default :
				printk("-> Brightness Minus 4\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_minus4_04,"s5k4ca_br_minus4_04");
			break;

			case 1:
				printk("-> Brightness Minus 3\n");	
				s5k4ca_sensor_write_list(client,s5k4ca_br_minus3_04,"s5k4ca_br_minus3_04");
			break;
			
			case 2:
				printk("-> Brightness Minus 2\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_minus2_04,"s5k4ca_br_minus2_04");
			break;
			
			case 3:				
				printk("-> Brightness Minus 1\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_minus1_04,"s5k4ca_br_minus1_04");
			break;
			
			case 4:
				printk("-> Brightness Zero\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_zero_04,"s5k4ca_br_zero_04");
			break;

			case 5:
				printk("-> Brightness Plus 1\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_plus1_04,"s5k4ca_br_plus1_04");
			break;

			case 6:
				printk("-> Brightness Plus 2\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_plus2_04,"s5k4ca_br_plus2_04");
			break;

			case 7:
				printk("-> Brightness Plus 3\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_plus3_04,"s5k4ca_br_plus3_04");
			break;

			case 8:
				printk("-> Brightness Plus 4\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_plus4_04,"s5k4ca_br_plus4_04");
			break;
			
		}

		return 0;

	}

	else
	{
		switch (type)
		{
			case 0:	
			default :
				printk("-> Brightness Minus 4\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_minus4,"s5k4ca_br_minus4");
			break;

			case 1:
				printk("-> Brightness Minus 3\n");	
				s5k4ca_sensor_write_list(client,s5k4ca_br_minus3,"s5k4ca_br_minus3");
			break;

			case 2:
				printk("-> Brightness Minus 2\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_minus2,"s5k4ca_br_minus2");
			break;

			case 3:
				printk("-> Brightness Minus 1\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_minus1,"s5k4ca_br_minus1");
			break;

			case 4:
				printk("-> Brightness Zero\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_zero,"s5k4ca_br_zero");
			break;

			case 5:
				printk("-> Brightness Plus 1\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_plus1,"s5k4ca_br_plus1");
			break;

			case 6:
				printk("-> Brightness Plus 2\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_plus2,"s5k4ca_br_plus2");
			break;
			
			case 7:
				printk("-> Brightness Plus 3\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_plus3,"s5k4ca_br_plus3");
			break;

			case 8:
				printk("-> Brightness Plus 4\n");
				s5k4ca_sensor_write_list(client,s5k4ca_br_plus4,"s5k4ca_br_plus4");
			break;
		}

		return 0;
	}
}

static int s5k4ca_sensor_change_wb(struct i2c_client *client, int type)
{
	int i, size;
	
	printk("White Balance Mode ");

	if (system_rev >= 0x40)
	{
		switch (type)
		{
			case 0:
			default :
				printk("-> WB auto mode\n");
				s5k4ca_sensor_write_list(client,s5k4ca_wb_auto_04,"s5k4ca_wb_auto_04");
				previous_WB_mode = type;
			break;
			
			case 1:
				printk("-> WB Sunny mode\n");
				s5k4ca_sensor_write_list(client,s5k4ca_wb_sunny_04,"s5k4ca_wb_sunny_04");
				previous_WB_mode = type;
			break;

			case 2:
				printk("-> WB Cloudy mode\n");
				s5k4ca_sensor_write_list(client,s5k4ca_wb_cloudy_04,"s5k4ca_wb_cloudy_04");
				previous_WB_mode = type;
			break;

			case 3:
				printk("-> WB Flourescent mode\n");
				s5k4ca_sensor_write_list(client,s5k4ca_wb_fluorescent_04,"s5k4ca_wb_fluorescent_04");
				previous_WB_mode = type;
			break;

			case 4:
				printk("-> WB Tungsten mode\n");
				s5k4ca_sensor_write_list(client,s5k4ca_wb_tungsten_04,"s5k4ca_wb_tungsten_04");
				previous_WB_mode = type;
			break;
		}
		return 0;
	}
	else
	{
		switch (type)
		{
			case 0:
			default :
				printk("-> WB auto mode\n");
				s5k4ca_sensor_write_list(client,s5k4ca_wb_auto,"s5k4ca_wb_auto");
			break;

			case 1:
				printk("-> WB Sunny mode\n");
				s5k4ca_sensor_write_list(client,s5k4ca_wb_sunny,"s5k4ca_wb_sunny");
			break;

			case 2:
				printk("-> WB Cloudy mode\n");
				s5k4ca_sensor_write_list(client,s5k4ca_wb_cloudy,"s5k4ca_wb_cloudy");
			break;

			case 3:
				printk("-> WB Flourescent mode\n");
				s5k4ca_sensor_write_list(client,s5k4ca_wb_fluorescent,"s5k4ca_wb_fluorescent");
			break;

			case 4:
				printk("-> WB Tungsten mode\n");
				s5k4ca_sensor_write_list(client,s5k4ca_wb_tungsten,"s5k4ca_wb_tungsten");
			break;
		}
	return 0;
	}
}

static int s5k4ca_sensor_change_scene_mode(struct i2c_client *client, int type)
{
	int i, size;

	if (system_rev >= 0x40)
	{
		printk("[CAM-SENSOR] =Scene Mode %d",type);
		if(previous_scene_mode != 0 && type != 0)
		{
			printk("-> Pre-auto-set");
			s5k4ca_sensor_write_list(client,s5k4ca_scene_mode_off_04,"s5k4ca_scene_mode_off_04");
		}

		switch (type)
		{
			case 0:
				printk("-> auto\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_mode_off_04,"s5k4ca_scene_mode_off_04");
				previous_scene_mode = type;
				break;
			case 1:
				printk("-> portrait\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_portrait_on_04,"s5k4ca_scene_portrait_on_04");
				previous_scene_mode = type;
				break;
			case 2:
				printk("-> landscape\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_landscape_on_04,"s5k4ca_scene_landscape_on_04");
				previous_scene_mode = type;
				break;
			case 3:
				printk("-> night\n");
				s5k4ca_sensor_write_list(client,s5k4ca_nightmode_on_04,"s5k4ca_nightmode_on_04");
				previous_scene_mode = type;
				break;
			case 4:
				printk("-> sunset\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_sunset_on_04,"s5k4ca_scene_sunset_on_04");
				previous_scene_mode = type;
				break;
			case 5:
				printk("-> sports\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_sports_on_04,"s5k4ca_scene_sports_on_04");
				previous_scene_mode = type;
				break;
			case 6:
				printk("-> fireworks\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_fireworks_on_04,"s5k4ca_scene_fireworks_on_04");
				previous_scene_mode = type;
				break;
			case 7:
				printk("-> candlelight\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_candlelight_on_04,"s5k4ca_scene_candlelight_on_04");
				previous_scene_mode = type;
				break;
			case 8:
				printk("-> text\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_text_on_04,"s5k4ca_scene_text_on_04");
				previous_scene_mode = type;
				break;
			case 9:
				printk("-> beach snow\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_beach_snow_on_04,"s5k4ca_scene_beach_snow_on_04");
				previous_scene_mode = type;
				break;
			case 10:
				printk("-> party indoor\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_party_indoor_on_04,"s5k4ca_scene_party_indoor_on_04");
				previous_scene_mode = type;
				break;
			default :
				printk("-> UnKnow Scene Mode\n");
				break;
		}

		previous_scene_mode = type;
	}
	else
	{
		printk("[CAM-SENSOR] =Scene Mode %d",type);
		switch (previous_scene_mode)
		{
			case 1:
				printk("-> portrait off");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_portrait_off,"s5k4ca_scene_portrait_off");
				break;
			case 2:
				printk("-> landscape off");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_landscape_off,"s5k4ca_scene_landscape_off");
				break;
			case 3:
				printk("-> night off");
				s5k4ca_sensor_write_list(client,s5k4ca_nightmode_off,"s5k4ca_nightmode_off");
				break;
			case 4:
				printk("-> sunset off");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_sunset_off,"s5k4ca_scene_sunset_off");
				break;
			default :
				printk("-> UnKnow Scene Mode off");
				break;
		}

		switch (type)
		{
			case 1:
				printk("-> portrait\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_portrait_on,"s5k4ca_scene_portrait_on");
				break;
			case 2:
				printk("-> landscape\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_landscape_on,"s5k4ca_scene_landscape_on");
				break;
			case 3:
				printk("-> night\n");
				s5k4ca_sensor_write_list(client,s5k4ca_nightmode_on,"s5k4ca_nightmode_on");
				break;
			case 4:
				printk("-> sunset\n");
				s5k4ca_sensor_write_list(client,s5k4ca_scene_sunset_on,"s5k4ca_scene_sunset_on");
				break;
			default :
				printk("-> UnKnown Scene Mode\n");
				break;
		}

		previous_scene_mode = type;		
	}
	return 0;
}

static int s5k4ca_sensor_photometry(struct i2c_client *client, int type)
{
	int i, size;	
	
	printk("Metering Mode ");

	switch (type)
	{
		case 0:
			printk("-> spot\n");
			s5k4ca_sensor_write_list(client,s5k4ca_metering_spot,"s5k4ca_metering_spot");
			break;
		case 1:
			printk("-> matrix\n");
			s5k4ca_sensor_write_list(client,s5k4ca_metering_matrix,"s5k4ca_metering_matrix");
			break;
		case 2:
			printk("-> center\n");
			s5k4ca_sensor_write_list(client,s5k4ca_metering_center,"s5k4ca_metering_center");
			break;
		default :
			printk("-> UnKnown Metering Mode\n");
			break;
	
	}
	return 0;	

}

static int s5k4ca_sensor_user_read(struct i2c_client *client, s5k4ca_t *r_data)
{
	s5k4ca_sensor_write(client, 0x002C, r_data->page);
	s5k4ca_sensor_write(client, 0x002E, r_data->subaddr);
	return s5k4ca_sensor_read(client, 0x0F12, &(r_data->value));
}

static int s5k4ca_sensor_user_write(struct i2c_client *client, unsigned short *w_data)
{
	return s5k4ca_sensor_write(client, w_data[0], w_data[1]);
}

static int s5k4ca_sensor_exif_read(struct i2c_client *client, exif_data_t *exif_data)
{
	int ret = 0;
	
//unsigned short lux = 0;
	unsigned short extime = 0;
	unsigned short iso_val = 0;

	s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	/// exposure time
	s5k4ca_sensor_write(client, 0x002C, 0x7000);
	s5k4ca_sensor_write(client, 0x002E, 0x1C3C);
//	msleep(100);
	s5k4ca_sensor_read(client, 0x0F12, &extime);

/*	msleep(100);

	s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	/// Incident Light value 
	s5k4ca_sensor_write(client, 0x002C, 0x7000);
	s5k4ca_sensor_write(client, 0x002E, 0x12FE);
	msleep(100);
	s5k4ca_sensor_read(client, 0x0F12, &lux); */

    s5k4ca_sensor_write(client, 0xFCFC, 0xD000);    /// exposure time
    s5k4ca_sensor_write(client, 0x002C, 0x7000);
    s5k4ca_sensor_write(client, 0x002E, 0x12FA);
//  msleep(100);
    s5k4ca_sensor_read(client, 0x0F12, &iso_val);

    
    iso_val = iso_val * 10;
    iso_val = iso_val / 256;

/*
    if(iso_val >= 10 && iso_val < 12)
        exif_data->iso = 64;
    else if(iso_val >= 12 && iso_val < 18)
        exif_data->iso = 100;
    else if(iso_val >= 18 && iso_val < 26)
        exif_data->iso = 125;
    else if(iso_val >= 26 && iso_val < 35)
        exif_data->iso = 200;
    else if(iso_val >= 36 && iso_val < 46)
        exif_data->iso = 250;
    else if(iso_val >= 46 && iso_val < 56)
        exif_data->iso = 320;
    else if(iso_val >= 56 && iso_val < 66)
        exif_data->iso = 400;
    else if(iso_val >= 66 && iso_val < 76)
        exif_data->iso = 500;
    else if(iso_val >= 76 && iso_val < 96)
        exif_data->iso = 640;
    else if(iso_val >= 96)
        exif_data->iso = 800;
    else 
        exif_data->iso = 0;
*/

    if (iso_val < 10)
    {
        printk("[CAM-SENSOR] =%s iso_val_read < 1.0 \n",__func__,iso_val);
        exif_data->iso = 50;
    }
    else if(iso_val >= 10 && iso_val < 19)
        exif_data->iso = 50;
    else if(iso_val >= 19 && iso_val < 23)
        exif_data->iso = 100;
    else if(iso_val >= 23 && iso_val < 28)
        exif_data->iso = 200;
    else if(iso_val >= 28)
    {
        printk("[CAM-SENSOR] =%s iso_val_read < 2.8 \n",__func__,iso_val);
        exif_data->iso = 400;
    }
    else 
    {
        printk("[CAM-SENSOR] =%s iso_val_read unknown range \n",__func__,iso_val);
        exif_data->iso = 0;
    }

	exif_data->exposureTime = extime/100;
	exif_data->lux = lux_value;
    printk("[CAM-SENSOR] =%s iso_val=%d, extime=%d, lux=%d,\n",__func__,exif_data->iso, extime/100,lux_value);


	return ret;
}

static int s5k4ca_sensor_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	struct v4l2_control *ctrl;
	unsigned short *w_data;		/* To support user level i2c */	
	s5k4ca_short_t *r_data;
	exif_data_t *exif_data;

	int ret=0;

	switch (cmd)
	{
		case SENSOR_INIT:
			ret = sensor_init(client);
			break;

		case USER_ADD:
			break;

		case USER_EXIT:
			s5k4ca_sensor_exit();
			break;

		case SENSOR_EFFECT:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_effect(client, ctrl->value);
			break;

		case SENSOR_BRIGHTNESS:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_br(client, ctrl->value);
			break;

		case SENSOR_WB:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_wb(client, ctrl->value);
			break;

		case SENSOR_SCENE_MODE:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_scene_mode(client, ctrl->value);
			break;

		case SENSOR_AF:
			ctrl = (struct v4l2_control *)arg;
			ret = s5k4ca_sensor_af_control(client, ctrl->value);
			break;

		case SENSOR_MODE_SET:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_mode_set(client, ctrl->value);
			break;

		case SENSOR_XGA:
			s5k4ca_sensor_change_size(client, SENSOR_XGA);	
			break;

		case SENSOR_QXGA:
			s5k4ca_sensor_change_size(client, SENSOR_QXGA);	
			break;

		case SENSOR_QSVGA:
			s5k4ca_sensor_change_size(client, SENSOR_QSVGA);
			break;

		case SENSOR_VGA:
			s5k4ca_sensor_change_size(client, SENSOR_VGA);
			break;

		case SENSOR_SVGA:
			s5k4ca_sensor_change_size(client, SENSOR_SVGA);
			break;

		case SENSOR_SXGA:
			s5k4ca_sensor_change_size(client, SENSOR_SXGA);
			break;

		case SENSOR_UXGA:
			s5k4ca_sensor_change_size(client, SENSOR_UXGA);
			break;

		case SENSOR_USER_WRITE:
			w_data = (unsigned short *)arg;
			s5k4ca_sensor_user_write(client, w_data);
			break;

		case SENSOR_USER_READ:
			r_data = (s5k4ca_short_t *)arg;
			s5k4ca_sensor_user_read(client, r_data);
			break;
	
		case SENSOR_FLASH_CAMERA:
			ctrl = (struct v4l2_control *)arg;
#ifdef CONFIG_FLASH_AAT1271A
			aat1271a_falsh_camera_control(ctrl->value);	
#endif			
			break;

		case SENSOR_FLASH_MOVIE:
			ctrl = (struct v4l2_control *)arg;
#ifdef CONFIG_FLASH_AAT1271A
			aat1271a_falsh_movie_control(ctrl->value);	
#endif
			break;

		case SENSOR_EXIF_DATA:
			exif_data = (exif_data_t *)arg;
			s5k4ca_sensor_exif_read(client, exif_data);	
			break;

		case SENSOR_PHOTOMETRY:
		    ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_photometry(client, ctrl->value);
			break;
		

		default:
		    printk("[CAM-SENSOR] no command type %d \n",cmd);
			break;
	}

	return ret;
}

static struct i2c_driver s5k4ca_driver = {
	.driver = {
		.name = "s5k4ca",
	},
	.id = S5K4CA_ID,
	.attach_adapter = s5k4ca_sensor_attach_adapter,
	.detach_client = s5k4ca_sensor_detach,
	.command = s5k4ca_sensor_command
};


#ifdef CONFIG_LOAD_FILE

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

static char *s5k4ca_regs_table = NULL;

static int s5k4ca_regs_table_size;

void s5k4ca_regs_table_init(void)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int i;
	int ret;
	mm_segment_t fs = get_fs();

	printk("%s %d\n", __func__, __LINE__);

	set_fs(get_ds());
#if 0
	filp = filp_open("/data/camera/s5k4ca.h", O_RDONLY, 0);
#else
	filp = filp_open("/sdcard/s5k4ca.h", O_RDONLY, 0);
#endif
	if (IS_ERR(filp)) {
		printk("file open error\n");
		return;
	}
	l = filp->f_path.dentry->d_inode->i_size;	
	printk("l = %ld\n", l);
	dp = kmalloc(l, GFP_KERNEL);
	if (dp == NULL) {
		printk("Out of Memory\n");
		filp_close(filp, current->files);
	}
	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	if (ret != l) {
		printk("Failed to read file ret = %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		return;
	}

	filp_close(filp, current->files);
	
	set_fs(fs);

	s5k4ca_regs_table = dp;
	
	s5k4ca_regs_table_size = l;

	*((s5k4ca_regs_table + s5k4ca_regs_table_size) - 1) = '\0';

	printk("s5k4ca_regs_table 0x%08x, %ld\n", dp, l);
}

void s5k4ca_regs_table_exit(void)
{
	printk("%s %d\n", __func__, __LINE__);
	if (s5k4ca_regs_table) {
		kfree(s5k4ca_regs_table);
		s5k4ca_regs_table = NULL;
	}	
}

static int s5k4ca_regs_table_write(char *name)
{
	char *start, *end, *reg, *data;	
	unsigned short addr, value;
	char reg_buf[7], data_buf[7];

	*(reg_buf + 6) = '\0';
	*(data_buf + 6) = '\0';

	start = strstr(s5k4ca_regs_table, name);
	
	end = strstr(start, "};");

	while (1) {	
		/* Find Address */	
		reg = strstr(start,"{ 0x");		
		if (reg)
			start = (reg + 16);
		if ((reg == NULL) || (reg > end))
			break;
		/* Write Value to Address */	
		if (reg != NULL) {
			memcpy(reg_buf, (reg + 2), 6);	
			memcpy(data_buf, (reg + 10), 6);	
			addr = (unsigned short)simple_strtoul(reg_buf, NULL, 16); 
			value = (unsigned short)simple_strtoul(data_buf, NULL, 16); 
//			printk("addr 0x%04x, value 0x%04x\n", addr, value);
/*
			if (addr == 0xdddd)
			{
			    if (value == 0x0010)
					mdelay(10);
				else if (value == 0x0020)
					mdelay(20);
				else if (value == 0x0030)
					mdelay(30);
				else if (value == 0x0040)
					mdelay(40);
				else if (value == 0x0050)
					mdelay(50);
				else if (value == 0x0100)
					mdelay(100);
	
				mdelay(value);

				printk("delay 0x%04x, value 0x%04x\n", addr, value);
			}	
			else
*/			
				s5k4ca_sensor_write(s5k4ca_data.sensor, addr, value);
		}
	}

	return 0;
}

#endif

static int s5k4ca_sensor_init(void)
{
	int ret;

#ifdef CONFIG_LOAD_FILE
	s5k4ca_regs_table_init();
#endif

//	s5k4ca_sensor_enable();
	
	s3c_camif_open_sensor(&s5k4ca_data);

	if (s5k4ca_data.sensor == NULL)
		if ((ret = i2c_add_driver(&s5k4ca_driver)))
			return ret;

	if (s5k4ca_data.sensor == NULL) {
		i2c_del_driver(&s5k4ca_driver);	
		return -ENODEV;
	}

	s3c_camif_register_sensor(&s5k4ca_data);
	
	return 0;
}

static void s5k4ca_sensor_exit(void)
{
	s5k4ca_sensor_disable();

#ifdef CONFIG_LOAD_FILE
	s5k4ca_regs_table_exit();
#endif
	
	if (s5k4ca_data.sensor != NULL)
		s3c_camif_unregister_sensor(&s5k4ca_data);
}

static struct v4l2_input s5k4ca_input = {
	.index		= 0,
	.name		= "Camera Input (S5K4CA)",
	.type		= V4L2_INPUT_TYPE_CAMERA,
	.audioset	= 1,
	.tuner		= 0,
	.std		= V4L2_STD_PAL_BG | V4L2_STD_NTSC_M,
	.status		= 0,
};

static struct v4l2_input_handler s5k4ca_input_handler = {
	s5k4ca_sensor_init,
	s5k4ca_sensor_exit	
};

#ifdef CONFIG_VIDEO_SAMSUNG_MODULE
static int s5k4ca_sensor_add(void)
{
#ifdef CONFIG_FLASH_AAT1271A
	aat1271a_flash_init();
#endif
	return s3c_camif_add_sensor(&s5k4ca_input, &s5k4ca_input_handler);
}

static void s5k4ca_sensor_remove(void)
{
	if (s5k4ca_data.sensor != NULL)
		i2c_del_driver(&s5k4ca_driver);
#ifdef CONFIG_FLASH_AAT1271A
	aat1271a_flash_exit();
#endif
	s3c_camif_remove_sensor(&s5k4ca_input, &s5k4ca_input_handler);
}

module_init(s5k4ca_sensor_add)
module_exit(s5k4ca_sensor_remove)

MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("I2C Client Driver For FIMC V4L2 Driver");
MODULE_LICENSE("GPL");
#else
int s5k4ca_sensor_add(void)
{
#ifdef CONFIG_FLASH_AAT1271A
	aat1271a_flash_init();
#endif	
#ifdef LOCAL_CONFIG_S5K4CA_I2C_TEST
	return s5k4ca_sensor_init();
#else
	return s3c_camif_add_sensor(&s5k4ca_input, &s5k4ca_input_handler);
#endif
}

void s5k4ca_sensor_remove(void)
{
	if (s5k4ca_data.sensor != NULL)
		i2c_del_driver(&s5k4ca_driver);
#ifdef CONFIG_FLASH_AAT1271A
	aat1271a_flash_exit();
#endif
	s3c_camif_remove_sensor(&s5k4ca_input, &s5k4ca_input_handler);
}
#endif
