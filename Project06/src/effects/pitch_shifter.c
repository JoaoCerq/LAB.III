#include "pitch_shifter.h"
#include "c55x.h"   /* _smpy, _sadd, _ssub */

#define Q15_ONE        ((Int16)32767)
#define PHASE_MOD      ((Int32)32768)     /* módulo do phasor */
#define HALF_PHASE     ((Int16)16384)     /* 0.5 em Q15 */

#define ONE_Q14        ((Int32)16384)     /* 1.0 em Q2.14 */

/* ---------- intrinsics helpers ---------- */
static inline Int16 q15_mul(Int16 a, Int16 b) { return _smpy(a, b); }
static inline Int16 q15_add(Int16 a, Int16 b) { return _sadd(a, b); }
static inline Int16 q15_sub(Int16 a, Int16 b) { return _ssub(a, b); }

static inline Int16 sat16(Int32 x)
{
    if (x > 32767)  return 32767;
    if (x < -32768) return -32768;
    return (Int16)x;
}

/* float -> Q2.14 (só na init) */
static Int32 float_to_q14(float x)
{
    if (x < 0.25f) x = 0.25f;
    if (x > 3.00f) x = 3.00f;

    /* 2.0 -> 32768, precisa Int32 */
    return (Int32)(x * 16384.0f);
}

/* janela triangular: 0..1 em Q15, pico em 0.5 */
static inline Int16 tri_gain_q15(Int16 phase_q15)
{
    Int32 g;
    if (phase_q15 < HALF_PHASE) {
        g = ((Int32)phase_q15) << 1;                 /* 2*phase */
    } else {
        g = ((PHASE_MOD - (Int32)phase_q15) << 1);   /* 2*(1-phase) */
    }
    return sat16(g);
}

/* Interpolação linear em Q15:
   y = v0*(1-frac) + v1*frac  (frac em Q15) */
static inline Int16 interp_q15(const Int16 *buf, Uint16 idx_int, Int16 frac_q15)
{
    Uint16 idx0 = idx_int & PS_MASK;
    Uint16 idx1 = (idx_int + 1u) & PS_MASK;

    Int16 v0 = buf[idx0];
    Int16 v1 = buf[idx1];

    Int16 one_minus = (Int16)(Q15_ONE - frac_q15);

    Int16 p0 = q15_mul(v0, one_minus);
    Int16 p1 = q15_mul(v1, frac_q15);

    return q15_add(p0, p1);
}

/* recalcula step a partir do pitch (Q2.14) */
static void update_phase_step(pitch_shifter_t *ps)
{
    /* phase_step = (1 - pitch) / window_size
       delta_q14 = (1.0 - pitch_q14) em Q2.14
       converte pra Q15: delta_q15 = delta_q14 << 1 */
    Int32 delta_q14 = (ONE_Q14 - ps->pitch_q14);  /* signed Int32 */
    Int32 num_q15   = (delta_q14 << 1);           /* vira Q15 */

    ps->phase_step_q15 = (Int16)(num_q15 / (Int32)ps->window_size);
}

void pitch_shifter_set_pitch_q14(pitch_shifter_t *ps, Int32 pitch_q14)
{
    ps->pitch_q14 = pitch_q14;
    update_phase_step(ps);
}

void pitch_shifter_init_q14(pitch_shifter_t *ps, Int32 pitch_q14)
{
    Uint16 i;

    for (i = 0; i < PS_BUFFER_SIZE; i++)
        ps->buffer[i] = 0;

    ps->write_index = 0;
    ps->phasor_q15  = 0;
    ps->window_size = (Uint16)(PS_BUFFER_SIZE - 2u);

    ps->pitch_q14 = pitch_q14;
    update_phase_step(ps);
}

void pitch_shifter_init(pitch_shifter_t *ps, float pitch_factor)
{
    pitch_shifter_init_q14(ps, float_to_q14(pitch_factor));
}

Int16 pitch_shifter_process(pitch_shifter_t *ps, Int16 input_q15)
{
    /* escreve amostra nova */
    ps->buffer[ps->write_index] = input_q15;

    /* pitch ~ 1 => step ~ 0: melhor bypass */
    if (ps->phase_step_q15 == 0) {
        ps->write_index = (ps->write_index + 1u) & PS_MASK;
        return input_q15;
    }

    /* atualiza phasor */
    ps->phasor_q15 += (Int32)ps->phase_step_q15;
    if (ps->phasor_q15 >= PHASE_MOD) ps->phasor_q15 -= PHASE_MOD;
    if (ps->phasor_q15 < 0)         ps->phasor_q15 += PHASE_MOD;

    Int16 phase_a = (Int16)ps->phasor_q15;

    Int32 pb = ps->phasor_q15 + (Int32)HALF_PHASE;
    if (pb >= PHASE_MOD) pb -= PHASE_MOD;
    Int16 phase_b = (Int16)pb;

    /* write em “Q15 amostras” */
    Int32 write_pos_q15 = ((Int32)ps->write_index) << 15;

    /* Tap A */
    Int32 delay_a_q15 = (Int32)((Uint32)phase_a * (Uint32)ps->window_size);
    Int32 read_a_q15  = write_pos_q15 - delay_a_q15;
    if (read_a_q15 < 0) read_a_q15 += ((Int32)PS_BUFFER_SIZE << 15);

    Uint16 idx_a  = (Uint16)((read_a_q15 >> 15) & PS_MASK);
    Int16  frac_a = (Int16)(read_a_q15 & 0x7FFF);

    Int16 sample_a = interp_q15(ps->buffer, idx_a, frac_a);
    Int16 gain_a   = tri_gain_q15(phase_a);

    /* Tap B */
    Int32 delay_b_q15 = (Int32)((Uint32)phase_b * (Uint32)ps->window_size);
    Int32 read_b_q15  = write_pos_q15 - delay_b_q15;
    if (read_b_q15 < 0) read_b_q15 += ((Int32)PS_BUFFER_SIZE << 15);

    Uint16 idx_b  = (Uint16)((read_b_q15 >> 15) & PS_MASK);
    Int16  frac_b = (Int16)(read_b_q15 & 0x7FFF);

    Int16 sample_b = interp_q15(ps->buffer, idx_b, frac_b);
    Int16 gain_b   = tri_gain_q15(phase_b);

    /* OLA */
    Int16 out_a = q15_mul(sample_a, gain_a);
    Int16 out_b = q15_mul(sample_b, gain_b);
    Int16 output = q15_add(out_a, out_b);

    /* avança write */
    ps->write_index = (ps->write_index + 1u) & PS_MASK;

    return output;
}
