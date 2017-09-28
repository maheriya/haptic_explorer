/*
 * ble.hpp
 *
 *  Created on: Jan 11, 2016
 */

#ifndef __DVIA_BLE_HPP__
#define __DVIA_BLE_HPP__

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <unistd.h>
//extern "C" {
//#include "lib/bluetooth.h"
//#include "lib/l2cap.h"
//#include "lib/hci.h"
//#include "lib/hci_lib.h"
//#include "lib/uuid.h"
//#include "lib/sdp.h"
//#include "attrib/att.h"
//#include "attrib/gattrib.h"
//#include "attrib/gatt.h"
//#include "src/shared/util.h"
////#include "btio/btio.h"
//}
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <cerrno>
#include <array>
#include <iomanip>
#include <vector>
#include <boost/optional.hpp>

#include <signal.h>

#include <stdexcept>

#include <blepp/logging.h>
#include <blepp/pretty_printers.h>
#include <blepp/blestatemachine.h> //for UUID. FIXME mofo
#include <blepp/lescan.h>

using namespace std;
using namespace BLEPP;

// BLE defines
#define EIR_NAME_SHORT     0x08  /* shortened local name */
#define EIR_NAME_COMPLETE  0x09  /* complete local name */
#define BLE_EVENT_TYPE     0x05
#define BLE_SCAN_RESPONSE  0x04

int find_ble_device(string adapter_name, string ble_dev_name, string ble_dev_name_alt, char (&addr) [18]);

#endif // __DVIA_BLE_HPP__
