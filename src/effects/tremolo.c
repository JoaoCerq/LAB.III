/*
 * tremolo.c
 *
 *  Created on: Nov 23, 2025
 *      Author: joao0
 */

#include "tremolo.h"

extern volatile Uint16 xmtPing[N];
extern volatile Uint16 xmtPong[N];
extern volatile Uint16 rcvPing[N];
extern volatile Uint16 rcvPong[N];

extern volatile Uint16 currentRxBuf; // 0 pra PING, 1 = PONG
extern volatile Uint16 currentTxBuf;
extern volatile Uint16 rxBufReady[2];// 1 indica que esta pronto pra processar
extern volatile Uint16 txBufEmpty[2]; // 0 indica que esta pronto pra enviar

void tremolo()
{
    Tremolo_processIfReady();
}

#ifndef PI
#define PI 3.14159265358979323846f
#endif
extern dma_init();


// estrutura do tremolo
TremoloState gTremolo;

// --------- Inicializacao

void Tremolo_init(float depth, float lfoFreq, float sampleRate)
{
    gTremolo.depth    = depth;
    gTremolo.offset   = 1.0f - depth;
    gTremolo.phase    = 0.0f;
    gTremolo.phaseInc = 2.0f * PI * lfoFreq / sampleRate;
    dma_init();
    int i;
    for (i = 0; i < N; i++)
    {
        xmtPing[i] = 0;
        xmtPong[i] = 0;
        rcvPing[i] = 0;
        rcvPong[i] = 0;
    }
}

// Processamento por amostra ---------

static inline Uint16 Tremolo_processSample(Uint16 x, TremoloState *st)
{
    // LFO senoidal
    float lfo = sinf(st->phase);           // varia de -1 a +1

    // Avança fase
    st->phase += st->phaseInc;
    if (st->phase >= 2.0f * PI) {
        st->phase -= 2.0f * PI;
    }

    // Ganho: offset + depth * lfo
    float gain = st->offset + st->depth * lfo;

    // Aplica no sinal (converte para float, aplica ganho, volta para Int16)
    float y = (float)x * gain;

    // Saturação simples em 16 bits
    if (y > 32767.0f)      y = 32767.0f;
    else if (y < -32768.0f) y = -32768.0f;

    return (Uint16)y;
}

// --------- Processamento de bloco ---------

void Tremolo_processBlock(volatile Uint16 *rcv, volatile Uint16 *xmt, Uint16 nSamples)
{
    Uint16 i;
    for (i = 0; i < nSamples; i++) {
        xmt[i] = Tremolo_processSample(rcv[i], &gTremolo); // outBuf eh o proprio xmt
    }
}

// Integraco com DMA ping–pong

void Tremolo_processIfReady(void)
{
    // Buffer PING (indice 0)
    if (rxBufReady[0] && txBufEmpty[0]) {
        Tremolo_processBlock(rcvPing, xmtPing, N);
        rxBufReady[0] = 0;
        txBufEmpty[0] = 0;
    }

    // Buffer PONG (indice 1)
    if (rxBufReady[1] && txBufEmpty[1]) {
        Tremolo_processBlock(rcvPong, xmtPong, N);
        rxBufReady[1] = 0;
        txBufEmpty[1] = 0;
    }
}
