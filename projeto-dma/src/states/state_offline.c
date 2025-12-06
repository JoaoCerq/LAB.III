/*
 * offline_state.c
 *
 *  Created on: Nov 22, 2025
 *      Author: joao0
 */

#include "state_offline.h"

typedef enum {
    SUB_MENU,
    SUB_GRAVANDO,
    SUB_PROCESSANDO,
    SUB_TOCANDO
} OfflineSubState;

void state_offline_enter()
{
    offline_lcd();
}

void state_offline(MachineStates* state)
{

}
