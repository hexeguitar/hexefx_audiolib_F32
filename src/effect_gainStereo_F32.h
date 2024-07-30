/**
 * @file effect_gainStereo_F32.h
 * @author Piotr Zapart
 * @brief Stereo volume + pan control
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 www.hexefx.com
 * 
 * This program is free software: you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. 
 * If not, see <https://www.gnu.org/licenses/>."
 */

#ifndef _AudioEffectGainStereo_F32_h
#define _AudioEffectGainStereo_F32_h

#include <arm_math.h> //ARM DSP extensions.  for speed!
#include <AudioStream_F32.h>
#include <basic_components.h>

class AudioEffectGainStereo_F32 : public AudioStream_F32
{
public:
	AudioEffectGainStereo_F32(void) : AudioStream_F32(2, inputQueueArray_f32) { setPan(0.0f);};
	AudioEffectGainStereo_F32(const AudioSettings_F32 &settings) : AudioStream_F32(2, inputQueueArray_f32){setPan(0.0f);};
	void update(void)
	{
		audio_block_f32_t *blockL, *blockR;
		blockL = AudioStream_F32::receiveWritable_f32(0);
		blockR = AudioStream_F32::receiveWritable_f32(1);
		float gL, gR;
		if (!blockL || !blockR)
		{
			if (blockL)
				AudioStream_F32::release(blockL);
			if (blockR)
				AudioStream_F32::release(blockR);
			return;
		}
		gainL += (gainLset - gainL) * 0.25f;
		gainR += (gainRset - gainR) * 0.25f;
		if (phase_flip)  { gL = -gainL; gR = -gainR; }
		else 			 { gL = gainL;	gR = gainR; }
		arm_scale_f32(blockL->data, gL, blockL->data, blockL->length); // use ARM DSP for speed!
		arm_scale_f32(blockR->data, gR, blockR->data, blockR->length);
		AudioStream_F32::transmit(blockL, 0);
		AudioStream_F32::transmit(blockR, 1);
		AudioStream_F32::release(blockL);
		AudioStream_F32::release(blockR);
	}
	void setGain(float g) 
	{ 
		float32_t gL, gR;
		gain = g; 
		gL = panL * gain;
		gR = panR * gain;
		__disable_irq();
		gainLset = gL;
		gainRset = gR;
		__enable_irq();

	}
	void setGain_dB(float gain_dB)
	{
		float gain = powf(10.0f, gain_dB / 20.0f);
		setGain(gain);
	}
	float getGain(void) { return gain; }
	float getGain_dB(void) { return 20.0 * log10(gain); }

	void setPan(float32_t p)
	{
		float32_t gL, gR;
		pan = constrain(p, 0.0f, 1.0f);
		mix_pwr(pan, &panR, &panL);

		gL = panL * gain;
		gR = panR * gain;	
		__disable_irq();
		gainLset = gL;
		gainRset = gR;
		__enable_irq();
	}
	float32_t getPan() { return pan;}
	void phase_inv(bool inv)
	{
		__disable_irq();
		phase_flip = inv;
		__enable_irq();	
	}

private:
	audio_block_f32_t *inputQueueArray_f32[2]; // memory pointer for the input to this module
	float32_t gain = 1.0f;						   // default value
	float32_t gainL, gainR, gainLset, gainRset;
	float32_t pan, panL, panR;
	bool phase_flip = false;
};

#endif