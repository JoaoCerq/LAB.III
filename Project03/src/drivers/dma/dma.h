/*
 * dma.h
 *
 * Created on: Nov 23, 2025
 * Author: joao0
 *
 * Description: Header file for DMA Ping-Pong buffering definitions.
 */

#ifndef DMA_H_
#define DMA_H_

#include "csl.h"       // Necessário para tipos como Uint16, Int16
#include "csl_dma.h"   // Necessário para tipos DMA, se for usar handles no main

//---------Global constants---------
#define N  128         // Tamanho do Buffer (Samples por frame)

//---------External Data Declarations---------
/* * Estes arrays estão definidos no dma.c.
 * Usamos 'extern' para que o main.c possa enxergá-los.
 */

// Buffers de Transmissão (Saída para o Codec)
extern volatile Uint16 xmtPing[N];
extern volatile Uint16 xmtPong[N];

// Buffers de Recepção (Entrada do Codec)
extern volatile Uint16 rcvPing[N];
extern volatile Uint16 rcvPong[N];

//---------Flags de Controle---------
/*
 * rxBufReady[0] = 1 -> Ping de Recepção está cheio e pronto para ser lido.
 * rxBufReady[1] = 1 -> Pong de Recepção está cheio e pronto para ser lido.
 */
extern volatile Uint16 rxBufReady[2];

/*
 * txBufEmpty[0] = 1 -> Ping de Transmissão foi enviado e está livre para escrita.
 * txBufEmpty[1] = 1 -> Pong de Transmissão foi enviado e está livre para escrita.
 */
extern volatile Uint16 txBufEmpty[2];

// Variáveis de estado atual (opcional, útil para debug)
extern volatile Uint16 currentRxBuf; // 0 = Ping, 1 = Pong
extern volatile Uint16 currentTxBuf; // 0 = Ping, 1 = Pong

//---------Function Prototypes---------

/**
 * @brief  Inicializa o CSL, vetores de interrupção, zera os buffers
 * e configura o DMA para o esquema Ping-Pong.
 * Também chama a inicialização do AIC3204.
 */
void dma_init(void);

#endif /* DMA_H_ */
