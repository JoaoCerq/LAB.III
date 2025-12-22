#include <stdint.h>
#include <stdio.h>
#include "tistdtypes.h"
#include "reverb.h"

// ==============================================================================
// Funções Auxiliares de Processamento
// ==============================================================================

// Processamento do Filtro Comb com Buffer Circular Otimizado
float process_comb(comb_filter_t *comb, float input, float damp, float feedback) {
    float output = comb->buffer[comb->idx];
    
    // Low-pass filter no feedback (Damping)
    comb->last = (output * (1.0f - damp)) + (comb->last * damp);
    
    // JUCE usa proteção contra denormalização, omitido aqui para simplicidade em C puro,
    // mas em DSPs dedicados pode ser necessário flush-to-zero.
    
    float temp = input + (comb->last * feedback);
    
    comb->buffer[comb->idx] = temp;
    
    // Incremento circular sem módulo (mais rápido que %)
    comb->idx++;
    if (comb->idx >= comb->size) {
        comb->idx = 0;
    }
    
    return output;
}

// Processamento do Filtro All-Pass
float process_allpass(allpass_filter_t *ap, float input) {
    float buffered_val = ap->buffer[ap->idx];
    
    float temp = input + (buffered_val * 0.5f);
    
    ap->buffer[ap->idx] = temp;
    
    ap->idx++;
    if (ap->idx >= ap->size) {
        ap->idx = 0;
    }
    
    return buffered_val - input;
}

// Atualiza coeficientes internos para evitar recálculo por amostra
void update_internal_params(reverb_params_t *p) {
    // Mapeamento idêntico ao JUCE setParameters
    float wet = p->wet_level * WET_SCALE_FACTOR;
    
    p->gain_dry = p->dry_level * DRY_SCALE_FACTOR;
    p->gain_wet1 = 0.5f * wet;
    p->gain_wet2 = 0.5f * wet;

    p->gain_damp = p->damping * DAMP_SCALE_FACTOR;
    p->gain_room = (p->room_size * ROOM_SCALE_FACTOR) + ROOM_OFFSET;
}

// ==============================================================================
// Implementação das Funções Solicitadas
// ==============================================================================

void reverb_init(float room_size, float damping, float wet_level, float dry_level, reverb_params_t *r_params)
{
    // Definir Parâmetros Iniciais
    r_params->room_size = room_size;
    r_params->damping = damping;
    r_params->wet_level = wet_level;
    r_params->dry_level = dry_level;

    // Configurar Tunings (Tamanhos dos Buffers)
    // Valores fixos para 44.1kHz conforme FreeVerb
    const int comb_tunings[] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    const int allpass_tunings[] = { 556, 441, 341, 225 };
    Uint16 i;
    for (i = 0; i < NUM_COMBS; i++) {
        r_params->combs[i].size = comb_tunings[i];
        r_params->combs[i].idx = 0;
        r_params->combs[i].last = 0.0f;
    }
    for (i = 0; i < NUM_ALLPASS; i++) {
        r_params->allpasses[i].size = allpass_tunings[i];
        r_params->allpasses[i].idx = 0;
    }

    // Calcular ganhos iniciais
    update_internal_params(r_params);
}

float reverb_process(float input_sample, reverb_params_t *r_params)
{
    // 1. Converter Int16 para Float (-1.0 a 1.0) e aplicar Input Gain
    // O JUCE usa um ganho fixo de 0.015f na entrada do algoritmo
    float input = input_sample * FIXED_GAIN;
    
    float output_accum = 0.0f;

    // 2. Processar Filtros Comb em Paralelo
    // Como estamos fazendo Mono, somamos todos os combs
    Uint16 i;
    for (i = 0; i < NUM_COMBS; i++) {
        output_accum += process_comb(&r_params->combs[i], input, r_params->gain_damp, r_params->gain_room);
    }

    // 3. Processar Filtros All-Pass em Série
    for (i = 0; i < NUM_ALLPASS; i++) {
        output_accum = process_allpass(&r_params->allpasses[i], output_accum);
    }

    // 4. Mixar Wet/Dry
    float wet_mix = output_accum * (r_params->gain_wet1 + r_params->gain_wet2);
    float dry_mix = input_sample * r_params->gain_dry;
    
    float final_sample = wet_mix + dry_mix;

    // 5. Hard Clipper e Conversão para Int16
    if (final_sample > 1.0f) final_sample = 1.0f;
    if (final_sample < -1.0f) final_sample = -1.0f;

    return final_sample;
}
