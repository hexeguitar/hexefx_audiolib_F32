/*
 * AudioEffectGain_F32
 *
 * Created: Chip Audette, November 2016
 * Purpose; Apply digital gain to the audio data.  Assumes floating-point data.
 *
 * This processes a single stream fo audio data (ie, it is mono)
 *
 * MIT License.  use at your own risk.
 */

#ifndef _AudioEffectGainStereo_F32_h
#define _AudioEffectGainStereo_F32_h

#include <arm_math.h> //ARM DSP extensions.  for speed!
#include <AudioStream_F32.h>

class AudioEffectGainStereo_F32 : public AudioStream_F32
{
public:
	// constructor
	AudioEffectGainStereo_F32(void) : AudioStream_F32(2, inputQueueArray_f32){};
	AudioEffectGainStereo_F32(const AudioSettings_F32 &settings) : AudioStream_F32(2, inputQueueArray_f32){};

	void update(void)
	{
		audio_block_f32_t *blockL, *blockR;
		blockL = AudioStream_F32::receiveWritable_f32(0);
		blockR = AudioStream_F32::receiveWritable_f32(1);
		if (!blockL || !blockR)
		{
			if (blockL)
				AudioStream_F32::release(blockL);
			if (blockR)
				AudioStream_F32::release(blockR);
			return;
		}
		arm_scale_f32(blockL->data, gain, blockL->data, blockL->length); // use ARM DSP for speed!
		arm_scale_f32(blockR->data, gain, blockR->data, blockR->length);
		AudioStream_F32::transmit(blockL, 0);
		AudioStream_F32::transmit(blockR, 1);
		AudioStream_F32::release(blockL);
		AudioStream_F32::release(blockR);
	}

	// methods to set parameters of this module
	void setGain(float g) { gain = g; }
	void setGain_dB(float gain_dB)
	{
		float gain = pow(10.0, gain_dB / 20.0);
		setGain(gain);
	}

	// methods to return information about this module
	float getGain(void) { return gain; }
	float getGain_dB(void) { return 20.0 * log10(gain); }

private:
	audio_block_f32_t *inputQueueArray_f32[2]; // memory pointer for the input to this module
	float gain = 1.0f;						   // default value
};

#endif