#include <stdlib.h>
#include <stdio.h>
#include "tistdtypes.h"
#include "wav_header.h"
#include "reverb.h"
#include "autotune.h"

#define NUM_SAMPLES 128
#define SAMPLE_RATE 44100

// Configurações do Reverb
#define ROOM_SIZE 0.30f    // Tamanho da sala (0.0 a 1.0). Maior = eco mais longo;.
#define DAMPING 0.15f     // Abafamento (0.0 a 1.0). 0.0 = som limpo; 1.0 = som abafado.
#define WET_LEVEL 0.275f   // Volume do efeito (0.0 a 1.0). Quanto de "eco" você quer ouvir na mistura.
#define DRY_LEVEL 0.225f    // Volume do som original (0.0 a 1.0). (Geralmente soma-se Wet+Dry <= 1.0).

// Configuração do Autotune
#define TARGET_NOTE 220.65f // Frequência alvo em Hz

Int16  data_buffer[NUM_SAMPLES];
Int8   chunks_buffer[2*NUM_SAMPLES];
Uint8  wave_header[44];

reverb_params_t r_params;
autotune_t      at_params;

void main()
{
    FILE   *file_in, *file_out;
    Int16  i,j;
    Uint32 cnt;
    size_t bytes_read_count, samples_count;
    float  temp_sample, processed_sample;

    printf("Audio Effects Processing: Autotune + Reverb \n");

    file_in = fopen("./data/original.wav", "rb");
    file_out = fopen("./data/06_output.wav", "wb");

    if (file_in == NULL)
    {
        printf("Problem opening the input audio file\n");
        exit(0);
    }

    fread(wave_header, sizeof(Int8), 44, file_in);
    fwrite(wave_header, sizeof(Int8), 44, file_out);

    // Inicializar os efeitos
    reverb_init(ROOM_SIZE, DAMPING, WET_LEVEL, DRY_LEVEL, &r_params);
    autotune_init(&at_params, TARGET_NOTE);

    cnt = 0;

    while ( (bytes_read_count = fread(&chunks_buffer, sizeof(Int8), 2*NUM_SAMPLES, file_in)) > 0 )
    {
        samples_count = bytes_read_count / 2;

        for (j=0, i=0; i < samples_count; i++)
        {
            // Reconstruir amostra de entrada
            data_buffer[i] = (chunks_buffer[j]&0xFF)|(chunks_buffer[j+1]<<8);

            // --- CADEIA DE EFEITOS ---
            
            // Converter para float (-1.0 a 1.0)
            temp_sample = data_buffer[i] / 32767.0f;

            processed_sample = autotune_process(&at_params, temp_sample);

            // Aplicar Reverb
            processed_sample = reverb_process(processed_sample, &r_params);

            // Converter de volta para Int16
            if (processed_sample > 1.0f) processed_sample = 1.0f;
            if (processed_sample < -1.0f) processed_sample = -1.0f;
            data_buffer[i] = (Int16)(processed_sample * 32767.0f);

            // -------------------------

            /* Salvar no buffer de saída */
            chunks_buffer[j++] = data_buffer[i]&0xFF;
            chunks_buffer[j++] = (data_buffer[i]>>8)&0xFF;
        }

        fwrite(&chunks_buffer, sizeof(Int8), bytes_read_count, file_out);
        cnt += samples_count;
    }
    
    update_wav_header(wave_header, SAMPLE_RATE, SAMPLE_RATE, cnt<<1);
    rewind(file_out);
    fwrite(wave_header, sizeof(Int8), 44, file_out);

    fclose(file_out);
    fclose(file_in);
    printf("Processing completed. %ld samples processed\n", cnt);
}
