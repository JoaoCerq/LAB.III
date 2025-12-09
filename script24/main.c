#include <stdlib.h>
#include <stdio.h>
#include "tistdtypes.h"
#include "wav_header.h"

#define NUM_SAMPLES 512
#define SAMPLE_RATE 44100
#define GAIN 0.2f

Int16  data_buffer[NUM_SAMPLES]; // buffer de samples processados
Int8   chunks_buffer[2*NUM_SAMPLES]; // buffer de bytes lidos do arquivo
Uint8  wave_header[44];

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
    file_out = fopen("../data/24_output.wav", "wb");

    if (file_in == NULL)
    {
        printf("Problem opening the input audio file\n");
        exit(0);
    }

    fread(wave_header, sizeof(Int8), 44, file_in);      // Advance input to speech data
    fwrite(wave_header, sizeof(Int8), 44, file_out);    // Write header for output file

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

            // Converter de volta para Int16
            if (processed_sample > 1.0f) processed_sample = 1.0f;
            if (processed_sample < -1.0f) processed_sample = -1.0f;
            data_buffer[i] = (Int16)(processed_sample * 32767.0f);

            /* Salvar no buffer de sa�da */
            chunks_buffer[j++] = data_buffer[i]&0xFF;
            chunks_buffer[j++] = (data_buffer[i]>>8)&0xFF;
        }

        fwrite(&chunks_buffer, sizeof(Int8), bytes_read_count, file_out);

        cnt += samples_count;

        printf("%ld data samples processed\n", cnt);
    }

    update_wav_header(wave_header, SAMPLE_RATE, SAMPLE_RATE, cnt<<1);
    rewind(file_out);
    fwrite(wave_header, sizeof(Int8), 44, file_out);

    fclose(file_out);
    fclose(file_in);
    printf("Processing completed. %ld samples processed\n", cnt);
}
