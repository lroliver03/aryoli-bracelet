/* Includes */
#include "bt/gatt_svc.h"
#include "common.h"
#include "bt/char/led_char.h"

static const char *TAG = "GATT";

/* Private function prototypes */

static int led_char_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */

static const ble_uuid16_t auto_io_svc_uuid = BLE_UUID16_INIT(0x1815);
static uint16_t led_char_value_handle;
static const ble_uuid128_t led_char_uuid = BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea,
                                                            0x5f, 0x78, 0x23, 0x15,
                                                            0xde, 0xef, 0x12, 0x12,
                                                            0x25, 0x15, 0x00, 0x00);

/* GATT services table */
static const struct ble_gatt_svc_def gatt_services[] = {
  // Automation IO service
  {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &auto_io_svc_uuid.u,
    .characteristics = (struct ble_gatt_chr_def[]){
      {
        .uuid = &led_char_uuid.u,
        .access_cb = led_char_access,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
        .val_handle = &led_char_value_handle
      },
      {0} // No more characteristics in service.
    }
  },
  { // No more services
    0
  }
};

/* Private functions */

static int led_char_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
  int rc = 0;

  ESP_LOGI(TAG, "LED characteristic access.");

  // Handle access events
  switch (ctxt->op) {

    // Read operation
    case BLE_GATT_ACCESS_OP_READ_CHR:
      ESP_LOGI(TAG, "  Operation: Read");

      if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "  Connection handle: %d", conn_handle);
      }
      ESP_LOGI(TAG, "  Attribute handle: %d", attr_handle);

      if (attr_handle == led_char_value_handle) {
        uint8_t led_state_buffer[1];
        led_state_buffer[0] = get_led_state();
        rc = os_mbuf_append(ctxt->om, &led_state_buffer, sizeof(led_state_buffer));
        if (rc != 0) {
          ESP_LOGE(TAG, "Error (%d): Failed to append data to characteristic buffer.", rc);
          return rc;
        }
      }

      break;

    // Write operation
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
      ESP_LOGI(TAG, "  Operation: Write");

      if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "  Connection handle: %d", conn_handle);
      }
      ESP_LOGI(TAG, "  Attribute handle: %d", attr_handle);

      // Check attribute handle
      if (attr_handle == led_char_value_handle) {
        // Check access buffer length
        if (ctxt->om->om_len == 1) {
          // Set/clear LED state
          if (ctxt->om->om_data[0]) {
            set_led();
            ESP_LOGI(TAG, "Set LED state!");
          } else {
            clr_led();
            ESP_LOGI(TAG, "Cleared LED state!");
          }
        } else {
          ESP_LOGE(TAG, "Unexpected buffer length (%d).", ctxt->om->om_len);
          goto error;
        }
      }

      break;

    // Unknown operation
    default:
      ESP_LOGE(TAG, "  Unexpected operation.");
      goto error;
  }

  return rc;

error:
  return BLE_ATT_ERR_UNLIKELY;
}

/* Public functions */

/**
 * @brief GATT server register callback.
 * 
 * @param ctxt GATT register context.
 * @param arg Additional arguments.
 */
void gatt_svr_register_callback(struct ble_gatt_register_ctxt *ctxt, void *arg) {
  char buf[BLE_UUID_STR_LEN];

  // Handle GATT attribute register events
  switch (ctxt->op) {

    // Register service event
    case BLE_GATT_REGISTER_OP_SVC:
      ESP_LOGD(TAG, "Registered service %s.", ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf));
      ESP_LOGD(TAG, "  Handle: %d", ctxt->svc.handle);
      break;

    // Register characteristic event
    case BLE_GATT_REGISTER_OP_CHR:
      ESP_LOGD(TAG, "Registered characteristic %s.", ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf));
      ESP_LOGD(TAG, "  Def handle: %d", ctxt->chr.def_handle);
      ESP_LOGD(TAG, "  Value handle: %d", ctxt->chr.val_handle);
      break;

    // Register descriptor event
    case BLE_GATT_REGISTER_OP_DSC:
      ESP_LOGD(TAG, "Registered descriptor %s.", ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf));
      ESP_LOGD(TAG, "  Handle: %d", ctxt->dsc.handle);
      break;

    // Unknown event
    default:
      ESP_LOGE(TAG, "Unknown register event.");
      assert(0);
      break;
  }
}

/**
 * @brief Initializes GATT services.
 * 
 * @return Error code.
 */
int gatt_svc_init() {
  int rc = 0;

  // Initialize GATT service
  ble_svc_gatt_init();

  // Update GATT services counter
  rc = ble_gatts_count_cfg(gatt_services);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error (%d): Failed to update GATT services counter.", rc);
    return rc;
  }

  // Add GATT services
  rc = ble_gatts_add_svcs(gatt_services);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error (%d): Failed to add GATT services.", rc);
    return rc;
  }

  return rc;
}