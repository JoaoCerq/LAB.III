/*
 * state_machine.c
 *
 *  Created on: Nov 22, 2025
 *      Author: joao0
 */

#include "../states/state_initial.h"
#include "../states/state_online.h"
#include "../states/state_offline.h"



void state_machine()
{
    MachineStates state = INITIAL_STATE;
    MachineStates previous = (MachineStates)(-1); // valor inválido só pra começar
    while(1)
    {
        if (state != previous)
        {
            switch(state)
            {
                case INITIAL_STATE:
                    state_initial_enter();
                    break;
                case ONLINE_STATE:
                    state_online_enter();
                    break;
                case OFFLINE_STATE:
                    state_offline_enter();
                    break;
            }
            previous = state;
        }
        switch(state)
        {
            case INITIAL_STATE:
                state_initial(&state);
                break;
            case ONLINE_STATE:
                state_online(&state);
                break;
            case OFFLINE_STATE:
                state_offline(&state);
                break;
            default:
                state = INITIAL_STATE;
        }

    }
}
