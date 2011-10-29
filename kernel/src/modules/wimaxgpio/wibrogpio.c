#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/proc_fs.h>
#include <linux/ethtool.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <asm/byteorder.h>
#include <asm/uaccess.h>

#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
#include <linux/i2c/pmic.h>
#endif

#include <plat/sdhci.h>
#include <plat/devs.h>
#include <linux/mmc/host.h>

#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
#include <mach/instinctq.h>	//cky 20100111 headers for quattro
#elif defined (CONFIG_MACH_VICTORY)
#include <mach/gpio-jupiter.h>	// Yongha for Victory WiMAX 20100208
#endif

#include "../wimax/wimax_plat.h" //cky 20100224
#include "wimaxgpio.h"
#include "wimaxproc.h"
#include "wimax_i2c.h"

#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
extern void usb_switch_mode(int);	//cky 20100209 USB path switch
#endif

#ifndef FILE_DEVICE_UNKNOWN
#define FILE_DEVICE_UNKNOWN 0x89	// 0x22
#endif

#define DRIVER_AUTHOR "<sangam.swamy@samsung.com>"
#define DRIVER_DESC "Samsung WiMax SDIO GPIO Device Driver"
#define WXMGPIOVERSION "0.1"

// Macro definition for defining IOCTL 
//
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
//
// Define the method codes for how buffers are passed for I/O and FS controls
//
#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3
//
// Define the access check value for any access
#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )	// file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )	// file & pipe

#define	CONTROL_IOCTL_WIMAX_POWER_CTL		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x821, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_EEPROM_PATH	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x837, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_MODE_CHANGE		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x838, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_EEPROM_DOWNLOAD		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x839, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define	WIMAX_GPIO_DRIVER_VERSION_STRING  "1.0.1"	
#define SWMXGPIOMAJOR	234

static const char driver_name[] = "WIMAXGPIO";
static char charName[] = "swmxctl";
static unsigned int	gDeviceOpen = 0;

unsigned long DatadwValue = 0;
unsigned long ControldwValue = 0;
unsigned int 	g_bUSBPath = USB_PATH_PDA;
unsigned int 	g_dwLineState = SDIO_MODE;	//default

#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
static void s3c_WIMAX_SDIO_on(void)
{
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO5, &reg_buff)) {
		pr_info("[WGPIO]%s: WIMAX SDIO 2.6V on(%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO5, 1);
	}
}

static void s3c_WIMAX_SDIO_off(void)
{
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO5, &reg_buff)) {
		pr_info("[WGPIO] %s: WIMAX SDIO 2.6V off(%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO5, 0);
	}
}
#endif	// CONFIG_MACH_QUATTRO

void gpio_wimax_poweron (void)
{
	if(gpio_get_value(WIMAX_EN))  // sangam enable if it is low..
	{
		DumpDebug("Already Wimax powered ON");
		return;
	}
	DumpDebug("Wimax power ON");

	if (g_dwLineState != SDIO_MODE)
	{
		DumpDebug("WiMAX USB Enable");
#if defined (CONFIG_MACH_QUATTRO)	//cky 20100224
		usb_switch_mode(3);	// USB path WiMAX
#endif
		gpio_set_value(WIMAX_USB_EN, GPIO_LEVEL_HIGH);
	}
	else
	{
		DumpDebug("SDIO MODE");
#if defined (CONFIG_MACH_QUATTRO)	//cky 20100224
		usb_switch_mode(1);	// USB path PDA
#endif
	}

	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(WIMAX_RESET, S3C_GPIO_SFN(1));

	mdelay(10);	

	//s3c_WIMAX_SDIO_on();

	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);
	mdelay(10);
	gpio_set_value(WIMAX_RESET, GPIO_LEVEL_HIGH);

	mdelay(500);  //sangam dbg : Delay important for probe

	sdhci_s3c_force_presence_change(&DEVICE_HSMMC);
}

void gpio_wimax_poweroff (void)
{
	if(!gpio_get_value(WIMAX_EN))  // sangam enable if it is low..
	{
		DumpDebug("Already Wimax powered OFF");
		return;
	}
	DumpDebug("Wimax power OFF");
	
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(WIMAX_RESET, S3C_GPIO_SFN(1));

	//s3c_gpio_setpull(GPIO_WIMAX_RESET_N, S3C_GPIO_PULL_NONE);
	gpio_set_value(WIMAX_RESET, GPIO_LEVEL_LOW);
	gpio_set_value(WIMAX_USB_EN, GPIO_LEVEL_LOW);
#if defined (CONFIG_MACH_QUATTRO)	//cky 20100224
	usb_switch_mode(1); // USB path PDA
#endif

	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

	sdhci_s3c_force_presence_change(&DEVICE_HSMMC);

	//s3c_WIMAX_SDIO_off();
}

static int
swmxdev_open (struct inode * inode, struct file * file)
{
	ENTER;
	if(gDeviceOpen) {
		DumpDebug("Device in Use by other applicationn");
		return  -EEXIST;
	}

	gDeviceOpen = 1;
	LEAVE;
	return 0;
}

static int
swmxdev_release (struct inode * inode, struct file * file)
{
	ENTER;
	gDeviceOpen = 0;
	LEAVE;
	return 0;
}

static int
swmxdev_ioctl (struct inode * inode, struct file * file, u_int cmd, u_long arg)
{
	int ret = 0;
	
	ENTER;
	DumpDebug("CMD: %x",cmd);

	switch(cmd)
	{
		case CONTROL_IOCTL_WIMAX_EEPROM_PATH:
		{
			gpio_wimax_poweron();
			DumpDebug("CONTROL_IOCTL_WIMAX_EEPROM_PATH..");
			if(((unsigned char *)arg)[0] == 0) {
				DumpDebug("changed to AP mode....");
			}
			else {
				DumpDebug("changed to WIMAX EEPROM mode....");
			}
			break;
		}
		case CONTROL_IOCTL_WIMAX_POWER_CTL:
		{
			DumpDebug("CONTROL_IOCTL_WIMAX_POWER_CTL..");
			if(((unsigned char *)arg)[0] == 0) {
				gpio_wimax_poweroff();
			}
			else {
				gpio_wimax_poweron();
				
			}
			break;
		}
		case CONTROL_IOCTL_WIMAX_MODE_CHANGE:
		{
			DumpDebug("CONTROL_IOCTL_WIMAX_MODE_CHANGE to %d..", ((unsigned char *)arg)[0]);
			if (((unsigned char *)arg)[0] == g_dwLineState) {
				DumpDebug("IOCTL Already in mode %d", g_dwLineState);
				return 0;
			}
			if( (((unsigned char *)arg)[0]  < 0) || (((unsigned char *)arg)[0]  > AUTH_MODE) ) {
				DumpDebug("Wrong mode %d", ((unsigned char *)arg)[0]);
				return 0;
			}

			gpio_wimax_poweroff();

			g_dwLineState = ((unsigned char *)arg)[0];

			gpio_wimax_poweron();
		break;		
		}
		case CONTROL_IOCTL_WIMAX_EEPROM_DOWNLOAD:
		{
			DumpDebug("CONTROL_IOCTL_WIMAX_EEPROM_DOWNLOAD");
			gpio_wimax_poweroff();
			
			WIMAX_BootInit();
			break;
		}
	}
	
	LEAVE;
	return ret;
}

static ssize_t
swmxdev_read (struct file * file, char * buf, size_t count, loff_t *ppos)
{
	ENTER;
	LEAVE;
	return 0;	
}

static ssize_t
swmxdev_write (struct file * file, const char * buf, size_t count, loff_t *ppos)
{
	ENTER;
	LEAVE;
	return 0;
}

static struct file_operations swmx_fops = {
	owner:		THIS_MODULE,
	open:		swmxdev_open,
	release:	swmxdev_release,
	ioctl:		swmxdev_ioctl,
	read:		swmxdev_read,
	write:		swmxdev_write,
};

static int __init wmxGPIO_init(void)
{
	int error = 0;
	pr_info("[WGPIO] %s: %s, " DRIVER_DESC "\n", driver_name, WIMAX_GPIO_DRIVER_VERSION_STRING);

	DumpDebug("IOCTL SDIO GPIO driver installing");
	error = register_chrdev(SWMXGPIOMAJOR, charName, &swmx_fops);

	if(error < 0) {
		DumpDebug("WiBroB_drv: register_chrdev() failed");
		return error;
	}

	WiMAX_Proc_Init();

	return error;
}

static void __exit wmxGPIO_exit(void)
{
	DumpDebug("SDIO GPIO driver Uninstall");
	unregister_chrdev(SWMXGPIOMAJOR, charName);

	WiMAX_Proc_Deinit();
}

module_init(wmxGPIO_init);
module_exit(wmxGPIO_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

