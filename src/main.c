/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <device.h>
#include <drivers/gpio.h>

#define MS_TO_INTERVAL(ms)  (ms*1.6)
/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define FW_VERSION CONFIG_FW_VERSION
#define DEVICE_NAME  CONFIG_DEVICE_NAME
#define DEVICE_NAME_LENGTH  sizeof(DEVICE_NAME) - 1

struct k_work workItem;

/* Button definition */
static const struct gpio_dt_spec userButton = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static struct gpio_callback buttonCallbackData;

/**
 * Manufacturer data
*/
static uint16_t counter = 0;
static uint8_t * const mfg_data = (uint8_t *) &counter;

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 2),
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LENGTH),
};

/**
 * Job which updates BLE advertising params
*/
void updateBleAdvParams(struct k_work *work){
	int err;

	/**
	 * Update BLE adversiting packets
	*/
	err = bt_le_adv_update_data(
		ad,
		ARRAY_SIZE(ad),
		sd,
		ARRAY_SIZE(sd)
	);

	if (err) {
		printk("Failed to update advertsing data (err %d)\n", err);
		return;
	}
}

/**
 * @brief Button press handler.
*/
void buttonPressed(struct device const * const dev, struct gpio_callback * const cb, uint32_t pins){
	counter++;
	k_work_submit(&workItem);
}

/**
 * @brief Configures given gpio as a button
*/
static void buttonSetup(struct gpio_dt_spec const * const button, struct gpio_callback * const cb){
	int ret;

	if (!device_is_ready(button->port)) {
		printk("Error: button device %s is not ready\n",
		       button->port->name);
		return;
	}

	ret = gpio_pin_configure_dt(button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button->port->name, button->pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button->port->name, button->pin);
		return;
	}

	gpio_init_callback(cb, buttonPressed, BIT(button->pin));
	gpio_add_callback(button->port, cb);
}

void main(void)
{
	int err;

	printk("Starting Broadcaster\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/**Start advertising
	 * Non-connectable
	 * Min Advertising interval: 50 ms.
	 * Max Adversiting interval: 100 ms.
	 * Peer Address: Null
	 */
	err = bt_le_adv_start(
		BT_LE_ADV_PARAM(0, MS_TO_INTERVAL(50), MS_TO_INTERVAL(100), NULL),
		ad,
		ARRAY_SIZE(ad),
		sd,
		ARRAY_SIZE(sd)
	);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	/**
	 * Setup a work item
	*/
	k_work_init(&workItem, updateBleAdvParams);

	/**
	 * Configure button ISR
	*/
	buttonSetup(&userButton, &buttonCallbackData);

	printk("Firmware Version: %s\n", FW_VERSION);
	while(1){

		printk("Advertising data: 0x%02X%02X\n", mfg_data[1], mfg_data[0]);
		k_msleep(2000);
	}
}