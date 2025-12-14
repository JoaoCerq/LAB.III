/*
 * main.c
 *
 *  Created on: Nov 21, 2025
 *      Author: joao0
 */
#include "stdio.h"
#include "ezdsp5502.h"
#include "../lcd-osd9616/lcd.h"
#include "src/state_machine/state_machine.h"

void main(void) {

    /* Initialize BSL */
    EZDSP5502_init( );

    osd9616_init( ); // Initialize  Display

    state_machine(); //roda a maquina de estados finita

}

