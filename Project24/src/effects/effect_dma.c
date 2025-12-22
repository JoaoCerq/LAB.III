#include <src/effects/reverb.h>
#include <src/effects/pitch_shifter.h>
#include "tistdtypes.h"


#ifndef N
#define N 10*128
#endif

extern volatile Uint16 xmtPing[N];
extern volatile Uint16 xmtPong[N];
extern volatile Uint16 rcvPing[N];
extern volatile Uint16 rcvPong[N];

extern volatile Uint16 rxBufReady[2];
extern volatile Uint16 txBufEmpty[2];

extern void dma_init(void);
static pitch_shifter_t ps_params;
static reverb_params_t gReverb;
#define GAIN 0.2f

static void process_block_dma(volatile Uint16 *rcv,
                                     volatile Uint16 *xmt,
                                     Uint16 n)
{
    Uint16 i;
    for (i = 0; i < n; i++)
    {
        Int16 out  = (Int16)rcv[i];

        out = out * GAIN;
        //out = pitch_shifter_process(&ps_params, out);
        //out = reverb_process(&gReverb, out);
        xmt[i] = (Uint16)out;
//        xmt[2*i + 1] = (Uint16)out;
    }
}


void effects_initDMA(float room_size,
                    float damping,
                    float wet_level,
                    float dry_level,
                    float pitch_factor)
{
    Uint16 i;

    /* Inicializa o algoritmo do reverb */
    reverb_init(&gReverb, room_size, damping, wet_level, dry_level);

    pitch_shifter_init(&ps_params, pitch_factor);

    // Inicializa DMA e McBSP
    dma_init();

    /* Zera buffers de ping–pong */
    for (i = 0; i < N; i++)
    {
        xmtPing[i] = 0;
        xmtPong[i] = 0;
    }
    for (i = 0; i < N; i++)
    {
        rcvPing[i] = 0;
        rcvPong[i] = 0;
    }

    /* Flags de controle */
    rxBufReady[0] = 0;
    rxBufReady[1] = 0;
    txBufEmpty[0] = 1;
    txBufEmpty[1] = 1;
}


static void try_process(volatile Uint16* rcv, volatile Uint16* xmt, volatile Uint16* rxFlag, volatile Uint16* txFlag)
{
    process_block_dma(rcv, xmt, N);
    *rxFlag = 0;
    *txFlag = 0;
}

void processIfReady(void)
{
    if (rxBufReady[0]) {
        if (txBufEmpty[0]) try_process(rcvPing, xmtPing, &rxBufReady[0], &txBufEmpty[0]);
        else if (txBufEmpty[1]) try_process(rcvPing, xmtPong, &rxBufReady[0], &txBufEmpty[1]);
    }

    if (rxBufReady[1]) {
        if (txBufEmpty[0]) try_process(rcvPong, xmtPing, &rxBufReady[1], &txBufEmpty[0]);
        else if (txBufEmpty[1]) try_process(rcvPong, xmtPong, &rxBufReady[1], &txBufEmpty[1]);
    }
}
