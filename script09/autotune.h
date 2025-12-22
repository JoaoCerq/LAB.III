#ifndef AUTOTUNE_H
#define AUTOTUNE_H

#include <stdint.h>

// Configurações do Autotune
#define AT_BUFFER_SIZE 256
#define AT_MIN_FREQ 80.0f     
#define AT_MAX_FREQ 600.0f    
#define AT_UPDATE_RATE 512    

typedef struct {
    // Parâmetros Básicos
    float target_freq;
    float sample_rate;

    // 0.0 (sem correção) a 1.0 (correção total/robótica)
    float correction_amount; 
    
    // Fator de inércia/suavidade (0.0001 a 0.1).
    // Quanto menor, mais lento e natural é o ajuste.
    float smooth_factor;

    // Estado do Detector de Pitch (AMDF)
    float input_buffer[AT_BUFFER_SIZE];
    int   write_pos;
    float current_detected_freq;
    int   samples_since_update;

    // Estado do Pitch Shifter
    float delay_buffer[AT_BUFFER_SIZE]; 
    int   d_write_idx;      
    float d_read_idx;       
    
    float current_ratio; // Armazena a razão atual para interpolação passo-a-passo
    
} autotune_t;

void autotune_init(autotune_t *at, float target_freq);
float autotune_process(autotune_t *at, float input);

#endif
