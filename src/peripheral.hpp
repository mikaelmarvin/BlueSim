extern "C" {
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
}

class Peripheral {
public:
  Peripheral(uint8_t id);
  virtual ~Peripheral();

  // Called when this peripheral got connected/disconnected
  virtual void onConnected(struct bt_conn *conn);
  virtual void onDisconnected(struct bt_conn *conn, uint8_t reason);

  // Static global connection callbacks
  static void bt_conn_cb_connected(struct bt_conn *conn, uint8_t err);
  static void bt_conn_cb_disconnected(struct bt_conn *conn, uint8_t reason);

  // Start advertising
  int start();

  // Map bt_conn back to Peripheral
  static Peripheral *fromConn(struct bt_conn *conn);

private:
  uint8_t _id;
  struct bt_le_ext_adv *_adv;
  struct bt_data _adv_data[1];
  struct bt_conn *_conn;

  static Peripheral *registry[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
};
