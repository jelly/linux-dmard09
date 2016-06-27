/*
 *  3-axis accelerometer driver for DMARD09 3-axis sensor.
 *
 * Copyright (c) 2016, Jelle van der Waa <jelle@vdwaa.nl>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/gpio/consumer.h>
#include <linux/iio/sysfs.h>

#define DMARD09_DRV_NAME	"dmard09"
#define DMARD09_REG_CONTROL	0x1d

#define DMARD09_REG_CHIPID      0x18
#define DMARD09_REG_CTRL	0x00
#define DMARD09_REG_DATA	0x01
#define DMARD09_REG_STAT	0x0A
#define DMARD09_REG_X		0x0C
#define DMARD09_REG_Y		0x0E
#define DMARD09_REG_Z		0x10

#define BUF_DATA_LEN 8
#define DMARD09_AXIS_X 0
#define DMARD09_AXIS_Y 1
#define DMARD09_AXIS_Z 2

/* Used for dev_info() */
struct dmard09_data {
	struct i2c_client *client;
	struct device *dev;
};

static const struct iio_chan_spec dmard09_channels[] = {
	{
		.type = IIO_ACCEL,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
		.modified = 1,
		.address = DMARD09_REG_X,
		.channel2 = IIO_MOD_X,
	},
	{
		.type = IIO_ACCEL,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
		.modified = 1,
		.address = DMARD09_REG_Y,
		.channel2 = IIO_MOD_Y,
	},
	{
		.type = IIO_ACCEL,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
		.modified = 1,
		.address = DMARD09_REG_Z,
		.channel2 = IIO_MOD_Z,
	}
};

static int dmard09_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int *val, int *val2, long mask)
{
	int ret;
	u8 buf[BUF_DATA_LEN] = {0};
	struct dmard09_data *data = iio_priv(indio_dev);

	switch (mask) {
		case IIO_CHAN_INFO_RAW:
			ret = i2c_smbus_read_i2c_block_data(data->client, DMARD09_REG_STAT, BUF_DATA_LEN, buf);
			/* FIXME: fix error messages. */
			if (ret == 0) {
				dev_info(data->dev, "Cannot read accelerometer data");
			} else if (ret < 0) {
				dev_info(data->dev, "Error reading!");
			} else {
				if (chan->address == DMARD09_REG_X)
					*val = (s16)((buf[(DMARD09_AXIS_X+1)*2+1] << 8) | (buf[(DMARD09_AXIS_X+1)*2]));
				if (chan->address == DMARD09_REG_Y)
					*val = (s16)((buf[(DMARD09_AXIS_Y+1)*2+1] << 8) | (buf[(DMARD09_AXIS_Y+1)*2]));
				if (chan->address == DMARD09_REG_Z)
					*val = (s16)((buf[(DMARD09_AXIS_Z+1)*2+1] << 8) | (buf[(DMARD09_AXIS_Z+1)*2]));
			}
			return 1;
	}

	return -EINVAL;
}

static const struct iio_info dmard09_info = {
	.read_raw 		= dmard09_read_raw,
};

static int dmard09_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret;
	struct iio_dev *indio_dev;
	struct dmard09_data *data;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev) {
		dev_err(&client->dev, "iio allocation failed!\n");
		return -ENOMEM;
	}

	data = iio_priv(indio_dev);
	data->dev = &client->dev;
	data->client = client;

	i2c_set_clientdata(client, indio_dev);
	indio_dev->dev.parent = &client->dev;
	indio_dev->name = DMARD09_DRV_NAME;
	indio_dev->modes = INDIO_DIRECT_MODE; // ????
	indio_dev->channels = dmard09_channels;
	indio_dev->num_channels = 3; // FIXME
	indio_dev->info = &dmard09_info;

	ret = iio_device_register(indio_dev);
	if (ret < 0) {
		dev_err(&client->dev,
			"unable to register iio device %d\n", ret);
	}

	#define DMARD09_REG_CTRL	0x00
	u8 buf[3] = {0};
	ret = i2c_smbus_read_i2c_block_data(data->client, 0x18, 1, buf);
	if (ret < 0) {
		/* Failed.. */
	}

	#define VALUE_INIT_READY        0x02    /*IC init ok*/
	#define VALUE_WHO_AM_I			0x95	/* D09 WMI */
	if (buf[0] == VALUE_WHO_AM_I)
		dev_info(&client->dev, "dmard09 init ready");
	else
		dev_info(&client->dev, "dmard09 init failed");
	

	return 0;
}

static int dmard09_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);
	iio_device_unregister(indio_dev);

	return 0;
}

static const struct i2c_device_id dmard09_id[] = {
	{ "dmard09",	0},
	{ },
};

MODULE_DEVICE_TABLE(i2c, dmard09_id);

static struct i2c_driver dmard09_driver = {
	.driver = {
		.name = DMARD09_DRV_NAME
	},
	.probe = dmard09_probe,
	.remove = dmard09_remove,
	.id_table = dmard09_id,
};

module_i2c_driver(dmard09_driver);

MODULE_AUTHOR("Jelle van der Waa <jelle@vdwaa.nl>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DMARD09 3-axis accelerometer driver");
