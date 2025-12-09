#include "pitch_shifter.h"

// Função auxiliar para interpolação linear (melhora qualidade do som)
static inline float interpolate(float *buffer, float index) {
    Int16 idx_int = (Int16)index;
    float frac = index - idx_int;
    
    // Garante o wrap-around seguro usando a máscara
    Int16 idx0 = idx_int & PS_MASK;
    Int16 idx1 = (idx_int + 1) & PS_MASK;
    
    float val0 = buffer[idx0];
    float val1 = buffer[idx1];
    
    return val0 + frac * (val1 - val0);
}

void pitch_shifter_init(pitch_shifter_t *ps, float pitch_factor) {
    Int16 i;
    // Limpa o buffer
    for(i = 0; i < PS_BUFFER_SIZE; i++) {
        ps->buffer[i] = 0.0f;
    }
    
    ps->write_index = 0;
    ps->phasor = 0.0f;
    ps->pitch_factor = pitch_factor;
    // Usamos um tamanho de janela ligeiramente menor que o buffer total para segurança
    ps->window_size = (float)(PS_BUFFER_SIZE - 2); 
}

float pitch_shifter_process(pitch_shifter_t *ps, float input_sample) {
    
    // 1. Escreve a nova amostra no buffer circular
    ps->buffer[ps->write_index] = input_sample;
    
    // 2. Calcula o passo da fase (Rate)
    // Se pitch = 1.0, step = 0 (delay constante)
    // Se pitch = 2.12, queremos ler mais rápido que escrevemos.
    // A fórmula fundamental para OLA Pitch Shifting é: step = (1 - pitch) / window_size
    // Mas como usamos um phasor cíclico 0..1, normalizamos:
    float phase_step = (1.0f - ps->pitch_factor) / ps->window_size;
    
    // Atualiza o phasor (oscilador dente de serra)
    ps->phasor += phase_step;
    
    // Mantém o phasor no intervalo [0.0, 1.0]
    if (ps->phasor >= 1.0f) ps->phasor -= 1.0f;
    if (ps->phasor < 0.0f)  ps->phasor += 1.0f;
    
    // 3. Calculamos dois "taps" (cabeçotes de leitura) defasados em 180 graus (0.5)
    // Isso é o coração do algoritmo OLA: enquanto um grão termina, o outro começa.
    float phase_a = ps->phasor;
    float phase_b = ps->phasor + 0.5f;
    if (phase_b >= 1.0f) phase_b -= 1.0f;
    
    // 4. Calcula o atraso (delay) baseado na fase
    // O delay varia de 0 a window_size
    float delay_a = phase_a * ps->window_size;
    float delay_b = phase_b * ps->window_size;
    
    // 5. Calcula as posições de leitura absolutas
    float read_idx_a = (float)ps->write_index - delay_a;
    float read_idx_b = (float)ps->write_index - delay_b;
    
    // Trata números negativos para o buffer circular
    while (read_idx_a < 0.0f) read_idx_a += PS_BUFFER_SIZE;
    while (read_idx_b < 0.0f) read_idx_b += PS_BUFFER_SIZE;
    
    // 6. Obtém as amostras interpoladas
    float sample_a = interpolate(ps->buffer, read_idx_a);
    float sample_b = interpolate(ps->buffer, read_idx_b);
    
    // 7. Calcula o ganho da janela (Triangular / Hanning simplificado)
    // Queremos que o volume seja 1.0 no centro da fase (0.5) e 0.0 nas bordas (0.0 e 1.0)
    // Uma janela triangular simples é eficiente para DSP:
    float gain_a, gain_b;
    
    // Ganho para Tap A (pico em 0.5)
    if (phase_a < 0.5f) gain_a = phase_a * 2.0f;
    else                gain_a = (1.0f - phase_a) * 2.0f;
    
    // Ganho para Tap B (pico em 0.5)
    if (phase_b < 0.5f) gain_b = phase_b * 2.0f;
    else                gain_b = (1.0f - phase_b) * 2.0f;

    // 8. Soma ponderada (Overlap-Add)
    float output = (sample_a * gain_a) + (sample_b * gain_b);
    
    // 9. Incrementa ponteiro de escrita
    ps->write_index++;
    ps->write_index &= PS_MASK; // Wrap around (rápido pois size é potência de 2)
    
    return output;
}
