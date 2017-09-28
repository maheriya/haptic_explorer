/*
 * dvia_send.cpp
 *
 *  Created on: Jan 17, 2016
 *  Description: Class encapsulating communication to the band via
 *  1. socket (+external Python app) implementation and (removed now)
 *  2. direct bluez internal API based communication.
 *
 */

#include <dvia_sender.hpp>
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


int dviaSender::init(void) {
	initBle();
}

int dviaSender::send(objectData_t& dataToSend) {
    sendToBle(dataToSend);
}

void dviaSender::stop(void) {
    stopBle();
}

int dviaSender::initBle(void) {
	// TODO: Initiate BLE connection
    const struct timespec Tble = {0, 500000000};
    g_print("==== initBle: BLUETOOTH setup started\n");
    char addr[18] = "00:00:00:00:00:00"; //"FE:B4:97:71:6C:2F";
    int result;
    result = find_ble_device("hci0", HAPTIC_BAND1, HAPTIC_BAND2, addr);
    if (result != 0) {
        g_print("==== initBle: Halting due to error scanning for the band (result=%d, addr=[%s])\n", result, addr);
        while(1);
    }
    ble_connect(addr); // this is going to run a task in a separate thread asynchronously.
    g_print("==== initBle: BLUETOOTH setup done\n");

}

int dviaSender::sendToBle(objectData_t& dataToSend) {
    (void)ble_send_pkt((uint8_t*)&dataToSend, sizeof(objectData_t));
#if DEBUG>2
    g_print("sendToBle: Sent x:%3d, depth:%3d\n", dataToSend.objs[0].x, dataToSend.objs[0].depth);
#endif
}

void dviaSender::stopBle(void) {
    ble_disconnect();
}


int dviaSender::find_ble_device(string adapter_name, string ble_dev_name, string ble_dev_name_alt, char (&addr) [18]) {
    int ret = -1;
    HCIScanner::ScanType type = HCIScanner::ScanType::Active;
    HCIScanner::FilterDuplicates filter = HCIScanner::FilterDuplicates::Software;

    filter = HCIScanner::FilterDuplicates::Both;


    log_level = LogLevels::Warning;
    HCIScanner scanner(true, filter, type);

    //Catch the interrupt signal. If the scanner is not
    //cleaned up properly, then it doesn't reset the HCI state.
    signal(SIGINT, catch_function);

    //Something to print to demonstrate the timeout.
    string throbber="/|\\-";

    //hide cursor, to make the throbber look nicer.
    cout << "[?25l" << flush;

    int i=0;
    while (1) {
        // Check to see if there's anything to read from the HCI
        // and wait if there's not.
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 300000;

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(scanner.get_fd(), &fds);
        int err = select(scanner.get_fd()+1, &fds, NULL, NULL,  &timeout);

        // Interrupted, so quit and clean up properly.
        if (err < 0 && errno == EINTR)
            break;

        if (FD_ISSET(scanner.get_fd(), &fds)) {
            //Only read id there's something to read
            vector<AdvertisingResponse> ads = scanner.get_advertisements();

            for (const auto& ad: ads) {

                if (ad.type == LeAdvertisingEventType::ADV_IND || ad.type == LeAdvertisingEventType::ADV_DIRECT_IND ||
                   ad.type == LeAdvertisingEventType::ADV_SCAN_IND || ad.type == LeAdvertisingEventType::ADV_NONCONN_IND)
                    continue;
                else {
                    cout << "Found device: " << ad.address << " ";
                    cout << "Scan response" << endl;
                }
                string name;
                if (ad.local_name) {
                    name = ad.local_name->name;
                    cout << "  Name: " << name << endl;
                } else {
                    continue;
                }
                for (const auto& uuid: ad.UUIDs)
                    cout << "  Service: " << to_str(uuid) << endl;
                string::size_type found = name.find(ble_dev_name);
                string::size_type found_alt = name.find(ble_dev_name_alt);
                if (found == string::npos && found_alt == string::npos) {
                    printf("find_ble_device: No match yet for %s device!\n", ble_dev_name.c_str());
                } else {
                    printf("find_ble_device: Found %s device with address=%s \n", ble_dev_name.c_str(), addr);
                    ret = 1;
                    break;
                }

                if(ad.rssi == 127)
                    cout << "  RSSI: unavailable" << endl;
                else if(ad.rssi <= 20)
                    cout << "  RSSI = " << (int) ad.rssi << " dBm" << endl;
                else
                    cout << "  RSSI = " << to_hex((uint8_t)ad.rssi) << " unknown" << endl;
            }
        } else
            cout << throbber[i%4] << "\b" << flush;
        i++;
    }

    //show cursor
    cout << "[?25h" << flush;

    return ret;
}
