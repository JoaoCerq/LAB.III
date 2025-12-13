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
volatile Uint16 xmtPing[2*N];
#pragma DATA_SECTION(xmtPong,"dmaMem")
volatile Uint16 xmtPong[2*N];
#pragma DATA_SECTION(rcvPing,"dmaMem")
volatile Uint16 rcvPing[2*N];
#pragma DATA_SECTION(rcvPong,"dmaMem")
volatile Uint16 rcvPong[2*N];

#pragma DATA_ALIGN(xmtPing, 4);
#pragma DATA_ALIGN(xmtPong, 4);
#pragma DATA_ALIGN(rcvPing, 4);
#pragma DATA_ALIGN(rcvPong, 4);

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
    (DMA_AdrPtr)&rcvPing[0],                      /* DMACDSAL */   //Destination start address (lower part)
    0,                                     /* DMACDSAU */    //Destination start address (upper part)
    2*N,                                     /* DMACEN   */ // Load DMACEN with the number of elements
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
    2*N,                                     /* DMACEN   */  // Load DMACEN with the number of elements you want in each frame.
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
    //MCBSP_start(hMcbsp,
    //            MCBSP_XMIT_START | MCBSP_RCV_START,
    //            0u);

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
    // Calculamos os endereços
    Uint32 addrPing = ((Uint32)(&xmtPing[0]) << 1);
    Uint32 addrPong = ((Uint32)(&xmtPong[0]) << 1);

    IRQ_clear(xmtEventId);

    Uint16 addrLo, addrHi;

    // Se acabamos de enviar o PING (0), vamos preparar o PONG (1)
    if (currentTxBuf == 0)
    {
        addrLo = (Uint16)(addrPong & 0xFFFF);
        addrHi = (Uint16)(addrPong >> 16);

        DMA_RSETH(hDmaXmt, DMACSSAL, addrLo);
        DMA_RSETH(hDmaXmt, DMACSSAU, addrHi);

        currentTxBuf = 1;
        txBufEmpty[0] = 1;
    }
    else // Acabamos de enviar o PONG (1), vamos preparar o PING (0)
    {
        addrLo = (Uint16)(addrPing & 0xFFFF);
        addrHi = (Uint16)(addrPing >> 16);

        DMA_RSETH(hDmaXmt, DMACSSAL, addrLo);
        DMA_RSETH(hDmaXmt, DMACSSAU, addrHi);

        currentTxBuf = 0;
        txBufEmpty[1] = 1; // Libera o Pong
    }

    // Apenas reinicia. O Config já está feito.
    DMA_start(hDmaXmt);
}

interrupt void dmaRcvIsr(void) {
    Uint32 addrPing = ((Uint32)(&rcvPing[0]) << 1);
    Uint32 addrPong = ((Uint32)(&rcvPong[0]) << 1);
    Uint16 addrLo, addrHi;

    IRQ_clear(rcvEventId);

    if (currentRxBuf == 0) // Encheu o Ping, vamos jogar o DMA pro Pong
    {
        addrLo = (Uint16)(addrPong & 0xFFFF);
        addrHi = (Uint16)(addrPong >> 16);

        // Aqui mudamos o DESTINO (CDSA), pois estamos recebendo do McBSP
        DMA_RSETH(hDmaRcv, DMACDSAL, addrLo);
        DMA_RSETH(hDmaRcv, DMACDSAU, addrHi);

        currentRxBuf = 1;
        rxBufReady[0] = 1; // Avisa o main que o Ping tá cheio de dados novos
    }
    else // Encheu o Pong, volta pro Ping
    {
        addrLo = (Uint16)(addrPing & 0xFFFF);
        addrHi = (Uint16)(addrPing >> 16);

        DMA_RSETH(hDmaRcv, DMACDSAL, addrLo);
        DMA_RSETH(hDmaRcv, DMACDSAU, addrHi);

        currentRxBuf = 0;
        rxBufReady[1] = 1; // Avisa o main que o Pong tá cheio
    }

    DMA_start(hDmaRcv);
}
