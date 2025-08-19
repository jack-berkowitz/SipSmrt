#pragma once //IDK THIS
#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/bluetooth/uuid.h>

/* Service UUID (bytes for adv + handle for GATT) */
#define BT_UUID_SIPSMRT_VAL       BT_UUID_128_ENCODE(0xf13a6b12, 0x6ee2, 0x4f4c, 0x9091, 0x7ce8dce35b60)
#define BT_UUID_SIPSMRT           BT_UUID_DECLARE_128(BT_UUID_SIPSMRT_VAL)

/* FSR Characteristic UUID */
#define BT_UUID_SIPSMRT_FSR_VAL   BT_UUID_128_ENCODE(0xf13a6b13, 0x6ee2, 0x4f4c, 0x9091, 0x7ce8dce35b60)
#define BT_UUID_SIPSMRT_FSR       BT_UUID_DECLARE_128(BT_UUID_SIPSMRT_FSR_VAL)

/* Battery Characteristic UUID */
#define BT_UUID_SIPSMRT_BAT_VAL   BT_UUID_128_ENCODE(0xf13a6b14, 0x6ee2, 0x4f4c, 0x9091, 0x7ce8dce35b60)
#define BT_UUID_SIPSMRT_BAT       BT_UUID_DECLARE_128(BT_UUID_SIPSMRT_BAT_VAL)

/* Calibrate Characteristic UUID */
#define BT_UUID_SIPSMRT_CAL_VAL   BT_UUID_128_ENCODE(0xf13a6b15, 0x6ee2, 0x4f4c, 0x9091, 0x7ce8dce35b60)
#define BT_UUID_SIPSMRT_CAL       BT_UUID_DECLARE_128(BT_UUID_SIPSMRT_CAL_VAL)

/* App callback types */
typedef void (*CAL_cb_t)(bool CAL);
typedef int  (*FSR_cb_t)(void);
typedef int  (*BAT_cb_t)(void);

struct my_sensor_cb {
    FSR_cb_t FSR_cb;
    BAT_cb_t BAT_cb;
    CAL_cb_t CAL_cb;
};

/* Service init */
int sensor_init(struct my_sensor_cb *callbacks);

#ifdef __cplusplus
}
#endif
