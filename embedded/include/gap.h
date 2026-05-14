#ifndef GAP_SVC_H
#define GAP_SVC_H

/* Includes */

// NimBLE GAP APIs
#include "host/ble_gap.h"
#include "services/gap/ble_svc_gap.h"

/* Defines */

#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_URI_PREFIX_HTTPS 0x17
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00
#define BLE_GAP_ADV_RSP_ITVL_MS (BLE_GAP_ADV_ITVL_MS(500))
#define BLE_GAP_ADV_ITVL_MIN_MS (BLE_GAP_ADV_ITVL_MS(500))
#define BLE_GAP_ADV_ITVL_MAX_MS (BLE_GAP_ADV_ITVL_MS(510))

/* Public functions prototypes */

int advertisement_init();
int gap_init();

#endif // GAP_SVC_H