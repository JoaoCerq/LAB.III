#ifndef __WAV_HEADER_H__
#define __WAV_HEADER_H__

void update_wav_header(unsigned char *wavHeader, float inputSampleRate, float outputSampleRate, unsigned long audioDataSize);

#endif
