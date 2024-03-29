/*  Stereo Ping Pong delay for Teensy 4
 *
 *  Author: Piotr Zapart
 *          www.hexefx.com
 *
 * Copyright (c) 2024 by Piotr Zapart
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "effect_delaystereo_F32.h"

#define TREBLE_LOSS_FREQ    (0.20f)
#define BASS_LOSS_FREQ      (0.05f)
#define BASS_FREQ      		(0.15f)

extern uint8_t external_psram_size;

AudioEffectDelayStereo_F32::AudioEffectDelayStereo_F32(uint32_t dly_range_ms, bool use_psram) : AudioStream_F32(2, inputQueueArray)
{
	begin(dly_range_ms, use_psram);
}

void AudioEffectDelayStereo_F32::begin(uint32_t dly_range_ms, bool use_psram)
{
	// failsafe if psram is required but not found
	// limit the delay time to 500ms (88200 bytes at 44.1kHz)
	if (psram_mode && external_psram_size == 0)
	{
		psram_mode = false;
		if (dly_range_ms > 500) dly_range_ms = 500;
	}	
	bool memOk = true;
	dly_length = ((float32_t)(dly_range_ms+500)/1000.0f) * AUDIO_SAMPLE_RATE_EXACT;	
	if (!dly0a.init(dly_length, use_psram)) memOk = false;
	if (!dly0b.init(dly_length, use_psram)) memOk = false;
	if (!dly1a.init(dly_length, use_psram)) memOk = false;
	if (!dly1b.init(dly_length, use_psram)) memOk = false;
	flt0L.init(BASS_LOSS_FREQ, &bassCut_k, TREBLE_LOSS_FREQ, &trebleCut_k);
	flt1L.init(BASS_LOSS_FREQ, &bass_k, TREBLE_LOSS_FREQ, &treble_k);
	flt0R.init(BASS_LOSS_FREQ, &bassCut_k, TREBLE_LOSS_FREQ, &trebleCut_k);
	flt1R.init(BASS_LOSS_FREQ, &bass_k, TREBLE_LOSS_FREQ, &treble_k);
	mix(0.5f);
	feedback(0.5f);
	cleanup_done = true;
	if (memOk) initialized = true;
}

void AudioEffectDelayStereo_F32::update()
{
	if (!initialized) return;
	if (!memsetup_done)
	{
		memsetup_done = memCleanup();
		return;		
	}

	audio_block_f32_t *blockL, *blockR;
	int i;
	float32_t acc1, acc2, outL, outR, mod_fr[4];
	uint32_t mod_int;
	static float32_t dly_time_flt = 0.0f;

    if (bp)
    {
		// mem cleanup not required in TRAILS mode
		if (!cleanup_done && bp_mode != BYPASS_MODE_TRAILS)
		{
			cleanup_done = memCleanup();
			tap_active = false;	// reset tap tempo
			tap_counter = 0;
		}
		if (infinite) freeze(false);
		switch(bp_mode)
		{
			case BYPASS_MODE_PASS:
				blockL = AudioStream_F32::receiveReadOnly_f32(0);
				blockR = AudioStream_F32::receiveReadOnly_f32(1);
				if (!blockL || !blockR) 
				{
					if (blockL) AudioStream_F32::release(blockL);
					if (blockR) AudioStream_F32::release(blockR);
					return;
				}
				AudioStream_F32::transmit(blockL, 0);	
				AudioStream_F32::transmit(blockR, 1);
				AudioStream_F32::release(blockL);
				AudioStream_F32::release(blockR);
				return;
				break;
			case BYPASS_MODE_OFF:
				blockL = AudioStream_F32::allocate_f32();
				if (!blockL) return;
				memset(&blockL->data[0], 0, blockL->length*sizeof(float32_t));
				AudioStream_F32::transmit(blockL, 0);	
				AudioStream_F32::transmit(blockL, 1);
				AudioStream_F32::release(blockL);	
				return;
				break;
			case BYPASS_MODE_TRAILS:
				inputGainSet = 0.0f;
				tap_active = false;	// reset tap tempo
				tap_counter = 0;
				break;
			default:
				break;
		}
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
	cleanup_done = false;

	for (i=0; i < blockL->length; i++) 
    {  
		inputGain += (inputGainSet - inputGain) * 0.25f;
		// tap tempo
		if (tap_active)
		{
			if (++tap_counter > tap_counter_max)
			{
				tap_active = false;
				tap_counter = 0;
			}
		}

		if (dly_time < dly_time_set)
		{
			dly_time += dly_time_step;
			if (dly_time > dly_time_set) dly_time = dly_time_set;
		}
		if (dly_time > dly_time_set)
		{
			dly_time -= dly_time_step;
			if (dly_time < dly_time_set) dly_time = dly_time_set;
		}
		// lowpass the delay time
		acc1 = dly_time - dly_time_flt;
		dly_time_flt += acc1 * 0.1f;
		dly_time = dly_time_flt;

		lfo.update();

		lfo.get(BASIC_LFO_PHASE_0, &mod_int, &mod_fr[0]);
		mod_fr[0] = (float32_t)mod_int + mod_fr[0];
		acc2 = (float32_t)dly_length - 1.0f - (dly_time + mod_fr[0]);
		if (acc2 < 0.0f) mod_fr[0] += acc2;

		lfo.get(BASIC_LFO_PHASE_60, &mod_int, &mod_fr[1]);
		mod_fr[1] = (float32_t)mod_int + mod_fr[1];
		acc2 = (float32_t)dly_length - 1.0f - (dly_time + mod_fr[1]);
		if (acc2 < 0.0f) mod_fr[1] += acc2;

		lfo.get(BASIC_LFO_PHASE_120, &mod_int, &mod_fr[2]);
		mod_fr[2] = (float32_t)mod_int + mod_fr[2];
		acc2 = (float32_t)dly_length - 1.0f - (dly_time + mod_fr[2]);
		if (acc2 < 0.0f) mod_fr[2] += acc2;	

		lfo.get(BASIC_LFO_PHASE_180, &mod_int, &mod_fr[3]);
		mod_fr[3] = (float32_t)mod_int + mod_fr[3];	
		acc2 = (float32_t)dly_length - 1.0f - (dly_time + mod_fr[3]);
		if (acc2 < 0.0f) mod_fr[3] += acc2;		

		acc1 = dly0b.getTapHermite(dly_time+mod_fr[0]);
		outR = acc1 * 0.6f;
		acc1 = flt0R.process(acc1) * feedb;
		acc1 += blockR->data[i] * inputGain;
		acc1 = flt1R.process(acc1);
		acc2 = dly0a.getTapHermite(dly_time+mod_fr[1]);
		dly0b.write_toOffset(acc2, 0);
		outL = acc2 * 0.6f;
		dly0a.write_toOffset(acc1, 0);

		acc1 = dly1b.getTapHermite(dly_time+mod_fr[2]);
		outR += acc1 * 0.6f;
		acc1 = flt0L.process(acc1) * feedb;
		acc1 += blockL->data[i] * inputGain;
		acc1 = flt1L.process(acc1);
		acc2 = dly1a.getTapHermite(dly_time+mod_fr[3]);
		dly1b.write_toOffset(acc2, 0);
		outL += acc2 * 0.6f;
		dly1a.write_toOffset(acc1, 0);

		dly0a.updateIndex();
		dly0b.updateIndex();
		dly1a.updateIndex();
		dly1b.updateIndex();
		blockL->data[i] = outL * wet_gain + blockL->data[i] * dry_gain;
		blockR->data[i] = outR * wet_gain + blockR->data[i] * dry_gain;

	}
    AudioStream_F32::transmit(blockL, 0);
	AudioStream_F32::transmit(blockR, 1);
	AudioStream_F32::release(blockL);
	AudioStream_F32::release(blockR);
}
void AudioEffectDelayStereo_F32::freeze(bool state)
{
	if (infinite == state) return;
	infinite = state;
	if (state)
	{
		feedb_tmp = feedb;      // store the settings
		inputGain_tmp = inputGainSet;
		bassCut_k_tmp = bassCut_k;
		trebleCut_k_tmp = trebleCut_k;
		__disable_irq();
		feedb = 1.0f; // infinite echo                                    
		inputGainSet = freeze_ingain;
		__enable_irq();
	}
	else
	{
		__disable_irq();
		feedb = feedb_tmp;
		inputGainSet = inputGain_tmp;
		bassCut_k = bassCut_k_tmp;
		trebleCut_k = trebleCut_k_tmp;
		__enable_irq();
	}
}

/**
 * @brief Partial memory clear
 * 	Clearing all the delay buffers at once, esp. if 
 * 	the PSRAM is used takes too long for the audio ISR.
 *  Hence the buffer clear is done in configurable portions
 *  spread over a few audio update routines.
 * 
 * @return true 	Memory clean is complete
 * @return false 	Memory clean still in progress
 */
bool AudioEffectDelayStereo_F32::memCleanup()
{
	static uint8_t dlyIdx = 0;
	bool result = false;
	if (dlyIdx == 0) // value 0 is used to reset the addr
	{
		memCleanupStart = 0;
		memCleanupEnd = memCleanupStep;
		flt0L.reset();
		flt0R.reset();
		flt1L.reset();
		flt1R.reset();
		dlyIdx = 1;
	}
	if (memCleanupEnd > dly_length) 		// last segment
	{
		memCleanupEnd = dly_length;
		result = true;
	}
	switch(dlyIdx)
	{
		case 1:
			dly0a.reset(memCleanupStart, memCleanupEnd);
			memCleanupStart = memCleanupEnd;
			memCleanupEnd += memCleanupStep;
			if (result) // if done, reset the mem addr
			{
				memCleanupStart = 0;
				memCleanupEnd = memCleanupStep;
				dlyIdx = 2;
				result = false;
			}
			break;
		case 2:
			dly0b.reset(memCleanupStart, memCleanupEnd);
			memCleanupStart = memCleanupEnd;
			memCleanupEnd += memCleanupStep;			
			if (result) // if done, reset the mem addr
			{
				memCleanupStart = 0;
				memCleanupEnd = memCleanupStep;
				dlyIdx = 3;
				result = false;
			}		
			break;
		case 3:
			dly1a.reset(memCleanupStart, memCleanupEnd);
			memCleanupStart = memCleanupEnd;
			memCleanupEnd += memCleanupStep;			
			if (result) // if done, reset the mem addr
			{
				memCleanupStart = 0;
				memCleanupEnd = memCleanupStep;
				dlyIdx = 4;
				result = false;
			}	
			break;
		case 4:
			dly1b.reset(memCleanupStart, memCleanupEnd);
			memCleanupStart = memCleanupEnd;
			memCleanupEnd += memCleanupStep;			
			if (result) // if done, reset the mem addr
			{
				dlyIdx = 0;
				result = true;
			}				
			break;
		default:
			dlyIdx = 0; // cleanup done, reset the dly line idx
			result = false;
			break;
	}

	
	return result;
}