#ifndef __REVERB_H__
#define __REVERB_H__

// ==============================================================================
// Definições de Tuning (Baseado no FreeVerb/JUCE para 44.1kHz)
// ==============================================================================

#define NUM_COMBS 8
#define NUM_ALLPASS 4
#define STEREO_SPREAD 23

// Tamanhos máximos para alocação estática (Tunings originais)
// Comb tunings: 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
// Allpass tunings: 556, 441, 341, 225
#define MAX_COMB_SIZE 1617
#define MAX_ALLPASS_SIZE 556

// Fatores de escala do JUCE
#define FIXED_GAIN 0.015f
#define WET_SCALE_FACTOR 3.0f
#define DRY_SCALE_FACTOR 2.0f
#define ROOM_SCALE_FACTOR 0.28f
#define ROOM_OFFSET 0.7f
#define DAMP_SCALE_FACTOR 0.4f

// ==============================================================================
// Estruturas de Dados
// ==============================================================================

// Estrutura para Filtro Comb (Pente)
typedef struct {
    float buffer[MAX_COMB_SIZE];
    int size;
    int idx;
    float last; // Armazena o último valor para o filtro low-pass (damping)
} comb_filter_t;

// Estrutura para Filtro All-Pass
typedef struct {
    float buffer[MAX_ALLPASS_SIZE];
    int size;
    int idx;
} allpass_filter_t;

// Estrutura Principal
typedef struct {
    // Parâmetros do Usuário
    float room_size;
    float damping;
    float wet_level;
    float dry_level;
    
    // Coeficientes Internos (Calculados a partir dos parâmetros)
    float gain_damp;
    float gain_room; // Feedback
    float gain_wet1;
    float gain_wet2;
    float gain_dry;

    // Estado Interno (Buffers)
    comb_filter_t combs[NUM_COMBS];
    allpass_filter_t allpasses[NUM_ALLPASS];
    
} reverb_params_t;

void reverb_init(float room_size, float damping, float wet_level, float dry_level, reverb_params_t *r_params);
float reverb_process(float input_sample, reverb_params_t *r_params);

#endif
