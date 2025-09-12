extern "C" {
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
}

#include "service.hpp"

struct advertiser_info {
  struct k_work work;
  struct bt_le_ext_adv *adv;
  struct bt_data adv_data[1];
};

class Peripheral {
public:
  Peripheral();
  virtual ~Peripheral();

  // Called when this peripheral got connected/disconnected
  virtual void onConnected(struct bt_conn *conn);
  virtual void onDisconnected(struct bt_conn *conn, uint8_t reason);

  // Static global connection callbacks
  static void bt_conn_cb_connected(struct bt_conn *conn, uint8_t err);
  static void bt_conn_cb_disconnected(struct bt_conn *conn, uint8_t reason);

  // Static work handler: required because k_work cannot call non-static member
  // functions
  static void workHandler(struct k_work *work);

  // Start advertising
  int start();

  // Map bt_conn back to Peripheral
  static Peripheral *fromConn(struct bt_conn *conn);

private:
  void submitAdvertiser();

  uint8_t _id;
  struct advertiser_info _advert;
  struct bt_conn *_conn;
  GattService *_services[MAX_SERVICES_PER_PERIPHERAL];
  uint8_t _serviceCount;

  static Peripheral *registry[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
};
