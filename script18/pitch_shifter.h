#ifndef PITCH_SHIFTER_H
#define PITCH_SHIFTER_H

#include "tistdtypes.h"

// Tamanho do buffer circular. Deve ser potência de 2 para eficiência (masking).
// 2048 ou 4096 é ideal para voz.
#define PS_BUFFER_SIZE 512
#define PS_MASK (PS_BUFFER_SIZE - 1)

typedef struct {
    float buffer[PS_BUFFER_SIZE];
    int   write_index;
    float phasor;        // Fase atual do oscilador de leitura (0.0 a 1.0)
    float pitch_factor;  // Fator de pitch (ex: 2.12)
    float window_size;   // Tamanho da "janela" ou grÃ£o (geralmente PS_BUFFER_SIZE - 1)
} pitch_shifter_t;

/**
 * Inicializa a estrutura do pitch shifter.
 * @param ps Ponteiro para a estrutura
 * @param pitch_factor Fator de mudanÃ§a (ex: 0.5 = oitava abaixo, 2.0 = oitava acima)
 */
void pitch_shifter_init(pitch_shifter_t *ps, float pitch_factor);

/**
 * Processa uma Ãºnica amostra (float) e retorna a amostra com pitch alterado.
 * @param ps Ponteiro para a estrutura
 * @param input_sample Amostra de entrada (-1.0 a 1.0)
 * @return Amostra processada
 */
float pitch_shifter_process(pitch_shifter_t *ps, float input_sample);

#endif
