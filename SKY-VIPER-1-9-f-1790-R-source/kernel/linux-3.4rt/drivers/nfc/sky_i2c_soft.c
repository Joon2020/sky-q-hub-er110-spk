#include <linux/kernel.h>
#include <linux/module.h>

#include <boardparms.h>
#include <linux/platform_device.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio.h>

#include <sky_nfc.h>

/* ================================================================================ */

// common module definitions
#define NFC_BUS_DRV_LICENSE "GPL"
#define NFC_BUS_DRV_DESC "Viper I2C Software Bus"
#define NFC_BUS_DRV_VERSION "0.0.4"

// I2C GPIO's
#define I2C_GPIO_SDA BP_GPIO_21_AH
#define I2C_GPIO_SCL BP_GPIO_20_AH
#define I2C_DEFAULT_BUS_ID	NFC_NXP_NTAG_I2C_BUS

// I2C bus timing details
// timeout
#define I2C_DEFAULT_BUS_TIMEOUT 10
// decrease this value to try to emulate a higher freq clk
#define I2C_DEFAULT_CLK_TOGGLE_DELAY 5

/* ================================================================================ */

static int _i2c_soft_create_bus(unsigned int id, 
		unsigned int sda, unsigned int scl, 
		struct platform_device **apdev);

static void _i2c_soft_delete_bus(struct platform_device **apdev);

/* ================================================================================ */

// global bus definition
static struct platform_device *g_bus = NULL;

/* ================================================================================ */

static int _i2c_soft_create_bus(unsigned int id, 
		unsigned int sda, unsigned int scl, 
		struct platform_device **apdev) {

	struct platform_device *pdev;
	struct i2c_gpio_platform_data pdata;
	int err = 0;

	pdev = platform_device_alloc("i2c-gpio", id);
	if (!pdev) {
		return -ENOMEM;
	}
	
	pdata.sda_pin = sda;
	pdata.scl_pin = scl;
	pdata.udelay = I2C_DEFAULT_CLK_TOGGLE_DELAY;
	pdata.timeout = I2C_DEFAULT_BUS_TIMEOUT;
	pdata.sda_is_open_drain = 0;
	pdata.scl_is_open_drain = 0;
	pdata.scl_is_output_only = 0;

	err = platform_device_add_data(pdev, &pdata, sizeof(pdata));
	if (err) {
		platform_device_put(pdev);
		return err;
	}

	err = platform_device_add(pdev);
	if (err) {
		platform_device_put(pdev);
		return err;
	}

	*apdev = pdev;
	return err;
}


static void _i2c_soft_delete_bus(struct platform_device **apdev) {
	if (*apdev) {
		platform_device_put(*apdev);
	}
}


/* ================================================================================ */

static int sky_bus_init(void) {
	int ret = 0;

	printk(KERN_INFO "Installing I2C Soft Bus\n");

	if ((ret = gpio_request(I2C_GPIO_SDA, "sda"))) {
		return ret;
	}

	if ((ret = gpio_request(I2C_GPIO_SCL, "scl"))) {
		return ret;
	}

	ret = _i2c_soft_create_bus(I2C_DEFAULT_BUS_ID, 
		I2C_GPIO_SDA, 
		I2C_GPIO_SCL, 
		&g_bus);

	return ret;
}


#ifdef MODULE
static int __init sky_i2c_soft_init(void) {
	return sky_bus_init();
}


static void __exit sky_i2c_soft_exit(void) {
	printk(KERN_INFO "Deinstalling I2C Soft Bus\n");
	_i2c_soft_delete_bus(&g_bus);
}


/// module initialization/deinitialization routines
module_init(sky_i2c_soft_init);
module_exit(sky_i2c_soft_exit);
#else

/// perform hardware initialization straight away if compiled in statically
subsys_initcall(sky_bus_init);

#endif

/* ================================================================================ */

/// Module details
MODULE_LICENSE(NFC_BUS_DRV_LICENSE);
MODULE_AUTHOR("Tomasz Wisniewski <tomasz.wisniewski2@bskyb.com>");
MODULE_DESCRIPTION(NFC_BUS_DRV_DESC);
MODULE_VERSION(NFC_BUS_DRV_VERSION);

/* ================================================================================ */

