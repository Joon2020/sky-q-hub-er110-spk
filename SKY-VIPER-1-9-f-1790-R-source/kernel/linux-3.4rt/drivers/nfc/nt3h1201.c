#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <asm/byteorder.h>
#include <linux/delay.h>

#include <sky_nfc.h>

/* ================================================================================ */

// common module definitions
#define NFC_DRV_LICENSE "GPL"
#define NFC_DRV_DESC "NXP NTAG NFC NT3H1201 SKY Driver"
#define NFC_DRV_VERSION "0.0.3"

/* ================================================================================ */

#define NXP_CHIP_WRITE_DELAY 30

/// defines the number of bytes stored in a single page
#define NFC_PAGE_SIZE 0x10

#define NFC_PAGE_REGISTERS 0x00
#define NFC_PAGE_USER_MEMORY_FIRST 0x01
#define NFC_PAGE_USER_MEMORY_LAST 0x77
#define NFC_PAGE_DYN_LOCKS 0x78
#define NFC_PAGE_CONFIG 0x7a
#define NFC_PAGE_SRAM_FIRST 0xf8
#define NFC_PAGE_SRAM_LAST 0xfb
#define NFC_PAGE_SESSION_REGS 0xfe

// enumeration of the configuration bytes
#define NTAG_CONF_BYTE_NC_REG 0
#define NTAG_CONF_BYTE_LAST_NDEF_PAGE 1
#define NTAG_CONF_BYTE_SM_REG 2
#define NTAG_CONF_BYTE_WDT_LS 3
#define NTAG_CONF_BYTE_WDT_MS 4
#define NTAG_CONF_BYTE_I2C_CLOCK_STR 5
#define NTAG_CONF_BYTE_REG_LOCK_NS_REG 6

// nc_reg register bit enumeration
#define NC_REG__I2C_RST_ON_OFF 7
#define NC_REG_PTHRU_ON_OFF 6
#define NC_REG_FD_OFF_1 5
#define NC_REG_FD_OFF_0 4
#define NC_REG_FD_ON_1 3
#define NC_REG_FD_ON_0 2
#define NC_REG_SRAM_MIRROR_ON_OFF 1
#define NC_REG_PTHRU_DIR 0

// reg_lock register bit enumeration
#define REG_LOCK_DIS_CONF_I2C 1
#define REG_LOCK_DIS_CONF_RF 0

// i2c_clock_str bit enumeration
#define I2C_CLOCK_STR 0

/* ================================================================================ */

// prototypes
static int _read_page(struct i2c_client *client, u8 page, u8 *buffer);
static int _write_page(struct i2c_client *client, u8 page, u8 *buffer);
static int _read_session_register(struct i2c_client *client, u8 reg, u8 *buffer) __attribute__((unused));
static int _write_session_register(struct i2c_client *client, u8 reg, u8 mask, u8 data);

/* ================================================================================ */

/**
 * @brief addresses
 */
static const unsigned short i2c_addresses[] = {
	NFC_NXP_NTAG_I2C_ADDR,
	I2C_CLIENT_END,
};


/**
 * @brief chip enumeration
 */
enum i2c_chips { 
	nt3h1201,
};


/*
 * Driver data (common to all clients)
 */
static const struct i2c_device_id nt3h1201_id[] = {
	{ NFC_NXP_NTAG_NAME, nt3h1201 },
	{  }
};
MODULE_DEVICE_TABLE(i2c, nt3h1201_id);


/* ================================================================================ */

struct nt3h1201_data {
	struct i2c_client *client;
	struct mutex lock; // prevent from concurrent access

	struct bin_attribute *eeprom;
	struct bin_attribute *sram;
};

/* ================================================================================ */

static int _read_page(struct i2c_client *client, u8 page, u8 *buffer) {

	if (unlikely(!buffer)) {
		return -EINVAL;
	}
	return i2c_smbus_read_i2c_block_data(client, page, NFC_PAGE_SIZE, buffer);
}


static int _write_page(struct i2c_client *client, u8 page, u8 *buffer) {
	if (unlikely(!buffer)) {
		return -EINVAL;
	}
	return i2c_smbus_write_i2c_block_data(client, page, NFC_PAGE_SIZE, buffer);
}


static int _read_session_register(struct i2c_client *client, u8 reg, u8 *buffer) {

	if (unlikely(!buffer)) {
		return -EINVAL;
	}

	i2c_smbus_write_i2c_block_data(client, NFC_PAGE_SESSION_REGS, 1, &reg);
	msleep_interruptible(NXP_CHIP_WRITE_DELAY);
	*buffer = i2c_smbus_read_byte(client);

	return 0;
}


static int _write_session_register(struct i2c_client *client, u8 reg, u8 mask, u8 data) {
	u8 buff[4] = { reg, mask, data };
	return i2c_smbus_write_i2c_block_data(client, NFC_PAGE_SESSION_REGS, 3, buff);
}


static void _sky_nt3h1201_init(struct i2c_client *client) {
	struct device *dev = &client->dev;
	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};
	u8 sync = 0;

	// lock
	mutex_lock(&chip->lock);

	_read_page(chip->client, NFC_PAGE_CONFIG, data);

	// disable clock stretching
	if (data[NTAG_CONF_BYTE_I2C_CLOCK_STR] != 0x01) {
		data[NTAG_CONF_BYTE_I2C_CLOCK_STR] = 0x01;
		sync = 1;
	}

	// disable access to configuration bytes through RF
	if (data[NTAG_CONF_BYTE_REG_LOCK_NS_REG] != 0x01) {
		data[NTAG_CONF_BYTE_REG_LOCK_NS_REG] = 0x01;
		sync = 1;
	}

	// reset the def ndef addr
	if (data[NTAG_CONF_BYTE_LAST_NDEF_PAGE] != 0x00) {
		data[NTAG_CONF_BYTE_LAST_NDEF_PAGE] = 0x00;
		sync = 1;
	}

	// disable the RF -> TAG write (RF read only)
	if ((data[NTAG_CONF_BYTE_NC_REG] & (0x01 << NC_REG_PTHRU_DIR))) {

		data[NTAG_CONF_BYTE_NC_REG] &= ~(0x01 << NC_REG_PTHRU_DIR);

		// update session register as well
		_write_session_register(chip->client, 
				NTAG_CONF_BYTE_NC_REG, 
				(0x01 << NC_REG_PTHRU_DIR), data[NTAG_CONF_BYTE_NC_REG]);

		msleep_interruptible(NXP_CHIP_WRITE_DELAY);
		sync = 1;
	}

	if (sync) {
		dev_warn(dev, "Syncing configuration settings");
		_write_page(chip->client, NFC_PAGE_CONFIG, data);
	}

	// unlock
	mutex_unlock(&chip->lock);
}


static ssize_t _sky_nt3h1201_eeprom_read(struct file *filp, 
		struct kobject *kobj,
		struct bin_attribute *attr,
		char *buf, loff_t off, size_t count) {

	struct i2c_client	*client;
	struct nt3h1201_data *chip;
	int	result;

	client = kobj_to_i2c_client(kobj);
	chip = i2c_get_clientdata(client);

	if (unlikely(off >= chip->eeprom->size)) return 0;

	if ((off + count) > chip->eeprom->size) {
		count = chip->eeprom->size - off;
	}

	if (unlikely(!count)) return count;

	result = 
		_read_page(client, NFC_PAGE_USER_MEMORY_FIRST + (off/16), buf);

	if (result < 0)
		dev_err(&client->dev, "%s error %d\n", "eeprom read", result);

	return result;
}

static ssize_t _sky_nt3h1201_eeprom_write(struct file *filp, 
		struct kobject *kobj,
		struct bin_attribute *attr,
		char *buf, loff_t off, size_t count) {

	struct i2c_client *client;
	struct nt3h1201_data *chip;
	u8 data[NFC_PAGE_SIZE] = {0x00};
	int	result;

	client = kobj_to_i2c_client(kobj);
	chip = i2c_get_clientdata(client);

	if (unlikely(off >= chip->eeprom->size)) {
		return -EFBIG;
	}

	if ((off + count) > chip->eeprom->size) {
		count = chip->eeprom->size - off;
	}

	if (unlikely(!count)) return count;

	mutex_lock(&chip->lock);

	count = count > NFC_PAGE_SIZE ? NFC_PAGE_SIZE : count;
	memcpy(data, buf, count);

	result = 
		_write_page(client, NFC_PAGE_USER_MEMORY_FIRST + (unsigned int)(off/16), data);

	// wait a while for the hardware
	msleep_interruptible(NXP_CHIP_WRITE_DELAY);
	mutex_unlock(&chip->lock);

	if (result < 0) {
		dev_err(&client->dev, "%s error %d\n", "eeprom write", result);
		return result;
	}

	return count;
}

/* ================================================================================ */

static ssize_t dlockbytes_show(struct device *dev,
				   struct device_attribute *attr, char *buf) {
	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};
	u8 i = 0;
	int len = 0;

	_read_page(chip->client, NFC_PAGE_DYN_LOCKS, data);

	while (i<2) {

		u8 b = 0;
		while (b<8) {
			int first = (((i<<3) + b) << 5) + 16;
			int last = first + 31;

			if (i == 1) {
				if (b == 6) {
					last = 479;
				}
				else if (b == 7) {
					break;
				}
			}

			len += sprintf(buf + len, "Bit: %d - page %d-%d: %s\n", 
					((i<<3) + b),
					first,
					last,
					(data[i] & (0x01 << b)) ? "locked" : "unlocked");
			b++;
		}
		i++;
	}
	return len;
}


static ssize_t dlockbytes_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t len) {
	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};
	unsigned int *value = (unsigned int *)data;

	mutex_lock(&chip->lock);

	if (unlikely(!sscanf(buf, "%04x", value))) {
		dev_warn(dev, "Invalid value: [%s]", buf);
		return -EINVAL;
	}

	le32_to_cpus(value);
	_write_page(chip->client, NFC_PAGE_DYN_LOCKS, data);
	mutex_unlock(&chip->lock);

	return len;
}

static DEVICE_ATTR(lockpages, S_IRUGO | S_IWUSR, dlockbytes_show, dlockbytes_store);

/* ================================================================================ */

static ssize_t uid_show(struct device *dev,
				   struct device_attribute *attr, char *buf) {
	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};

	_read_page(chip->client, NFC_PAGE_REGISTERS, data);
	return sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", 
			data[0],
			data[1],
			data[2],
			data[3],
			data[4],
			data[5],
			data[6]);
}

static DEVICE_ATTR(uid, S_IRUGO, uid_show, NULL);

/* ================================================================================ */

static ssize_t cc_show(struct device *dev,
				   struct device_attribute *attr, char *buf) {
	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};
	_read_page(chip->client, NFC_PAGE_REGISTERS, data);
	return sprintf(buf, "%02x:%02x:%02x:%02x\n", 
			data[12],
			data[13],
			data[14],
			data[15]);
}


static ssize_t cc_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t len) {
	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};
	u32 *value = (u32 *)&data[12];

	mutex_lock(&chip->lock);
	_read_page(chip->client, NFC_PAGE_REGISTERS, data);

	if (unlikely(!sscanf(buf, "%08x", value))) {
		dev_warn(dev, "Invalid value: [%s]", buf);
		return -EINVAL;
	}

	if (unlikely(data[0] != 0x04)) {
		dev_warn(dev, "Read error");
		return -EINVAL;
	}

	// this must be restored manually since there is no
	// read access to first byte and chip always returns 0x04
	data[0] = (NFC_NXP_NTAG_I2C_ADDR << 1);

	dev_info(dev, "Written: 0x%02x%02x%02x%02x\n", 
			data[12], data[13],
			data[14], data[15]);

	_write_page(chip->client, NFC_PAGE_REGISTERS, data);
	mutex_unlock(&chip->lock);
	return len;
}

static DEVICE_ATTR(cc, S_IRUGO | S_IWUSR, cc_show, cc_store);

/* ================================================================================ */

static ssize_t erase_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t len) {

	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};
	int i = 0;

	if (buf && buf[0] != '1') {
		dev_warn(dev, "echo 1 in order to erase the EEPROM\n");
		return -EINVAL;
	}

	mutex_lock(&chip->lock);

	for (i = NFC_PAGE_USER_MEMORY_FIRST; i <= NFC_PAGE_USER_MEMORY_LAST; i++) {
		_write_page(chip->client, i, data);
		msleep_interruptible(NXP_CHIP_WRITE_DELAY);
	}

	mutex_unlock(&chip->lock);
	return len;
}

static DEVICE_ATTR(erase, S_IWUSR, NULL, erase_store);

/* ================================================================================ */

static ssize_t pass_thru_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t len) {

	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};

	if (buf && buf[0] != '1' && buf[0] != '0') {
		dev_warn(dev, "echo 0/1 to dis/en pass through mode\n");
		return -EINVAL;
	}

	mutex_lock(&chip->lock);
	_read_page(chip->client, NFC_PAGE_CONFIG, data);

	if (buf[0] == '0') {
		// disable
		if ((data[NTAG_CONF_BYTE_NC_REG] & (0x01 << NC_REG_PTHRU_ON_OFF))) {
			data[NTAG_CONF_BYTE_NC_REG] &= ~(0x01 << NC_REG_PTHRU_ON_OFF);
			_write_page(chip->client, NFC_PAGE_CONFIG, data);
		}
	}
	else {
		// enable
		if (!(data[NTAG_CONF_BYTE_NC_REG] & (0x01 << NC_REG_PTHRU_ON_OFF))) {
			data[NTAG_CONF_BYTE_NC_REG] |= (0x01 << NC_REG_PTHRU_ON_OFF);
			_write_page(chip->client, NFC_PAGE_CONFIG, data);
		}
	}

	mutex_unlock(&chip->lock);
	return len;
}

static ssize_t pass_thru_show(struct device *dev,
				   struct device_attribute *attr, char *buf) {

	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};
	_read_page(chip->client, NFC_PAGE_CONFIG, data);
	return sprintf(buf, "%d\n", data[NTAG_CONF_BYTE_NC_REG] & (0x01 << NC_REG_PTHRU_ON_OFF));
}

static DEVICE_ATTR(pass_thru, S_IRUGO | S_IWUSR, pass_thru_show, pass_thru_store);

/* ================================================================================ */

static ssize_t pass_thru_dir_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t len) {

	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};

	if (buf && buf[0] != '1' && buf[0] != '0') {
		dev_warn(dev, "echo 0(I2C->RF)/1(RF->I2C) in order to change the data flow direction in pass through mode\n");
		return -EINVAL;
	}

	mutex_lock(&chip->lock);

	_read_page(chip->client, NFC_PAGE_CONFIG, data);

	if (buf[0] == '0') {
		// disable
		if ((data[NTAG_CONF_BYTE_NC_REG] & (0x01 << NC_REG_PTHRU_DIR))) {

			data[NTAG_CONF_BYTE_NC_REG] &= ~(0x01 << NC_REG_PTHRU_DIR);
			_write_page(chip->client, NFC_PAGE_CONFIG, data);

			msleep_interruptible(NXP_CHIP_WRITE_DELAY);

			// update session register as well
			_write_session_register(chip->client, 
					NTAG_CONF_BYTE_NC_REG, 
					(0x01 << NC_REG_PTHRU_DIR), data[NTAG_CONF_BYTE_NC_REG]);
		}
	}
	else {
		// enable
		if (!(data[NTAG_CONF_BYTE_NC_REG] & (0x01 << NC_REG_PTHRU_DIR))) {

			data[NTAG_CONF_BYTE_NC_REG] |= (0x01 << NC_REG_PTHRU_DIR);
			_write_page(chip->client, NFC_PAGE_CONFIG, data);

			msleep_interruptible(NXP_CHIP_WRITE_DELAY);

			// update session register as well
			_write_session_register(chip->client, 
					NTAG_CONF_BYTE_NC_REG, 
					(0x01 << NC_REG_PTHRU_DIR), data[NTAG_CONF_BYTE_NC_REG]);
		}
	}

	mutex_unlock(&chip->lock);
	return len;
}

static ssize_t pass_thru_dir_show(struct device *dev,
				   struct device_attribute *attr, char *buf) {


	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};
	_read_page(chip->client, NFC_PAGE_CONFIG, data);
	return sprintf(buf, "%d\n", 
			data[NTAG_CONF_BYTE_NC_REG] & (0x01 << NC_REG_PTHRU_DIR));
}

static DEVICE_ATTR(pass_thru_dir, S_IRUGO | S_IWUSR, pass_thru_dir_show , pass_thru_dir_store);

/* ================================================================================ */

static ssize_t sram_mirror_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t len) {

	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};

	if (buf && buf[0] != '1' && buf[0] != '0') {
		dev_warn(dev, "echo 0/1 to dis/en sram mirror mode\n");
		return -EINVAL;
	}

	mutex_lock(&chip->lock);
	_read_page(chip->client, NFC_PAGE_CONFIG, data);

	if (buf[0] == '0') {
		// disable
		if ((data[NTAG_CONF_BYTE_NC_REG] & (0x01 << NC_REG_SRAM_MIRROR_ON_OFF))) {
			data[NTAG_CONF_BYTE_NC_REG] &= ~(0x01 << NC_REG_SRAM_MIRROR_ON_OFF);
			_write_page(chip->client, NFC_PAGE_CONFIG, data);
		}
	}
	else {
		// enable
		if (!(data[NTAG_CONF_BYTE_NC_REG] & (0x01 << NC_REG_SRAM_MIRROR_ON_OFF))) {
			data[NTAG_CONF_BYTE_NC_REG] |= (0x01 << NC_REG_SRAM_MIRROR_ON_OFF);
			_write_page(chip->client, NFC_PAGE_CONFIG, data);
		}
	}

	mutex_unlock(&chip->lock);
	return len;
}

static ssize_t sram_mirror_show(struct device *dev,
				   struct device_attribute *attr, char *buf) {

	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};
	_read_page(chip->client, NFC_PAGE_CONFIG, data);

	return sprintf(buf, "%d\n", 
			data[NTAG_CONF_BYTE_NC_REG] & (0x01 << NC_REG_SRAM_MIRROR_ON_OFF));
}

static DEVICE_ATTR(sram_mirror, S_IRUGO | S_IWUSR, sram_mirror_show, sram_mirror_store);

/* ================================================================================ */

static ssize_t sram_mirror_addr_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t len) {

	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};
	unsigned int value = 0x00;

	mutex_lock(&chip->lock);
	_read_page(chip->client, NFC_PAGE_CONFIG, data);

	if (unlikely(!sscanf(buf, "%02x", &value))) {
		dev_warn(dev, "Invalid value: [%s]", buf);
		return -EINVAL;
	}

	data[NTAG_CONF_BYTE_SM_REG] = value;
	_write_page(chip->client, NFC_PAGE_CONFIG, data);
	mutex_unlock(&chip->lock);
	return len;
}

static ssize_t sram_mirror_addr_show(struct device *dev,
				   struct device_attribute *attr, char *buf) {

	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};
	_read_page(chip->client, NFC_PAGE_CONFIG, data);
	return sprintf(buf, "%02x\n", data[NTAG_CONF_BYTE_SM_REG]);
}

static DEVICE_ATTR(sram_mirror_addr, S_IRUGO | S_IWUSR, sram_mirror_addr_show, sram_mirror_addr_store);

/* ================================================================================ */

static ssize_t conf_show(struct device *dev,
				   struct device_attribute *attr, char *buf) {

	struct nt3h1201_data *chip =  dev_get_drvdata(dev);
	u8 data[NFC_PAGE_SIZE] = {0x00};
	_read_page(chip->client, NFC_PAGE_CONFIG, data);
	return sprintf(buf, "%02x %02x %02x %02x %02x %02x %02x %02x\n", 
			data[0],
			data[1],
			data[2],
			data[3],
			data[4],
			data[5],
			data[6],
			data[7]);
}

static DEVICE_ATTR(conf, S_IRUGO, conf_show, NULL);

/* ================================================================================ */
static struct attribute *sysfs_attrs_ctrl[] = {
	&dev_attr_lockpages.attr,
	&dev_attr_uid.attr,
	&dev_attr_cc.attr,

	&dev_attr_conf.attr,
	&dev_attr_erase.attr,
	&dev_attr_pass_thru.attr,
	&dev_attr_pass_thru_dir.attr,
	&dev_attr_sram_mirror.attr,
	&dev_attr_sram_mirror_addr.attr,
	NULL
};

static struct attribute_group nt3h1201_attrs[] = {
	{
		.name = "config",
		.attrs = sysfs_attrs_ctrl, 
	},
};

/* ================================================================================ */

static int sky_nt3h1201_probe(struct i2c_client *client, const struct i2c_device_id *id) {
	int err = 0;
	struct device *dev = &client->dev;
	struct i2c_adapter *adapter = to_i2c_adapter(dev->parent);
	struct nt3h1201_data *chip;

	printk(KERN_INFO "Probing for nt3h1201 presence\n");

	chip = kzalloc(sizeof *chip, GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	i2c_set_clientdata(client, chip);
	chip->client  = client;
	mutex_init(&chip->lock);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		dev_err(&chip->client->dev, "The bus doesn't support byte transfers\n");
		err = -EIO;
		goto label_failure;
	}

	err = sysfs_create_group(&chip->client->dev.kobj,
			nt3h1201_attrs);

	if (err < 0) {
		dev_err(&chip->client->dev, "Sysfs registration failed\n");
		goto label_failure;
	}

	if (!(chip->eeprom = kzalloc(sizeof(struct bin_attribute), GFP_KERNEL))) {
		err = -ENOMEM;
		goto label_failure;
	}

	if (!(chip->sram = kzalloc(sizeof(struct bin_attribute), GFP_KERNEL))) {
		err = -ENOMEM;
		goto label_failure;
	}

	chip->eeprom->attr.name = "eeprom";
	chip->eeprom->attr.mode = S_IRUGO | S_IWUSR;
	sysfs_bin_attr_init(chip->eeprom);
	chip->eeprom->size = 
		(NFC_PAGE_USER_MEMORY_LAST - NFC_PAGE_USER_MEMORY_FIRST + 1)*16;
	chip->eeprom->write = _sky_nt3h1201_eeprom_write;
	chip->eeprom->read = _sky_nt3h1201_eeprom_read;

	if ((err = sysfs_create_bin_file(&chip->client->dev.kobj, chip->eeprom))) {
		goto label_failure;
	}

	chip->sram->attr.name = "sram";
	chip->sram->attr.mode = S_IRUGO | S_IWUSR;
	sysfs_bin_attr_init(chip->sram);
	chip->sram->size = 64;
	chip->sram->write = NULL;
	chip->sram->read = NULL;

	if ((err = sysfs_create_bin_file(&chip->client->dev.kobj, chip->sram))) {
		goto label_failure;
	}

	_sky_nt3h1201_init(client);
	return 0;

label_failure:
	if (chip->eeprom) kfree(chip->eeprom);
	if (chip->sram) kfree(chip->sram);
	kfree(chip);
	return err;
}


static int sky_nt3h1201_remove(struct i2c_client *client) {
	struct nt3h1201_data *chip = i2c_get_clientdata(client);
	sysfs_remove_group(&chip->client->dev.kobj, nt3h1201_attrs);
	kfree(chip);
	return 0;
}

/* ================================================================================ */

/**
 * @brief driver definition
 */
static struct i2c_driver sky_nt3h1201_driver = {
	.driver = {
		.name = NFC_NXP_NTAG_NAME,
		.owner = THIS_MODULE,
	},

	.probe = sky_nt3h1201_probe,	
	.remove = sky_nt3h1201_remove,
	.id_table	= nt3h1201_id,
	.address_list	= i2c_addresses,
};


static int sky_nfc_init(void) {
	printk(KERN_INFO "Initializing " NFC_NXP_NTAG_NAME "\n");
	return i2c_add_driver(&sky_nt3h1201_driver);
}


#ifdef MODULE
static int __init sky_nt3h1201_init(void) {
	return sky_nfc_init();
}


static void __exit sky_nt3h1201_exit(void) {
	printk(KERN_INFO "Deinitializing " NFC_NXP_NTAG_NAME "\n");
	return i2c_del_driver(&sky_nt3h1201_driver);
}


/// module initialization/deinitialization routines
module_init(sky_nt3h1201_init);
module_exit(sky_nt3h1201_exit);
#else

/// perform hardware initialization straight away if compiled in statically
subsys_initcall(sky_nfc_init);

#endif

/* ================================================================================ */

/// Module details
MODULE_LICENSE(NFC_DRV_LICENSE);
MODULE_AUTHOR("Tomasz Wisniewski <tomasz.wisniewski2@bskyb.com>");
MODULE_DESCRIPTION(NFC_DRV_DESC);
MODULE_VERSION(NFC_DRV_VERSION);

/* ================================================================================ */

