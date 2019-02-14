/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <led_strip.h>

#include <errno.h>
#include <string.h>

#ifdef DT_COLORWAY_LPD8806_0
#define DT_COLORWAY_LPD880X_0 DT_COLORWAY_LPD8806_0
#define DT_COLORWAY_LPD880X_0_BASE_ADDRESS DT_COLORWAY_LPD8806_0_BASE_ADDRESS
#define DT_COLORWAY_LPD880X_0_BUS_NAME DT_COLORWAY_LPD8806_0_BUS_NAME
#define DT_COLORWAY_LPD880X_0_LABEL DT_COLORWAY_LPD8806_0_LABEL
#define DT_COLORWAY_LPD880X_0_SPI_MAX_FREQUENCY DT_COLORWAY_LPD8806_0_SPI_MAX_FREQUENCY
#else
#define DT_COLORWAY_LPD880X_0 DT_COLORWAY_LPD8803_0
#define DT_COLORWAY_LPD880X_0_BASE_ADDRESS DT_COLORWAY_LPD8803_0_BASE_ADDRESS
#define DT_COLORWAY_LPD880X_0_BUS_NAME DT_COLORWAY_LPD8803_0_BUS_NAME
#define DT_COLORWAY_LPD880X_0_LABEL DT_COLORWAY_LPD8803_0_LABEL
#define DT_COLORWAY_LPD880X_0_SPI_MAX_FREQUENCY DT_COLORWAY_LPD8803_0_SPI_MAX_FREQUENCY
#endif

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lpd880x);

#include <zephyr.h>
#include <device.h>
#include <spi.h>
#include <misc/util.h>

/*
 * LPD880X SPI master configuration:
 *
 * - mode 0 (the default), 8 bit, MSB first, one-line SPI
 * - no shenanigans (no CS hold, release device lock, not an EEPROM)
 */
#define LPD880X_SPI_OPERATION (SPI_OP_MODE_MASTER |	\
			       SPI_TRANSFER_MSB |	\
			       SPI_WORD_SET(8) |	\
			       SPI_LINES_SINGLE)

struct lpd880x_data {
	struct device *spi;
	struct spi_config config;
};

static int lpd880x_update(struct device *dev, void *data, size_t size)
{
	struct lpd880x_data *drv_data = dev->driver_data;
	/*
	 * Per the AdaFruit reverse engineering notes on the protocol,
	 * a zero byte propagates through at most 32 LED driver ICs.
	 * The LPD8803 is the worst case, at 3 output channels per IC.
	 */
	u8_t reset_size = ceiling_fraction(ceiling_fraction(size, 3), 32);
	u8_t reset_buf[reset_size];
	u8_t last = 0x00;
	const struct spi_buf bufs[3] = {
		{
			/* Prepares the strip to shift in new data values. */
			.buf = reset_buf,
			.len = reset_size
		},
		{
			/* Displays the serialized pixel data. */
			.buf = data,
			.len = size
		},
		{
			/* Ensures the last byte of pixel data is displayed. */
			.buf = &last,
			.len = sizeof(last)
		}

	};
	const struct spi_buf_set tx = {
		.buffers = bufs,
		.count = 3
	};
	size_t rc;

	(void)memset(reset_buf, 0x00, reset_size);

	rc = spi_write(drv_data->spi, &drv_data->config, &tx);
	if (rc) {
		LOG_ERR("can't update strip: %d", rc);
	}

	return rc;
}

static int lpd880x_strip_update_rgb(struct device *dev,
				    struct led_rgb *pixels,
				    size_t num_pixels)
{
	u8_t *px = (u8_t *)pixels;
	u8_t r, g, b;
	size_t i;

	/*
	 * Overwrite a prefix of the pixels array with its on-wire
	 * representation, eliminating padding/scratch garbage, if any.
	 */
	for (i = 0; i < num_pixels; i++) {
		r = 0x80 | (pixels[i].r >> 1);
		g = 0x80 | (pixels[i].g >> 1);
		b = 0x80 | (pixels[i].b >> 1);

		/*
		 * GRB is the ordering used by commonly available
		 * LPD880x strips.
		 */
		*px++ = g;
		*px++ = r;
		*px++ = b;
	}

	return lpd880x_update(dev, pixels, 3 * num_pixels);
}

static int lpd880x_strip_update_channels(struct device *dev, u8_t *channels,
					 size_t num_channels)
{
	size_t i;

	for (i = 0; i < num_channels; i++) {
		channels[i] = 0x80 | (channels[i] >> 1);
	}

	return lpd880x_update(dev, channels, num_channels);
}

static int lpd880x_strip_init(struct device *dev)
{
	struct lpd880x_data *data = dev->driver_data;
	struct spi_config *config = &data->config;

	data->spi = device_get_binding(DT_COLORWAY_LPD880X_0_BUS_NAME);
	if (!data->spi) {
		LOG_ERR("SPI device %s not found",
			    DT_COLORWAY_LPD880X_0_BUS_NAME);
		return -ENODEV;
	}

	config->frequency = DT_COLORWAY_LPD880X_0_SPI_MAX_FREQUENCY;
	config->operation = LPD880X_SPI_OPERATION;
	config->slave = DT_COLORWAY_LPD880X_0_BASE_ADDRESS; /* MOSI/CLK only; CS is not supported. */
	config->cs = NULL;

	return 0;
}

static struct lpd880x_data lpd880x_strip_data;

static const struct led_strip_driver_api lpd880x_strip_api = {
	.update_rgb = lpd880x_strip_update_rgb,
	.update_channels = lpd880x_strip_update_channels,
};

DEVICE_AND_API_INIT(lpd880x_strip, DT_COLORWAY_LPD880X_0_LABEL,
		    lpd880x_strip_init, &lpd880x_strip_data,
		    NULL, POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY,
		    &lpd880x_strip_api);
