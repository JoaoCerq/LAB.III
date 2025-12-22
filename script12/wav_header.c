#include "tistdtypes.h"
#include "wav_header.h"

void update_wav_header(Uint8 *wavHeader, float inputSampleRate, float outputSampleRate, Uint32 audioDataSize)
{
    Int32 tempValue;
    
    // 1. Atualiza o tamanho dos dados (Subchunk2Size) nos bytes 40-43
    tempValue = audioDataSize;
    wavHeader[40] = (tempValue >> 0)  & 0xff;
    wavHeader[41] = (tempValue >> 8)  & 0xff;
    wavHeader[42] = (tempValue >> 16) & 0xff;
    wavHeader[43] = (tempValue >> 24) & 0xff;

    // 2. Atualiza o tamanho do "Chunk" principal (ChunkSize) nos bytes 4-7
    // O tamanho do chunk é o tamanho dos dados + 36 bytes de metadados restantes
    tempValue += 36;
    wavHeader[4] = (tempValue >> 0)  & 0xff;
    wavHeader[5] = (tempValue >> 8)  & 0xff;
    wavHeader[6] = (tempValue >> 16) & 0xff;
    wavHeader[7] = (tempValue >> 24) & 0xff;

    // Fator de conversão para reamostragem
    float resamplingRatio = outputSampleRate / inputSampleRate;

    // 3. Lê, ajusta e reescreve a Taxa de Amostragem (Sample Rate) nos bytes 24-27
    // Reconstrói o inteiro de 32 bits a partir dos bytes (Little Endian)
    Int32 currentSampleRate = wavHeader[24] | (wavHeader[25] << 8) | 
                              ((Int32)wavHeader[26] << 16) | ((Int32)wavHeader[27] << 24);
    
    // Aplica a mudança de frequência
    Int32 newSampleRate = (Int32)((float)currentSampleRate * resamplingRatio);

    wavHeader[24] = (newSampleRate >> 0)  & 0xff;
    wavHeader[25] = (newSampleRate >> 8)  & 0xff;
    wavHeader[26] = (newSampleRate >> 16) & 0xff;
    wavHeader[27] = (newSampleRate >> 24) & 0xff;

    // 4. Lê, ajusta e reescreve a Taxa de Bytes (Byte Rate) nos bytes 28-31
    Int32 currentByteRate = wavHeader[28] | (wavHeader[29] << 8) | 
                            ((Int32)wavHeader[30] << 16) | ((Int32)wavHeader[31] << 24);
                            
    Int32 newByteRate = (Int32)((float)currentByteRate * resamplingRatio);

    wavHeader[28] = (newByteRate >> 0)  & 0xff;
    wavHeader[29] = (newByteRate >> 8)  & 0xff;
    wavHeader[30] = (newByteRate >> 16) & 0xff;
    wavHeader[31] = (newByteRate >> 24) & 0xff;

    return;
}

