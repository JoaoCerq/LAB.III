/*
 * state_machine.h
 *
 *  Created on: Nov 22, 2025
 *      Author: joao0
 */

#ifndef SRC_STATE_MACHINE_STATE_MACHINE_H_
#define SRC_STATE_MACHINE_STATE_MACHINE_H_

void state_machine();

typedef enum {
    INITIAL_STATE,
    ONLINE_STATE,
    OFFLINE_STATE
} MachineStates;



#endif /* SRC_STATE_MACHINE_STATE_MACHINE_H_ */
