/**
 * @file switch_selectorStereo_F32.h
 * @author Piotr Zapart
 * @brief Signal selector for routing mono to stereo 
 * @version 0.1
 * @date 2024-03-21
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

#ifndef _SWITCH_SELECTORSTEREO_F32_H_
#define _SWITCH_SELECTORSTEREO_F32_H_
#include <AudioStream_F32.h>
#include <arm_math.h>

class AudioSwitchSelectorStereo : public AudioStream_F32
{
public:
	AudioSwitchSelectorStereo(void) : AudioStream_F32(2, inputQueueArray){};
	typedef enum
	{
		SIGNAL_SELECT_LR,	 	// default stereo operation
		SIGNAL_SELECT_L,		// left input as mono input 
		SIGNAL_SELECT_R			// right input as mono input
	}selector_mode_t;

	selector_mode_t setMode(selector_mode_t m)
	{
		if (m <= 2)
		{
			__disable_irq();
			mode = m;
			__enable_irq();
		}
		return mode;
	}
	selector_mode_t getMode() {return mode;};
	void update()
	{
		audio_block_f32_t *blockL, *blockR, *outL, *outR;
		blockL = AudioStream_F32::receiveWritable_f32(0);
		blockR = AudioStream_F32::receiveWritable_f32(1);


		if (!blockL || !blockR)
		{
			if (blockL) AudioStream_F32::release(blockL);
			if (blockR) AudioStream_F32::release(blockR);



			return;
		}
		switch(mode)
		{
			case SIGNAL_SELECT_LR:
				outL = blockL;
				outR = blockR;
				break;
			case SIGNAL_SELECT_L:
				outL = blockL;
				outR = blockL;
				break;
			case SIGNAL_SELECT_R:
				outL = blockR;
				outR = blockR;
				break;
			default:
				outL = blockL;
				outR = blockR;
				break;			
		}
		AudioStream_F32::transmit(outL, 0);
		AudioStream_F32::transmit(outR, 1);
		AudioStream_F32::release(blockL);
		AudioStream_F32::release(blockR);		
	}

private:
	audio_block_f32_t *inputQueueArray[2];
	selector_mode_t mode = SIGNAL_SELECT_LR;
};

#endif // _SWITCH_SELECTORSTEREO_F32_H_
