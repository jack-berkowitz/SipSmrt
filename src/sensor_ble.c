#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "sensor_ble.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(Lesson4_Exercise1);

static bool notify_FSR_enabled;
int FSR_VAL;
int BAT_VAL;
static struct my_sensor_cb sensor_cb;



/* STEP 6 - Implement the write callback function of the LED characteristic */

/* STEP 5 - Implement the read callback function of the Button characteristic */

//// FSR read callback function
// static ssize_t read_FSR(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buff, uint16_t len, uint16_t offset) {
//     // Pointer to FSR val whihc is passed in the BT_GATT_CHARACTERISTIC() and stored in attr -> user_data
//     const char *value = attr->user_data;
//     LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

//     if (sensor_cb.FSR_cb) {
//         // Call the application callback function to update/get the current value of the FSR
//         FSR_VAL = sensor_cb.FSR_cb();
//         return bt_gatt_attr_read(conn, attr, buff, len, offset, value, sizeof(*value));
//     }

//     return 0;
// }

static void sensorbc_ccc_FSR_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    notify_FSR_enabled = (value == BT_GATT_CCC_NOTIFY);
}

// BAT read callback function
static ssize_t read_BAT(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buff, uint16_t len, uint16_t offset) {
    // Pointer to BAT val whihc is passed in the BT_GATT_CHARACTERISTIC() and stored in attr -> user_data
    const char *value = attr->user_data;
    LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

    if (sensor_cb.BAT_cb) {
        // Call the application callback function to update/get the current value of the BAT
        BAT_VAL = sensor_cb.BAT_cb();
        return bt_gatt_attr_read(conn, attr, buff, len, offset, value, sizeof(*value));
    }

    return 0;
}

// Calibration write callback function
static ssize_t write_CAL(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buff, uint16_t len, uint16_t offset, uint8_t flags) {
    LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle, (void *)conn);

    if (len != 1U) {
        LOG_DBG("Write CAL: Incorrect data length");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    if (offset != 0) {
        LOG_DBG("Write CAL: Incorrect data offset");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if (sensor_cb.CAL_cb) {
        // Read the recieved value
        uint8_t val = *((uint8_t *)buff);

        if (val == 0x00 || val == 0x01) {
            // Call the application callback function to update the CAL state
            sensor_cb.CAL_cb(val? true : false);
        } else {
            LOG_DBG("Write CAL: Incorrect value");
            return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
        }
    }

    return len;
}


/* Sensor Service Declaration */

BT_GATT_SERVICE_DEFINE(my_sensor_svc, 
    BT_GATT_PRIMARY_SERVICE(BT_UUID_SIPSMRT), 
    BT_GATT_CHARACTERISTIC(BT_UUID_SIPSMRT_FSR, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(sensorbc_ccc_FSR_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_SIPSMRT_BAT, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_BAT, NULL, &BAT_VAL),
    BT_GATT_CHARACTERISTIC(BT_UUID_SIPSMRT_CAL, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE, NULL, write_CAL, NULL),
);

/* A function to register application callbacks for the FSR, BAT, & CAL characteristics  */
int sensor_init(struct my_sensor_cb *callbacks)
{
	if (callbacks) {
		sensor_cb.FSR_cb = callbacks->FSR_cb;
        sensor_cb.BAT_cb = callbacks->BAT_cb;
        sensor_cb.CAL_cb = callbacks->CAL_cb;
	}

	return 0;
}

int my_sensor_send_FSR_notify(uint32_t FSR_VAL) {
    if (!notify_FSR_enabled) {
        return -EACCES;
    }

    return bt_gatt_notify(NULL, &my_sensor_svc.attrs[2], &FSR_VAL, sizeof(FSR_VAL));
}
