#ifndef GATT_SVR_H
#define GATT_SVR_H

/* Includes */
// NimBLE GATT APIs
#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"

// NimBLE GAP APIs
#include "host/ble_gap.h"

/* Public functions */
void gatt_svr_register_callback(struct ble_gatt_register_ctxt *ctxt, void *arg);
void gatt_svr_subscribe_callback(struct ble_gap_event *event);
int gatt_svc_init();

#endif // GATT_SVR_H