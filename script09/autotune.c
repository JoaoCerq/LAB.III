#include "tistdtypes.h"
#include "autotune.h"


// FunÃ§Ã£o auxiliar para leitura com interpolaÃ§Ã£o linear (previne aliasing bÃ¡sico)
float get_interpolated_sample(float *buffer, float read_idx, int buffer_size) {
    int idx_int = (int)read_idx;
    float frac = read_idx - idx_int;

    int idx_next = idx_int + 1;
    if (idx_next >= buffer_size) idx_next = 0;

    return (buffer[idx_int] * (1.0f - frac)) + (buffer[idx_next] * frac);
}

float detect_pitch_amdf(float *buffer, int size, float sample_rate) {
    Uint16 min_period = (int)(sample_rate / AT_MAX_FREQ);
    int max_period = (int)(sample_rate / AT_MIN_FREQ);
    
    if (max_period >= size / 2) max_period = size / 2;

    int best_period = 0;
    float min_diff = 1000000.0f; // Valor alto inicial

    // Varre os possÃ­veis perÃ­odos (lags)
    Uint16 p;
    for (p = min_period; p <= max_period; p++) {
        float diff = 0.0f;
        
        // Compara o sinal com ele mesmo deslocado por 'p'
        // Limitamos 'k' para nÃ£o estourar o buffer e manter performance
        Uint16 checks = 0;
        Uint16 k;
        for (k = 0; k < size - p; k ++) {
            float delta = buffer[k] - buffer[k + p];
            diff += (delta > 0 ? delta : -delta);
            checks++;
        }
        
        // Normaliza pela quantidade de checagens
        if (checks > 0) diff /= checks;

        if (diff < min_diff) {
            min_diff = diff;
            best_period = p;
        }
    }

    // Limiar de silÃªncio/ruÃ­do: se a diferenÃ§a for muito alta, nÃ£o Ã© uma nota clara
    if (min_diff > 0.1f) return 0.0f; 

    if (best_period > 0) {
        return sample_rate / (float)best_period;
    }
    
    return 0.0f;
}

void autotune_init(autotune_t *at, float target_freq) {
    at->target_freq = target_freq;
    at->sample_rate = 44100.0f;
    at->current_detected_freq = target_freq; 
    at->d_write_idx = 0;
    at->d_read_idx = AT_BUFFER_SIZE / 2.0f; 
    at->correction_amount = 0.25f;
    at->smooth_factor = 0.05f;   // Ajuste bem lento (suave)
    at->current_ratio = 0.8f;     // ComeÃ§a tocando na velocidade normal
}

float autotune_process(autotune_t *at, float input) {
    // 1. Armazenar no buffer de anÃ¡lise
    at->input_buffer[at->write_pos] = input;
    
    // 2. Armazenar no buffer de delay
    at->delay_buffer[at->d_write_idx] = input;

    // ==========================================================
    // ESTÃ�GIO 1: DETECÃ‡ÃƒO
    // ==========================================================
    at->samples_since_update++;
    if (at->samples_since_update >= AT_UPDATE_RATE) {
        float detected = detect_pitch_amdf(at->input_buffer, AT_BUFFER_SIZE, at->sample_rate);
        if (detected > 0.0f) {
            // Dica: Aumentei o peso do histÃ³rico (0.7) para estabilizar a detecÃ§Ã£o
            at->current_detected_freq = (at->current_detected_freq * 0.5f) + (detected * 0.5f);
        }
        at->samples_since_update = 0;
    }

    // ==========================================================
    // ESTÃ�GIO 2: CÃ�LCULO DA RAZÃƒO (MODIFICADO PARA SUAVIZAÃ‡ÃƒO)
    // ==========================================================
    
    float freq_in = (at->current_detected_freq < AT_MIN_FREQ) ? at->target_freq : at->current_detected_freq;
    
    // 1. Calcula a razÃ£o "Ideal" (RobÃ³tica)
    float ideal_ratio = at->target_freq / freq_in;

    // 2. Aplica a ForÃ§a de CorreÃ§Ã£o (Correction Amount)
    // Se amount for 0, target_ratio vira 1.0 (som original).
    // Se amount for 1, target_ratio vira ideal_ratio (correÃ§Ã£o total).
    float target_ratio = 1.0f + (ideal_ratio - 1.0f) * at->correction_amount;

    // Clamp de seguranÃ§a no alvo
    if (target_ratio > 2.0f) target_ratio = 2.0f;
    if (target_ratio < 0.5f) target_ratio = 0.5f;

    // 3. Aplica SuavizaÃ§Ã£o (Glide / Slew Rate)
    // Move o ratio atual em direÃ§Ã£o ao target_ratio lentamente a cada amostra
    at->current_ratio += (target_ratio - at->current_ratio) * at->smooth_factor;

    // ==========================================================
    // ESTÃ�GIO 3: PITCH SHIFTING (USANDO current_ratio)
    // ==========================================================

    float ratio = at->current_ratio; // Usa a versÃ£o suavizada

    // --- LÃ³gica de Dupla Leitura (Mantida igual, apenas usando o novo ratio) ---
    float read_head_1 = at->d_read_idx;
    float read_head_2 = at->d_read_idx + (AT_BUFFER_SIZE / 2.0f);

    if (read_head_1 >= AT_BUFFER_SIZE) read_head_1 -= AT_BUFFER_SIZE;
    if (read_head_2 >= AT_BUFFER_SIZE) read_head_2 -= AT_BUFFER_SIZE;

    // CÃ¡lculo dos pesos (Mantido)
    float dist = read_head_1 - (float)at->d_write_idx;
    if (dist < 0) dist += AT_BUFFER_SIZE;

    float half_buffer = AT_BUFFER_SIZE / 2.0f;
    float raw_val = (dist - half_buffer) / half_buffer;
    float abs_val = (raw_val < 0.0f) ? -raw_val : raw_val;
    float weight_1 = 1.0f - abs_val;
    if (weight_1 < 0.0f) weight_1 = 0.0f;
    float weight_2 = 1.0f - weight_1;

    // Leitura e Mixagem
    float sample1 = get_interpolated_sample(at->delay_buffer, read_head_1, AT_BUFFER_SIZE);
    float sample2 = get_interpolated_sample(at->delay_buffer, read_head_2, AT_BUFFER_SIZE);
    float sample_out = (sample1 * weight_1) + (sample2 * weight_2);

    // AtualizaÃ§Ã£o dos Ponteiros
    at->d_write_idx++;
    if (at->d_write_idx >= AT_BUFFER_SIZE) at->d_write_idx = 0;
    
    at->write_pos++;
    if (at->write_pos >= AT_BUFFER_SIZE) at->write_pos = 0;

    // AQUI: Usa o ratio suavizado para mover o ponteiro
    at->d_read_idx += ratio;
    if (at->d_read_idx >= AT_BUFFER_SIZE) at->d_read_idx -= AT_BUFFER_SIZE;

    return sample_out;
}
