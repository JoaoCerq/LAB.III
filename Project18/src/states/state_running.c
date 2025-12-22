/*
 * online_state.c
 *
 *  Created on: Nov 23, 2025
 *      Author: joao0
 */
#include <src/effects/reverb.h>
#include "state_initial.h"
#include "ezdsp5502.h"
#include "ezdsp5502_i2cgpio.h"
#include "csl_gpio.h"
#include "stdio.h"
#include "src/drivers/lcd/LCD.h"
#include "../state_machine/state_machine.h"
#include "state_running.h"



#include <src/effects/reverb.h>
#include <src/effects/pitch_shifter.h>

void running_lcd();



void state_running(MachineStates* state)
{
    EZDSP5502_I2CGPIO_configLine(  SW0, IN );
    while(1){
        /* Setup I2C GPIOs for Switches */


        /* Detect Switches */
        if(!EZDSP5502_I2CGPIO_readLine(SW0))
        {
            while(!EZDSP5502_I2CGPIO_readLine(SW0));
            *state = STATE_INITIAL;  // STOP
            return;
        }
        processIfReady();
    }

}


void state_running_enter()
{
    running_lcd();


}



