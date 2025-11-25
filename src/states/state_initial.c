/*
 * initial_state.c
 *
 *  Created on: Nov 22, 2025
 *      Author: joao0
 */

#include <src/drivers/lcd/LCD.h>
#include "state_initial.h"
#include "ezdsp5502.h"
#include "ezdsp5502_i2cgpio.h"
#include "csl_gpio.h"
#include "stdio.h"

void state_initial(MachineStates* state)
{
    /* Setup I2C GPIOs for Switches */
    EZDSP5502_I2CGPIO_configLine(  SW0, IN );
    EZDSP5502_I2CGPIO_configLine(  SW1, IN );

    /* Detect Switches */
    if(!EZDSP5502_I2CGPIO_readLine(SW0))
    {
        while(!EZDSP5502_I2CGPIO_readLine(SW0));
        printf("SW1 Pressed\n");
        *state = ONLINE_STATE;
        return;
    }
    if(!EZDSP5502_I2CGPIO_readLine(SW1))
        {
            while(!EZDSP5502_I2CGPIO_readLine(SW1));
            printf("SW2 Pressed\n");
            *state = OFFLINE_STATE;
            return;
        }
}

void state_initial_enter()
{
    initial_lcd();
}
