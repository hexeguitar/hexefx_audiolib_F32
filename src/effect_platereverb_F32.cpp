/*  Stereo plate reverb for Teensy 4
 *
 *  Author: Piotr Zapart
 *          www.hexefx.com
 *
 * Copyright (c) 2020 by Piotr Zapart
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include <Arduino.h>
#include "effect_platereverb_F32.h"

#define INP_ALLP_COEFF      (0.65f)
#define LOOP_ALLOP_COEFF    (0.65f)

#define TREBLE_LOSS_FREQ        (0.3f)
#define TREBLE_LOSS_FREQ_MAX    (0.08f)
#define BASS_LOSS_FREQ        	(0.06f)

#define RV_MASTER_LOWPASS_F (0.6f)                           // master lowpass scaled frequency coeff. 

bool AudioEffectPlateReverb_F32::begin()
{
	inputGainSet = 0.5f;
    inputGain = 0.5f;
	inputGain_tmp = 0.5f;
    wet_gain = 1.0f;        // default mode: wet signal only
    dry_gain = 0.0f;
    in_allp_k = INP_ALLP_COEFF;
	loop_allp_k = LOOP_ALLOP_COEFF;
	rv_time_scaler = 1.0f;
	rv_time_k = 0.2f;
	pitch_semit = 0;
	pitchShim_semit = 0;

	if(!in_allp_1L.init(&in_allp_k)) return false;
	if(!in_allp_2L.init(&in_allp_k)) return false;
	if(!in_allp_3L.init(&in_allp_k)) return false;
	if(!in_allp_4L.init(&in_allp_k)) return false;

	if(!in_allp_1R.init(&in_allp_k)) return false;
	if(!in_allp_2R.init(&in_allp_k)) return false;
	if(!in_allp_3R.init(&in_allp_k)) return false;
	if(!in_allp_4R.init(&in_allp_k)) return false;

	in_allp_out_L = 0.0f;
    in_allp_out_R = 0.0f;

	if(!lp_allp_1.init(&loop_allp_k)) return false;
	if(!lp_allp_2.init(&loop_allp_k)) return false;
	if(!lp_allp_3.init(&loop_allp_k)) return false;
	if(!lp_allp_4.init(&loop_allp_k)) return false;

    lp_allp_out = 0.0f;

	if(!lp_dly1.init(LP_DLY1_BUF_LEN)) return false;
	if(!lp_dly2.init(LP_DLY2_BUF_LEN)) return false;
	if(!lp_dly3.init(LP_DLY3_BUF_LEN)) return false;
	if(!lp_dly4.init(LP_DLY4_BUF_LEN)) return false;

    lp_hidamp_k = 1.0f;
    lp_lodamp_k = 0.0f;
	flt1.init(BASS_LOSS_FREQ, &lp_lodamp_k, TREBLE_LOSS_FREQ, &lp_hidamp_k);
	flt2.init(BASS_LOSS_FREQ, &lp_lodamp_k, TREBLE_LOSS_FREQ, &lp_hidamp_k);
	flt3.init(BASS_LOSS_FREQ, &lp_lodamp_k, TREBLE_LOSS_FREQ, &lp_hidamp_k);
	flt4.init(BASS_LOSS_FREQ, &lp_lodamp_k, TREBLE_LOSS_FREQ, &lp_hidamp_k);

	master_lp_k = 1.0f;
	master_hp_k = 0.0f;
	flt_masterL.init(0.08f, &master_hp_k, 0.1f, &master_lp_k);
	flt_masterR.init(0.08f, &master_hp_k, 0.1f, &master_lp_k);

	if(!pitchL.init()) return false;
	if(!pitchR.init()) return false;
	pitchL.setPitch(1.0f); //natural pitch
	pitchR.setPitch(1.0f); //natural pitch
	pitchL.setTone(0.36f);
    pitchR.setTone(0.36f);
	pitchL.setMix(0.0f);
	pitchR.setMix(0.0f);

	shimmerRatio = 0.0f;
	if(!pitchShimL.init()) return false;
	if(!pitchShimR.init()) return false;
	pitchShimL.setPitch(2.0f);
	pitchShimR.setPitch(2.0f);
	pitchShimL.setTone(0.26f);
	pitchShimR.setTone(0.26f);
	pitchShimL.setMix(0.0f);
	pitchShimR.setMix(0.0f);

	flags.bypass = 1;
    flags.freeze = 0;
	initialised = true;
	return true;
}

void AudioEffectPlateReverb_F32::update()
{
#if defined(__IMXRT1062__)	
	if (!initialised) return;
    audio_block_f32_t *blockL, *blockR;
	int16_t i;
	float acc;
    float rv_time;
	uint32_t offset;
	float lfo_fr;

	blockL = AudioStream_F32::receiveWritable_f32(0);
	blockR = AudioStream_F32::receiveWritable_f32(1);
	if (!bypass_process(&blockL, &blockR, bp_mode, (bool)flags.bypass))
		return;

    // handle bypass, 1st call will clean the buffers to avoid continuing the previous reverb tail
    if (flags.bypass)
    {
		if (!flags.cleanup_done && bp_mode != BYPASS_MODE_TRAILS)
		{
			in_allp_1L.reset();
			in_allp_2L.reset();
			in_allp_3L.reset();
			in_allp_4L.reset();
			in_allp_1R.reset();
			in_allp_2R.reset();
			in_allp_3R.reset();
			in_allp_4R.reset();
			lp_allp_1.reset();
			lp_allp_2.reset();
			lp_allp_3.reset();
			lp_allp_4.reset();
			lp_dly1.reset();
			lp_dly2.reset();
			lp_dly3.reset();
			lp_dly4.reset();
			flags.cleanup_done = 1;
		}
		if (bp_mode != BYPASS_MODE_TRAILS)
		{
			AudioStream_F32::transmit(blockL, 0);
			AudioStream_F32::transmit(blockR, 1);
			AudioStream_F32::release(blockL);
			AudioStream_F32::release(blockR);
			return;
		}
	}
	
	flags.cleanup_done = 0;
    rv_time = rv_time_k;

	for (i=0; i < blockL->length; i++) 
    {
        // do the LFOs
		lfo1.update();
		lfo2.update();

		inputGain += (inputGainSet - inputGain) * 0.25f;

		acc = blockL->data[i] * inputGain;

        // chained input allpasses, channel L
		acc = in_allp_1L.process(acc);
		acc = in_allp_2L.process(acc);
		acc = in_allp_3L.process(acc);
		in_allp_out_L = in_allp_4L.process(acc);
		in_allp_out_L = pitchL.process(in_allp_out_L); 

		// chained input allpasses, channel R
        acc = blockR->data[i] * inputGain;

		acc = in_allp_1R.process(acc);
		acc = in_allp_2R.process(acc);
		acc = in_allp_3R.process(acc);
		in_allp_out_R = in_allp_4R.process(acc);

		acc = pitchShimR.process(lp_allp_out + in_allp_out_R); // shimmer

	   	acc = lp_dly1.process(acc);
		acc = flt1.process(acc) * rv_time * rv_time_scaler;

		acc = lp_allp_2.process(acc + in_allp_out_L);
		acc = lp_dly2.process(acc);
		acc = flt2.process(acc) * rv_time * rv_time_scaler;

		acc = pitchShimL.process(acc + in_allp_out_R); // shimmer

		acc = lp_allp_3.process(acc);
	   	acc = lp_dly3.process(acc);
		acc = flt3.process(acc) * rv_time * rv_time_scaler;

		acc = lp_allp_4.process(acc + in_allp_out_L);
		acc = lp_dly4.process(acc);
		
		lp_allp_out = flt4.process(acc) * rv_time * rv_time_scaler; 

		acc  = lp_dly1.getTap(lp_dly1_offset_L) * 0.8f;
		acc += lp_dly2.getTap(lp_dly2_offset_L) * 0.7f;
		acc += lp_dly3.getTap(lp_dly3_offset_L) * 0.6f;
		acc += lp_dly4.getTap(lp_dly4_offset_L) * 0.5f;

        // Master lowpass filter
		acc = flt_masterL.process(acc);

		blockL->data[i] = acc * wet_gain + blockL->data[i] * dry_gain; 
        // ChannelR
		acc  = lp_dly1.getTap(lp_dly1_offset_R) * 0.8f;
		acc += lp_dly2.getTap(lp_dly2_offset_R) * 0.7f;
		acc += lp_dly3.getTap(lp_dly3_offset_R) * 0.6f;
		acc += lp_dly4.getTap(lp_dly4_offset_R) * 0.5f;
        // Master lowpass filter
		acc = flt_masterR.process(acc);
		blockR->data[i] = acc * wet_gain + blockR->data[i] * dry_gain;

		// modulate the delay lines
		// delay 1
		lfo1.get(BASIC_LFO_PHASE_0, &offset, &lfo_fr); 				// lfo1 sin output
		acc = lp_dly1.getTap(offset, lfo_fr);
		lp_dly1.write_toOffset(acc, LFO_AMPL*2);
		lp_dly1.updateIndex();

		// delay 2
		lfo1.get(BASIC_LFO_PHASE_90, &offset, &lfo_fr); 			// lfo1 cos output
		acc = lp_dly2.getTap(offset, lfo_fr);
		lp_dly2.write_toOffset(acc, LFO_AMPL*2);
		lp_dly2.updateIndex();

		// delay 3
		lfo2.get(BASIC_LFO_PHASE_0, &offset, &lfo_fr); 				// lfo2 sin output
		acc = lp_dly3.getTap(offset, lfo_fr);
		lp_dly3.write_toOffset(acc, LFO_AMPL*2);
		lp_dly3.updateIndex();
 
		// delay 4
		lfo2.get(BASIC_LFO_PHASE_90, &offset, &lfo_fr); 			// lfo2 cos output
		acc = lp_dly4.getTap(offset, lfo_fr);
		lp_dly4.write_toOffset(acc, LFO_AMPL*2);
		lp_dly4.updateIndex();		
	}
	if (LFO_AMPL != LFO_AMPLset) 
	{
		lfo1.setDepth(LFO_AMPL);
		lfo2.setDepth(LFO_AMPL);
		LFO_AMPL = LFO_AMPLset;
	}
    AudioStream_F32::transmit(blockL, 0);
	AudioStream_F32::transmit(blockR, 1);
	AudioStream_F32::release(blockL);
	AudioStream_F32::release(blockR);
	#endif
}
