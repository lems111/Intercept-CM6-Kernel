#include "wimax_i2c.h"

#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/vmalloc.h>

#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
#include <mach/instinctq.h>	//cky 20100111 headers for quattro
#elif defined (CONFIG_MACH_VICTORY)
#include <mach/gpio-jupiter.h>	// Yongha for Victory WiMAX 20100208
#endif

#include "wimaxgpio.h"
#include "firmware.h"
#include "../wimax/wimax_plat.h"

extern int Wimax730_init(void);
extern void Wimax730_exit(void);
extern int Wimax730_read(int offset, char *rxData, int length);
extern int Wimax730_write(int offset, char *rxData, int length);

#define	WIMAX_BOOTIMAGE_PATH "/system/etc/wimax_boot.bin"
#define	MAX_WIMAXBOOTIMAGE_SIZE	6000	// 6k bytes
#define	MAX_BOOT_WRITE_LENGTH			128
#define	WIMAX_BOOT_OVER128_CHECK
#define dim(x)		( sizeof(x)/sizeof(x[0]) )

typedef struct _IMAGE_DATA_Tag {	
	unsigned int    uiSize;
	unsigned int    uiWorkAddress;
	unsigned int 	uiOffset;
	struct mutex	lock;	// sangam : For i2c read write lock
	unsigned char * pImage;
} IMAGE_DATA, *PIMAGE_DATA;

IMAGE_DATA g_stWiMAXBootImage;

// I2C
void WiMAX_I2C_WriteByte(char c, int len)
{
	int i;
	char level;

	// 8 bits
	for (i = 0; i < len; i++)
	{
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
		
		level = (c >> (len - i - 1)) & 0x1;
		s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
		gpio_set_value(EEPROM_SDA, level);
		
		udelay(1);
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
		udelay(2);
	}
}

char WiMAX_I2C_ReadByte(int len)
{
	char val = 0;
	int j;
	
	for (j = 0; j < len; j++)
	{
		// read data
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
		s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(0));
		udelay(1);
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
		udelay(2);
		val |= (gpio_get_value(EEPROM_SDA) << (len - j - 1));
	}

	return val;
}

void WiMAX_I2C_Init(void)
{
	int DEVICE_ADDRESS = 0x50;
	
	// initial
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);
	udelay(2);

	// start bit
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);
	udelay(2);

	WiMAX_I2C_WriteByte(DEVICE_ADDRESS, 7);	// send 7 bits address
}

void WiMAX_I2C_Deinit(void)
{
	// stop
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	udelay(1);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(1);
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);

	mdelay(10);
	
}

void WiMAX_I2C_Ack(void)
{
	// ack
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	udelay(1);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(1);
}

void WiMAX_I2C_NoAck(void)
{
	// no ack
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	udelay(1);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(1);
}

int WiMAX_I2C_CheckAck(void)
{
	char val;
	
	// ack
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(0));
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(2);
	val = gpio_get_value(EEPROM_SDA);

	if (val == 1)
	{
		DumpDebug("EEPROM ERROR: NO ACK!!");
		return -1;
	}

	return 0;
}

void WiMAX_I2C_WriteCmd(void)
{
	// write cmd
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);	// 0
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(2);

	WiMAX_I2C_CheckAck();
}

void WiMAX_I2C_ReadCmd(void)
{
	// read cmd
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);	// 1
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(2);

	WiMAX_I2C_CheckAck();
}

void WiMAX_I2C_EEPROMAddress(short addr)
{
	char buf[2] = {0};

	buf[0] = addr & 0xff;
	buf[1] = (addr >> 8) & 0xff;

	// send 2 bytes mem address
	WiMAX_I2C_WriteByte(buf[1], 8);
	WiMAX_I2C_CheckAck();

	WiMAX_I2C_WriteByte(buf[0], 8);
	WiMAX_I2C_CheckAck();
}

void WiMAX_I2C_WriteBuffer(char *data, int len)
{
	int i;

	for(i = 0; i < len; i++)
	{
		WiMAX_I2C_WriteByte(data[i], 8);
		WiMAX_I2C_CheckAck();
	}
}

void WiMAX_EEPROM_Write(unsigned short addr, unsigned char *data, int length)
{
	// write buffer
	WiMAX_I2C_Init();
	WiMAX_I2C_WriteCmd();
	WiMAX_I2C_EEPROMAddress(addr);
	WiMAX_I2C_WriteBuffer(data, length);
	WiMAX_I2C_Deinit();
}

int LoadWiMaxBootImage(void)
{	
	//unsigned long dwImgSize;		
	struct file *fp;	
	int read_size = 0;

	fp = klib_fopen(WIMAX_BOOTIMAGE_PATH, O_RDONLY, 0);

	if(fp)
	{
		DumpDebug("LoadWiMAXBootImage ..");
		g_stWiMAXBootImage.pImage = (char *) vmalloc(MAX_WIMAXBOOTIMAGE_SIZE);//__get_free_pages(GFP_KERNEL, buforder);
		if(!g_stWiMAXBootImage.pImage)
		{
			DumpDebug("Error: Memory alloc failure.");
			klib_fclose(fp);
			return -1;
		}

		memset(g_stWiMAXBootImage.pImage, 0, MAX_WIMAXBOOTIMAGE_SIZE);				
		read_size = klib_flen_fcopy(g_stWiMAXBootImage.pImage, MAX_WIMAXBOOTIMAGE_SIZE, fp);
//		read_size = klib_fread(g_stWiMAXBootImage.pImage, dwImgSize, fp);
		g_stWiMAXBootImage.uiSize = read_size;
		g_stWiMAXBootImage.uiWorkAddress = 0;			
		g_stWiMAXBootImage.uiOffset = 0;
		mutex_init(&g_stWiMAXBootImage.lock);

		klib_fclose(fp);
	}
	else {
		DumpDebug("Error: WiMAX image file open failed");
		return -1;
	}
	return 0;
}

//
// Write boot image to EEPROM
//
#if defined (CONFIG_MACH_VICTORY)	//cky 20100225
void WIMAX_BootInit(void)
{
	unsigned short ucsize=MAX_BOOT_WRITE_LENGTH;
	unsigned char *buffer;

	if(LoadWiMaxBootImage() < 0)
	{
		DumpDebug("ERROR READ WIMAX BOOT IMAGE");
		return 0;
	}

	// power on
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);
	mdelay(100);
	
	s3c_gpio_cfgpin(I2C_SEL, S3C_GPIO_SFN(1));
	gpio_set_value(I2C_SEL, GPIO_LEVEL_HIGH);
	
	s3c_gpio_cfgpin(EEPROM_SCL, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SCL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SDA, S3C_GPIO_PULL_NONE);
	
	g_stWiMAXBootImage.uiOffset = 0;
	mutex_lock(&g_stWiMAXBootImage.lock);
	
	while(g_stWiMAXBootImage.uiSize > g_stWiMAXBootImage.uiOffset)
	{
		buffer =(unsigned char *)(g_stWiMAXBootImage.pImage+g_stWiMAXBootImage.uiOffset);
		ucsize=MAX_BOOT_WRITE_LENGTH;

		// write buffer
		WiMAX_EEPROM_Write((unsigned short)g_stWiMAXBootImage.uiOffset, buffer, ucsize);
		
		g_stWiMAXBootImage.uiOffset += MAX_BOOT_WRITE_LENGTH;

		if((g_stWiMAXBootImage.uiSize - g_stWiMAXBootImage.uiOffset) < MAX_BOOT_WRITE_LENGTH)
		{
			buffer = (unsigned char *)(g_stWiMAXBootImage.pImage+g_stWiMAXBootImage.uiOffset);
			ucsize = g_stWiMAXBootImage.uiSize - g_stWiMAXBootImage.uiOffset;

			// write buffer
			WiMAX_EEPROM_Write((unsigned short)g_stWiMAXBootImage.uiOffset, buffer, ucsize);
			
			g_stWiMAXBootImage.uiOffset += MAX_BOOT_WRITE_LENGTH;				
		}
	}

	mutex_unlock(&g_stWiMAXBootImage.lock);

	// power off
	s3c_gpio_cfgpin(I2C_SEL, S3C_GPIO_SFN(1));
	gpio_set_value(I2C_SEL, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

	DumpDebug("EEPROM WRITING DONE.");

}
#else
void WIMAX_BootInit(void)
{
	int ret = 0;
	unsigned char ucReadBuffer[MAX_BOOT_WRITE_LENGTH];
	unsigned char ucBootCmp[MAX_BOOT_WRITE_LENGTH] = 
		            {0x0A,0x00,0x00,0xEA,0x42,0xF2,0xA0,0xE3,0x82,0xF2,0xA0,0xE3,0xC2,0xF2,0xA0,0xE3};

#ifdef WIMAX_BOOT_OVER128_CHECK
	unsigned char ucReadBuffer_over128[MAX_BOOT_WRITE_LENGTH];
	unsigned char ucBootCmp_over128[MAX_BOOT_WRITE_LENGTH] = 
					{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
#endif

	unsigned char ucWriteBuffer[MAX_BOOT_WRITE_LENGTH];
	unsigned char *buffer;
	unsigned short ucsize=MAX_BOOT_WRITE_LENGTH;

	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	//temp s3c_gpio_cfgpin(GPIO_WIMAX_I2C_CON, S3C_GPIO_SFN(1));

	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);
	mdelay(10);
	//temp gpio_set_value(GPIO_WIMAX_I2C_CON, GPIO_LEVEL_HIGH);
	mdelay(300);

	Wimax730_init();

	Wimax730_read(0x0, ucReadBuffer, 16); // offset, buffer, len

#ifdef WIMAX_BOOT_OVER128_CHECK
	memset( ucReadBuffer_over128, 0, dim(ucReadBuffer_over128) );
	Wimax730_read( 0x80, ucReadBuffer_over128, 16 );
#endif

	// if boot was already written , pass the write process
	if( strncmp( ucReadBuffer, ucBootCmp, 16 ) == 0 )
	{
#if !defined(WIMAX_BOOT_OVER128_CHECK) // org
		ret = 1;
		goto exit;
#else
//		ddd( ucReadBuffer_over128, 16, "READBUFFER_128" );
		if( strncmp( ucReadBuffer_over128, ucBootCmp_over128, 16 ) == 0 )
		{
			DumpDebug("Error first 128 bytes have valid values, but the values over 128 have invalid - all FFs");
			// fall throught..
		}
		else
		{
			DumpDebug("EEPROM has valid boot code");
			// EEPROM has valid boot code
			ret = 1;
			goto exit;
		}
#endif
	}

	if(LoadWiMaxBootImage() < 0)
	{
		ret = 1;
		goto exit;
	}

	g_stWiMAXBootImage.uiOffset = 0;
	mutex_lock(&g_stWiMAXBootImage.lock);
	
	while(g_stWiMAXBootImage.uiSize > g_stWiMAXBootImage.uiOffset)
	{
		buffer =(unsigned char *)(g_stWiMAXBootImage.pImage+g_stWiMAXBootImage.uiOffset);
		ucsize=MAX_BOOT_WRITE_LENGTH;

		ret = Wimax730_write((unsigned short)g_stWiMAXBootImage.uiOffset, buffer, ucsize);
		if(ret <= 0) 
			break;		

		g_stWiMAXBootImage.uiOffset += MAX_BOOT_WRITE_LENGTH;
		//clk_busy_wait(2000);
		mdelay(2);

		if((g_stWiMAXBootImage.uiSize - g_stWiMAXBootImage.uiOffset) < MAX_BOOT_WRITE_LENGTH)
		{
		//	clk_busy_wait(2000);
			mdelay(2);
			buffer = (unsigned char *)(g_stWiMAXBootImage.pImage+g_stWiMAXBootImage.uiOffset);
			ucsize = g_stWiMAXBootImage.uiSize - g_stWiMAXBootImage.uiOffset;

			ret = Wimax730_write((unsigned short)g_stWiMAXBootImage.uiOffset, buffer, ucsize);
			if(ret <= 0)
				break;
			
			g_stWiMAXBootImage.uiOffset += MAX_BOOT_WRITE_LENGTH;				
		}
	}

	mutex_unlock(&g_stWiMAXBootImage.lock);
exit:
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);
	//temp gpio_set_value(GPIO_WIMAX_I2C_CON, GPIO_LEVEL_LOW);

	Wimax730_exit()	;	
	return;
	
}
#endif



