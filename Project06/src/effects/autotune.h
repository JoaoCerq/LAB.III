#ifndef AUTOTUNE_H
#define AUTOTUNE_H

#include "tistdtypes.h"

// Configurações do Autotune
#define AT_BUFFER_SIZE 2048
#define AT_MIN_FREQ 80.0f
#define AT_MAX_FREQ 600.0f
#define AT_UPDATE_RATE 512

// Parâmetros de inicialização (antes em autotune_init)
#define AT_SAMPLE_RATE 44100.0f
#define AT_CORRECTION_AMOUNT 0.25f
#define AT_SMOOTH_FACTOR 0.05f

// Constantes em Q15
#define Q15_ONE 32767
#define AT_CORRECTION_AMOUNT_Q15 ((Int16)(AT_CORRECTION_AMOUNT * 32768.0f))
#define AT_SMOOTH_FACTOR_Q15     ((Int16)(AT_SMOOTH_FACTOR * 32768.0f))

typedef struct {
    // Parâmetros Básicos
    float target_freq; // Mantido como float para cálculo do ratio

    // Parâmetros de controle em Q15
    Int16 correction_amount_q15;
    Int16 smooth_factor_q15;

    // Estado do Detector de Pitch (AMDF)
    Int16 input_buffer[AT_BUFFER_SIZE];
    Int16 write_pos;
    float current_detected_freq; // A detecção ainda pode ser float
    Int16 samples_since_update;

    // Estado do Pitch Shifter
    Int16 delay_buffer[AT_BUFFER_SIZE];
    Int16 d_write_idx;
    Int32 d_read_idx_q16; // Usando Q16.15 para a parte fracionária

    Int16 current_ratio_q15; // Armazena a razão atual para interpolação

} autotune_t;

void autotune_init(autotune_t *at, float target_freq);
Int16 autotune_process(autotune_t *at, Int16 input);

#endif