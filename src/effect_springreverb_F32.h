/*  Stereo spring reverb  for Teensy 4
 *
 *  Author: Piotr Zapart
 *          www.hexefx.com
 *
 * Copyright (c) 2024 by Piotr Zapart
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
#ifndef _EFFECT_SPRINGREVERB_F32_H
#define _EFFECT_SPRINGREVERB_F32_H

#include <Arduino.h>
#include "AudioStream.h"
#include "AudioStream_F32.h"
#include "arm_math.h"
#include "basic_components.h"

// Chirp allpass params
#define SPRVB_CHIRP_AMNT   16      //must be mult of 8
#define SPRVB_CHIRP1_LEN    3
#define SPRVB_CHIRP2_LEN    5
#define SPRVB_CHIRP3_LEN    6
#define SPRVB_CHIRP4_LEN    7

#define SPRVB_ALLP1A_LEN	(224)
#define SPRVB_ALLP1B_LEN	(420)
#define SPRVB_ALLP1C_LEN	(856)
#define SPRVB_ALLP1D_LEN	(1089)

#define SPRVB_ALLP2A_LEN	(156)
#define SPRVB_ALLP2B_LEN	(478)
#define SPRVB_ALLP2C_LEN	(956)
#define SPRVB_ALLP2D_LEN	(1289)

#define SPRVB_DLY1_LEN	(1945)
#define SPRVB_DLY2_LEN	(1363)

class AudioEffectSpringReverb_F32 : public AudioStream_F32
{
public:
    AudioEffectSpringReverb_F32();
	~AudioEffectSpringReverb_F32(){};

    virtual void update();

    void time(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
        n = map (n, 0.0f, 1.0f, 0.7f, rv_time_k_max);
        float32_t gain = map(n, 0.0f, rv_time_k_max, 0.5f, 0.2f);
		inputGain_tmp = gain;
        __disable_irq();
        rv_time_k = n;
        inputGainSet = gain;
        __enable_irq();
    }

    void treble_cut(float n)
    {
        n = 1.0f - constrain(n, 0.0f, 1.0f);
		__disable_irq();
        lp_TrebleCut_k = n;
		__enable_irq();
    }
    
    void bass_cut(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
        n = 2.0f * n - (n*n);
        __disable_irq();
        in_BassCut_k = -n;
        __enable_irq();
    }
    void mix(float m)
    {
		float32_t dry, wet;
		m = constrain(m, 0.0f, 1.0f);
		mix_pwr(m, &wet, &dry);
		__disable_irq();
		wet_gain = wet;
		dry_gain = dry;
		__enable_irq();
	}
	
    void wet_level(float wet)
    {
		wet = constrain(wet, 0.0f, 6.0f);
		__disable_irq();
		wet_gain = wet;
		__enable_irq();
    }

    void dry_level(float dry)
    {
        dry = constrain(dry, 0.0f, 1.0f);
		__disable_irq();
		dry_gain = dry;
		__enable_irq();
    }
    float32_t get_size(void) {return rv_time_k;}

	void bypass_setMode(bypass_mode_t m)
	{
		if (m <= BYPASS_MODE_TRAILS) bp_mode = m;
	}
	bypass_mode_t bypass_geMode() {return bp_mode;}
    bool bypass_get(void) {return bp;}
    void bypass_set(bool state) 
	{
		bp = state;
		if (bp && bp_mode==BYPASS_MODE_TRAILS)
		{
			inputGainSet = 0.0f;
		}
		if (bp == false) inputGainSet = inputGain_tmp;
	}
    bool bypass_tgl(void) 
    {
		bypass_set(bp^1);
        return bp;
    } 
private:
    audio_block_f32_t *inputQueueArray[2];

	float32_t inputGainSet = 0.5f;
    float32_t inputGain = 0.5f;
	float32_t inputGain_tmp = 0.5f;
    float32_t  wet_gain;
    float32_t  dry_gain;
    float32_t in_allp_k; // input allpass coeff (default 0.6)
    float32_t chrp_allp_k[4] = {-0.7f, -0.65f, -0.6f, -0.5f};

    bool bp = false;
	bypass_mode_t bp_mode = BYPASS_MODE_PASS;
	bool cleanup_done = false;
    uint16_t chrp_alp1_idx[SPRVB_CHIRP_AMNT] = {0};
    uint16_t chrp_alp2_idx[SPRVB_CHIRP_AMNT] = {0};
    uint16_t chrp_alp3_idx[SPRVB_CHIRP_AMNT] = {0};
    uint16_t chrp_alp4_idx[SPRVB_CHIRP_AMNT] = {0};

    static constexpr float32_t rv_time_k_max = 0.97f;
    float32_t rv_time_k;

	AudioFilterAllpass<SPRVB_ALLP1A_LEN> sp_lp_allp1a;
	AudioFilterAllpass<SPRVB_ALLP1B_LEN> sp_lp_allp1b;
	AudioFilterAllpass<SPRVB_ALLP1C_LEN> sp_lp_allp1c;
	AudioFilterAllpass<SPRVB_ALLP1D_LEN> sp_lp_allp1d;

	AudioFilterAllpass<SPRVB_ALLP2A_LEN> sp_lp_allp2a;
	AudioFilterAllpass<SPRVB_ALLP2B_LEN> sp_lp_allp2b;	
	AudioFilterAllpass<SPRVB_ALLP2D_LEN> sp_lp_allp2c;
	AudioFilterAllpass<SPRVB_ALLP2D_LEN> sp_lp_allp2d;	

    float32_t *sp_chrp_alp1_buf;
    float32_t *sp_chrp_alp2_buf;
    float32_t *sp_chrp_alp3_buf;
    float32_t *sp_chrp_alp4_buf;

	AudioBasicDelay lp_dly1;
	AudioBasicDelay lp_dly2;

	float32_t in_TrebleCut_k;
	float32_t in_BassCut_k;
	float32_t lp_TrebleCut_k;
	float32_t lp_BassCut_k;
	

	AudioFilterShelvingLPHP flt_in;
	AudioFilterShelvingLPHP flt_lp1;
	AudioFilterShelvingLPHP flt_lp2;

	static const uint8_t lfo_ampl = 10;
	AudioBasicLfo lfo = AudioBasicLfo(1.35f, lfo_ampl);

	bool initialized = false;
};

#endif