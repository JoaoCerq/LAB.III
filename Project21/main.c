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

#define ROOM_SIZE 0.7f    // Tamanho da sala (0.0 a 1.0). Maior = eco mais longo;.
#define DAMPING 0.1f      // Abafamento (0.0 a 1.0). 0.0 = som limpo; 1.0 = som abafado.
#define WET_LEVEL 0.3f    // Volume do efeito (0.0 a 1.0). Quanto de "eco" você quer ouvir na mistura.
#define DRY_LEVEL 0.7f    // Volume do som original (0.0 a 1.0). (Geralmente soma-se Wet+Dry <= 1.0).

#define PITCH_FACTOR 1.5f // Mudança de tom

void effects_initDMA(float room_size,
                    float damping,
                    float wet_level,
                    float dry_level,
                    float pitch_factor);

void main(void) {

    /* Initialize BSL */
    EZDSP5502_init( );

    osd9616_init( ); // Initialize  Display

    effects_initDMA(
                ROOM_SIZE,   // room_size
                DAMPING,   // damping
                WET_LEVEL,   // wet_level
                DRY_LEVEL, // dry_level
                PITCH_FACTOR); //do pitch

    state_machine(); //roda a maquina de estados finita

}

