/*
 * online_state.c
 *
 *  Created on: Nov 23, 2025
 *      Author: joao0
 */
#include "state_initial.h"
#include "ezdsp5502.h"
#include "ezdsp5502_i2cgpio.h"
#include "csl_gpio.h"
#include "stdio.h"
#include "src/drivers/lcd/LCD.h"
#include "../state_machine/state_machine.h"
#include "state_online.h"


#include "../effects/tremolo.h"

typedef enum {
    ONLINE_E0,
    ONLINE_E3,
    ONLINE_E6,
    ONLINE_E9,
    ONLINE_E12,
    ONLINE_E15,
    ONLINE_E18,
    ONLINE_E21,
    ONLINE_E24,

    ONLINE_E0_PROCESSANDO,
    ONLINE_E3_PROCESSANDO
} OnlineSubState;

static OnlineSubState _subState = ONLINE_E0;
static OnlineSubState _subStatePrevious = ONLINE_E0;
void online_E0(OnlineSubState* _subState);
void online_E3(OnlineSubState* _subState);
void online_E0_processando(OnlineSubState* _subState);
void online_E3_processando(OnlineSubState* _subState);

void state_online(MachineStates* state)
{
    if( _subState != _subStatePrevious)
    {
        switch(_subState)
        {
            case ONLINE_E0:
                online_e0_lcd();
                break;
            case ONLINE_E0_PROCESSANDO:
                processando_lcd();
                break;
            case ONLINE_E3:
                online_e3_lcd();
                break;
            case ONLINE_E3_PROCESSANDO:
                Tremolo_init(N, TREM_FR, TREM_FS);
                processando_lcd();
                break;
            case ONLINE_E6:
                //online_e6_lcd();
                break;
            case ONLINE_E9:
                //online_e9_lcd();
                break;
            case ONLINE_E12:
                //online_e12_lcd();
                break;
            case ONLINE_E15:
                //online_e15_lcd();
                break;
            case ONLINE_E18:
                //online_e18_lcd();
                break;
            case ONLINE_E21:
                //online_e21_lcd();
                break;
            case ONLINE_E24:
                //online_e24_lcd();
                break;

        }
        _subStatePrevious = _subState;
    }
    switch(_subState)
    {
        case ONLINE_E0:
            online_E0(&_subState);
            break;
        case ONLINE_E0_PROCESSANDO:
            online_E0_processando(&_subState);
            break;
        case ONLINE_E3:
            online_E3(&_subState);
            break;
        case ONLINE_E3_PROCESSANDO:
            online_E3_processando(&_subState);
            break;
        case ONLINE_E6:
            //online_E6(&_subState);
            break;
        case ONLINE_E9:
            //online_E9(&_subState);
            break;
        case ONLINE_E12:
            //online_E12(&_subState);
            break;
        case ONLINE_E15:
            //online_E15(&_subState);
            break;
        case ONLINE_E18:
            //online_E18(&_subState);
            break;
        case ONLINE_E21:
            //online_E21(&_subState);
            break;
        case ONLINE_E24:
            //online_E24(&_subState);
            break;

    }

}


void state_online_enter()
{

    online_lcd();
}

void online_E0(OnlineSubState* _subState)
{
    /* Setup I2C GPIOs for Switches */
    EZDSP5502_I2CGPIO_configLine(  SW0, IN );
    EZDSP5502_I2CGPIO_configLine(  SW1, IN );

    /* Detect Switches */
    if(!EZDSP5502_I2CGPIO_readLine(SW0))
    {
        while(!EZDSP5502_I2CGPIO_readLine(SW0));
        *_subState = ONLINE_E3;
        return;
    }
    if(!EZDSP5502_I2CGPIO_readLine(SW1))
    {
        while(!EZDSP5502_I2CGPIO_readLine(SW1));
        *_subState = ONLINE_E0_PROCESSANDO;
        return;
    }
}

void online_E0_processando(OnlineSubState* _subState)
{
    if(!EZDSP5502_I2CGPIO_readLine(SW0)) // PARAR
    {
        while(!EZDSP5502_I2CGPIO_readLine(SW0));
        *_subState = ONLINE_E0;
        return;
    }

//    effects_E0();
}

void online_E3(OnlineSubState* _subState)
{
    /* Setup I2C GPIOs for Switches */
    EZDSP5502_I2CGPIO_configLine(  SW0, IN );
    EZDSP5502_I2CGPIO_configLine(  SW1, IN );

    /* Detect Switches */
    if(!EZDSP5502_I2CGPIO_readLine(SW0))
    {
        while(!EZDSP5502_I2CGPIO_readLine(SW0));
        //*_subState = ONLINE_E6;  // ainda não
        return;
    }
    if(!EZDSP5502_I2CGPIO_readLine(SW1))
    {
        while(!EZDSP5502_I2CGPIO_readLine(SW1));

        *_subState = ONLINE_E3_PROCESSANDO;
        return;
    }
}

void online_E3_processando(OnlineSubState* _subState)
{
    if(!EZDSP5502_I2CGPIO_readLine(SW0)) // PARAR
    {
        while(!EZDSP5502_I2CGPIO_readLine(SW0));
        *_subState = ONLINE_E3;
        return;
    }

    tremolo();
}
