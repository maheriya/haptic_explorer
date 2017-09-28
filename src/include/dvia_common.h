/*
 * dvia_common.h
 *
 *  Created on: Jan 09, 2016
 *  Description: A common include file for dvia project. Keep this C friendly.
 */
#ifndef __DVIA_COMMON_H__
#define __DVIA_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
//
#define HAPTIC_BAND1       "Adafruit Bluefruit LE"
#define HAPTIC_BAND2       "DVIA"
#define DEBUG              2
#define SEND_TO_BLUEZ      1

#define GUI_ENABLED        1
#define DEBUG_THREADS      0

typedef enum { Object = 0, Stairs = 1, Curb = 2, Door = 3 } ObjType_t;

// Total 12 bytes. Packed structure. Handles up to two objects.
#define MAX_HAPTIC_OBJECTS  2 // maximum number of simultaneous objects rendered via haptics
typedef struct __attribute__ ((packed)) _objectData {
    uint8_t       sop      :8;  // B0: for BLE transfer. Always '!' (start of packet mark)
    uint8_t       pkt_type :8;  // B1: for BLE transfer. 'O' for object data type. Can use others for control packets
    struct __attribute__ ((packed)) _object {            // B2-B7 (6 bytes)
        ObjType_t obj      :8;  //   Type of object
        uint8_t   x        :8;  //   0: max left,   127: center, 255: max right
        uint8_t   depth    :8;  //   0: Closest; 255: farthest
    } objs[MAX_HAPTIC_OBJECTS];
    uint32_t      _rsvd     :24;  // B8-10
    uint8_t       cksum    :8;   // B11
} objectData_t;

//TODO: USE libblepp instead of Bluez

#endif // __DVIA_COMMON_H__
