#include "bluetooth/BtManager.hpp"

class Central : public CentralManager {
protected:
  void on_connected(struct bt_conn *conn, uint8_t err) override;
  void on_disconnected(struct bt_conn *conn, uint8_t reason) override;
  void on_security_changed(struct bt_conn *conn, bt_security_t level,
                           enum bt_security_err err) override;
};
