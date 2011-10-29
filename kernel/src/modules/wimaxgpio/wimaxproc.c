#include "wimaxproc.h"

#include <linux/proc_fs.h>
#include "wimaxgpio.h"
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/module.h>
#include <linux/delay.h>

#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
#include <mach/instinctq.h>	//cky 20100111 headers for quattro
#elif defined (CONFIG_MACH_VICTORY)
#include <mach/gpio-jupiter.h>	// Yongha for Victory WiMAX 20100208
#endif

#include "wimax_i2c.h"
#include "../wimax/wimax_plat.h"

extern unsigned int 	g_dwLineState;

// proc entry
static struct proc_dir_entry *g_proc_wmxmode_dir = NULL;
static struct proc_dir_entry *g_proc_wmxuart = NULL;
static struct proc_dir_entry *g_proc_eeprom = NULL;
static struct proc_dir_entry *g_proc_onoff = NULL;

static int wimax_proc_mode(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int len;
	ENTER;
	len = sprintf(buf, "%c\n", g_dwLineState);
	if(len > 0) *eof = 1;
	LEAVE;
	return len;
}

// switch UART path
static int proc_read_wmxuart(char *page, char **start, off_t off, int count, int *eof, void *data)
{

	return 0;
}

static int proc_write_wmxuart(struct file *foke, const char *buffer, unsigned long count, void *data)
{
	if (buffer == NULL) return 0;
	
	if (buffer[0] == '0')
	{
		//printk("<1>UART Path: AP!\n");
#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(GPIO_UART_SEL2, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);	// TRUE for AP, FALSE for WIMAX
		gpio_set_value(GPIO_UART_SEL2, GPIO_LEVEL_HIGH);
#elif defined (CONFIG_MACH_VICTORY)
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(GPIO_UART_SEL1, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);
		gpio_set_value(GPIO_UART_SEL1, GPIO_LEVEL_LOW);
#endif
	}
	else if (buffer[0] == '1')
	{
		//printk("<1>UART Path: WiMAX!\n");
#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(GPIO_UART_SEL2, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);	// TRUE for AP, FALSE for WIMAX
		gpio_set_value(GPIO_UART_SEL2, GPIO_LEVEL_HIGH);
#elif defined (CONFIG_MACH_VICTORY)
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(GPIO_UART_SEL1, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);
		gpio_set_value(GPIO_UART_SEL1, GPIO_LEVEL_HIGH);
#endif
	}

	return 0;
}

// write EEPROM
static int proc_read_eeprom(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	DumpDebug("Write EEPROM!!");
	WIMAX_BootInit();
	return 0;
}

static int proc_write_eeprom(struct file *foke, const char *buffer, unsigned long count, void *data)
{
	return 0;
}

// on/off test
static int proc_read_onoff(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	gpio_wimax_poweroff();
	
	return 0;
}

static int proc_write_onoff(struct file *foke, const char *buffer, unsigned long count, void *data)
{
	if (buffer[0] == 's')
	{
		if (g_dwLineState != SDIO_MODE)
		{
			gpio_wimax_poweroff();
			mdelay(10);
			g_dwLineState = SDIO_MODE;
			gpio_wimax_poweron();
		}
	}
	else if (buffer[0] == 'w')
	{
		if (g_dwLineState != WTM_MODE)
		{
			gpio_wimax_poweroff();
			mdelay(10);
			g_dwLineState = WTM_MODE;
			gpio_wimax_poweron();
		}
	}
	else if (buffer[0] == 'u')
	{
		if (g_dwLineState != USB_MODE)
			{
			gpio_wimax_poweroff();
			mdelay(10);
			g_dwLineState = USB_MODE;
			gpio_wimax_poweron();
		}
	}
	else if (buffer[0] == 'a')
	{
		if (g_dwLineState != AUTH_MODE)
			{
			gpio_wimax_poweroff();
			mdelay(10);
			g_dwLineState = AUTH_MODE;
			gpio_wimax_poweron();
		}
	}

	return 0;
}

void WiMAX_Proc_Init(void)
{
	/* create procfs directory & entry */
	g_proc_wmxmode_dir = proc_mkdir("wmxmode", NULL);
	create_proc_read_entry("mode", 0766, g_proc_wmxmode_dir,
		wimax_proc_mode, NULL);

	// switch UART path
	g_proc_wmxuart = create_proc_entry("wmxuart", 0644, g_proc_wmxmode_dir);
	g_proc_wmxuart->data = NULL;
	g_proc_wmxuart->owner = THIS_MODULE;
	g_proc_wmxuart->read_proc = proc_read_wmxuart;
	g_proc_wmxuart->write_proc = proc_write_wmxuart;

	// write EEPROM
	g_proc_eeprom = create_proc_entry("eeprom", 0644, g_proc_wmxmode_dir);
	g_proc_eeprom->data = NULL;
	g_proc_eeprom->owner = THIS_MODULE;
	g_proc_eeprom->read_proc = proc_read_eeprom;
	g_proc_eeprom->write_proc = proc_write_eeprom;

	// on/off test
	g_proc_onoff = create_proc_entry("onoff", 0644, g_proc_wmxmode_dir);
	g_proc_onoff->data = NULL;
	g_proc_onoff->owner = THIS_MODULE;
	g_proc_onoff->read_proc = proc_read_onoff;
	g_proc_onoff->write_proc = proc_write_onoff;
	
}

void WiMAX_Proc_Deinit(void)
{
	remove_proc_entry("onoff", g_proc_onoff);
	remove_proc_entry("wmxuart", g_proc_wmxmode_dir);
	remove_proc_entry("mode", g_proc_wmxmode_dir);
	remove_proc_entry("wmxmode", NULL);
}

