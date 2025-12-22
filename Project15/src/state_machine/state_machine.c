/*
 * state_machine.c
 *
 *  Created on: Nov 22, 2025
 *      Author: joao0
 */

#include "../states/state_initial.h"
#include "../states/state_running.h"

MachineStates state = STATE_INITIAL;
MachineStates previous = (MachineStates)(-1); // valor inválido só pra começar

void state_machine()
{

    while(1)
    {
        if (state != previous)
        {
            switch(state)
            {
                case STATE_INITIAL:
                    state_initial_enter();
                    break;
                case STATE_RUNNING:
                    state_running_enter();
                    break;
            }
            previous = state;
        }
        switch(state)
        {
            case STATE_INITIAL:
                state_initial(&state);
                break;
            case STATE_RUNNING:
                state_running(&state);
                break;
            default:
                state = STATE_INITIAL;
        }

    }
}
