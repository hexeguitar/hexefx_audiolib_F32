#ifndef _BASIC_TEMPBUFFER_H_
#define _BASIC_TEMPBUFFER_H_

#include <Arduino.h>
#include "AudioStream_F32.h"

class AudioBasicTempBuffer_F32 : public AudioStream_F32
{
public:
	AudioBasicTempBuffer_F32() : AudioStream_F32(1, inputQueueArray_f32)
	{
		blockSize = AUDIO_BLOCK_SAMPLES;
		memset(&data[0], 0, AUDIO_BLOCK_SAMPLES * sizeof(float32_t));
		dataPtr = &data[0];
	};
	AudioBasicTempBuffer_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32)
	{
		blockSize = settings.audio_block_samples;
		memset(&data[0], 0, AUDIO_BLOCK_SAMPLES * sizeof(float32_t));
		dataPtr = &data[0];
	};
	~AudioBasicTempBuffer_F32(){};
	void update(void)
	{
		audio_block_f32_t* block = AudioStream_F32::receiveReadOnly_f32();
		if (!block) return;
		memcpy(&data[0], block->data, blockSize * sizeof(float32_t));
		AudioStream_F32::release(block);
	}
	float32_t* dataPtr;
private:
	audio_block_f32_t *inputQueueArray_f32[1];
	uint16_t blockSize = AUDIO_BLOCK_SAMPLES;
	float32_t data[AUDIO_BLOCK_SAMPLES];
};

#endif // _BASIC_TEMPBUFFER_H_
