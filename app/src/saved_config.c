#include "saved_config.h"
#include <zephyr.h>
#include <string.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(saved_config, LOG_LEVEL_INF);

#include <device.h>
#include <i2c.h>
#include <gpio.h>
#include <crc.h>

static struct device *dev;
#define ADDR 0x50
#define PAGE_SIZE 64
#define NR_CONFIG_BACKUP 2

static uint32_t crc(const struct saved_config_t *config)
{
	return crc32_ieee((u8_t *)config, sizeof(config)-sizeof(config->crc));
}

static int write_page(u16_t addr, const u8_t *data, size_t size)
{
	int ret;
	u8_t tx[2+PAGE_SIZE] = {addr >> 8, addr};
	LOG_DBG("write to eeprom(addr=%"PRIu16",size=%zu)", addr, size);

	if (size > PAGE_SIZE)
	{
		LOG_ERR("size > PAGE_SIZE=");
		return -EINVAL;
	}

	if (addr % PAGE_SIZE)
	{
		LOG_ERR("addr %% PAGE_SIZE");
		return -EINVAL;
	}

	memcpy(&tx[2], data, size);

	ret = i2c_write(dev, tx, size+2, ADDR);
	if (ret)
	{
		LOG_ERR("i2c_write=%d", ret);
		return -EIO;
	}

	uint32_t timeout = 100;

	for (;;)
	{
		struct i2c_msg msgs[1];
		u8_t dst;

		/* Send the ADDRess to read from */
		msgs[0].buf = &dst;
		msgs[0].len = 0;
		msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

		ret = i2c_transfer(dev, &msgs[0], 1, ADDR);
		if (ret == 0)
		{
			break;
		}

		if (timeout-- <= 0)
		{
			LOG_ERR("timeout");
			return -EIO;
		}

		k_sleep(1);
	}

	return 0;
}

static int write_config_nr(u8_t nr, const struct saved_config_t *config)
{
	int ret;

	if (nr >= NR_CONFIG_BACKUP)
	{
		return -EINVAL;
	}

	size_t szconfig = ((sizeof(*config)+(PAGE_SIZE-1))/PAGE_SIZE) * PAGE_SIZE;
	size_t n = (sizeof(*config)+(PAGE_SIZE-1))/PAGE_SIZE;
	const u8_t *src = (const u8_t *)config;

	for (size_t i = 0; i < n ; i++)
	{
		ret = write_page(szconfig * nr + i* PAGE_SIZE, &src[PAGE_SIZE * i], PAGE_SIZE);
		if (ret)
		{
			return ret;
		}
	}

	return 0;
}

int saved_config_save(struct saved_config_t *config)
{
	int ret;
	int failed = 0;

	LOG_DBG("saved_config_save");
	config->revision++;
	config->crc = crc(config);

	for (size_t i = 0; i < NR_CONFIG_BACKUP; i++)
	{
		ret = write_config_nr(i, config);
		if (ret)
		{
			LOG_ERR("saved_config_save:failed:%d", ret);
			failed++;
		}
	}

	return failed == NR_CONFIG_BACKUP;
}

static int eeprom_read(u16_t addr, u8_t *data, size_t size)
{
	int ret;
	const u8_t tx[] = {addr >> 8, addr};

	LOG_DBG("read from eeprom (addr=%"PRIu16", size=%zu)", addr, size);
	ret = i2c_write_read(dev, ADDR, tx, ARRAY_SIZE(tx), data, size);
	if (ret)
	{
		LOG_ERR("i2c_write_read=%d", ret);
		return ret;
	}

	return 0;
}

static int saved_config_read_nr(u8_t nr, struct saved_config_t *config)
{
	size_t szconfig = ((sizeof(*config)+(PAGE_SIZE-1))/PAGE_SIZE) * PAGE_SIZE;

	LOG_DBG("saved_config_read_nr (nr=%"PRIu8")", nr);

	return eeprom_read(szconfig * nr, (u8_t *)config, sizeof(*config));
}

void saved_config_default(struct saved_config_t *config)
{
	memset(config, 0x0, sizeof(*config));
	config->version = 1;
}

int saved_config_read(struct saved_config_t *config)
{
	int ret;
	static struct saved_config_t tmp;
	int valid = 0;
	int invalid = 0;
	int latest = 0;

	LOG_DBG("saved_config_read");

	for (size_t i = 0 ; i < NR_CONFIG_BACKUP ; i++)
	{
		ret = saved_config_read_nr(i, &tmp);
		if (ret)
		{
			LOG_ERR("failed");
			invalid = 1;
			continue;
		}

		switch(tmp.version)
		{
			case 1:
			if (crc(&tmp) != tmp.crc)
			{
				LOG_INF("config[%zu] bad crc", i);
				invalid = 1;
				continue;
			}
			break;

			default:
				LOG_INF("config[%zu] unknown revision", i);
				invalid = 1;
				continue;
			break;
		}

		if (!valid)
		{
			LOG_INF("config[%zu] valid", i);
			valid = 1;
			*config = tmp;
			latest = tmp.revision;
			continue;
		}

		if (latest - tmp.revision > 0)
		{
			LOG_INF("config[%zu] valid, revision:%"PRIu32, i, tmp.revision);
			*config = tmp;
			latest = tmp.revision;
		}
	}

	if (valid == 1 && invalid == 0)
	{
		LOG_DBG("All configs in sync");
		return 0;
	}

	if (valid == 0 && invalid == 1)
	{
		LOG_WRN("Zero valid config, initalize config");
		saved_config_default(config);
	}
	else
	{
		LOG_WRN("Found at least one valid config, syncronizing");
	}

	return saved_config_save(config);
}

#include <pinmux/stm32/pinmux_stm32.h>
static const struct pin_config pinconf[] = {
	{STM32_PIN_PB6, STM32_MODER_OUTPUT_MODE | STM32_OTYPER_OPEN_DRAIN},
	{STM32_PIN_PB7, STM32_MODER_OUTPUT_MODE | STM32_OTYPER_OPEN_DRAIN},
};

void pin_setup(void)
{
	static struct device *dev;

	/* Make sure both pins are at '1' to prevent glitches */
	dev = device_get_binding(DT_GPIO_STM32_GPIOB_LABEL);

	/* Prevent glitch on i2c */
	gpio_pin_write(dev, 6, 1);
	gpio_pin_write(dev, 7, 1);

	/* Real pins setup */
	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));
}

void saved_config_init(void)
{
	struct saved_config_t config;
	struct saved_config_t config_backup;
	int ret;

	/* GPIOs MUST be set before using the GPIO_I2C driver */
	pin_setup();

	dev = device_get_binding(CONFIG_I2C_GPIO_0_NAME);
	LOG_DBG("i2c=%p", dev);
	if (!dev)
	{
		LOG_ERR("device_get_binding(%s) failed", CONFIG_I2C_GPIO_0_NAME);
		return;
	}

	ret = i2c_configure(dev, I2C_MODE_MASTER | I2C_SPEED_SET(I2C_SPEED_FAST));
	if (ret)
	{
		LOG_ERR("i2c_configure=%d", ret);
	}

	if (0)
	for (u8_t i = 4; i <= 0x77; i++) {
		struct i2c_msg msgs[1];
		u8_t dst;

		/* Send the ADDRess to read from */
		msgs[0].buf = &dst;
		msgs[0].len = 0;
		msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

		if (i2c_transfer(dev, &msgs[0], 1, i) == 0) {
			LOG_DBG("0x%2x FOUND\n", i);
		}
	}

	ret = saved_config_read(&config);
	if (ret)
	{
		LOG_ERR("saved_config_read=%d", ret);
	}

	config.boot_count++;
	config_backup = config;
	config_backup.revision++;

	ret = saved_config_save(&config);
	if (ret)
	{
		LOG_ERR("saved_config_save=%d", ret);
	}

	ret = saved_config_read(&config);
	if (ret)
	{
		LOG_ERR("saved_config_read=%d", ret);
	}

	if (memcmp(&config, &config_backup, sizeof(config)-sizeof(config.crc)))
	{
		LOG_ERR("config save/load failed");
	}

	LOG_INF("boot count : %"PRIu32, config.boot_count);
	LOG_DBG("config revision : %"PRIu32, config.revision);
}
