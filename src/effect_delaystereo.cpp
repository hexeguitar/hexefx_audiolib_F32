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
#include "effect_delaystereo.h"

#define TREBLE_LOSS_FREQ    (0.20f)
#define BASS_LOSS_FREQ      (0.05f)
#define BASS_FREQ      		(0.15f)



AudioEffectDelayStereo_F32::AudioEffectDelayStereo_F32(uint32_t dly_range_ms, bool use_psram) : AudioStream_F32(2, inputQueueArray)
{
	psram_mode = use_psram;
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
		dly0a.reset();
		dly0b.reset();
		dly1a.reset();
		dly1b.reset();
		memsetup_done = true;
		return;		
	}

	audio_block_f32_t *blockL, *blockR;
	int i;
	float32_t acc1, acc2, outL, outR, mod_fr[4];
	uint32_t mod_int;
	static float32_t dly_time_flt = 0.0f;

    if (bp)
    {
		if (!cleanup_done)
		{
			dly0a.reset();
			dly0b.reset();
			dly1a.reset();
			dly1b.reset();
			cleanup_done = true;
			tap_active = false;	// reset tap tempo
			tap_counter = 0;
		}
        if (dry_gain > 0.0f) 		// if dry/wet mixer is used
		{
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
		}
		blockL = AudioStream_F32::allocate_f32();
		if (!blockL) return;
		memset(&blockL->data[0], 0, blockL->length*sizeof(float32_t));
		AudioStream_F32::transmit(blockL, 0);	
		AudioStream_F32::transmit(blockL, 1);
		AudioStream_F32::release(blockL);	
        return;
	}
	cleanup_done = false;
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
	for (i=0; i < blockL->length; i++) 
    {  
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
		// lowpass the dely time
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

		float32_t idx = dly_time + mod_fr[0];

		acc1 = dly0b.getTapHermite(dly_time+mod_fr[0]);
		outR = acc1 * 0.6f;
		acc1 = flt0R.process(acc1) * feedb;
		acc1 += blockR->data[i] * input_attn;
		acc1 = flt1R.process(acc1);
		acc2 = dly0a.getTapHermite(dly_time+mod_fr[1]);
		dly0b.write_toOffset(acc2, 0);
		outL = acc2 * 0.6f;
		dly0a.write_toOffset(acc1, 0);

		acc1 = dly1b.getTapHermite(dly_time+mod_fr[2]);
		outR += acc1 * 0.6f;
		acc1 = flt0L.process(acc1) * feedb;
		acc1 += blockL->data[i] * input_attn;
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