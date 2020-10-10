//
// Created by Jonathan Taylor on 10/6/20.
//

#ifndef PUMPMODULE_CONFIG_H
#define PUMPMODULE_CONFIG_H

// Pin setup

#define cePin 10
#define csPin 9

// TODO: tweak network throttle

#define NETWORK_THROTTLE 1000
#define SERIAL_THROTTLE 50

static uint16_t node_address_set[16] = {00,
                                        01, 02, 03, 04, 05,
                                        011, 012, 013, 014, 015,
                                        021, 022, 023, 024, 025};

#define NODE_ADDRESS 0

/*  0       (00)                        = Master
 *  1-5     (01, 02, 03, 04, 05)        = Children of Master (00)
 *  6-10    (011, 012, 013, 014, 015)   = Children of One (01)
 *  11-15   (021, 022, 023, 024, 025)   = Children of Two (02)
 *
 */

uint16_t this_node = 0;

#endif //PUMPMODULE_CONFIG_H
