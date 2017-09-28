/*
 * dvia_sender.hpp
 *
 *  Created on: Jan 17, 2016
 */
#ifndef __DVIA_SENDER_H__
#define __DVIA_SENDER_H__

#include "dvia_common.h"     // Common includes and typedefs
//#if DEBUG>1
////#include <opencv2/core/utility.hpp>  // for getTick*()
//#include <opencv2/core/core.hpp>  // for getTick*()
//#endif

#include "hci.hpp"           // HCI - BLE Scanner
#include "gatt.hpp"          // Gatt - BLE connection and transfer

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

};

#endif // __DVIA_SENDER_H__
