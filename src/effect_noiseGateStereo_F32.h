/*
 * AudioEffectNoiseGate_F32
 *
 * Created: Max Huster, Feb 2021
 * Purpose: This module mutes the Audio completly, when it's below a given threshold.
 *
 * This processes a single stream fo audio data (ie, it is mono)
 *
 * MIT License.  use at your own risk.
 */

#ifndef _AudioEffectNoiseGateStereo_F32_h
#define _AudioEffectNoiseGateStereo_F32_h

#include <arm_math.h> //ARM DSP extensions.  for speed!
#include <AudioStream_F32.h>

class AudioEffectNoiseGateStereo_F32 : public AudioStream_F32
{
public:
	// constructor
	AudioEffectNoiseGateStereo_F32(void) : AudioStream_F32(4, inputQueueArray_f32){};
	AudioEffectNoiseGateStereo_F32(const AudioSettings_F32 &settings) : AudioStream_F32(4, inputQueueArray_f32){};

	void update(void)
	{
		audio_block_f32_t *blockL, *blockR, *blockSideChL, *blockSideChR, *blockSideCh, *blockGain;
		blockL = AudioStream_F32::receiveWritable_f32(0);
		blockR = AudioStream_F32::receiveWritable_f32(1);
		blockSideChL = AudioStream_F32::receiveReadOnly_f32(2); // side chain inputL
		blockSideChR = AudioStream_F32::receiveReadOnly_f32(3); // side chain inputR
		blockSideCh = AudioStream_F32::allocate_f32();			// allocate new block for summed L+R
		blockGain = AudioStream_F32::allocate_f32();			// create a new audio block for the gain


		if (!blockL || !blockR || !blockSideChL || !blockSideChR || !blockSideCh || !blockGain) 
		{
			if (blockSideChL) AudioStream_F32::release(blockSideChL);
			if (blockSideChR) AudioStream_F32::release(blockSideChR);
			if (blockSideCh) AudioStream_F32::release(blockSideCh);
			if (blockGain) AudioStream_F32::release(blockGain);			
			if (blockL) AudioStream_F32::release(blockL);
			if (blockR) AudioStream_F32::release(blockR);
			return;			
		} 
		// sum L + R
		arm_add_f32(blockSideChL->data, blockSideChR->data, blockSideCh->data, blockSideCh->length);
		arm_scale_f32(blockSideCh->data, 0.5f, blockSideCh->data, blockSideCh->length); // divide by 2
		
		// calculate the desired gain
		calcGain(blockSideCh, blockGain);
		// smooth the "blocky" gain block
		calcSmoothedGain(blockGain);

		// multiply it to the input singal
		arm_mult_f32(blockGain->data, blockL->data, blockL->data, blockL->length);
		arm_mult_f32(blockGain->data, blockR->data, blockR->data, blockR->length);
		// release gainBlock
		AudioStream_F32::release(blockGain);
		AudioStream_F32::release(blockSideCh);
		AudioStream_F32::release(blockSideChL);
		AudioStream_F32::release(blockSideChR);

		// transmit the block and be done
		AudioStream_F32::transmit(blockL, 0);
		AudioStream_F32::transmit(blockR, 1);
		AudioStream_F32::release(blockL);
		AudioStream_F32::release(blockR);
	}

	void setThreshold(float dbfs)
	{
		// convert dbFS to linear value to comapre against later
		linearThreshold = pow10f(dbfs / 20.0f);
	}

	void setOpeningTime(float timeInSeconds)
	{
		openingTimeConst = expf(-1.0f / (timeInSeconds * AUDIO_SAMPLE_RATE));
	}

	void setClosingTime(float timeInSeconds)
	{
		closingTimeConst = expf(-1.0f / (timeInSeconds * AUDIO_SAMPLE_RATE));
	}

	void setHoldTime(float timeInSeconds)
	{
		holdTimeNumSamples = timeInSeconds * AUDIO_SAMPLE_RATE;
	}

	bool infoIsOpen()
	{
		return _isOpenDisplay;
	}

private:
	float32_t linearThreshold;
	float32_t prev_gain_dB = 0;
	float32_t openingTimeConst, closingTimeConst;
	float lastGainBlockValue = 0;
	int32_t counter, holdTimeNumSamples = 0;
	audio_block_f32_t *inputQueueArray_f32[4];
	bool falling = false;

	bool _isOpen = false;
	bool _isOpenDisplay = false;
	bool _extSideChain = true;

	void calcGain(audio_block_f32_t *input, audio_block_f32_t *gainBlock)
	{
		_isOpen = false;
		for (int i = 0; i < input->length; i++)
		{
			// take absolute value and compare it to the set threshold
			bool isAboveThres = abs(input->data[i]) > linearThreshold;
			_isOpen |= isAboveThres;
			// if above the threshold set volume to 1 otherwise to 0, we did not account for holdtime
			gainBlock->data[i] = isAboveThres ? 1 : 0;

			// if we are falling and are above the threshold, the level is not falling
			if (falling & isAboveThres)
			{
				falling = false;
			}
			// if we have a falling signal
			if (falling || lastGainBlockValue > gainBlock->data[i])
			{
				// check whether the hold time is not reached
				if (counter < holdTimeNumSamples)
				{
					// signal is (still) falling
					falling = true;
					counter++;
					gainBlock->data[i] = 1.0f;
				}
				// otherwise the signal is already muted due to the line: "gainBlock->data[i] = isAboveThres ? 1 : 0;"
			}
			// note the last gain value, so we can compare it if the signal is falling in the next sample
			lastGainBlockValue = gainBlock->data[i];
		}
		// note the display value
		_isOpenDisplay = _isOpen;
	};

	// this method applies the "opening" and "closing" constants to smooth the
	// target gain level through time.
	void calcSmoothedGain(audio_block_f32_t *gain_block)
	{
		float32_t gain;
		float32_t one_minus_opening_const = 1.0f - openingTimeConst;
		float32_t one_minus_closing_const = 1.0f - closingTimeConst;
		for (int i = 0; i < gain_block->length; i++)
		{
			gain = gain_block->data[i];

			// smooth the gain using the opening or closing constants
			if (gain > prev_gain_dB)
			{ // are we in the opening phase?
				gain_block->data[i] = openingTimeConst * prev_gain_dB + one_minus_opening_const * gain;
			}
			else
			{ // or, we're in the closing phase
				gain_block->data[i] = closingTimeConst * prev_gain_dB + one_minus_closing_const * gain;
			}

			// save value for the next time through this loop
			prev_gain_dB = gain_block->data[i];
		}
		return; // the output here is gain_block
	}
};

#endif
