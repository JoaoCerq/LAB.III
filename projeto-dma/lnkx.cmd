-stack    0x2000      /* Primary stack size   */
-sysstack 0x1000      /* Secondary stack size */
-heap     0x2000      /* Heap area size       */

-c                    /* Use C linking conventions: auto-init vars at runtime */
-u _Reset             /* Force load of reset interrupt handler                */

/*============================*/
/*        MEMORY MAP          */
/*============================*/

MEMORY
{
 PAGE 0:  /* ---- Unified Program/Data Address Space ---- */

  MMR    (RWIX): origin = 0x000000, length = 0x0000C0  /* MMRs */
  BTRSVD (RWIX): origin = 0x0000C0, length = 0x000240  /* Reserved for Boot Loader */
  DARAM  (RWIX): origin = 0x000300, length = 0x00FB00  /* 64KB - MMRs - VECS*/
  VECS   (RWIX): origin = 0x00FE00, length = 0x000200  /* 256 bytes Vector Table */

  CE0          : origin = 0x010000, length = 0x3f0000  /* 4M minus 64K Bytes */
  CE1          : origin = 0x400000, length = 0x400000
  CE2          : origin = 0x800000, length = 0x400000
  CE3          : origin = 0xC00000, length = 0x3F8000

  PDROM   (RIX): origin = 0xFF8000, length = 0x008000  /* 32KB */

 PAGE 2:  /* -------- 64K-word I/O Address Space -------- */

  IOPORT (RWI) : origin = 0x000000, length = 0x020000
}

/*============================*/
/*        SECTIONS             */
/*============================*/

SECTIONS
{
   /* Código */
   .text     >  DARAM align(32) fill = 20h { * (.text) }

   .text1   >  DARAM align(32) fill = 20h

   /* Primary system stack        */
   .stack    >  DARAM align(32) fill = 00h
   /* Secondary system stack      */
   .sysstack >  DARAM align(32) fill = 00h

   /* CSL data                    */
   .csldata  >  DARAM align(32) fill = 00h

   /* Initialized vars            */
   .data     >  DARAM align(32) fill = 00h

   /* Global & static vars        */
   .bss      >  DARAM align(32) fill = 00h

   /* Constant data               */
   .const    >  DARAM align(32) fill = 00h

   /* Dynamic memory (malloc)     */
   .sysmem   >  DARAM

   /* Switch statement tables     */
   .switch   >  DARAM

   /* Auto-initialization tables  */
   .cinit    >  DARAM

   /* Initialization fn tables    */
   .pinit    >  DARAM align(32) fill = 00h

   /* C I/O buffers               */
   .cio      >  DARAM align(32) fill = 00h

   /* Arguments to main()         */
   .args     >  DARAM align(32) fill = 00h

   /* interrupt vector table must be on 256 "page" boundry */
   vectors   >  VECS
   .vectors   >  VECS
   /* Global & static ioport vars */
   .ioport   >  IOPORT PAGE 2

   /* ========= SEÇÃO EXTRA DO EXEMPLO DMA =========
    * Qualquer coisa marcada no código com:
    *   #pragma DATA_SECTION(algumaCoisa, "dmaMem");
    * vai cair aqui.
    */
   dmaMem    >  DARAM align(32) fill = 00h
}
