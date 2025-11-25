/*
 * dma.c
 *
 *  Created on: Nov 23, 2025
 *      Author: joao0
 */

#include <stdio.h>

#include <csl_mcbsp.h>
#include <csl_dma.h>
#include <csl_irq.h>
#include "csl.h"
#include "ezdsp5502.h"
#include "ezdsp5502_mcbsp.h"


//---------Global constants---------
#define N       128

//---------Global data definition---------

/* Define transmit and receive buffers */
#pragma DATA_SECTION(xmtPing,"dmaMem")
volatile Uint16 xmtPing[N];
#pragma DATA_SECTION(xmtPong,"dmaMem")
volatile Uint16 xmtPong[N];
#pragma DATA_SECTION(rcvPing,"dmaMem")
volatile Uint16 rcvPing[N];
#pragma DATA_SECTION(rcvPong,"dmaMem")
volatile Uint16 rcvPong[N];

volatile Uint16 currentRxBuf = 0; // 0 pra PING, 1 = PONG
volatile Uint16 currentTxBuf = 0;
volatile Uint16 rxBufReady[2] = {0, 0};// no inicio, nao ha nada. Entao, os dois nao estao prontos. Usaremos isso como flag!
volatile Uint16 txBufEmpty[2] = {1, 1}; // No inicio, os dois estao vazios

extern Int16 aic3204_INIT();


DMA_Config  dmaRcvConfig = {
  DMA_DMACSDP_RMK(
    DMA_DMACSDP_DSTBEN_NOBURST, // Isso permitiria varios acessos de uma so vez, mas o McBSP nao suporta. Essa config desliga isso.
    DMA_DMACSDP_DSTPACK_OFF,   // serve pra empacotar dados antes de enviar.
    DMA_DMACSDP_DST_DARAMPORT1, // Isso define o destino do dma. Esta enviando para a RAM (onde colocamos os buffers)
    DMA_DMACSDP_SRCBEN_NOBURST, // Isso permitiria varios acessos de uma so vez, mas o McBSP nao suporta. Essa config desliga isso.
    DMA_DMACSDP_SRCPACK_OFF, // empacotamento tbm
    DMA_DMACSDP_SRC_PERIPH, // Define de onde o DMA esta lendo. Periferico eh o McBSP
    DMA_DMACSDP_DATATYPE_16BIT //o tamanho do data. Vamos deixar em 16 pra otimizar? (ignora os outros 16)(ALTERADO)
  ),                                       /* DMACSDP  */
  DMA_DMACCR_RMK(
    DMA_DMACCR_DSTAMODE_POSTINC, // Modo de enderecamento do destino. PostInc = inc a cada dado, eh isso que queremos (RAM).
    DMA_DMACCR_SRCAMODE_CONST, // Modo de enderecaamento da fonte. Const pois o endereco eh o do McBSP.
    DMA_DMACCR_ENDPROG_ON,
    DMA_DMACCR_WP_DEFAULT,// write posting (default = desativado)
    DMA_DMACCR_REPEAT_OFF, //So repita o DMA se ENDPROG = 1
    DMA_DMACCR_AUTOINIT_OFF, // Ele nao volta a escrever no bloco quando finaliza ele
    DMA_DMACCR_EN_STOP, // Determina se esta funcionando ou nao. DMA_start() configura ele pra 1
    DMA_DMACCR_PRIO_LOW, // determina a prioridade a esse DMA em especifico
    DMA_DMACCR_FS_DISABLE,// 0 = O evento de sincronizacao dispara 1 elemento. // 1 = O evento de sincronizacao dispara o frame inteiro.
    DMA_DMACCR_SYNC_REVT1 // determina qual evento dispara o DMA. Revt1 = McBSP 1 receive event
  ),                                       /* DMACCR   */
  DMA_DMACICR_RMK(
    DMA_DMACICR_AERRIE_OFF, // Chama interrupcao quando ocorre erro de memoria. Tirei de inicio mas pode ser importante (ALTERADO)
    DMA_DMACICR_BLOCKIE_ON, // Determina se ha uma interrupcao quando o bloco inteiro enche(ALTERADO)
    DMA_DMACICR_LASTIE_OFF, // Interrupcao do ultimo frame, nao vamos usar
    DMA_DMACICR_FRAMEIE_OFF,// Determina se ha uma interrupcao quando o FRAME inteiro enche (ALTERADO)
    DMA_DMACICR_FIRSTHALFIE_OFF,
    DMA_DMACICR_DROPIE_OFF,
    DMA_DMACICR_TIMEOUTIE_OFF
  ),                                       /* DMACICR  */
    (DMA_AdrPtr)(MCBSP_ADDR(DRR11)),        /* DMACSSAL */ //Source start address (lower part)
    0,                                     /* DMACSSAU */  //Source start address (upper part)
    (DMA_AdrPtr)&rcvPing,                      /* DMACDSAL */   //Destination start address (lower part)
    0,                                     /* DMACDSAU */    //Destination start address (upper part)
    N,                                     /* DMACEN   */ // Load DMACEN with the number of elements
    1,                                     /* DMACFN   */ //Load DMACFN with the number of frames you want in each block.
    0,                                     /* DMACFI   */
    0                                      /* DMACEI   */
};

DMA_Config  dmaXmtConfig = {
  DMA_DMACSDP_RMK(
    DMA_DMACSDP_DSTBEN_NOBURST, // Isso permitiria varios acessos de uma so vez, mas o McDSP nao suporta. Essa config desliga isso.
    DMA_DMACSDP_DSTPACK_OFF, // serve pra empacotar dados antes de enviar.
    DMA_DMACSDP_DST_PERIPH, // Isso define o destino do dma. Esta enviando para os perifericos (McBSP)
    DMA_DMACSDP_SRCBEN_NOBURST, //Isso permitiria varios acessos de uma so vez, mas o McDSP nao suporta. Essa config desliga isso.
    DMA_DMACSDP_SRCPACK_OFF,  //empacotamento tbm
    DMA_DMACSDP_SRC_DARAMPORT0, // Define de onde o DMA esta lendo. Ta lendo da RAM pela porta 0
    DMA_DMACSDP_DATATYPE_16BIT //o tamanho do data. Vamos deixar em 16 pra otimizar?(ALTERADO)
  ),                                       /* DMACSDP  */
  DMA_DMACCR_RMK(
    DMA_DMACCR_DSTAMODE_CONST, // Modo de enderecamento do destino. Const pois o endereco eh o do McBSP.
    DMA_DMACCR_SRCAMODE_POSTINC, // Modo de enderecamento da fonte. PostInc = inc a cada dado, eh isso que queremos (RAM).
    DMA_DMACCR_ENDPROG_ON, // determina que terminou de programar
    DMA_DMACCR_WP_DEFAULT, // write posting (default = desativado)
    DMA_DMACCR_REPEAT_OFF, // so repita o DMA se ENDPROG = 1
    DMA_DMACCR_AUTOINIT_OFF, // Ele nao volta a escrever no bloco quando finaliza ele
    DMA_DMACCR_EN_STOP, // Determina se esta funcionando ou nao. DMA_start() configura ele pra 1
    DMA_DMACCR_PRIO_LOW,// determina a prioridade a esse DMA em especifco
    DMA_DMACCR_FS_DISABLE, // 0 = O evento de sincronizacao dispara 1 elemento. // 1 = O evento de sincronizacao dispara o frame inteiro.
    DMA_DMACCR_SYNC_XEVT1 // determina qual evento dispara o DMA. Ativa quando o McBSP estiver pronto pra receber dados!
  ),                                       /* DMACCR   */
  DMA_DMACICR_RMK(
    DMA_DMACICR_AERRIE_OFF, // Chama interrupcao quando ocorre erro de memoria. Tirei de inicio mas pode ser importante (ALTERADO)
    DMA_DMACICR_BLOCKIE_ON, // Determina se ha uma interrupcao quando o bloco inteiro enche (ALTERADO)
    DMA_DMACICR_LASTIE_OFF, // interrupcao do ultimo frame, nao vamos usar
    DMA_DMACICR_FRAMEIE_OFF,// Determina se ha uma interrupcao quando o FRAME inteiro enche (ALTERADO)
    DMA_DMACICR_FIRSTHALFIE_OFF,
    DMA_DMACICR_DROPIE_OFF,
    DMA_DMACICR_TIMEOUTIE_OFF
  ),                                       /* DMACICR  */
    (DMA_AdrPtr)&xmtPing[0],                      /* DMACSSAL */ //Source start address (lower part)
    0,                                     /* DMACSSAU */ //Source start address (upper part)
    (DMA_AdrPtr)(MCBSP_ADDR(DXR11)),       /* DMACDSAL */  //Destination start address (lower part)
    0,                                     /* DMACDSAU */   //Destination start address (upper part)
    N,                                     /* DMACEN   */  // Load DMACEN with the number of elements you want in each frame.
    1,                                     /* DMACFN   */  //Load DMACFN with the number of frames you want in each block.
    0,                                     /* DMACFI   */
    0                                      /* DMACEI   */
};

/* Define a DMA_Handle object to be used with DMA_open function */
DMA_Handle hDmaRcv, hDmaXmt;

/* Define a MCBSP_Handle object to be used with MCBSP_open function */
MCBSP_Handle hMcbsp;

volatile Uint16 transferComplete = FALSE;
Uint16 err = 0;
Uint16 old_intm;
Uint16 xmtEventId, rcvEventId;

//---------Function prototypes---------

/* Reference the start of the interrupt vector table */
/* This symbol is defined in file vectors.s55        */
extern void VECSTART(void);

/* Protoype for interrupt functions */
interrupt void dmaXmtIsr(void);
interrupt void dmaRcvIsr(void);
void taskFxn(void);

//---------main routine---------
void dma_init(void)
{
    Uint16 i;

    /* Set IVPH/IVPD to start of interrupt vector table */
    IRQ_setVecs((Uint32)(&VECSTART));

    for (i = 0; i <= N - 1; i++) {
        xmtPing[i] =  0;
        xmtPong[i] =  0;
        rcvPing[i] = 0;
        rcvPong[i] = 0;
    }

    /* Call function to effect transfer */
    taskFxn();
}

Uint16 srcAddrHi, srcAddrLo;
Uint16 dstAddrHi, dstAddrLo;

void taskFxn(void)
{
//    Uint16 i;

    /* By default, the TMS320C55xx compiler assigns all data symbols word */
    /* addresses. The DMA however, expects all addresses to be byte       */
    /* addresses. Therefore, we must shift the address by 2 in order to   */
    /* change the word address to a byte address for the DMA transfer.    */


    srcAddrHi = (Uint16)(((Uint32)(MCBSP_ADDR(DRR11))) >> 15) & 0xFFFFu;
    srcAddrLo = (Uint16)(((Uint32)(MCBSP_ADDR(DRR11))) << 1) & 0xFFFFu;
    dstAddrHi = (Uint16)(((Uint32)(&rcvPing[0])) >> 15) & 0xFFFFu;
    dstAddrLo = (Uint16)(((Uint32)(&rcvPing[0])) << 1) & 0xFFFFu;

    dmaRcvConfig.dmacssal = (DMA_AdrPtr)srcAddrLo;
    dmaRcvConfig.dmacssau = srcAddrHi;
    dmaRcvConfig.dmacdsal = (DMA_AdrPtr)dstAddrLo;
    dmaRcvConfig.dmacdsau = dstAddrHi;

    srcAddrHi = (Uint16)(((Uint32)(&xmtPing[0])) >> 15) & 0xFFFFu;
    srcAddrLo = (Uint16)(((Uint32)(&xmtPing[0])) << 1) & 0xFFFFu;
    dstAddrHi = (Uint16)(((Uint32)(MCBSP_ADDR(DXR11))) >> 15) & 0xFFFFu;
    dstAddrLo = (Uint16)(((Uint32)(MCBSP_ADDR(DXR11))) << 1) & 0xFFFFu;



    dmaXmtConfig.dmacssal = (DMA_AdrPtr)srcAddrLo;
    dmaXmtConfig.dmacssau = srcAddrHi;
    dmaXmtConfig.dmacdsal = (DMA_AdrPtr)dstAddrLo;
    dmaXmtConfig.dmacdsau = dstAddrHi;


    /* Open MCBSP Port 1 and set registers to their power on defaults */
//    hMcbsp = MCBSP_open(MCBSP_PORT1, MCBSP_OPEN_RESET);


    /* Open DMA channels 4 & 5 and set regs to power on defaults */
    hDmaRcv = DMA_open(DMA_CHA4,DMA_OPEN_RESET);
    hDmaXmt = DMA_open(DMA_CHA5,DMA_OPEN_RESET);

    /* Get interrupt event associated with DMA receive and transmit */
    xmtEventId = DMA_getEventId(hDmaXmt);
    rcvEventId = DMA_getEventId(hDmaRcv);

    /* Temporarily disable interrupts and clear any pending */
    /* interrupts for MCBSP transmit */
    old_intm = IRQ_globalDisable();

    /* Clear any pending interrupts for DMA channels */
    IRQ_clear(xmtEventId);
    IRQ_clear(rcvEventId);

    /* Enable DMA interrupt in IER register */
    IRQ_enable(xmtEventId);
    IRQ_enable(rcvEventId);

    /* Place DMA interrupt service addresses at associate vector */
    IRQ_plug(xmtEventId,&dmaXmtIsr);
    IRQ_plug(rcvEventId,&dmaRcvIsr);

    /* Write values from configuration structure to MCBSP control regs */
//    MCBSP_config(hMcbsp, &ConfigLoopBack16);

    /* Write values from configuration structure to DMA control regs */
    DMA_config(hDmaRcv,&dmaRcvConfig);
    DMA_config(hDmaXmt,&dmaXmtConfig);

   /* Enable all maskable interrupts */
    IRQ_globalEnable();

    /* Start Sample Rate Generator and Enable Frame Sync */
////    MCBSP_start(hMcbsp,
////                MCBSP_SRGR_START | MCBSP_SRGR_FRAMESYNC,
////                0x300u);

    /* Enable DMA */

    DMA_start(hDmaRcv); // coloca EN = 1
    DMA_start(hDmaXmt);

    /* Take MCBSP transmit and receive out of reset */
    MCBSP_start(hMcbsp,
                MCBSP_XMIT_START | MCBSP_RCV_START,
                0u);

//     IRQ_globalRestore(old_intm);
//
//     MCBSP_close(hMcbsp);
//     DMA_close(hDmaRcv);
//     DMA_close(hDmaXmt);


    aic3204_INIT();
}
volatile Uint32 dmaRcvCount = 0;
volatile Uint32 dmaXmtCount = 0;


interrupt void dmaXmtIsr(void) {
    if (currentTxBuf == 0) // 0 eh ping. Na chamada, o Ping esta sendo enviado e devemos trocar pro buffer do Pong.
    {
        srcAddrHi = (Uint16)(((Uint32)(&xmtPong[0])) >> 15) & 0xFFFFu;
        srcAddrLo = (Uint16)(((Uint32)(&xmtPong[0])) << 1) & 0xFFFFu;

        dmaXmtConfig.dmacssal = (DMA_AdrPtr)srcAddrLo;
        dmaXmtConfig.dmacssau = srcAddrHi;

        currentTxBuf = 1;


        DMA_config(hDmaXmt,&dmaXmtConfig);
        DMA_start(hDmaXmt);
        txBufEmpty[0] = 1; // Flag que indica que o buffer no Ping agora esta vazio

    }
    else if (currentTxBuf == 1) // Ou seja, agora, o Pong esta sendo enviado e devemos mudar para o Ping
    {
        srcAddrHi = (Uint16)(((Uint32)(&xmtPing[0])) >> 15) & 0xFFFFu;
        srcAddrLo = (Uint16)(((Uint32)(&xmtPing[0])) << 1) & 0xFFFFu;

        dmaXmtConfig.dmacssal = (DMA_AdrPtr)srcAddrLo;
        dmaXmtConfig.dmacssau = srcAddrHi;

        currentTxBuf = 0;


        DMA_config(hDmaXmt,&dmaXmtConfig);
        DMA_start(hDmaXmt);
        txBufEmpty[1] = 1; // Flag que indica que o buffer no Pong agora esta vazio

    }

}

interrupt void dmaRcvIsr(void) {
    if (currentRxBuf == 0) // 0 eh ping. Na chamada, o ping ficou cheio e devemos trocar pro buffer do pong.
    {
        dstAddrHi = (Uint16)(((Uint32)(&rcvPong[0])) >> 15) & 0xFFFFu;
        dstAddrLo = (Uint16)(((Uint32)(&rcvPong[0])) << 1) & 0xFFFFu;

        dmaRcvConfig.dmacdsal = (DMA_AdrPtr)dstAddrLo;
        dmaRcvConfig.dmacdsau = dstAddrHi;

        currentRxBuf = 1;

        rxBufReady[0] = 1; // Flag que indica que o buffer no Ping esta pronto para ser processado
    }
    else if (currentRxBuf == 1) // Ou seja, agora, o Pong ficou cheio e devemos mudar para o Ping
    {
        dstAddrHi = (Uint16)(((Uint32)(&rcvPing[0])) >> 15) & 0xFFFFu;
        dstAddrLo = (Uint16)(((Uint32)(&rcvPing[0])) << 1) & 0xFFFFu;

        dmaRcvConfig.dmacdsal = (DMA_AdrPtr)dstAddrLo;
        dmaRcvConfig.dmacdsau = dstAddrHi;


        currentRxBuf = 0;

        rxBufReady[1] = 1; // Flag que indica que o buffer no Pong esta pronto para ser processado
    }

    DMA_config(hDmaRcv,&dmaRcvConfig);
    DMA_start(hDmaRcv);
}
