#ifndef AUTOTUNE_H
#define AUTOTUNE_H

#include "tistdtypes.h"

/* ========= Ajuste esses conforme seu projeto ========= */
#define AT_SAMPLE_RATE      48000u

#define AT_MIN_FREQ         80u
#define AT_MAX_FREQ         400u

/* Tamanho do buffer de análise/delay (potência de 2 facilita wrap) */
#define AT_BUFFER_SIZE      512u
#define AT_MASK             (AT_BUFFER_SIZE - 1u)

/* A cada quantas amostras recalcula pitch (mais alto = mais leve e estável) */
#define AT_UPDATE_RATE      256u

/* Threshold AMDF (média do |x[n]-x[n+p]|) em Q15 (~0.10 -> 3277) */
#define AT_AMDF_THRESHOLD_Q15  ((Int16)3277)

/* Força da correção e suavização (Q15) */
#define AT_CORRECTION_AMOUNT_Q15  ((Int16)16384)  /* 0.5 */
#define AT_SMOOTH_FACTOR_Q15      ((Int16)4096)   /* 0.125 */

/* Q-format helpers */
#define Q15_ONE   ((Int16)32767)
#define Q16_ONE   ((Int32)32768)   /* 1.0 em Q16.15 */

typedef struct
{
    /* alvo (em Hz), usado só para referência/telemetria */
    Uint16 target_freq_hz;

    /* buffers Q15 */
    Int16  input_buffer[AT_BUFFER_SIZE];
    Int16  delay_buffer[AT_BUFFER_SIZE];

    /* índices */
    Uint16 write_pos;       /* para input_buffer */
    Uint16 d_write_idx;     /* para delay_buffer */

    /* leitura Q16.15 */
    Int32  d_read_idx_q16;  /* Q16.15 (1 amostra = 1<<15) */

    /* estado de detecção */
    Uint16 samples_since_update;
    Uint16 current_period;  /* período detectado em amostras */

    /* razão atual Q16.15 (0.5..2.0) */
    Int32  current_ratio_q16;

    /* parâmetros */
    Int16 correction_amount_q15;
    Int16 smooth_factor_q15;

    /* alvo em período Q16.15: target_period = Fs/target_freq */
    Int32 target_period_q16;

} autotune_t;

/* Init: você passa o alvo em Hz (sem float) */
void  autotune_init_hz(autotune_t *at, Uint16 target_freq_hz);

/* Processa 1 amostra Q15 e retorna 1 amostra Q15 */
Int16 autotune_process(autotune_t *at, Int16 input_q15);

#endif
