/*
 * effect.h
 *
 *  Created on: Dec 12, 2025
 *      Author: joao0
 */

#ifndef SRC_EFFECTS_REVERB_H_
#define SRC_EFFECTS_REVERB_H_


#include "tistdtypes.h"

/* ------------------------------------------------------------------------
 * Configuração geral
 * --------------------------------------------------------------------- */

#define NUM_COMBS      8
#define NUM_ALLPASS    4

/* Tunings dos delays (FreeVerb, 44.1 kHz) */
#define COMB_BUF_0   1116
#define COMB_BUF_1   1188
#define COMB_BUF_2   1277
#define COMB_BUF_3   1356
#define COMB_BUF_4   1422
#define COMB_BUF_5   1491
#define COMB_BUF_6   1557
#define COMB_BUF_7   1617

#define AP_BUF_0      556
#define AP_BUF_1      441
#define AP_BUF_2      341
#define AP_BUF_3      225

/* Q15 helpers */
#define Q15_ONE       32767

/* Constantes FreeVerb em Q15 */
#define REV_FIXED_GAIN_Q15   ((Int16)492)   /* 0.015  * 32768 ≈ 492   */
#define DAMP_SCALE_Q15       ((Int16)13107) /* 0.40   * 32768 ≈ 13107 */
#define ROOM_SCALE_Q15       ((Int16)9175)  /* 0.28   * 32768 ≈ 9175  */
#define ROOM_OFFSET_Q15      ((Int16)22938) /* 0.70   * 32768 ≈ 22938 */

/* ------------------------------------------------------------------------
 * Estruturas em fixed-point (Q15)
 * --------------------------------------------------------------------- */

typedef struct {
    Int16 *buffer;  /* buffer de atraso */
    Int16  size;    /* tamanho do buffer */
    Int16  idx;     /* índice circular  */
    Int16  last;    /* estado low-pass (Q15) */
} comb_filter_t;

typedef struct {
    Int16 *buffer;
    Int16  size;
    Int16  idx;
} allpass_filter_t;

typedef struct {
    /* filtros */
    comb_filter_t     combs[NUM_COMBS];
    allpass_filter_t  allpasses[NUM_ALLPASS];

    /* parâmetros em Q15 (0..1) */
    Int16 room_size;   /* 0..1 */
    Int16 damping;     /* 0..1 */
    Int16 wet_level;   /* 0..1 */
    Int16 dry_level;   /* 0..1 */

    /* ganhos internos em Q15 */
    Int16 gain_dry;
    Int16 gain_wet1;
    Int16 gain_damp;
    Int16 gain_room;
} reverb_params_t;

/* ------------------------------------------------------------------------
 * API do algoritmo (sem DMA)
 * --------------------------------------------------------------------- */

/* Inicializa o reverb:
 *  - room_size, damping, wet_level, dry_level em [0.0f, 1.0f]
 *  - converte uma vez para Q15; processamento depois é todo fixed-point
 */
void reverb_init(reverb_params_t *r_params,
                 float room_size,
                 float damping,
                 float wet_level,
                 float dry_level);

/* Processa 1 amostra em Q15 (Int16) */
Int16 reverb_process(reverb_params_t *r_params, Int16 input_sample);

/* Processa um bloco de N amostras (in/out em Q15) */
void reverb_process_block(reverb_params_t *r_params,
                          Int16 *in,
                          Int16 *out,
                          Uint16 nSamples);

/* ------------------------------------------------------------------------
 * API específica para integração com DMA ping-pong
 * (usa rcvPing/rcvPong, xmtPing/xmtPong, rxBufReady, txBufEmpty)
 * --------------------------------------------------------------------- */

/* Inicializa o reverb E o DMA (chama dma_init() lá dentro) */
void Reverb_initDMA(float room_size,
                    float damping,
                    float wet_level,
                    float dry_level);

/* Chama isso dentro do while(1): processa blocos quando o DMA sinaliza */
void processIfReady(void);


#endif /* SRC_EFFECTS_REVERB_H_ */
