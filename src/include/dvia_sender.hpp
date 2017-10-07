/*
 * dvia_sender.hpp
 *
 *  Created on: Jan 17, 2016
 */
#ifndef __DVIA_SENDER_H__
#define __DVIA_SENDER_H__

#include "dvia_common.h"     // Common includes and typedefs
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <string.h>
#include <stdio.h>
#include <cerrno>
#include <signal.h>
#include <stdexcept>

#include <blepp/logging.h>
#include <blepp/pretty_printers.h>
#include <blepp/blestatemachine.h>
#include <blepp/lescan.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <blepp/blestatemachine.h>
#include <blepp/float.h>
#include <sys/time.h>
#include <unistd.h>
using namespace std;
using namespace BLEPP;

using namespace std;

class dviaSender {
public:
    // Constructor
    dviaSender() {
        //
    }

    int  init(void);
    int  send(objectData_t& dataToSend);
    void stop(void);

    // Destructor
    ~dviaSender() {
        this->stop();
    }

private:
    int  initBle(void);
    int  sendToBle(objectData_t& dataToSend);
    void stopBle(void);
    int find_ble_device(string adapter_name, string ble_dev_name, string ble_dev_name_alt, string& addr);

    // BLEGATTStateMachine: This class does all of the GATT interactions using a callback based interface
    // Provide callbacks to make it do anything useful. It won't start its own main loop.
    //BLEGATTStateMachine gatt;

};

#endif // __DVIA_SENDER_H__
