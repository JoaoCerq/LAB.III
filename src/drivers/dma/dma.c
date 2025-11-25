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
volatile Uint16 rxBufReady[2] = {0, 0};// no in�cio, n�o h� nada. Ent�o, os dois n�o est�o prontos. Usaremos isso como flag!
volatile Uint16 txBufEmpty[2] = {1, 1}; // No inicio, os dois est�o vazios

extern Int16 aic3204_INIT();

//  MCBSP_Config ConfigLoopBack16= {
//  MCBSP_SPCR1_RMK(
//    MCBSP_SPCR1_DLB_ON,                    /* DLB    = 1 */
//    MCBSP_SPCR1_RJUST_RZF,                 /* RJUST  = 0 */
//    MCBSP_SPCR1_CLKSTP_DISABLE,            /* CLKSTP = 0 */
//    MCBSP_SPCR1_DXENA_NA,                  /* DXENA  = 0 */
//    MCBSP_SPCR1_ABIS_DISABLE,              /* ABIS   = 0 */
//    MCBSP_SPCR1_RINTM_RRDY,                /* RINTM  = 0 */
//    0,                                     /* RSYNCER = 0 */
//    MCBSP_SPCR1_RRST_DISABLE               /* RRST   = 0 */
//   ),
//    MCBSP_SPCR2_RMK(
//    MCBSP_SPCR2_FREE_NO,                   /* FREE   = 0 */
//    MCBSP_SPCR2_SOFT_NO,                   /* SOFT   = 0 */
//    MCBSP_SPCR2_FRST_RESET,                /* FRST   = 0 */
//    MCBSP_SPCR2_GRST_RESET,                /* GRST   = 0 */
//    MCBSP_SPCR2_XINTM_XRDY,                /* XINTM  = 0 */
//    0,                                     /* XSYNCER = N/A */
//    MCBSP_SPCR2_XRST_DISABLE               /* XRST   = 0 */
//   ),
//  MCBSP_RCR1_RMK(
//  MCBSP_RCR1_RFRLEN1_OF(0),                /* RFRLEN1 = 0 */
//  MCBSP_RCR1_RWDLEN1_16BIT                 /* RWDLEN1 = 5 */
//  ),
// MCBSP_RCR2_RMK(
//    MCBSP_RCR2_RPHASE_SINGLE,              /* RPHASE  = 0 */
//    MCBSP_RCR2_RFRLEN2_OF(0),              /* RFRLEN2 = 0 */
//    MCBSP_RCR2_RWDLEN2_8BIT,               /* RWDLEN2 = 0 */
//    MCBSP_RCR2_RCOMPAND_MSB,               /* RCOMPAND = 0 */
//    MCBSP_RCR2_RFIG_YES,                   /* RFIG    = 0 */
//    MCBSP_RCR2_RDATDLY_0BIT                /* RDATDLY = 0 */
//    ),
//   MCBSP_XCR1_RMK(
//    MCBSP_XCR1_XFRLEN1_OF(0),              /* XFRLEN1 = 0 */
//    MCBSP_XCR1_XWDLEN1_16BIT               /* XWDLEN1 = 5 */
//
// ),
// MCBSP_XCR2_RMK(
//    MCBSP_XCR2_XPHASE_SINGLE,              /* XPHASE  = 0 */
//    MCBSP_XCR2_XFRLEN2_OF(0),              /* XFRLEN2 = 0 */
//    MCBSP_XCR2_XWDLEN2_8BIT,               /* XWDLEN2 = 0 */
//    MCBSP_XCR2_XCOMPAND_MSB,               /* XCOMPAND = 0 */
//    MCBSP_XCR2_XFIG_YES,                   /* XFIG    = 0 */
//    MCBSP_XCR2_XDATDLY_0BIT                /* XDATDLY = 0 */
//  ),
// MCBSP_SRGR1_RMK(
//   MCBSP_SRGR1_FWID_OF(1),                /* FWID    = 1 */
//   MCBSP_SRGR1_CLKGDV_OF(1)               /* CLKGDV  = 1 */
// ),
// MCBSP_SRGR2_RMK(
//    MCBSP_SRGR2_GSYNC_FREE,                /* FREE    = 0 */
//    MCBSP_SRGR2_CLKSP_RISING,              /* CLKSP   = 0 */
//    MCBSP_SRGR2_CLKSM_INTERNAL,            /* CLKSM   = 1 */
//    MCBSP_SRGR2_FSGM_DXR2XSR,              /* FSGM    = 0 */
//    MCBSP_SRGR2_FPER_OF(15)                /* FPER    = 0 */
// ),
// MCBSP_MCR1_DEFAULT,
// MCBSP_MCR2_DEFAULT,
// MCBSP_PCR_RMK(
//   MCBSP_PCR_XIOEN_SP,                     /* XIOEN    = 0   */
//   MCBSP_PCR_RIOEN_SP,                     /* RIOEN    = 0   */
//   MCBSP_PCR_FSXM_INTERNAL,                /* FSXM     = 1   */
//   MCBSP_PCR_FSRM_EXTERNAL,                /* FSRM     = 0   */
//   MCBSP_PCR_CLKXM_OUTPUT,                 /* CLKXM    = 1   */
//   MCBSP_PCR_CLKRM_INPUT,                  /* CLKRM    = 0   */
//   MCBSP_PCR_SCLKME_NO,                    /* SCLKME   = 0   */
//   0,                                      /* DXSTAT = N/A   */
//   MCBSP_PCR_FSXP_ACTIVEHIGH,              /* FSXP     = 0   */
//   MCBSP_PCR_FSRP_ACTIVEHIGH,              /* FSRP     = 0   */
//   MCBSP_PCR_CLKXP_RISING,                 /* CLKXP    = 0   */
//   MCBSP_PCR_CLKRP_FALLING                 /* CLKRP    = 0   */
// ),
// MCBSP_RCERA_DEFAULT,
// MCBSP_RCERB_DEFAULT,
// MCBSP_RCERC_DEFAULT,
// MCBSP_RCERD_DEFAULT,
// MCBSP_RCERE_DEFAULT,
// MCBSP_RCERF_DEFAULT,
// MCBSP_RCERG_DEFAULT,
// MCBSP_RCERH_DEFAULT,
// MCBSP_XCERA_DEFAULT,
// MCBSP_XCERB_DEFAULT,
// MCBSP_XCERC_DEFAULT,
// MCBSP_XCERD_DEFAULT,
// MCBSP_XCERE_DEFAULT,
// MCBSP_XCERF_DEFAULT,
// MCBSP_XCERG_DEFAULT,
// MCBSP_XCERH_DEFAULT
//};

DMA_Config  dmaRcvConfig = {
  DMA_DMACSDP_RMK(
    DMA_DMACSDP_DSTBEN_NOBURST, // Isso permitiria varios acessos de uma so vez, mas o McBSP nao suporta. Essa config desliga isso.
    DMA_DMACSDP_DSTPACK_OFF,   // serve pra empacotar dados antes de enviar.
    DMA_DMACSDP_DST_DARAMPORT1, // Isso define o destino do dma. Esta enviando para a RAM (onde colocamos os buffers)
    DMA_DMACSDP_SRCBEN_NOBURST, // Isso permitiria varios acessos de uma s� vez, mas o McBSP nao suporta. Essa config desliga isso.
    DMA_DMACSDP_SRCPACK_OFF, // empacotamento tbm
    DMA_DMACSDP_SRC_PERIPH, // Define de onde o DMA esta lendo. Periferico eh o McBSP
    DMA_DMACSDP_DATATYPE_16BIT //o tamanho do data. Vamos deixar em 16 pra otimizar? (ignora os outros 16)(ALTERADO)
  ),                                       /* DMACSDP  */
  DMA_DMACCR_RMK(
    DMA_DMACCR_DSTAMODE_POSTINC, // Modo de enderecamento do destino. PostInc = inc a cada dado, � isso que queremos (RAM).
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

    /* Initialize CSL library - This is REQUIRED !!! */
//    CSL_init();

    /* Set IVPH/IVPD to start of interrupt vector table */
    IRQ_setVecs((Uint32)(&VECSTART));

//    for (i = 0; i <= N - 1; i++) {  // para fins de testes
//        xmtPing[i] =  i + 1;
//        rcvPing[i] = 0;
//    }

    for (i = 0; i <= N - 1; i++) {
        xmtPing[i] =  0xFFFF;
        xmtPong[i] =  0xFFFF;
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
    /* Wait for DMA transfer to be complete */
//     while (!transferComplete){
//         ;
//     }
//
//     /*------------------------------------------*\
//      * Compare values
//     \*------------------------------------------*/
//     for(i = 0; i <= N - 1; i++){
//         if (rcv[i] != xmt[i]){
//             ++err;
//             break;
//        }
//     }
//
//     printf ("%s\n",err?"TEST FAILED" : "TEST PASSED");
//
//     /* Restore status of global interrupt enable flag */
//     IRQ_globalRestore(old_intm);
//
//     /* We're done with MCBSP and DMA , so close them */
//     MCBSP_close(hMcbsp);
//     DMA_close(hDmaRcv);
//     DMA_close(hDmaXmt);


    aic3204_INIT();


//    Int16 sec, msec;
//    Int16 sample, dataLeft, dataRight;
//    Int32 data;
//    data = 0;
//    dataLeft = 0;
//    dataRight = 0;
//    for ( sec = 0 ; sec < 30 ; sec++ )
//    {
//        for ( msec = 0 ; msec < 1000 ; msec++ )
//        {
//            for ( sample = 0 ; sample < 48 ; sample++ )
//            {
//                EZDSP5502_MCBSP_read(&data);      // RX right channel
//                EZDSP5502_MCBSP_write(data);      // TX left channel first (FS Low)
//
////                EZDSP5502_MCBSP_read(&dataRight);      // RX left channel ->> N�o precisa mais desses dois. Agora vai pros 2 lados
////                EZDSP5502_MCBSP_write( dataRight);      // TX right channel next (FS High)
//            }
//        }
//    }
}
volatile Uint32 dmaRcvCount = 0;
volatile Uint32 dmaXmtCount = 0;


interrupt void dmaXmtIsr(void) {
    dmaXmtCount++;   // <<< debug
    int i;
    if (currentTxBuf == 0) // 0 eh ping. Na chamada, o Ping esta sendo enviado e devemos trocar pro buffer do Pong.
    {
        srcAddrHi = (Uint16)(((Uint32)(&xmtPong[0])) >> 15) & 0xFFFFu;
        srcAddrLo = (Uint16)(((Uint32)(&xmtPong[0])) << 1) & 0xFFFFu;

        dmaXmtConfig.dmacssal = (DMA_AdrPtr)srcAddrLo;
        dmaXmtConfig.dmacssau = srcAddrHi;

        currentTxBuf = 1;


        DMA_config(hDmaXmt,&dmaXmtConfig);
        DMA_start(hDmaXmt);
//        for (i = 0; i < N; i++)
//        {
//            xmtPing[i] = 0;
//        }
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
//        for (i = 0; i < N; i++)
//        {
//            xmtPong[i] = 0;
//        }
        txBufEmpty[1] = 1; // Flag que indica que o buffer no Pong agora esta vazio

    }

}

interrupt void dmaRcvIsr(void) {
    dmaRcvCount++;   // <<< debug
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
