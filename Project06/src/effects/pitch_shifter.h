#ifndef PITCH_SHIFTER_H
#define PITCH_SHIFTER_H

#include "tistdtypes.h"

/* Buffer circular (potência de 2) */
#define PS_BUFFER_SIZE 512u
#define PS_MASK        (PS_BUFFER_SIZE - 1u)

/* Q2.14: 1.0 = 16384, 2.0 = 32768 (precisa Int32) */
#define PITCH_1X_Q14   ((Int32)16384)
#define PITCH_2X_Q14   ((Int32)32768)

typedef struct {
    Int16  buffer[PS_BUFFER_SIZE];  /* Q15 */
    Uint16 write_index;

    Int32  phasor_q15;              /* 0..32767 com wrap em 32768 */
    Int32  pitch_q14;               /* Q2.14 (Int32) */
    Int16  phase_step_q15;          /* Q15 (pode ser negativo) */
    Uint16 window_size;             /* <= PS_BUFFER_SIZE-2 */
} pitch_shifter_t;

/* Init usando float APENAS aqui (fora do tempo real) */
void  pitch_shifter_init(pitch_shifter_t *ps, float pitch_factor);

/* Init sem float */
void  pitch_shifter_init_q14(pitch_shifter_t *ps, Int32 pitch_q14);

/* Troca pitch em runtime sem float */
void  pitch_shifter_set_pitch_q14(pitch_shifter_t *ps, Int32 pitch_q14);

/* Processa 1 amostra Q15 */
Int16 pitch_shifter_process(pitch_shifter_t *ps, Int16 input_q15);

#endif
