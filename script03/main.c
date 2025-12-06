#include <stdlib.h>
#include <stdio.h>
#include "tistdtypes.h"
#include "wav_header.h"
#include "reverb.h"

#define NUM_SAMPLES 512

// Configurações do Reverb
#define ROOM_SIZE 0.9f    // Tamanho da sala (0.0 a 1.0). Maior = eco mais longo;.
#define DAMPING 0.1f      // Abafamento (0.0 a 1.0). 0.0 = som limpo; 1.0 = som abafado.
#define WET_LEVEL 0.25f    // Volume do efeito (0.0 a 1.0). Quanto de "eco" você quer ouvir na mistura.
#define DRY_LEVEL 0.75f    // Volume do som original (0.0 a 1.0). (Geralmente soma-se Wet+Dry <= 1.0).

#define GAIN 0.3f

Int16  data_buffer[NUM_SAMPLES]; // buffer de samples processados
Int8   chunks_buffer[2*NUM_SAMPLES]; // buffer de bytes lidos do arquivo
Uint8  wave_header[44];
reverb_params_t r_params;

void main()
{
    FILE   *file_in,*file_out;
    Int16  i,j;
    Uint32 cnt;
    size_t bytes_read_count, samples_count;
    float  processed_sample;

    printf("Reverb Effect Processing\n");

    // O programa roda dentro da pasta binaries, tem que voltar uma pasta para achar os arquivos
    file_in = fopen("../data/original.wav", "rb");
    file_out = fopen("../data/03_output.wav", "wb");

    if (file_in == NULL)
    {
        printf("Problem opening the input audio file\n");
        exit(0);
    }

    fread(wave_header, sizeof(Int8), 44, file_in);      // Advance input to speech data
    fwrite(wave_header, sizeof(Int8), 44, file_out);    // Write header for output file

    reverb_init(&r_params, ROOM_SIZE, DAMPING, WET_LEVEL, DRY_LEVEL);

    cnt = 0;

    while ( (bytes_read_count = fread(&chunks_buffer, sizeof(Int8), 2*NUM_SAMPLES, file_in)) > 0 )
    {
        samples_count = bytes_read_count / 2; // Cada sample tem 2 bytes (Int16)

        for (j=0, i=0; i < samples_count; i++)
        {
            // Reconstruir amostra de entrada
            data_buffer[i] = (chunks_buffer[j]&0xFF)|(chunks_buffer[j+1]<<8);

            // Converter para float (-1.0 a 1.0)
            processed_sample = data_buffer[i] / 32768.0f;

            // Aplicar ganho
            processed_sample = processed_sample * GAIN;

            // Aplicar Reverb
            processed_sample = reverb_process(&r_params, processed_sample);

            // Converter de volta para Int16
            if (processed_sample > 1.0f) processed_sample = 1.0f;
            if (processed_sample < -1.0f) processed_sample = -1.0f;
            data_buffer[i] = (Int16)(processed_sample * 32767.0f);

            /* Salvar no buffer de saída */
            chunks_buffer[j++] = data_buffer[i]&0xFF;
            chunks_buffer[j++] = (data_buffer[i]>>8)&0xFF;
        }

        fwrite(&chunks_buffer, sizeof(Int8), bytes_read_count, file_out);

        cnt += samples_count;

        printf("%ld data samples processed\n", cnt);
    }

    update_wav_header(wave_header, 8000, 8000, cnt<<1);
    rewind(file_out);
    fwrite(wave_header, sizeof(Int8), 44, file_out);

    fclose(file_out);
    fclose(file_in);
    printf("Processing completed. %ld samples processed\n", cnt);
}
