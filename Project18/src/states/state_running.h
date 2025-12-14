/*
 * online_state.h
 *
 *  Created on: Nov 23, 2025
 *      Author: joao0
 */

#ifndef SRC_ONLINE_STATE_ONLINE_STATE_H_
#define SRC_ONLINE_STATE_ONLINE_STATE_H_
#include "../state_machine/state_machine.h"

extern MachineStates state;

void state_online_enter();
void state_online(MachineStates* state);
void state_running_enter();
void state_running();

#endif /* SRC_ONLINE_STATE_ONLINE_STATE_H_ */
