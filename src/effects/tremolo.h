/*
 * tremolo.h
 *
 *  Created on: Nov 23, 2025
 *      Author: joao0
 */

#ifndef SRC_EFFECTS_TREMOLO_H_
#define SRC_EFFECTS_TREMOLO_H_

#include <stdio.h>
#include <math.h>
#include "tistdtypes.h"

#define N           128        // mesmo N dos seus buffers
#define TREM_FS     48000.0f   // taxa de amostragem (ajuste p/ seu sistema)
#define TREM_FR     5.0f       // frequência do LFO (Hz)
#define TREM_DEPTH  0.7f       // profundidade (0.0 a 1.0)

// Estado do tremolo
typedef struct {
    float depth;       // profundidade (0–1)
    float offset;      // 1 - depth
    float phase;       // fase atual do LFO (rad)
    float phaseInc;    // incremento de fase por amostra
} TremoloState;

// Variável global de estado (se quiser)
extern TremoloState gTremolo;

void tremolo();

// Inicializa o tremolo
void Tremolo_init(float depth, float lfoFreq, float sampleRate);

// Processa um bloco (N amostras)
void Tremolo_processBlock(volatile Uint16 *inBuf, volatile Uint16 *outBuf, Uint16 nSamples);

// Função de alto nível que usa as flags rxBufReady/txBufEmpty e os buffers
void Tremolo_processIfReady(void);

#endif /* SRC_EFFECTS_TREMOLO_H_ */
