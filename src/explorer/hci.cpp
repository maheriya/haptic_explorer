/*
 * hci.cpp
 *
 *  Created on: Jan 10, 2016
 */

#include <stdlib.h>
#include <errno.h>
#include "hci.hpp"
static int dvia_le_set_scan_parameters(int dd, uint8_t type, uint16_t interval, uint16_t window,
                                        uint8_t own_type, uint8_t filter, int to) {
        struct hci_request rq;
        le_set_scan_parameters_cp param_cp;
        uint8_t status;

        memset(&param_cp, 0, sizeof(param_cp));
        param_cp.type = type;
        param_cp.interval = interval;
        param_cp.window = window;
        param_cp.own_bdaddr_type = own_type;
        param_cp.filter = filter;

        memset(&rq, 0, sizeof(rq));
        rq.ogf = OGF_LE_CTL;
        rq.ocf = OCF_LE_SET_SCAN_PARAMETERS;
        rq.cparam = &param_cp;
        rq.clen = LE_SET_SCAN_PARAMETERS_CP_SIZE;
        rq.rparam = &status;
        rq.rlen = 1;

        if (hci_send_req(dd, &rq, to) < 0)
                return -1;

        // For some reason, status is always 12 even if things are working. Disable this check
        //if (status) {
        //        errno = EIO;
        //        return -1;
        //}

        return 0;
}

static int dvia_le_set_scan_enable(int dd, uint8_t enable, uint8_t filter_dup, int to) {
        struct hci_request rq;
        le_set_scan_enable_cp scan_cp;
        uint8_t status;

        memset(&scan_cp, 0, sizeof(scan_cp));
        scan_cp.enable = enable;
        scan_cp.filter_dup = filter_dup;

        memset(&rq, 0, sizeof(rq));
        rq.ogf = OGF_LE_CTL;
        rq.ocf = OCF_LE_SET_SCAN_ENABLE;
        rq.cparam = &scan_cp;
        rq.clen = LE_SET_SCAN_ENABLE_CP_SIZE;
        rq.rparam = &status;
        rq.rlen = 1;

        if (hci_send_req(dd, &rq, to) < 0)
                return -1;

        // For some reason, status is always 12 even if things are working. Disable this check
        //if (status) {
        //        errno = EIO;
        //        return -1;
        //}

        return 0;
}



int find_ble_device(string adapter_name, string ble_dev_name, string ble_dev_name_alt, char (&addr) [18]) {
    int      dev_id = hci_devid(adapter_name.c_str());
    int device_desc = hci_open_dev(dev_id);

    //const int device_desc = hci_open_dev(hci_get_route(NULL));
    printf("Bluetooth: device id %d is open for communication device_desc=%0d\n", hci_get_route(NULL), device_desc);
 
    // Enable scan mode
    int result;
    uint8_t scan_type     = 0x01;
    uint16_t interval     = htobs(0x0010);
    uint16_t window       = htobs(0x0010);
    uint8_t own_type      = 0x00;
    uint8_t filter_policy = 0x00;

    if (dvia_le_set_scan_parameters(device_desc, scan_type, interval, window, own_type, filter_policy, 10000) < 0) {
        perror("ERROR! Bluetooth: hci_le_set_scan_params failed");
        return result;
    }

    if (dvia_le_set_scan_enable(device_desc, 0x01, 1, 10000) < 0) {
        perror("ERROR! Bluetooth: hci_le_scan_enable failed");
        return result;
    }

    // Get the advertisements of that device
    struct hci_filter old_options;
    socklen_t slen = sizeof(old_options);
    if (getsockopt(device_desc, SOL_HCI, HCI_FILTER, &old_options, &slen) != 0) {
        perror("Could not get socket options");
        return -1;
    }

    struct hci_filter new_options;
    hci_filter_clear(&new_options);
    hci_filter_set_ptype(HCI_EVENT_PKT, &new_options);
    hci_filter_set_event(EVT_LE_META_EVENT, &new_options);
    if (setsockopt(device_desc, SOL_HCI, HCI_FILTER, &new_options, sizeof(new_options)) < 0) {
        perror("Could not set socket options");
        return -1;
    }

    int len;
    unsigned char buffer[HCI_MAX_EVENT_SIZE];
    int timeout = 5;
    int ts = time(NULL);
    // This loop is to find the ble_dev_name device
    int ret = 0; // return value
    printf("Scanning...\n");
    while (1) {
        // removed select(). It is not required, and was causing the failure
        len = read(device_desc, buffer, sizeof(buffer));

        // process input
        unsigned char* ptr = buffer + HCI_EVENT_HDR_SIZE + 1;
        evt_le_meta_event* meta = (evt_le_meta_event*) ptr;

        if (meta->subevent != 0x02 || (uint8_t) buffer[BLE_EVENT_TYPE] != BLE_SCAN_RESPONSE) {
            continue;
        }

        le_advertising_info* info;
        info = (le_advertising_info*) (meta->data + 1);

        ba2str(&info->bdaddr, addr);

        // Parse name
        size_t offset = 0;
        size_t size   = info->length;
        uint8_t* data = info->data;
        string name   = "";
        while (offset < size) {
            uint8_t field_len = data[0];
            size_t name_len;

            if (field_len == 0 || offset + field_len > size)
                continue;

            switch (data[1]) {
            case EIR_NAME_SHORT:
            case EIR_NAME_COMPLETE:
                name_len = field_len - 1;
                if (name_len > size)
                    continue;

                name = string((const char*) (data + 2), name_len);
            }
            printf("\n");

            offset += field_len + 1;
            data += field_len + 1;
        }
        cout << "find_ble_device: Found device with name [" << name << "]" << endl;
        string::size_type found = name.find(ble_dev_name);
        string::size_type found_alt = name.find(ble_dev_name_alt);
        if (found == string::npos && found_alt == string::npos) {
            printf("find_ble_device: No match yet for %s device!\n", ble_dev_name.c_str());
        } else {
            printf("find_ble_device: Found %s device with address=%s \n", ble_dev_name.c_str(), addr);
            break;
        }
        int elapsed = time(NULL) - ts;
        if (elapsed >= timeout) {
            printf("find_ble_device: Scan timed out\n");
            ret = -1;
            break;
        }
    }

CLEANUP:
    setsockopt(device_desc, SOL_HCI, HCI_FILTER, &old_options, sizeof(old_options));

    // Disable scan
    result = hci_le_set_scan_enable(device_desc, 0x00, 1, 10000);

    // This closing of the hci device should b ein different function
    // Close device -- this is important
    hci_close_dev(device_desc);
    printf("find_ble_device: Closed hci device.\n");
    return ret;
}
