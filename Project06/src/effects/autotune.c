#include "autotune.h"
#include "c55x.h" // Intrinsics da família C55x

/* =========================================================================
 * Helpers de saturação / Q15 e intrinsics (semelhante ao reverb.c)
 * ========================================================================= */

/* Saturação manual 32 → Q15 */
static inline Int16 sat_q15(Int32 x) {
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (Int16)x;
}

/* Multiplicação Q15 usando intrinsic saturada fracionária */
static inline Int16 q15_mul(Int16 a, Int16 b) {
    return _smpy(a, b);
}

/* Soma Q15 saturada */
static inline Int16 q15_add(Int16 a, Int16 b) {
    return _sadd(a, b);
}

/* Subtração Q15 saturada */
static inline Int16 q15_sub(Int16 a, Int16 b) {
    return _ssub(a, b);
}

/* Conversão float [-1,1) -> Q15 */
static Int16 float_to_q15(float x) {
    if (x >= 0.999969f) return 32767;
    if (x <= -1.0f) return -32768;
    return (Int16)(x * 32768.0f);
}

/* Conversão Q15 -> float */
static float q15_to_float(Int16 x) {
    return ((float)x) / 32768.0f;
}


/* =========================================================================
 * Funções de processamento de áudio (mantidas em float para simplicidade)
 * ========================================================================= */

// Detecção de pitch AMDF (mantida em float)
float detect_pitch_amdf(float *buffer, int size, float sample_rate) {
    int min_period = (int)(sample_rate / AT_MAX_FREQ);
    int max_period = (int)(sample_rate / AT_MIN_FREQ);
    if (max_period >= size / 2) max_period = size / 2;

    int best_period = 0;
    float min_diff = 1e9f;
    int p, k;

    for (p = min_period; p <= max_period; p++) {
        float diff = 0.0f;
        int checks = 0;
        for (k = 0; k < size - p; k++) {
            float delta = buffer[k] - buffer[k + p];
            diff += (delta > 0 ? delta : -delta);
            checks++;
        }
        if (checks > 0) diff /= checks;

        if (diff < min_diff) {
            min_diff = diff;
            best_period = p;
        }
    }

    if (min_diff > 0.1f) return 0.0f;
    if (best_period > 0) return sample_rate / (float)best_period;
    return 0.0f;
}

/* =========================================================================
 * Funções de processamento em Q15
 * ========================================================================= */

// Leitura interpolada para o buffer de delay em Q15
static Int16 get_interpolated_sample_q15(Int16 *buffer, Int32 read_idx_q16, int buffer_size) {
    Int16 idx_int = (Int16)(read_idx_q16 >> 15);
    Int16 frac_q15 = (Int16)(read_idx_q16 & 0x7FFF); // Máscara para pegar os 15 bits fracionários

    Int16 idx_next = idx_int + 1;
    if (idx_next >= buffer_size) idx_next = 0;

    Int16 sample1 = buffer[idx_int];
    Int16 sample2 = buffer[idx_next];

    Int16 term1 = q15_mul(sample1, q15_sub(Q15_ONE, frac_q15));
    Int16 term2 = q15_mul(sample2, frac_q15);

    return q15_add(term1, term2);
}


void autotune_init(autotune_t *at, float target_freq) {
    int i;
    at->target_freq = target_freq;
    at->correction_amount_q15 = AT_CORRECTION_AMOUNT_Q15;
    at->smooth_factor_q15 = AT_SMOOTH_FACTOR_Q15;

    at->current_detected_freq = target_freq;
    at->write_pos = 0;
    at->samples_since_update = 0;

    at->d_write_idx = 0;
    // Inicia o ponteiro de leitura na metade, em formato Q16.15
    at->d_read_idx_q16 = ((Int32)(AT_BUFFER_SIZE / 2)) << 15;
    at->current_ratio_q15 = Q15_ONE; // Razão 1.0

    for (i = 0; i < AT_BUFFER_SIZE; i++) {
        at->input_buffer[i] = 0;
        at->delay_buffer[i] = 0;
    }
}

Int16 autotune_process(autotune_t *at, Int16 input) {
    // 1. Armazenar no buffer de análise e delay
    at->input_buffer[at->write_pos] = input;
    at->delay_buffer[at->d_write_idx] = input;

    // ==========================================================
    // ESTÁGIO 1: DETECÇÃO (convertendo para float temporariamente)
    // ==========================================================
    at->samples_since_update++;
    if (at->samples_since_update >= AT_UPDATE_RATE) {
        // Buffer temporário para a detecção em float
        static float float_buf[AT_BUFFER_SIZE];
        int i;
        for(i=0; i<AT_BUFFER_SIZE; i++) {
            float_buf[i] = q15_to_float(at->input_buffer[i]);
        }

        float detected = detect_pitch_amdf(float_buf, AT_BUFFER_SIZE, AT_SAMPLE_RATE);
        if (detected > 0.0f) {
            at->current_detected_freq = (at->current_detected_freq * 0.5f) + (detected * 0.5f);
        }
        at->samples_since_update = 0;
    }

    // ==========================================================
    // ESTÁGIO 2: CÁLCULO DA RAZÃO (em float, depois convertido para Q15)
    // ==========================================================
    float freq_in = (at->current_detected_freq < AT_MIN_FREQ) ? at->target_freq : at->current_detected_freq;
    float ideal_ratio = at->target_freq / freq_in;

    // Aplica a força da correção
    float target_ratio_f = 1.0f + (ideal_ratio - 1.0f) * AT_CORRECTION_AMOUNT;

    // Clamp de segurança
    if (target_ratio_f > 2.0f) target_ratio_f = 2.0f;
    if (target_ratio_f < 0.5f) target_ratio_f = 0.5f;

    Int16 target_ratio_q15 = float_to_q15(target_ratio_f / 2.0f); // Normaliza para Q15 (div por 2 pois ratio pode ser > 1)

    // Suavização em Q15
    Int16 diff = q15_sub(target_ratio_q15, at->current_ratio_q15);
    Int16 adjustment = q15_mul(diff, at->smooth_factor_q15);
    at->current_ratio_q15 = q15_add(at->current_ratio_q15, adjustment);


    // ==========================================================
    // ESTÁGIO 3: PITCH SHIFTING (em Q15)
    // ==========================================================
    Int16 ratio_q15 = at->current_ratio_q15;

    // Lógica de dupla leitura
    Int32 read_head_1_q16 = at->d_read_idx_q16;
    Int32 read_head_2_q16 = at->d_read_idx_q16 + (((Int32)(AT_BUFFER_SIZE / 2)) << 15);

    // Wrap around dos ponteiros (em Q16.15)
    Int32 buffer_size_q16 = ((Int32)AT_BUFFER_SIZE) << 15;
    if (read_head_1_q16 >= buffer_size_q16) read_head_1_q16 -= buffer_size_q16;
    if (read_head_2_q16 >= buffer_size_q16) read_head_2_q16 -= buffer_size_q16;

    // Cálculo dos pesos (cross-fade)
    Int32 dist_q16 = read_head_1_q16 - (((Int32)at->d_write_idx) << 15);
    if (dist_q16 < 0) dist_q16 += buffer_size_q16;

    Int32 half_buffer_q16 = buffer_size_q16 >> 1;
    Int32 raw_val_q16 = dist_q16 - half_buffer_q16;

    // Normaliza para Q15
    Int16 weight_1 = float_to_q15(1.0f - (float)abs(raw_val_q16) / half_buffer_q16);
    if (weight_1 < 0) weight_1 = 0;
    Int16 weight_2 = q15_sub(Q15_ONE, weight_1);

    // Leitura e Mixagem
    Int16 sample1 = get_interpolated_sample_q15(at->delay_buffer, read_head_1_q16, AT_BUFFER_SIZE);
    Int16 sample2 = get_interpolated_sample_q15(at->delay_buffer, read_head_2_q16, AT_BUFFER_SIZE);

    Int16 term1 = q15_mul(sample1, weight_1);
    Int16 term2 = q15_mul(sample2, weight_2);
    Int16 sample_out = q15_add(term1, term2);

    // Atualização dos Ponteiros
    at->d_write_idx++;
    if (at->d_write_idx >= AT_BUFFER_SIZE) at->d_write_idx = 0;

    at->write_pos++;
    if (at->write_pos >= AT_BUFFER_SIZE) at->write_pos = 0;

    // Avança o ponteiro de leitura usando a razão (multiplicação Q15)
    // Multiplicamos por 2 porque a razão foi normalizada
    Int32 step = (Int32)q15_mul(_sadd(ratio_q15, ratio_q15), Q15_ONE);
    at->d_read_idx_q16 += step;
    if (at->d_read_idx_q16 >= buffer_size_q16) at->d_read_idx_q16 -= buffer_size_q16;

    return sample_out;
}
