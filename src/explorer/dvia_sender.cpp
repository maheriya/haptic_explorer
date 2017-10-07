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
#include <mutex>

static mutex chread_mutex;
static BLEGATTStateMachine gatt;
static Characteristic uart_tx_char(&gatt);
static void catch_function(int) {
    cerr << "\nInterrupted!\n";
    // TODO: Do something to clean up HCI: Add this as an auto function inside the signal() call
    gatt.close();
    std::exit(2);
}

double get_time_of_day() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

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
    const struct timespec Tble = {0, 500000000};

    // BLEPP logging setup
    //log_level = LogLevels::Debug;
    log_level = LogLevels::Warning;

    cout << "==== BLE setup started" << endl;
    string addr = "00:00:00:00:00:00";
    int result;
    result = find_ble_device("hci0", HAPTIC_BAND1, HAPTIC_BAND2, addr);
    if (result != 0) {
        printf("==== ERROR!! Exiting due to error during scanning for the band (result=%d, addr=[%s])\n", result, addr.c_str());
        std::exit(1);
    }

    int count = -1;
    double prev_time = 0;
    float voltage=0;

    ////////////////////////////////////////////////////////////////////////////////
    // This is an example of a callback which responds to notifications or indications.
    // Currently, BLEGATTStateMachine responds automatically to indications.
    // Right now this is just a dummy function
    std::function<void(const PDUNotificationOrIndication&)> notify_cb = [&](const PDUNotificationOrIndication& n) {
        if (count == -1) {
            prev_time = get_time_of_day();
        }
        count++;

        if (count == 10) {
            double t = get_time_of_day();
            cout << 10 / (t-prev_time)  << " packets per second\n";

            prev_time = t;
            count=0;
        }
        cout << "Got notification from DVIA!" << endl;

    };


    //-///////////////////////////////////////////////////////////////////////////////////////
    // Setup callbacks for gatt state machine
    //
    // Following callback will be run when the client characteristic configuration is retrieved.
    // Essentially this is when all the most useful device information has been received and
    // the device can now be used.
    //
    // Usually, we want to search for characteristics to enable/disable features.
    // Following example code activates notifications on a device; this will have to be changed.
    //
    // Search for the service and attribute and set up notifications and the appropriate callback.

    // Write response callback. For now, just print a msg in return
    std::function <void()> cb_write_resp = []() {
        cout << "Write response received" << endl;
    };

    bool enable=true;
    uint8_t data[12] = {0x21, 0x4f, 0x00, 0x7f, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x12};
    std::function <void()> cb = [&data, &cb_write_resp]() {
        //cout << "===============TEMP: Pretty printing services tree" << endl;
        //pretty_print_tree(gatt);
        //lock_guard<mutex> lock(chread_mutex); // guard essentially for chars_read
        for (auto& service: gatt.primary_services) {
            if (service.uuid == UUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e")) {         // UART Service: 6e400001-b5a3-f393-e0a9-e50e24dcca9e
                for (auto& ch: service.characteristics) {
                    if (ch.uuid == UUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e")) { // TX::handle: 0x0025, uuid: 6e400002-b5a3-f393-e0a9-e50e24dcca9e
                        uart_tx_char = ch;
#if DEBUG>2
                        cout << "===============: Found UART TXD service" << endl;
                        printf("===============:\tHandle: 0x%04x\n", ch.value_handle);
#endif
                    }
                }
            }
        }
        gatt.cb_write_response = cb_write_resp; // set up a callback for write response
        // uart_tx_char.write_request(data, 12); // for debug testing
    };

    ////////////////////////////////////////////////////////////////////////////////
    // Set up callback for disconnection
    // Failure to connect also comes here.
    gatt.cb_disconnected = [](BLEGATTStateMachine::Disconnect d) {
        cerr << "Disconnected: " << BLEGATTStateMachine::get_disconnect_string(d) << endl;
        exit(1);
    };


    ////////////////////////////////////////////////////////////////////////////////
    // Query/scan the services of the device with the callback cb.
    // This callback cb can setup all other necessary callbacks.
    gatt.setup_standard_scan(cb);



    // Used to be ble_connect(addr) from gatt.c;
    // Establish a non-blocking connection so that this function can return to caller
    ////////////////////////////////////////////////////////////////////////////////
    // A few errors are handled by exceptions. std::runtime errors happen if nearly fatal
    // but recoverable-with-effort errors happen, such as a failure in socket allocation.
    // It is very unlikely you will encounter a runtime error.
    //
    // std::logic_error happens if you abuse the BLEGATTStateMachine. For example trying
    // to issue a new instruction before the callback indicating the in-progress one has
    // finished has been called. These errors mean the program is incorrect.
    bool blocking=true; // true for blocking connection
    bool addrPub=false; // false for random address type
    try {
        // Connect as a non blocking call
#if DEBUG>2
        cout << "===============: Initiating connection with " << addr << endl;
#endif
        gatt.connect(addr, blocking, addrPub); // Connect to address: addr, connection type: blocking, and addr type: random   // connect_blocking(addr);
#if DEBUG>2
        cout << "===============: Completed connection with " << addr << endl;
#endif
    }
    catch (const std::runtime_error e) {
        cerr << "Connect failed. Something's stopping bluetooth working: " << e.what() << endl;
    }
    catch (const std::logic_error e) {
        cerr << "Connect failed. Check code: " << e.what() << endl;
    }
    catch (...) {
        std::exception_ptr p = std::current_exception();
        cerr << "Connect failed. Unknown error: " << (p ? p.__cxa_exception_type()->name() : "null") << endl;
    }

    // Wait for gatt FSM to be ready
    for (int i = 0;; i++) {
        if (gatt.is_idle()) {
            // Idle is the indication that the FSM is ready after connecting to device
            cout << "Gatt FSM is in Idle state. Init is done." << endl;
            break;
        }
        if (i>200) {
            cout << "Timeout: Gatt FSM didn't go to Idle. Exiting." << endl;
            std::exit(1);
        }
        //-///////////////////////////////////////////////////////////////////////////////////
        // Calling read_and_process_next() is the key to using libblepp. Spend a lot of time
        // reading the libblepp code to figure this out!! Their examples didn't work for our
        // push-to-write application. Having said that, the libblepp code is really well written.
        //
        // Following call moves the FSM through various states as appropriate. We call
        // read_and_process_next repeatedly so that the FSM reads all services and calls all
        // the appropriate callbacks including the callback we set by setup_standard_scan().
        // We quit once we know that scanning has completed. This is indicated by chars_read
        // set by our own callback cb which is the last one to be called. At that point, the
        // FSM will be reset and in Idle state (I had to modify LibBlepp to do this -- there
        // was no other way to reset the FSM as required for our application -- the only other
        // way is to let this run in background forever and handle writes in a foreground
        // thread -- that is exactly what we did with previous gatt.c code and it was not very
        // robust). In this method, we are not setting any explicit threads.
        gatt.read_and_process_next();
    }
    cout << "==== BLE setup done" << endl;

}

int dviaSender::sendToBle(objectData_t& dataToSend) {
    try {
        const uint8_t* data = (const uint8_t*) &dataToSend;
        uart_tx_char.write_request(data, sizeof(dataToSend));
        //gatt.send_write_request(TX_WR_HANDLE, data, sizeof(dataToSend));
        gatt.read_and_process_next(); // wait for response. Need to setup cb_write_response
        // TODO: Wait for write_response?? Can be done. Not done right now.
    }
    catch (std::runtime_error e) {
        cerr << "LibBle++ runtime error: " << e.what() << endl;
    }
    catch (std::logic_error e) {
        cerr << "Error while sending BLE data: " << e.what() << endl;
    }
#if DEBUG>3
    printf("sendToBle: Sent x:%3d, depth:%3d\n", dataToSend.objs[0].x, dataToSend.objs[0].depth);
#endif
}

void dviaSender::stopBle(void) {
    gatt.close();
}


int dviaSender::find_ble_device(string adapter_name, string ble_dev_name, string ble_dev_name_alt, string& addr) {
    int ret = -1;
    HCIScanner::ScanType type = HCIScanner::ScanType::Active;
    HCIScanner::FilterDuplicates filter = HCIScanner::FilterDuplicates::Software;

    filter = HCIScanner::FilterDuplicates::Both;


    HCIScanner scanner(true, filter, type);

    // Catch the interrupt signal. If the scanner is not cleaned up properly, then it doesn't reset the HCI state.
    signal(SIGINT, catch_function);

    while (1) { // This loop will never end until the device is found. This is intentional.
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
            // Only read id if there's something to read
            vector<AdvertisingResponse> ads = scanner.get_advertisements();

            for (const auto& ad: ads) {
                addr = ad.address;
#if DEBUG>1
                cout << "Found device: " << addr << " ";
#endif
                if (ad.type == LeAdvertisingEventType::ADV_IND || ad.type == LeAdvertisingEventType::ADV_DIRECT_IND ||
                    ad.type == LeAdvertisingEventType::ADV_SCAN_IND || ad.type == LeAdvertisingEventType::ADV_NONCONN_IND) {
#if DEBUG>1
                    cout << "ADV_IND|DIRECT_IND|SCAN_IND|NONCONN_IND" << endl;
#endif
                    continue;
                }
#if DEBUG>1
                else {
                    cout << "Scan response" << endl;
                }
#endif
#if DEBUG>2
                for (const auto& uuid: ad.UUIDs)
                    cout << "  Service: " << to_str(uuid) << endl;
                if(ad.rssi == 127)
                    cout << "  RSSI: unavailable" << endl;
                else if(ad.rssi <= 20)
                    cout << "  RSSI = " << (int) ad.rssi << " dBm" << endl;
                else
                    cout << "  RSSI = " << to_hex((uint8_t)ad.rssi) << " unknown" << endl;
#endif

                string name;
                if (ad.local_name) {
                    name = ad.local_name->name;
#if DEBUG>1
                    cout << "  Name: " << name << endl;
#endif
                } else {
                    continue;
                }
                string::size_type found = name.find(ble_dev_name);
                string::size_type found_alt = name.find(ble_dev_name_alt);
                if (found != string::npos || found_alt != string::npos) {
                    string devname = ble_dev_name.c_str();
                    if (found_alt != string::npos) {
                        devname = ble_dev_name_alt.c_str();
                    }
                    cout << "==== Found requested device '" << devname << "' with addr=" << addr << endl;
                    scanner.stop();
                    // Found the device we are interested in. Return with success.
                    return 0;
                }
            }
        }
    }
    return ret;
}


