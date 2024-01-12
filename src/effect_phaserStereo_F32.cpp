/*  Mono Phaser/Vibrato effect for Teensy Audio library
 *
 *  Author: Piotr Zapart
 *          www.hexefx.com
 *
 * Copyright (c) 2021 by Piotr Zapart
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

#include <Arduino.h>
#include "effect_phaserStereo_F32.h"

// ---------------------------- INTERNAL LFO -------------------------------------
#define LFO_LUT_BITS					8
#define LFO_MAX_F						(AUDIO_SAMPLE_RATE_EXACT / 2.0f)		
#define LFO_INTERP_INT_SHIFT			(32-LFO_LUT_BITS)
#define LFO_INTERP_FRACT_MASK			((1<<LFO_INTERP_INT_SHIFT)-1)
#define LFO_LUT_SIZE_MASK               ((1<<LFO_LUT_BITS)-1)

// parabollic/hypertriangular waveform used for the internal LFO
extern "C" {
extern const uint16_t AudioWaveformHyperTri[];
}
// ---------------------------- /INTERNAL LFO ------------------------------------

AudioEffectPhaserStereo_F32::AudioEffectPhaserStereo_F32() : AudioStream_F32(2, inputQueueArray_f32)
{
	memset(allpass_x, 0, PHASER_STEREO_STAGES * sizeof(float32_t) * 2);
	memset(allpass_y, 0, PHASER_STEREO_STAGES * sizeof(float32_t) * 2);
    bps = false;
    lfo_phase_acc = 0;
    lfo_add = 0;
    lfo_lrphase = 0.0f;
    lfo_lroffset = 0;
    feedb = 0.0f;
    mix_ratio = 0.5f;         // start with classic phaser sound 
    stg = PHASER_STEREO_STAGES;
}
AudioEffectPhaserStereo_F32::~AudioEffectPhaserStereo_F32()
{
}


void AudioEffectPhaserStereo_F32::update()
{
#if defined(__ARM_ARCH_7EM__)
    audio_block_f32_t *blockL, *blockR; 
    const audio_block_f32_t *blockMod;    // inputs
    bool internalLFO = false;                    // use internal LFO of no modulation input
    uint16_t i = 0;
    float32_t modSigL, modSigR;
    uint32_t phaseAcc = lfo_phase_acc;
    uint32_t phaseAdd = lfo_add;
    float32_t _lfo_scaler = lfo_scaler;
    float32_t _lfo_bias = lfo_bias;
    uint32_t y0, y1, fract;
    uint64_t y;
    float32_t inSigL, drySigL, inSigR, drySigR;
    float32_t fdb = feedb;

    blockL = AudioStream_F32::receiveWritable_f32(0);       // audio data
    blockR = AudioStream_F32::receiveWritable_f32(1);       // audio data
    blockMod = AudioStream_F32::receiveReadOnly_f32(2);      // bipolar/int16_t control input
    
    if (!blockL || !blockR)
    {
        if (blockMod) AudioStream_F32::release((audio_block_f32_t *)blockMod);
        return;
    }
    if (!blockMod)  internalLFO = true;         // no modulation input provided -> use internal LFO
    if (bps)
    {
        AudioStream_F32::transmit((audio_block_f32_t *)blockL,0);
        AudioStream_F32::transmit((audio_block_f32_t *)blockR,1);
        AudioStream_F32::release((audio_block_f32_t *)blockL);
        AudioStream_F32::release((audio_block_f32_t *)blockR);
        if (blockMod) AudioStream_F32::release((audio_block_f32_t *)blockMod);
        return;
    }
	for (i=0; i < blockL->length; i++) 
    {
        if(internalLFO)
        {
            uint32_t LUTaddr = phaseAcc >> LFO_INTERP_INT_SHIFT;	//8 bit address
            fract = phaseAcc & LFO_INTERP_FRACT_MASK;				// fractional part mask
            y0 = AudioWaveformHyperTri[LUTaddr];
            y1 = AudioWaveformHyperTri[LUTaddr+1];
            y = ((int64_t) y0 * (LFO_INTERP_FRACT_MASK - fract));
            y += ((int64_t) y1 * (fract));
            modSigL = (float32_t)(y>>LFO_INTERP_INT_SHIFT) / 65535.0f;
            if (lfo_lroffset)
            {
                LUTaddr = (LUTaddr + lfo_lroffset) & LFO_LUT_SIZE_MASK;
                y0 = AudioWaveformHyperTri[LUTaddr];
                y1 = AudioWaveformHyperTri[LUTaddr+1];
                y = ((int64_t) y0 * (LFO_INTERP_FRACT_MASK - fract));
                y += ((int64_t) y1 * (fract));
                modSigR = (float32_t)(y>>LFO_INTERP_INT_SHIFT) / 65535.0f;               
            }
            else    modSigR = modSigL;

            phaseAcc += phaseAdd;
        }
        else    // external modulation signal does not use modulation offset between LR 
        {
            modSigL = ((float32_t)blockMod->data[i] + 32768.0f) / 65535.0f;    // mod signal is 0.0 to 1.0
            modSigR = modSigL;  
        }
        // apply scale/offset to the modulation wave
        modSigL = modSigL * _lfo_scaler + _lfo_bias;
        modSigR = modSigR * _lfo_scaler + _lfo_bias;

        drySigL = blockL->data[i] * (1.0f - abs(fdb)*0.25f);  // attenuate the input if using feedback
        inSigL = drySigL + last_sampleL * fdb;
        drySigR = blockR->data[i] * (1.0f - abs(fdb)*0.25f);
        inSigR = drySigR + last_sampleR * fdb;

        y0 = stg;
        while (y0)  // process allpass filters in pairs
        {
            y0--;
		    allpass_y[0][y0] = modSigL * (allpass_y[0][y0] + inSigL) - allpass_x[0][y0];    // left channel
		    allpass_x[0][y0] = inSigL;
		    allpass_y[1][y0] = modSigR * (allpass_y[1][y0] + inSigR) - allpass_x[1][y0];    // right channel
		    allpass_x[1][y0] = inSigR;
            y0--;
		    allpass_y[0][y0] = modSigL * (allpass_y[0][y0] + allpass_y[0][y0+1]) - allpass_x[0][y0];
		    allpass_x[0][y0] = allpass_y[0][y0+1];
            inSigL = allpass_y[0][y0];
            allpass_y[1][y0] = modSigR * (allpass_y[1][y0] + allpass_y[1][y0+1]) - allpass_x[1][y0];
		    allpass_x[1][y0] = allpass_y[1][y0+1];
            inSigR = allpass_y[1][y0];
        }
        last_sampleL = inSigL;
        last_sampleR = inSigR;
        blockL->data[i] = drySigL * (1.0f - mix_ratio) + last_sampleL * mix_ratio;     // dry/wet mixer
        blockR->data[i] = drySigR * (1.0f - mix_ratio) + last_sampleR * mix_ratio;     // dry/wet mixer

    }
    lfo_phase_acc = phaseAcc;
    AudioStream_F32::transmit(blockL, 0);
    AudioStream_F32::transmit(blockR, 1);
	AudioStream_F32::release(blockL);
    AudioStream_F32::release(blockR);
    if (blockMod) AudioStream_F32::release((audio_block_f32_t *)blockMod);
#endif


}
