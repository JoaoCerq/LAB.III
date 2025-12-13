#include "tistdtypes.h"
#include "effect.h"

/* ------------------------------------------------------------------------
 *  - N é o tamanho do bloco do DMA (mesmo de dma.c)
 *  - rcvPing/rcvPong: buffers de entrada (16 bits, mono)
 *  - xmtPing/xmtPong: buffers de saída (16 bits, mono)
 *  - rxBufReady[i]: 1 => buffer i pronto para processar
 *  - txBufEmpty[i]: 1 => buffer i livre para escrever
 *  - dma_init(): já configura McBSP + DMA ping–pong
 * --------------------------------------------------------------------- */

#ifndef N
#define N 128
#endif

extern volatile Uint16 xmtPing[2*N];
extern volatile Uint16 xmtPong[2*N];
extern volatile Uint16 rcvPing[2*N];
extern volatile Uint16 rcvPong[2*N];

extern volatile Uint16 rxBufReady[2];
extern volatile Uint16 txBufEmpty[2];

/* Função definida no teu dma.c */
extern void dma_init(void);

/* Reverb global */
static reverb_params_t gReverb;

/* Helper: processa bloco usando tipos compatíveis com DMA (Uint16/volatile) */
static void reverb_process_block_dma(volatile Uint16 *rcv,
                                     volatile Uint16 *xmt,
                                     Uint16 nSamples)
{
    Uint16 i;
    for (i = 0; i < nSamples; i++)
    {
        /* entrada e saída são Q15, mas armazenadas como 16 bits crus */
        Int16 in  = (Int16)rcv[2*i];
        Int16 out = reverb_process(&gReverb, in);
        xmt[2*i] = (Uint16)out;
        xmt[2*i + 1] = (Uint16)out;
    }
}

/* ------------------------------------------------------------------------
 * Inicialização conjunta: Reverb + DMA + buffers
 * Chame isso uma vez no começo (em vez de Tremolo_init)
 * --------------------------------------------------------------------- */

void Reverb_initDMA(float room_size,
                    float damping,
                    float wet_level,
                    float dry_level)
{
    Uint16 i;

    /* Inicializa o algoritmo do reverb (parâmetros em float 0..1) */
    reverb_init(&gReverb, room_size, damping, wet_level, dry_level);

    /* Inicializa DMA e McBSP (tua função existente) */
    dma_init();

    /* Zera buffers de ping–pong */
    for (i = 0; i < 2*N; i++)
    {
        xmtPing[i] = 0;
        xmtPong[i] = 0;
    }
    for (i = 0; i < 2*N; i++)
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

/* ------------------------------------------------------------------------
 * Chamada no laço principal
 *  - verifica flags do DMA
 *  - quando um bloco está pronto, aplica o reverb nele
 * --------------------------------------------------------------------- */

static void try_process(volatile Uint16* rcv, volatile Uint16* xmt, volatile Uint16* rxFlag, volatile Uint16* txFlag)
{
    reverb_process_block_dma(rcv, xmt, N);
    *rxFlag = 0;
    *txFlag = 0;
}

void Reverb_processIfReady(void)
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
