/*
 * offline_state.h
 *
 *  Created on: Nov 22, 2025
 *      Author: joao0
 */

#ifndef SRC_OFFLINE_STATE_OFFLINE_STATE_H_
#define SRC_OFFLINE_STATE_OFFLINE_STATE_H_
#include "../state_machine/state_machine.h"

void state_offline_enter();
void state_offline(MachineStates* state);

#endif /* SRC_OFFLINE_STATE_OFFLINE_STATE_H_ */
