#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SipSmrtTest, LOG_LEVEL_INF);

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>

#include "sensor_ble.h"

// For testing
#include <dk_buttons_and_leds.h>
#define USER_LED DK_LED3

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define COMPANY_ID_CODE 0x0069 //NEED TO SET THIS WHEN MANUFACTURING --> become bluetooth SIG memeber to get Company ID
//#define SIPSMRT_SERVICE_UUID    BT_UUID_128_ENCODE(0xf13a6b98, 0x6ee2, 0x4f4c, 0x9091, 0x7ce8dce35b60) //This is random   

#define NOTIFY_INTERVAL 250
#define STACKSIZE 1024
#define PRIORITY 7

static uint32_t app_FSR_val = 5;
static uint32_t app_BAT_state = 95;

struct bt_conn *my_conn = NULL;
static struct k_work adv_work;

// Advertising options 800 <-> 801 --> 500 <-> 500.625 
static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN|BT_LE_ADV_OPT_USE_IDENTITY, 800, 801, NULL);

// Declare Manufacutring Data
typedef struct adv_mfg_data {
        uint16_t company_code;
} adv_mfg_data_type;
static adv_mfg_data_type adv_mfg_data = {COMPANY_ID_CODE};

// Advertising Packet
static const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
        BT_DATA(BT_DATA_MANUFACTURER_DATA, (unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),
};


// Scan Response Packet (currently empty)
static const struct bt_data sd[] = { 
        BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_SIPSMRT_VAL)
};


// Advertising handler
static void adv_work_handler(struct k_work *work) {
        int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

        if (err) {
                printk("Advertising failed to start (err %d)\n", err);
                return;
        }

        printk("Advertising successfully started\n");
}

// Advertising start
static void advertising_start(void) {
        k_work_submit(&adv_work);
}

//Setting PHY mode
// static void update_phy(struct bt_conn *conn) {
//         int err;
//         const struct bt_conn_le_phy_param preferred_phy = {
//                 .options = BT_CONN_LE_PHY_OPT_NONE,
//                 //Set PHY mode below
//                 .pref_rx_phy = BT_GAP_LE_PHY_1M,
//                 .pref_tx_phy = BT_GAP_LE_PHY_1M,
//         };
//         err = bt_conn_le_phy_update(conn, &preferred_phy);
//         if (err) {
//                 LOG_ERR("bt_conn_le_phy_update() returned %d", err);
//         }
// }

// On Connected (CB)
void on_connected(struct bt_conn *conn, uint8_t err) {
        if (err) {
                LOG_ERR("Connection error %d", err);
                return;
        }
        LOG_INF("Connected");
        my_conn = bt_conn_ref(conn);


        // Get connection INFO
        struct bt_conn_info info;
        err = bt_conn_get_info(conn, &info);
        if (err) {
                LOG_ERR("bt_conn_get_info() returned %d", err);
                return;
        }
        double connection_interval = info.le.interval * 1.25; //in ms
        uint16_t supervision_timeout = info.le.timeout * 10; //in ms
        LOG_INF("Connection parameters: interval %.2f ms, latency %d intervals, timeout %d ms", connection_interval, info.le.latency, supervision_timeout);

}

// On Disconnected (CB)
void on_disconnected(struct bt_conn *conn, uint8_t reason) {
        LOG_INF("Disconnected. Reason %d", reason);
        bt_conn_unref(my_conn);
}

// On Recycled --> a connection object is returned to the pool = typically when you have enoucntered a disconnection and on of the connections has been unfereenced; therefore can start advertising again
void on_recycled(void) {
        advertising_start();
}

// On connection param update
void on_le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout) {
        double connection_interval = interval * 1.25;
        uint16_t supervision_timeout = timeout * 10;
        LOG_INF("Connection parameters update: interval %.2f ms, latency %d intervals, timeout %d ms", connection_interval, latency, supervision_timeout);
}

// // On PHY updated - IGNORED --> Central has complex nigotiation -- leave it up to the central to decide
// void on_le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param) {
//         if (param->tx_phy == BT_GAP_LE_PHY_1M) {
//                 LOG_INF("PHY updated. New PHY: 1M");
//         } else if (param->tx_phy == BT_GAP_LE_PHY_2M) {
//                 LOG_INF("PHY updated. New PHY: 2M");
//         } else if (param->tx_phy == BT_GAP_LE_PHY_CODED) {
//                 LOG_INF("PHY updated. New PHY: Long range");
//         }
// }

// Callback for the CAL
static void app_CAL_cb(bool CAL_state) {
        dk_set_led(USER_LED, CAL_state);
        LOG_INF("**** CAL state changed ****");
}

// // Callback for the FSR bv    
// static int app_FSR_cb(void) {
//         return app_FSR_val;
// }
static void simulate_data(void) {
        app_FSR_val++;
        if (app_FSR_val == 200) {
                app_FSR_val = 100;
        }
}

void send_data_thread(void) {
        while(1){
                simulate_data();
                my_sensor_send_FSR_notify(app_FSR_val);
                k_sleep(K_MSEC(NOTIFY_INTERVAL));
        }
}

K_THREAD_DEFINE(send_data_thread_id, STACKSIZE, send_data_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

// Callback for the BAT
static int app_BAT_cb(void) {
        return app_BAT_state;
}

// Callbacks -- GATT exchange
static struct my_sensor_cb app_callbacks = {
        .CAL_cb = app_CAL_cb,
        //.FSR_cb = app_FSR_cb,
        .BAT_cb = app_BAT_cb,
};



// Callbacks -- BT connection
BT_CONN_CB_DEFINE(connection_callbacks) = {
        .connected = on_connected,
        .disconnected = on_disconnected, 
        .recycled = on_recycled,
        .le_param_updated = on_le_param_updated,
        //.le_phy_updated = on_le_phy_updated,
};


int main(void)    
{
        int err;
        int blink_status = 0;

        LOG_INF("Starting DUT");

        /* Initialize DK LEDs (required before dk_set_led) */
        err = dk_leds_init();
        if (err) {
                LOG_ERR("dk_leds_init failed (err %d)", err);
                return -1;
        }

        bt_addr_le_t addr;
        err = bt_addr_le_from_str("FF:EE:DD:CC:BB:AA", "random", &addr); //create new random static address
        if (err) {
                printk("Invalid BT address (err %d)\n", err);
        }
        err = bt_id_create(&addr, NULL); //convert address to string for BLE
        if (err < 0) {
                printk("Creating new ID failed (err %d)\n", err);
        }


        // Enable BLE on board
        err = bt_enable(NULL);
        if (err) {
                LOG_ERR("Bluetooth init failed (err %d)", err);
                return -1;
        }
        LOG_INF("BLE initialized");


        k_work_init(&adv_work, adv_work_handler);
        advertising_start();

        LOG_INF("Advertising successfully started");

        // Pass application callback functions store in app_callbacks to the My Sensor service
        err = sensor_init(&app_callbacks);
        if (err) {
                printk("Failed to init LBS (err:%d)\n", err);
        }
        //Forever loop
        for (;;) {
                dk_set_led(RUN_STATUS_LED, (++blink_status) & 0x1);
                k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
        }
}
      