#include "autotune.h"
#include "c55x.h"  /* _smpy, _sadd, _ssub */

/* ========= Helpers intrinsics ========= */
static inline Int16 q15_mul(Int16 a, Int16 b) { return _smpy(a, b); }
static inline Int16 q15_add(Int16 a, Int16 b) { return _sadd(a, b); }
static inline Int16 q15_sub(Int16 a, Int16 b) { return _ssub(a, b); }

static inline Int16 sat16(Int32 x)
{
    if (x > 32767)  return 32767;
    if (x < -32768) return -32768;
    return (Int16)x;
}

/* abs seguro em Int32 (sem <stdlib.h>) */
static inline Int32 iabs32(Int32 x) { return (x < 0) ? -x : x; }

/* ========= Interpolação linear Q15 =========
   read_idx_q16 é Q16.15:
   idx_int = read>>15; frac = read & 0x7FFF (Q15)
*/
static inline Int16 interp_q15(const Int16 *buf, Int32 read_idx_q16)
{
    Uint16 idx = (Uint16)((read_idx_q16 >> 15) & AT_MASK);
    Uint16 idx1 = (idx + 1u) & AT_MASK;

    Int16 frac = (Int16)(read_idx_q16 & 0x7FFF);     /* Q15 [0..32767] */
    Int16 one_minus = (Int16)(Q15_ONE - frac);

    Int16 v0 = buf[idx];
    Int16 v1 = buf[idx1];

    Int16 p0 = q15_mul(v0, one_minus);
    Int16 p1 = q15_mul(v1, frac);

    return q15_add(p0, p1);
}

/* ========= Crossfade triangular em Q15 =========
   Usa distância entre read_head_1 e write (em Q16.15), pico no meio do buffer
*/
static inline Int16 tri_weight_q15(Int32 dist_q16, Int32 half_q16)
{
    /* dist em [0..buffer) -> centraliza em half */
    Int32 x = dist_q16 - half_q16;
    x = iabs32(x);
    if (x > half_q16) x = half_q16;

    /* w = 1 - |x|/half  em Q15 */
    Int32 w = ((half_q16 - x) << 15) / half_q16;  /* 0..32768 */
    if (w > 32767) w = 32767;
    return (Int16)w;
}

/* ========= AMDF em inteiro (Q15) =========
   Retorna período (samples) ou 0 se “sem pitch confiável”.
*/
static Uint16 detect_period_amdf_q15(const Int16 *buf)
{
    Uint16 min_period = (Uint16)(AT_SAMPLE_RATE / AT_MAX_FREQ);
    Uint16 max_period = (Uint16)(AT_SAMPLE_RATE / AT_MIN_FREQ);

    if (max_period >= (AT_BUFFER_SIZE / 2u))
        max_period = (AT_BUFFER_SIZE / 2u);

    Uint16 best_p = 0;
    Int32  best_avg = 0x7FFFFFFF;
    Uint16 p, k;
    for (p = min_period; p <= max_period; p++)
    {
        Int32 acc = 0;
        Uint16 checks = (Uint16)(AT_BUFFER_SIZE - p);

        for (k = 0; k < checks; k++)
        {
            Int32 d = (Int32)buf[k] - (Int32)buf[k + p];
            acc += iabs32(d);
        }

        /* média em “Q15 raw”: acc/checks (a unidade é Q15) */
        Int32 avg = acc / (Int32)checks;

        if (avg < best_avg)
        {
            best_avg = avg;
            best_p = p;
        }
    }

    /* threshold: se média muito alta, não confia */
    if (best_p == 0) return 0;

    if (best_avg > (Int32)AT_AMDF_THRESHOLD_Q15)
        return 0;

    return best_p;
}

/* ========= Atualiza ratio (Q16.15) a partir de período detectado =========
   ratio = detected_period / target_period
   target_period_q16 = Fs/target_freq em Q16.15
*/
static Int32 period_to_ratio_q16(Uint16 detected_period, Int32 target_period_q16)
{
    if (detected_period == 0) return Q16_ONE; /* bypass */

    /* ratio_q16 = detected_period / (target_period_q16/32768)
       => ratio_q16 = (detected_period<<15) / target_period_q16   (Q16.15)
    */
    Int32 num = ((Int32)detected_period) << 15;
    Int32 den = target_period_q16;
    if (den <= 0) return Q16_ONE;

    Int32 r = num / den; /* ainda Q16.15 */

    /* clamp 0.5..2.0 */
    if (r < (Q16_ONE >> 1)) r = (Q16_ONE >> 1);
    if (r > (Q16_ONE << 1)) r = (Q16_ONE << 1);

    return r;
}

void autotune_init(autotune_t *at, Uint16 target_freq_hz)
{
    at->target_freq_hz = target_freq_hz;
    at->correction_amount_q15 = AT_CORRECTION_AMOUNT_Q15;
    at->smooth_factor_q15     = AT_SMOOTH_FACTOR_Q15;

    at->write_pos = 0;
    at->d_write_idx = 0;
    at->samples_since_update = 0;

    at->current_period = 0;

    /* target_period_q16 = Fs/target_freq em Q16.15:
       target_period = Fs/target_freq (em samples).
       Q16.15 => (Fs<<15)/target_freq
    */
    at->target_period_q16 = ((Int32)AT_SAMPLE_RATE << 15) / (Int32)target_freq_hz;

    at->current_ratio_q16 = Q16_ONE;

    /* leitura começa no meio do buffer */
    at->d_read_idx_q16 = ((Int32)(AT_BUFFER_SIZE / 2u)) << 15;
    Uint16 i = 0;
    for (i = 0; i < AT_BUFFER_SIZE; i++)
    {
        at->input_buffer[i] = 0;
        at->delay_buffer[i] = 0;
    }
}

Int16 autotune_process(autotune_t *at, Int16 input_q15)
{
    /* 1) grava nos buffers */
    at->input_buffer[at->write_pos] = input_q15;
    at->delay_buffer[at->d_write_idx] = input_q15;

    /* 2) a cada AT_UPDATE_RATE, detecta período e ajusta ratio */
    at->samples_since_update++;
    if (at->samples_since_update >= AT_UPDATE_RATE)
    {
        Uint16 p = detect_period_amdf_q15(at->input_buffer);
        if (p != 0) at->current_period = p;

        Int32 ideal_ratio_q16 = period_to_ratio_q16(at->current_period, at->target_period_q16);

        /* target_ratio = 1 + (ideal-1)*correction_amount */
        Int32 delta_q16 = ideal_ratio_q16 - Q16_ONE;
        Int32 scaled = (delta_q16 * (Int32)at->correction_amount_q15) >> 15;
        Int32 target_ratio_q16 = Q16_ONE + scaled;

        /* smooth: current += (target-current)*smooth */
        Int32 diff = target_ratio_q16 - at->current_ratio_q16;
        Int32 adj  = (diff * (Int32)at->smooth_factor_q15) >> 15;
        at->current_ratio_q16 += adj;

        /* clamp final 0.5..2.0 */
        if (at->current_ratio_q16 < (Q16_ONE >> 1)) at->current_ratio_q16 = (Q16_ONE >> 1);
        if (at->current_ratio_q16 > (Q16_ONE << 1)) at->current_ratio_q16 = (Q16_ONE << 1);

        at->samples_since_update = 0;
    }

    /* 3) pitch shifting (duas leituras + crossfade) */
    Int32 buffer_size_q16 = ((Int32)AT_BUFFER_SIZE) << 15;
    Int32 half_q16        = buffer_size_q16 >> 1;

    Int32 write_q16 = ((Int32)at->d_write_idx) << 15;

    Int32 read1_q16 = at->d_read_idx_q16;
    Int32 read2_q16 = read1_q16 + half_q16;
    if (read2_q16 >= buffer_size_q16) read2_q16 -= buffer_size_q16;

    /* distância read1->write para o crossfade */
    Int32 dist_q16 = read1_q16 - write_q16;
    if (dist_q16 < 0) dist_q16 += buffer_size_q16;

    Int16 w1 = tri_weight_q15(dist_q16, half_q16);
    Int16 w2 = q15_sub(Q15_ONE, w1);

    Int16 s1 = interp_q15(at->delay_buffer, read1_q16);
    Int16 s2 = interp_q15(at->delay_buffer, read2_q16);

    Int16 y1 = q15_mul(s1, w1);
    Int16 y2 = q15_mul(s2, w2);
    Int16 out = q15_add(y1, y2);

    /* 4) avança ponteiros */
    at->d_write_idx = (at->d_write_idx + 1u) & AT_MASK;
    at->write_pos   = (at->write_pos + 1u) & AT_MASK;

    /* step em Q16.15 = current_ratio_q16 (coerente!) */
    at->d_read_idx_q16 += at->current_ratio_q16;
    if (at->d_read_idx_q16 >= buffer_size_q16) at->d_read_idx_q16 -= buffer_size_q16;

    return out;
}
