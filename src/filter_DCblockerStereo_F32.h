/**
 * @file filter_DCblockerStereo_F32.h
 * @author Piotr Zapart
 * @brief simple IIR based stereo DB blocking filter
 * @version 0.1
 * @date 2024-06-01
 * 
 * @copyright Copyright (c) 2024
 * This program is free software: you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. 
 * If not, see <https://www.gnu.org/licenses/>."
 */

#ifndef _FILTER_DCBLOCKERSTEREO_F32_H_
#define _FILTER_DCBLOCKERSTEREO_F32_H_

#include <AudioStream_F32.h>
#include "basic_DSPutils.h"
#include <arm_math.h>

//   y = x - xm1 + 0.995 * ym1;
//   xm1 = x;
//   ym1 = y;

class AudioFilterDCblockerStereo_F32 : public AudioStream_F32
{
public:
	AudioFilterDCblockerStereo_F32(void) : AudioStream_F32(2, inputQueueArray)
	{
		fs_Hz = AUDIO_SAMPLE_RATE_EXACT;
		blockSize = AUDIO_BLOCK_SAMPLES;
	}

	AudioFilterDCblockerStereo_F32(const AudioSettings_F32 &settings) : AudioStream_F32(2, inputQueueArray)
	{
		fs_Hz = settings.sample_rate_Hz;
		blockSize = settings.audio_block_samples;	
	}

	void update()
	{
		audio_block_f32_t *blockL, *blockR;
		uint16_t i;
		float32_t tmpf32;

		if (bp) // handle bypass
		{
			blockL = AudioStream_F32::receiveReadOnly_f32(0);
			blockR = AudioStream_F32::receiveReadOnly_f32(1);
			if (!blockL || !blockR)
			{
				if (blockL)
					AudioStream_F32::release(blockL);
				if (blockR)
					AudioStream_F32::release(blockR);
				return;
			}
			AudioStream_F32::transmit(blockL, 0);
			AudioStream_F32::transmit(blockR, 1);
			AudioStream_F32::release(blockL);
			AudioStream_F32::release(blockR);
			return;
		}
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

		float32_t _xRegL = xRegL;
		float32_t _xRegR = xRegR;
		float32_t _yRegL = yRegL;
		float32_t _yRegR = yRegR;

		for (i=0; i<blockSize; i=i+4)
		{
			tmpf32 = blockL->data[i] - _xRegL + k * _yRegL;
			_yRegL = tmpf32;
			_xRegL = blockL->data[i];
			blockL->data[i] = _yRegL;

			tmpf32 = blockL->data[i+1] - _xRegL + k * _yRegL;
			_yRegL = tmpf32;
			_xRegL = blockL->data[i+1];
			blockL->data[i+1] = _yRegL;

			tmpf32 = blockL->data[i+2] - _xRegL + k * _yRegL;
			_yRegL = tmpf32;
			_xRegL = blockL->data[i+2];
			blockL->data[i+2] = _yRegL;

			tmpf32 = blockL->data[i+3] - _xRegL + k * _yRegL;
			_yRegL = tmpf32;
			_xRegL = blockL->data[i+3];
			blockL->data[i+3] = _yRegL;

			tmpf32 = blockR->data[i] - _xRegR + k * _yRegR;
			_yRegR = tmpf32;
			_xRegR = blockR->data[i];
			blockR->data[i] = _yRegR;

			tmpf32 = blockR->data[i+1] - _xRegR + k * _yRegR;
			_yRegR = tmpf32;
			_xRegR = blockR->data[i+1];
			blockR->data[i+1] = _yRegR;

			tmpf32 = blockR->data[i+2] - _xRegR + k * _yRegR;
			_yRegR = tmpf32;
			_xRegR = blockR->data[i+2];
			blockR->data[i+2] = _yRegR;

			tmpf32 = blockR->data[i+3] - _xRegR + k * _yRegR;
			_yRegR = tmpf32;
			_xRegR = blockR->data[i+3];
			blockR->data[i+3] = _yRegR;
		}
		xRegL = _xRegL;
		yRegL = _yRegL;
		xRegR = _xRegR;
		yRegR = _yRegR;

		AudioStream_F32::transmit(blockL, 0);
		AudioStream_F32::transmit(blockR, 1);
		AudioStream_F32::release(blockL);
		AudioStream_F32::release(blockR);

	}

private:
	audio_block_f32_t *inputQueueArray[2];
	float32_t fs_Hz;
	uint16_t blockSize;
	float k = 0.995f;
	bool bp = false; // bypass flag
	float32_t xRegL = 0.0f;
	float32_t xRegR = 0.0f;
	float32_t yRegL = 0.0f;
	float32_t yRegR = 0.0f;
};


#endif // _FILTER_DCBLOCKERSTEREO_F32_H_
