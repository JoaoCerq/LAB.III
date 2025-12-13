#include "tistdtypes.h"
#include "c55x.h"      /* Intrinsics da família C55x */
#include "effect.h"

/* =========================================================================
 * Buffers estáticos (delay lines) dos filtros comb e allpass
 * ========================================================================= */

#pragma DATA_ALIGN(combBuf0,2);
#pragma DATA_ALIGN(combBuf1,2);
#pragma DATA_ALIGN(combBuf2,2);
#pragma DATA_ALIGN(combBuf3,2);
#pragma DATA_ALIGN(combBuf4,2);
#pragma DATA_ALIGN(combBuf5,2);
#pragma DATA_ALIGN(combBuf6,2);
#pragma DATA_ALIGN(combBuf7,2);

#pragma DATA_SECTION(combBuf0, ".reverbMem")
static Int16 combBuf0[COMB_BUF_0];
#pragma DATA_SECTION(combBuf1, ".reverbMem")
static Int16 combBuf1[COMB_BUF_1];
#pragma DATA_SECTION(combBuf2, ".reverbMem")
static Int16 combBuf2[COMB_BUF_2];
#pragma DATA_SECTION(combBuf3, ".reverbMem")
static Int16 combBuf3[COMB_BUF_3];
#pragma DATA_SECTION(combBuf4, ".reverbMem")
static Int16 combBuf4[COMB_BUF_4];
#pragma DATA_SECTION(combBuf5, ".reverbMem")
static Int16 combBuf5[COMB_BUF_5];
#pragma DATA_SECTION(combBuf6, ".reverbMem")
static Int16 combBuf6[COMB_BUF_6];
#pragma DATA_SECTION(combBuf7, ".reverbMem")
static Int16 combBuf7[COMB_BUF_7];

#pragma DATA_ALIGN(apBuf0,2);
#pragma DATA_ALIGN(apBuf1,2);
#pragma DATA_ALIGN(apBuf2,2);
#pragma DATA_ALIGN(apBuf3,2);

#pragma DATA_SECTION(apBuf0, ".reverbMem")
static Int16 apBuf0[AP_BUF_0];
#pragma DATA_SECTION(apBuf1, ".reverbMem")
static Int16 apBuf1[AP_BUF_1];
#pragma DATA_SECTION(apBuf2, ".reverbMem")
static Int16 apBuf2[AP_BUF_2];
#pragma DATA_SECTION(apBuf3, ".reverbMem")
static Int16 apBuf3[AP_BUF_3];

/* =========================================================================
 * Helpers de saturação / Q15 e intrinsics
 * ========================================================================= */

/* Saturação manual 32 → Q15 (usada quando acumulamos em 32 bits) */
static inline Int16 sat_q15(Int32 x)
{
    if (x > 32767)  return 32767;
    if (x < -32768) return -32768;
    return (Int16)x;
}

/* Multiplicação Q15 usando intrinsic saturada fracionária */
static inline Int16 q15_mul(Int16 a, Int16 b)
{
    /* _smpy: produto fracionário saturado Q15 * Q15 -> Q15 */
    return _smpy(a, b);
}

/* Soma Q15 saturada */
static inline Int16 q15_add(Int16 a, Int16 b)
{
    /* _sadd: soma saturada de 16 bits */
    return _sadd(a, b);
}

/* Subtração Q15 saturada */
static inline Int16 q15_sub(Int16 a, Int16 b)
{
    /* _ssub: subtração saturada de 16 bits */
    return _ssub(a, b);
}

/* Conversão float [-1,1) -> Q15 (só usada na init, não em tempo real) */
static Int16 float_to_q15(float x)
{
    if (x >= 0.999969f)
        return 32767;
    if (x <= -1.0f)
        return -32768;

    float scaled = x * 32768.0f;
    if (scaled >  32767.0f) scaled =  32767.0f;
    if (scaled < -32768.0f) scaled = -32768.0f;

    return (Int16)(scaled);
}

/* =========================================================================
 * Atualização dos parâmetros internos (gains em Q15)
 * ========================================================================= */

static void update_internal_params(reverb_params_t *p)
{
    Int32 tmp;

    /* gain_dry ≈ dry_level * 2.0  (shift à esquerda de 1) */
    tmp = ((Int32)p->dry_level) << 1;   /* Q15 * 2 -> ainda Q15 com possível saturação */
    p->gain_dry = sat_q15(tmp);

    /* gain_wet1 ≈ wet_level * 3.0  (3 = 2 + 1, aqui deixei igual ao seu código antigo) */
    tmp = (Int32)p->wet_level * 3;
    p->gain_wet1 = sat_q15(tmp);

    /* gain_damp = damping * DAMP_SCALE (Q15 * Q15) */
    p->gain_damp = q15_mul(p->damping, DAMP_SCALE_Q15);

    /* gain_room = room_size * ROOM_SCALE + ROOM_OFFSET (tudo em Q15) */
    {
        Int16 scaled = q15_mul(p->room_size, ROOM_SCALE_Q15);
        p->gain_room = q15_add(scaled, ROOM_OFFSET_Q15);
    }
}

/* =========================================================================
 * Filtro COMB em Q15 usando intrinsics
 * ========================================================================= */

static Int16 process_comb(comb_filter_t *comb,
                          Int16 input_q15,
                          Int16 damp_q15,
                          Int16 feedback_q15)
{
    Int16 output = comb->buffer[comb->idx];   /* Q15 */
    Int16 one_minus_damp;
    Int16 term1, term2;
    Int16 fb_term;

    /* one_minus_damp = 1.0 - damp (em Q15) */
    one_minus_damp = (Int16)(Q15_ONE - damp_q15);

    /* comb->last = output * (1 - damp) + last * damp;
     * tudo em Q15 com intrinsics fracionários
     */
    term1 = q15_mul(output,     one_minus_damp); /* Q15 */
    term2 = q15_mul(comb->last, damp_q15);       /* Q15 */
    comb->last = q15_add(term1, term2);          /* saturado */

    /* temp = input + last * feedback */
    fb_term = q15_mul(comb->last, feedback_q15); /* Q15 */
    comb->buffer[comb->idx] = q15_add(input_q15, fb_term);

    /* índice circular */
    comb->idx++;
    if (comb->idx >= comb->size)
        comb->idx = 0;

    return output;  /* Q15 */
}

/* =========================================================================
 * Filtro ALLPASS em Q15 usando intrinsics
 * y[n] = -x[n] + z[n-D]
 * z[n] = x[n] + 0.5 * z[n-D]
 * ========================================================================= */

static Int16 process_allpass(allpass_filter_t *ap,
                             Int16 input_q15)
{
    Int16 buf_val = ap->buffer[ap->idx];  /* Q15 */
    Int16 half_buf;
    Int16 temp;

    /* temp = input + 0.5 * buf_val  (0.5 => >>1)  */
    half_buf = (Int16)(buf_val >> 1);        /* Q15/2 */
    temp     = q15_add(input_q15, half_buf); /* saturado */
    ap->buffer[ap->idx] = temp;

    /* índice circular */
    ap->idx++;
    if (ap->idx >= ap->size)
        ap->idx = 0;

    /* saída: buf_val - input */
    return q15_sub(buf_val, input_q15);
}

/* =========================================================================
 * Inicialização do reverb (externa)
 * ========================================================================= */

void reverb_init(reverb_params_t *r_params,
                 float room_size,
                 float damping,
                 float wet_level,
                 float dry_level)
{
    Int16 i;

    /* 1) Converte parâmetros float [0,1] pra Q15 */
    r_params->room_size = float_to_q15(room_size);
    r_params->damping   = float_to_q15(damping);
    r_params->wet_level = float_to_q15(wet_level);
    r_params->dry_level = float_to_q15(dry_level);

    /* 2) Zera buffers e estados */
    for (i = 0; i < COMB_BUF_0; i++) combBuf0[i] = 0;
    for (i = 0; i < COMB_BUF_1; i++) combBuf1[i] = 0;
    for (i = 0; i < COMB_BUF_2; i++) combBuf2[i] = 0;
    for (i = 0; i < COMB_BUF_3; i++) combBuf3[i] = 0;
    for (i = 0; i < COMB_BUF_4; i++) combBuf4[i] = 0;
    for (i = 0; i < COMB_BUF_5; i++) combBuf5[i] = 0;
    for (i = 0; i < COMB_BUF_6; i++) combBuf6[i] = 0;
    for (i = 0; i < COMB_BUF_7; i++) combBuf7[i] = 0;

    for (i = 0; i < AP_BUF_0; i++) apBuf0[i] = 0;
    for (i = 0; i < AP_BUF_1; i++) apBuf1[i] = 0;
    for (i = 0; i < AP_BUF_2; i++) apBuf2[i] = 0;
    for (i = 0; i < AP_BUF_3; i++) apBuf3[i] = 0;

    /* 3) Liga buffers às estruturas COMB */
    r_params->combs[0].buffer = combBuf0;
    r_params->combs[0].size   = COMB_BUF_0;
    r_params->combs[0].idx    = 0;
    r_params->combs[0].last   = 0;

    r_params->combs[1].buffer = combBuf1;
    r_params->combs[1].size   = COMB_BUF_1;
    r_params->combs[1].idx    = 0;
    r_params->combs[1].last   = 0;

    r_params->combs[2].buffer = combBuf2;
    r_params->combs[2].size   = COMB_BUF_2;
    r_params->combs[2].idx    = 0;
    r_params->combs[2].last   = 0;

    r_params->combs[3].buffer = combBuf3;
    r_params->combs[3].size   = COMB_BUF_3;
    r_params->combs[3].idx    = 0;
    r_params->combs[3].last   = 0;

    r_params->combs[4].buffer = combBuf4;
    r_params->combs[4].size   = COMB_BUF_4;
    r_params->combs[4].idx    = 0;
    r_params->combs[4].last   = 0;

    r_params->combs[5].buffer = combBuf5;
    r_params->combs[5].size   = COMB_BUF_5;
    r_params->combs[5].idx    = 0;
    r_params->combs[5].last   = 0;

    r_params->combs[6].buffer = combBuf6;
    r_params->combs[6].size   = COMB_BUF_6;
    r_params->combs[6].idx    = 0;
    r_params->combs[6].last   = 0;

    r_params->combs[7].buffer = combBuf7;
    r_params->combs[7].size   = COMB_BUF_7;
    r_params->combs[7].idx    = 0;
    r_params->combs[7].last   = 0;

    /* 4) Liga buffers às estruturas ALLPASS */
    r_params->allpasses[0].buffer = apBuf0;
    r_params->allpasses[0].size   = AP_BUF_0;
    r_params->allpasses[0].idx    = 0;

    r_params->allpasses[1].buffer = apBuf1;
    r_params->allpasses[1].size   = AP_BUF_1;
    r_params->allpasses[1].idx    = 0;

    r_params->allpasses[2].buffer = apBuf2;
    r_params->allpasses[2].size   = AP_BUF_2;
    r_params->allpasses[2].idx    = 0;

    r_params->allpasses[3].buffer = apBuf3;
    r_params->allpasses[3].size   = AP_BUF_3;
    r_params->allpasses[3].idx    = 0;

    /* 5) Calcula ganhos internos já em Q15 */
    update_internal_params(r_params);
}

/* =========================================================================
 * Processamento de 1 amostra em Q15 com intrinsics
 * ========================================================================= */

Int16 reverb_process(reverb_params_t *r_params, Int16 input_sample)
{
    //return input_sample;
    Int16 i;
    Int32 acc32;
    Int16 combOut;
    Int16 apOut;

    /* 1) Entrada com ganho fixo 0.015 em Q15: input_q15 = input * G */
    Int16 input_q15 = q15_mul(input_sample, REV_FIXED_GAIN_Q15);

    /* 2) Filtros Comb em paralelo */
    acc32 = 0;
    for (i = 0; i < NUM_COMBS; i++)
    {
        Int16 c = process_comb(&r_params->combs[i],
                               input_q15,
                               r_params->gain_damp,
                               r_params->gain_room);
        acc32 += (Int32)c;   /* acumula em 32 bits para evitar wrap imediato */
    }
    combOut = sat_q15(acc32);   /* volta para Q15 */

    /* 3) Allpass em série */
    apOut = combOut;
    for (i = 0; i < NUM_ALLPASS; i++)
    {
        apOut = process_allpass(&r_params->allpasses[i], apOut);
    }

    /* 4) Mix wet/dry usando intrinsics */
    {
        Int16 wet_q15 = q15_mul(apOut,       r_params->gain_wet1);
        Int16 dry_q15 = q15_mul(input_sample, r_params->gain_dry);
        Int16 y_q15   = q15_add(wet_q15, dry_q15);

        return y_q15;
    }
}

/* =========================================================================
 * Processamento de bloco
 * ========================================================================= */

void reverb_process_block(reverb_params_t *r_params,
                          Int16 *in,
                          Int16 *out,
                          Uint16 nSamples)
{
    Uint16 n;
    for (n = 0; n < nSamples; n++)
    {
        out[n] = reverb_process(r_params, in[n]);
    }
}
