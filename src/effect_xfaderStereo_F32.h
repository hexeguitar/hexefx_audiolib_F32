/**
 * @file effect_xfaderStereo_F32.h
 * @author Piotr Zapart
 * @brief constant power crossfader for two stereo signals
 * @version 0.1
 * @date 2024-03-21
 * 
 * @copyright Copyright (c) 2024
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

#ifndef _EFFECT_XFADERSTEREO_F32_H_
#define _EFFECT_XFADERSTEREO_F32_H_

#include <arm_math.h> 
#include <AudioStream_F32.h>
#include "basic_components.h"
class AudioEffectXfaderStereo_F32 : public AudioStream_F32
{
public:
	AudioEffectXfaderStereo_F32(void) : AudioStream_F32(4, inputQueueArray_f32){};
	AudioEffectXfaderStereo_F32(const AudioSettings_F32 &settings) : AudioStream_F32(2, inputQueueArray_f32){};
	void mix(float32_t m)
	{
		float32_t gA, gB;
		m = constrain(m, 0.0f, 1.0f);
		mix_pwr(m, &gB, &gA);
		__disable_irq()
		gainA = gA;
		gainB = gB;
		__enable_irq();
	}
	void update()
	{
		audio_block_f32_t *blockLa, *blockRa, *blockLb, *blockRb;
		audio_block_f32_t *blockOutLa, *blockOutRa,*blockOutLb, *blockOutRb, *blockZero;

		blockZero = AudioStream_F32::allocate_f32();
		if(!blockZero) return;
		memset(&blockZero->data[0], 0, blockZero->length*sizeof(float32_t));


		blockLa = AudioStream_F32::receiveReadOnly_f32(0);
		blockRa = AudioStream_F32::receiveReadOnly_f32(1);
		blockLb = AudioStream_F32::receiveReadOnly_f32(2);
		blockRb = AudioStream_F32::receiveReadOnly_f32(3);

		if (!blockLa) blockLa = blockZero;
		if (!blockLb) blockLb = blockZero;
		if (!blockRa) blockRa = blockZero;
		if (!blockRb) blockRb = blockZero;

		// max A, B mited
		if (gainA == 1.0f)
		{
			AudioStream_F32::transmit(blockLa, 0);
			AudioStream_F32::transmit(blockRa, 1);
			if (blockLa != blockZero) AudioStream_F32::release(blockLa);
			if (blockRa != blockZero) AudioStream_F32::release(blockRa);
			if (blockLb != blockZero) AudioStream_F32::release(blockLb);
			if (blockRb != blockZero) AudioStream_F32::release(blockRb);
			AudioStream_F32::release(blockZero);
			return;
		}
		if (gainB == 1.0f)
		{
			AudioStream_F32::transmit(blockLb, 0);
			AudioStream_F32::transmit(blockRb, 1);
			if (blockLa != blockZero) AudioStream_F32::release(blockLa);
			if (blockRa != blockZero) AudioStream_F32::release(blockRa);
			if (blockLb != blockZero) AudioStream_F32::release(blockLb);
			if (blockRb != blockZero) AudioStream_F32::release(blockRb);
			AudioStream_F32::release(blockZero);
			return;
		}
		blockOutLa = AudioStream_F32::allocate_f32();
		blockOutRa = AudioStream_F32::allocate_f32();
		blockOutLb = AudioStream_F32::allocate_f32();
		blockOutRb = AudioStream_F32::allocate_f32();
		if (!blockOutLa || !blockOutRa || !blockOutLa || !blockOutRa)
		{
			if (blockOutLa) AudioStream_F32::release(blockOutLa);
			if (blockOutRa)  AudioStream_F32::release(blockOutRa);
			if (blockOutLb) AudioStream_F32::release(blockOutLb);
			if (blockOutRb)  AudioStream_F32::release(blockOutRb);		
			return;
		}	

		arm_scale_f32(blockLa->data, gainA, blockOutLa->data, blockOutLa->length);
		arm_scale_f32(blockRa->data, gainA, blockOutRa->data, blockOutRa->length);
		arm_scale_f32(blockLb->data, gainB, blockOutLb->data, blockOutLb->length);
		arm_scale_f32(blockRb->data, gainB, blockOutRb->data, blockOutRb->length);
		arm_add_f32(blockOutLa->data, blockOutLb->data, blockOutLa->data, blockOutLa->length);
		arm_add_f32(blockOutRa->data, blockOutRb->data, blockOutRa->data, blockOutRa->length);
		AudioStream_F32::transmit(blockOutLa, 0);
		AudioStream_F32::transmit(blockOutRa, 1);
		if (blockLa != blockZero) AudioStream_F32::release(blockLa);
		if (blockRa != blockZero) AudioStream_F32::release(blockRa);
		if (blockLb != blockZero) AudioStream_F32::release(blockLb);
		if (blockRb != blockZero) AudioStream_F32::release(blockRb);
		AudioStream_F32::release(blockZero);
		AudioStream_F32::release(blockOutLa);
		AudioStream_F32::release(blockOutRa);
		AudioStream_F32::release(blockOutLb);
		AudioStream_F32::release(blockOutRb);

	}
private:
	audio_block_f32_t *inputQueueArray_f32[4];
	float32_t gainA = 1.0f;
	float32_t gainB = 0.0f;
};
#endif // _EFFECT_XFADERSTEREO_F32_H_
