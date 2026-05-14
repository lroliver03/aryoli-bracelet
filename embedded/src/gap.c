/* Includes */
#include "gap.h"
#include "common.h"

static const char *TAG = "GAP";

/* Private variables */

static uint8_t own_addr_type;
static uint8_t addr_value[6] = {0};
static uint8_t esp_uri[] = {BLE_GAP_URI_PREFIX_HTTPS, '/', '/', 'a', 'r', 'y', 'o', 'l', 'i'};

/* Private function prototypes */

inline static void format_address(uint8_t addr[], char *addr_str);
static void print_connection_descriptor(struct ble_gap_conn_desc *descriptor);
static int start_advertising();
static int gap_event_handler(struct ble_gap_event *event, void *arg);

/* Private functions */

/**
 * @brief Formats address into string.
 * 
 * @param [out]  addr_str  Output address string.
 * @param [in]  addr  Input address array.
 */
inline static void format_address(uint8_t addr[], char *addr_str) {
  sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X",
          addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

/**
 * @brief Prints connection descriptor to logs.
 * 
 * @param descriptor Connection descriptor.
 */
static void print_connection_descriptor(struct ble_gap_conn_desc *descriptor) {
  char addr_str[18] = {0};

  ESP_LOGI(TAG, "Connection descriptor:");

  // Connection handle
  ESP_LOGI(TAG, "  Connection handle: %d", descriptor->conn_handle);

  // Device address
  format_address(descriptor->our_id_addr.val, addr_str);
  ESP_LOGI(TAG, "  Device ID address type: %d", descriptor->our_id_addr.type);
  ESP_LOGI(TAG, "  Device ID address: %s", addr_str);

  // Peer address
  format_address(descriptor->peer_id_addr.val, addr_str);
  ESP_LOGI(TAG, "  Peer ID address type: %d", descriptor->peer_id_addr.type);
  ESP_LOGI(TAG, "  Peer ID address: %s", addr_str);

  // Other parameters
  ESP_LOGI(TAG, "  Connection interval: %d", descriptor->conn_itvl);
  ESP_LOGI(TAG, "  Connection latency: %d", descriptor->conn_latency);
  ESP_LOGI(TAG, "  Supervision timeout: %d", descriptor->supervision_timeout);
  ESP_LOGI(TAG, "  Encrypted: %d", descriptor->sec_state.encrypted);
  ESP_LOGI(TAG, "  Authenticated: %d", descriptor->sec_state.authenticated);
  ESP_LOGI(TAG, "  Bonded: %d", descriptor->sec_state.bonded);
}

/**
 * @brief Configures and starts advertising.
 * 
 * @return Return code. If 0, OK. Check NimBLE API for reference.
 */
static int start_advertising() {
  int rc = 0; // Return code
  const char *name;
  struct ble_hs_adv_fields advertisement_fields = {0};
  struct ble_hs_adv_fields response_fields = {0};
  struct ble_gap_adv_params advertisement_parameters = {0};

  // Set advertising flags
  advertisement_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

  // Set device name
  name = ble_svc_gap_device_name(); // Get device name from service
  advertisement_fields.name = (uint8_t*)name;
  advertisement_fields.name_len = strlen(name);
  advertisement_fields.name_is_complete = 1;

  // Set device transmission power
  advertisement_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO; // Define power automatically
  advertisement_fields.tx_pwr_lvl_is_present = 1;

  // Set device appearance, use generic tag
  advertisement_fields.appearance = BLE_GAP_APPEARANCE_GENERIC_TAG;
  advertisement_fields.appearance_is_present = 1;

  // Set device LE role as peripheral
  advertisement_fields.le_role = BLE_GAP_LE_ROLE_PERIPHERAL; // "le role" french jumpscare
  advertisement_fields.le_role_is_present = 1;

  // Set advertisement fields
  rc = ble_gap_adv_set_fields(&advertisement_fields);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error (%d): Failed to set advertisement data.", rc);
    return rc;
  }

  // Set device address
  response_fields.device_addr = addr_value;
  response_fields.device_addr_type = own_addr_type;
  response_fields.device_addr_is_present = 1;

  // Set URI
  response_fields.uri = esp_uri;
  response_fields.uri_len = sizeof(esp_uri);

  // Set advertisement response interval
  response_fields.adv_itvl = BLE_GAP_ADV_RSP_ITVL_MS;
  response_fields.adv_itvl_is_present = 1;

  // Set scan response fields
  rc = ble_gap_adv_rsp_set_fields(&response_fields);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error (%d): Failed to scan response fields.", rc);
    return rc;
  }

  // Set undirected connectable and general discoverable mode
  advertisement_parameters.conn_mode = BLE_GAP_CONN_MODE_UND; // Connectable to any device
  advertisement_parameters.disc_mode = BLE_GAP_DISC_MODE_GEN; // Discoverable by any device

  // Set advertising interval range
  advertisement_parameters.itvl_min = BLE_GAP_ADV_ITVL_MIN_MS;
  advertisement_parameters.itvl_max = BLE_GAP_ADV_ITVL_MAX_MS;

  // Start advertising
  rc = ble_gap_adv_start(own_addr_type,             // Address type stack uses for itself
                         NULL,                      // Directed advertising, not the case
                         BLE_HS_FOREVER,            // Advertise forever
                         &advertisement_parameters, // Parameters
                         gap_event_handler,         // Event handler passed as callback 
                         NULL);                     // Event handler arguments

  if (rc != 0) {
    ESP_LOGE(TAG, "Error (%d): Failed to start advertising.", rc);
    return rc;
  }

  char addr_str[18] = {0};
  format_address(addr_value, addr_str);
  ESP_LOGI(TAG, "Started advertising!");
  ESP_LOGI(TAG, "  Name: %s", name);
  ESP_LOGI(TAG, "  Address: %s", addr_str);

  return rc;
}

/**
 * @brief GAP event handler.
 * 
 * @param event 
 * @param arg 
 * @return Return code. If 0, OK. Check NimBLE API for reference.
 */
static int gap_event_handler(struct ble_gap_event *event, void *arg) {
  int rc = 0;
  struct ble_gap_conn_desc descriptor;

  // GAP event type switch
  switch (event->type) {
    
    // Connect event
    case BLE_GAP_EVENT_CONNECT:
      ESP_LOGI(TAG, "GAP event: BLE_GAP_EVENT_CONNECT");

      // New connetion established or connection failed
      ESP_LOGI(TAG, "Connection status: %d -> Connection %s", 
                event->connect.status,
                event->connect.status == 0 ? "established!" : "failed.");

      if (event->connect.status == 0) {
        // If connection succeeded, check connection handle
        rc = ble_gap_conn_find(event->connect.conn_handle, &descriptor);
        if (rc != 0) {
          ESP_LOGE(TAG, "Error (%d): Failed to find connection by handle.", rc);
          return rc;
        }

        // Print connection descriptor
        print_connection_descriptor(&descriptor);

        // Try to update parameters
        struct ble_gap_upd_params parameters = {
          .itvl_min = descriptor.conn_itvl,
          .itvl_max = descriptor.conn_itvl,
          .latency = 3,
          .supervision_timeout = descriptor.supervision_timeout
        };
        rc = ble_gap_update_params(event->connect.conn_handle, &parameters);
        if (rc != 0) {
          ESP_LOGE(TAG, "Error (%d): Failed to update connection parameters.", rc);
          return rc;
        }
      } else {
        // If connection failed, start advertising again.
        start_advertising();
      }
      break;

    // Disconnect event
    case BLE_GAP_EVENT_DISCONNECT:
      ESP_LOGI(TAG, "GAP event: BLE_GAP_EVENT_DISCONNECT");
      ESP_LOGI(TAG, "Disconnected for reason: %d", event->disconnect.reason);

      // Just go back to advertising.
      start_advertising();

      break;

    // Connection parameters update event
    case BLE_GAP_EVENT_CONN_UPDATE:
      ESP_LOGI(TAG, "GAP event: BLE_GAP_EVENT_CONN_UPDATE");
      ESP_LOGI(TAG, "Connection updated. New status: %d", event->conn_update.status);

      // Print descriptor.
      rc = ble_gap_conn_find(event->conn_update.conn_handle, &descriptor);
      if (rc != 0) {
        ESP_LOGE(TAG, "Error (%d): Failed to find connection by handle.", rc);
        return rc;
      }

      // Print connection descriptor
      print_connection_descriptor(&descriptor);
      
      break;

    // Advertising complete event
    case BLE_GAP_EVENT_ADV_COMPLETE:
      ESP_LOGI(TAG, "GAP event: BLE_GAP_EVENT_ADV_COMPLETE");
      ESP_LOGI(TAG, "Advertise complete. Reason: %d", event->adv_complete.reason);
      start_advertising();
      break;

    // Notification sent event
    case BLE_GAP_EVENT_NOTIFY_TX:
      ESP_LOGI(TAG, "GAP event: BLE_GAP_EVENT_NOTIFY_TX");

      // Print notification info on error
      if ((event->notify_tx.status != 0) && (event->notify_tx.status != BLE_HS_EDONE)) {
        ESP_LOGI(TAG, "Notify event.");
        ESP_LOGI(TAG, "  conn_handle: %d", event->notify_tx.conn_handle);
        ESP_LOGI(TAG, "  attr_handle: %d", event->notify_tx.attr_handle);
        ESP_LOGI(TAG, "  status: %d", event->notify_tx.status);
        ESP_LOGI(TAG, "  is_indication: %d", event->notify_tx.indication);
      }

      break;

    // Subscribe event
    case BLE_GAP_EVENT_SUBSCRIBE:
      ESP_LOGI(TAG, "GAP event: BLE_GAP_EVENT_SUBSCRIBE");

      // Print subscription info to log.
      ESP_LOGI(TAG, "Subscribe event.");
      ESP_LOGI(TAG, "  conn_handle: %d", event->subscribe.conn_handle);
      ESP_LOGI(TAG, "  attr_handle: %d", event->subscribe.attr_handle);
      ESP_LOGI(TAG, "  reason: %d", event->subscribe.reason);
      ESP_LOGI(TAG, "  prev_notify: %d", event->subscribe.prev_notify);
      ESP_LOGI(TAG, "  cur_notify: %d", event->subscribe.cur_notify);
      ESP_LOGI(TAG, "  prev_indicate: %d", event->subscribe.prev_indicate);
      ESP_LOGI(TAG, "  cur_indicate: %d", event->subscribe.cur_indicate);
      
      // TODO: GATT subscribe event callback.
      ESP_LOGD(TAG, "TODO: GATT subscription event callback");

      break;

    // MTU update event
    case BLE_GAP_EVENT_MTU:
      ESP_LOGI(TAG, "GAP event: BLE_GAP_EVENT_MTU");

      // Print MTU update info to log.
      ESP_LOGI(TAG, "MTU update event.");
      ESP_LOGI(TAG, "  conn_handle: %d", event->mtu.conn_handle);
      ESP_LOGI(TAG, "  channel_id: %d", event->mtu.channel_id);
      ESP_LOGI(TAG, "  value: %d", event->mtu.value);

      break;
  }

  return rc;
}

/* Public functions */

/**
 * @brief Initializes BLE advertisement.
 * 
 * @returns Return code. If 0, OK. Check NimBLE API for reference.
 */
int advertisement_init() {
  int rc = 0; // Return code.
  char addr_str[18] = {0};

  // Insure proper BT identity address
  rc = ble_hs_util_ensure_addr(0);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error (%d): Device has no available BT address.", rc);
    return rc;
  }

  // Figure out BT address for advertisement
  rc = ble_hs_id_infer_auto(0, &own_addr_type);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error (%d): Failed to infer address type.", rc);
    return rc;
  }

  // Log address
  rc = ble_hs_id_copy_addr(own_addr_type, addr_value, NULL);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error (%d): Failed to copy device address.", rc);
    return rc;
  }
  format_address(addr_value, addr_str);
  ESP_LOGI(TAG, "Device address: %s", addr_str);

  // Start advertising
  rc = start_advertising();
  if (rc != 0) {
    ESP_LOGE(TAG, "Error (%d): Failed to advertise.", rc);
    return rc;
  }

  return rc;
}

int gap_init() {
  int rc = 0;

  ble_svc_gap_init();

  rc = ble_svc_gap_device_name_set(DEVICE_NAME);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error (%d): Failed to set device name to %s", rc, DEVICE_NAME);
    return rc;
  }

  return rc;
}