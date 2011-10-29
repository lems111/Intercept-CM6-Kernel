/* Slave address */
//.m [VinsQ][PINEONE][Start] MAX17043 Fuel Gauge    Pineone hgwoo    2010.01.11
#if defined(__FUEL_GAUGES_IC_MAX17043__)
#define MAX17043_SLAVE_ADDR	0x6D
#define I2C_CHANNEL 5
#else
#define MAX17040_SLAVE_ADDR	0x6D
#endif
//.m [VinsQ][PINEONE][End] MAX17043 Fuel Gauge    Pineone hgwoo    2010.01.11

/* Register address */
#define VCELL0_REG			0x02
#define VCELL1_REG			0x03
#define SOC0_REG			0x04
#define SOC1_REG			0x05
#define MODE0_REG			0x06
#define MODE1_REG			0x07
#define RCOMP0_REG			0x0C
#define RCOMP1_REG			0x0D
#define CMD0_REG			0xFE
#define CMD1_REG			0xFF

#ifdef __FUEL_GAUGES_IC_MAX17043_SLEEP_ALERT__
#define ALERT_THRESHOLD_VALUE 0x1E //1%
//#define ALERT_THRESHOLD_VALUE 0x1E //5%
#endif
/* Definitions */
#define VCELL_ARR_SIZE			6

//	int orig_temp = 0;			// added by kimjh

extern int rcomp_temp;										// added by kimjh
extern int s3c_bat_temp_read(void);
extern int s3c_bat_soc_read(void);							// added by kimjh
static struct i2c_driver fg_i2c_driver;
static struct i2c_client *fg_i2c_client = NULL;

static unsigned short fg_normal_i2c[] = { I2C_CLIENT_END };
static unsigned short fg_ignore[] = { I2C_CLIENT_END };

//.m [VinsQ][PINEONE][Start] MAX17043 Fuel Gauge    Pineone hgwoo    2010.01.11
#if defined(__FUEL_GAUGES_IC_MAX17043__) 
static unsigned short fg_probe[] = { I2C_CHANNEL, (MAX17043_SLAVE_ADDR >> 1),	I2C_CLIENT_END };
#else
static unsigned short fg_probe[] = { 0, (MAX17040_SLAVE_ADDR >> 1),	I2C_CLIENT_END };
#endif
//.m [VinsQ][PINEONE][Start] MAX17043 Fuel Gauge    Pineone hgwoo    2010.01.11

static struct i2c_client_address_data fg_addr_data = {
	.normal_i2c	= fg_normal_i2c,
	.ignore		= fg_ignore,
	.probe		= fg_probe,
};

static int is_reset_soc = 0;

static int fg_i2c_read(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;
	u8 buf[1];
	struct i2c_msg msg[2];

	buf[0] = reg; 

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) 
		return -EIO;

	*data = buf[0];
	
	return 0;
}

static int fg_i2c_write(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;
	u8 buf[3];
	struct i2c_msg msg[1];

	buf[0] = reg;
	buf[1] = *data;
	buf[2] = *(data + 1);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 3;
	msg[0].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) 
		return -EIO;

	return 0;
}

static unsigned int fg_get_vcell_data(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	u32 vcell = 0;

	if (fg_i2c_read(client, VCELL0_REG, &data[0]) < 0) {
		pr_err("%s: Failed to read VCELL0\n", __func__);
		return -1;
	}
	if (fg_i2c_read(client, VCELL1_REG, &data[1]) < 0) {
		pr_err("%s: Failed to read VCELL1\n", __func__);
		return -1;
	}
	vcell = ((((data[0] << 4) & 0xFF0) | ((data[1] >> 4) & 0xF)) * 125)/100;
	pr_debug("%s: VCELL=%d\n", __func__, vcell);
	return vcell;
}

unsigned int fg_read_vcell(void)
{
	u32 vcell_arr[VCELL_ARR_SIZE];
	u32 vcell_max = 0;
	u32 vcell_min = 0;
	u32 vcell_total = 0;
	int i;

	for (i = 0; i < VCELL_ARR_SIZE; i++) {
		vcell_arr[i] = fg_get_vcell_data();
		pr_debug("%s: vcell_arr = %d\n", __func__, vcell_arr[i]);
		if (i != 0) {
			if (vcell_arr[i] > vcell_max) 
				vcell_max = vcell_arr[i];
			else if (vcell_arr[i] < vcell_min)
				vcell_min = vcell_arr[i];
		} else {
			vcell_max = vcell_arr[0];
			vcell_min = vcell_arr[0];
		}
		vcell_total += vcell_arr[i];
	}

	pr_debug("%s: vcell_max = %d, vcell_min = %d\n",
			__func__, vcell_max, vcell_min);


	return (vcell_total - vcell_max - vcell_min) / (VCELL_ARR_SIZE - 2);
}

#if defined(CONFIG_MACH_VINSQ) || defined(CONFIG_MACH_VITAL)
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
unsigned int fg_read_soc_decimal_point(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[1];

	if (fg_i2c_read(client, SOC1_REG, &data[0]) < 0) {
		pr_err("%s: Failed to read SOC1\n", __func__);
		return -1;
	}
	//printk("%s: Decimal Point SOC [0]=%d\n", __func__, data[0]);
    data[0] = ((data[0]*100)/255);
	if(data[0] >=100)
		data[0]=99;
	//printk("%s: Recalc. Decimal Point SOC [0]=%d\n", __func__, data[0]);	
	if (is_reset_soc) {
		pr_info("%s: Reseting SOC\n", __func__);
		return -1;
	} else {
		return data[0];
	}
}
#endif
#endif

unsigned int fg_read_soc(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];

	if (fg_i2c_read(client, SOC0_REG, &data[0]) < 0) {
		pr_err("%s: Failed to read SOC0\n", __func__);
		return -1;
	}
	if (fg_i2c_read(client, SOC1_REG, &data[1]) < 0) {
		pr_err("%s: Failed to read SOC1\n", __func__);
		return -1;
	}
	pr_debug("%s: SOC [0]=%d [1]=%d\n", __func__, data[0], data[1]);

	if (is_reset_soc) {
		pr_info("%s: Reseting SOC\n", __func__);
		return -1;
	} else {
#ifndef SOC_LB_FOR_POWER_OFF
		return data[0];
#else /* SOC_LB_FOR_POWER_OFF */
		if (data[0])
			return data[0];
		else {
			if (data[1] > SOC_LB_FOR_POWER_OFF)
				return 1;
			else
				return 0;
		}
#endif /* SOC_LB_FOR_POWER_OFF */
	}
}

unsigned int fg_rcomp_init(void)
{
	struct i2c_client *client = fg_i2c_client;
#ifdef __FUEL_GAUGES_IC_MAX17043_SLEEP_ALERT__
	u8 rcomp_data[2];
#else
	u8 rcomp_data[1];
#endif

	s32 ret = 0;

	rcomp_data[0] = 0xA7;
#ifdef __FUEL_GAUGES_IC_MAX17043_SLEEP_ALERT__
	rcomp_data[1] = ALERT_THRESHOLD_VALUE;	// 10%	Threshold
#else
	rcomp_data[1] = 0x1E;	// 10%	Threshold
#endif
	ret = fg_i2c_write(client, RCOMP0_REG, rcomp_data);

	printk("%s, rcomp_data[0] : 0x%02x\n",__func__, rcomp_data[0]);
#ifdef __FUEL_GAUGES_IC_MAX17043_SLEEP_ALERT__
	printk("%s, rcomp_data[1] : 0x%02x\n",__func__, rcomp_data[1]);
#endif
	if (ret)
		pr_err("%s: failed rcomp_data(%d)\n", __func__, ret);

	msleep(500);
	return ret;
}

#ifdef __FUEL_GAUGES_IC_MAX17043_SLEEP_ALERT__
unsigned int fg_set_alertlevel_cal(void)
{
	struct i2c_client *client = fg_i2c_client;

	u8 rcomp_data[2];
	s32 ret = 0;
	rcomp_data[0] = 0xA7;

	rcomp_data[1] = 0x1E;	// 10%	Threshold

	ret = fg_i2c_write(client, RCOMP0_REG, rcomp_data);

	msleep(500);
	return ret;
}
#endif

#ifdef __FUEL_GAUGES_IC_MAX17043_SLEEP_ALERT__
unsigned int fg_alert_int_clear(void)		// added by kimjh
{
	struct i2c_client *client = fg_i2c_client;
#ifdef __FUEL_GAUGES_IC_MAX17043_SLEEP_ALERT__
	u8 rcomp_data[2];
#else
	u8 rcomp_data[1];
#endif
	u8 data[1];

	s32 ret = 0;

	rcomp_data[0] = 0xA7;
#ifdef __FUEL_GAUGES_IC_MAX17043_SLEEP_ALERT__
	rcomp_data[1] = ALERT_THRESHOLD_VALUE;
#endif

	printk("%s: Now Clear Interrupt Bit Rcomp Register ===> 0Dh\n", __FUNCTION__);
	ret = fg_i2c_write(client, RCOMP0_REG, rcomp_data);
	msleep(500);

#ifdef _DEBUG_
	if (fg_i2c_read(client, RCOMP1_REG, &data[0]) < 0) {
		pr_err("%s: Failed to read rcomp1\n", __func__);
		return -1;
	}
	data[0] = ((data[0] >> 5) & 0x1);
#endif
	if (ret)
		printk("%s: Failed rcomp_Interrupt Clear Bit (%d)\n", __func__, ret);

	return ret;
}
#endif
#if 0		// commnet by kimjh
unsigned int fg_rcomp_low_temp(int rcomp_temp)		// added by kimjh
{
	struct i2c_client *client = fg_i2c_client;
	u8 rcomp_data[2];
	s32 ret = 0;

	// 167 = 0xA7
	rcomp_data[0] = 167+5*(20-rcomp_temp);
//	rcomp_data[1] = 0;
	
	ret = fg_i2c_write(client, RCOMP0_REG, rcomp_data);

	printk("%s, rcomp_data[0] : 0x%02x\n",__func__, rcomp_data[0]);
	if (ret)
		pr_err("%s: failed rcomp_data(%d)\n", __func__, ret);

	msleep(500);
	return ret;
}

unsigned int fg_rcomp_high_temp(int rcomp_temp)		// added by kimjh
{
	struct i2c_client *client = fg_i2c_client;
	u8 rcomp_data[2];
	s32 ret = 0;
	
	// 167 = 0xA7
	rcomp_data[0] = 167-2*(rcomp_temp-20);
//	rcomp_data[1] = 0;
	
	ret = fg_i2c_write(client, RCOMP0_REG, rcomp_data);

	printk("%s, rcomp_data[0] : 0x%02x\n",__func__, rcomp_data[0]);
	if (ret)
		pr_err("%s: failed rcomp_data(%d)\n", __func__, ret);

	msleep(500);
	return ret;
}
#endif

#ifdef __FUEL_GAUGES_IC_MAX17043_SLEEP_ALERT__
unsigned int Is_Interrupt(void)
{

	u8 data[1];
	struct i2c_client *client = fg_i2c_client;	
	if (fg_i2c_read(client, RCOMP1_REG, &data[0]) < 0) {
		pr_err("%s: Failed to read rcomp1\n", __func__);
		return -1;
	}
	data[0] = ((data[0] >> 5) & 0x1);
	if(data[0] == 1)
		printk("interrupt accured ============= > OK\n");
	//else	printk("None Interrupt =============== > Another Interrupt...\n");
	return data[0];
}
#endif

unsigned int fg_reset_soc(void)
{
	int soc;

	struct i2c_client *client = fg_i2c_client;
	u8 rst_cmd[2];

	s32 ret = 0;
	
	is_reset_soc = 1;
	/* Quick-start */
	rst_cmd[0] = 0x40;
	rst_cmd[1] = 0x00;

	ret = fg_i2c_write(client, MODE0_REG, rst_cmd);
	if (ret)
		pr_err("%s: failed reset SOC(%d)\n", __func__, ret);
	
	printk("%s, line : %d\n",__FUNCTION__,__LINE__);

	msleep(500);
	is_reset_soc = 0;
#if 0
	soc = fg_read_soc();
	s3c_bat_soc_read("/sys/class/power_supply/battery/fg_soc", (char*)&soc);	// added by kimjh
#endif
	s3c_bat_soc_read();
	printk("%s, line : %d\n",__FUNCTION__,__LINE__);
	return ret;
}

static int fg_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	int ret;
				
	pr_info("%s\n", __func__);

	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));

	strncpy(c->name, fg_i2c_driver.driver.name, I2C_NAME_SIZE);
	c->addr = addr;
	c->adapter = adap;
	c->driver = &fg_i2c_driver;

	if ((ret = i2c_attach_client(c)))
		goto error;

	fg_i2c_client = c;

error:
	return ret;
}

static int fg_attach_adapter(struct i2c_adapter *adap)
{
	pr_info("%s\n", __func__);
	return i2c_probe(adap, &fg_addr_data, fg_attach);
}

static int fg_detach_client(struct i2c_client *client)
{
	pr_info("%s\n", __func__);
	i2c_detach_client(client);
	return 0;
}

static struct i2c_driver fg_i2c_driver = {
	.driver = {
		.name = "Fuel Gauge I2C",
		.owner = THIS_MODULE,
	},
	.id 		= 0,
	.attach_adapter	= fg_attach_adapter,
	.detach_client	= fg_detach_client,
	.command	= NULL,
};
