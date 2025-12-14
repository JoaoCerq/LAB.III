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

#define ROOM_SIZE 0.9f    // Tamanho da sala (0.0 a 1.0). Maior = eco mais longo;.
#define DAMPING 0.1f      // Abafamento (0.0 a 1.0). 0.0 = som limpo; 1.0 = som abafado.
#define WET_LEVEL 0.25f    // Volume do efeito (0.0 a 1.0). Quanto de "eco" voc� quer ouvir na mistura.
#define DRY_LEVEL 0.75f    // Volume do som original (0.0 a 1.0). (Geralmente soma-se Wet+Dry <= 1.0).

#define TARGET_NOTE 220.65f // Frequência alvo em Hz

#include <src/effects/reverb.h>
#include <src/effects/autotune.h>

void running_lcd();
void effects_initDMA(float room_size,
                    float damping,
                    float wet_level,
                    float dry_level,
                    float pitch_factor);


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

    effects_initDMA(
                ROOM_SIZE,   // room_size
                DAMPING,   // damping
                WET_LEVEL,   // wet_level
                DRY_LEVEL, // dry_level
                TARGET_NOTE); //do autotune
}



