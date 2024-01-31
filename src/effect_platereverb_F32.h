/*  Stereo plate reverb for ESP32
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

/***
 * Algorithm based on plate reverbs developed for SpinSemi FV-1 DSP chip
 * 
 * Allpass + modulated delay line based lush plate reverb
 * 
 * Input parameters are float in range 0.0 to 1.0:
 * 
 * size - reverb time
 * hidamp - hi frequency loss in the reverb tail
 * lodamp - low frequency loss in the reverb tail
 * lowpass - output/master lowpass filter, useful for darkening the reverb sound 
 * diffusion - lower settings will make the reverb tail more "echoey".
 * freeze - infinite reverb tail effect
 * 
 */

#ifndef _EFFECT_PLATEREVERB_F32_H_
#define _EFFECT_PLATEREVERB_F32_H_

#include <Arduino.h>
#include "Audio.h"
#include "AudioStream.h"
#include "AudioStream_F32.h"
#include "arm_math.h"
#include "basic_components.h"


class AudioEffectPlateReverb_F32 : public AudioStream_F32
{
public:
    AudioEffectPlateReverb_F32();
	~AudioEffectPlateReverb_F32(){};
    virtual void update();

    bool begin(void);

    void size(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
		n = 2*n - n*n;
        n = map(n, 0.0f, 1.0f, 0.2f, rv_time_k_max);
        //float attn = map(n, 0.2f, rv_time_k_max, 0.5f, 0.25f);
		__disable_irq();
        rv_time_k = n;
        input_attn = 0.5f;
		__enable_irq();
    }

	float size_get(void) {return rv_time_k;}

    void hidamp(float n)
    {
        n = 1.0f - constrain(n, 0.0f, 1.0f);
		__disable_irq();
        lp_hidamp_k = n;
		__enable_irq();
    }
    
    void lodamp(float n)
    {
        n = -constrain(n, 0.0f, 1.0f);
		float32_t tscal = 1.0f + n*0.12f; //n is negativbe here
		__disable_irq();
        lp_lodamp_k = n;
        rv_time_scaler = tscal;        // limit the max reverb time, otherwise it will clip
		__enable_irq();
	}

    void lowpass(float n)
    {
        n = 1.0f - constrain(n, 0.0f, 1.0f);
		__disable_irq();
		master_lp_k = n;
		__enable_irq();
    }
    void hipass(float n)
	{
		n = -constrain(n, 0.0f, 1.0f);
		__disable_irq();
		master_hp_k = n;
		__enable_irq();
	}
    void diffusion(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
        n = map(n, 0.0f, 1.0f, 0.005f, 0.65f);
		__disable_irq();
        in_allp_k = n;
        loop_allp_k = n;
		__enable_irq();
    }

    void freeze(bool state)
    {
        flags.freeze = state;
        if (state)
        {
            rv_time_k_tmp = rv_time_k;      // store the settings
            lp_lodamp_k_tmp = lp_lodamp_k;
            lp_hidamp_k_tmp = lp_hidamp_k;
            __disable_irq();
            rv_time_k = freeze_rvtime_k;                                      
            input_attn = freeze_ingain;
            rv_time_scaler = 1.0f;
            lp_lodamp_k = freeze_lodamp_k;
            lp_hidamp_k = freeze_hidamp_k;
			pitchShimL.setMix(0.0f);	// shimmer off
			pitchShimR.setMix(0.0f);
			__enable_irq();
        }
        else
        {
            //float attn = map(rv_time_k_tmp, 0.0f, rv_time_k_max, 0.5f, 0.25f);    // recalc the in attenuation
            float sc = 1.0f - lp_lodamp_k_tmp * 0.12f;									// scale up the reverb time due to bass loss
			__disable_irq();
            rv_time_k = rv_time_k_tmp;                                      // restore the value
            input_attn = 0.5f;
            rv_time_scaler = sc;
            lp_hidamp_k = lp_hidamp_k_tmp;
            lp_lodamp_k = lp_lodamp_k_tmp;
			shimmer(shimmerRatio);
			__enable_irq();
        }
    }
	/**
	 * @brief Allows to bleed some signal in while in freeze mode
	 * 			has to be relatively low value to avoid oscillation
	 * 
	 * @param b - amount if input signal injected to the freeze reverb
	 * 				range 0.0 to 1.0
	 */
	void freezeBleedIn(float b)
	{
		b = constrain(b, 0.0f, 1.0f);
		b = map(b, 0.0f, 1.0f, 0.0f, 0.1f);
		freeze_ingain = b;
		if (flags.freeze) input_attn = b; // update input gain if freeze is enabled
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

    bool freeze_tgl() {flags.freeze ^= 1; freeze(flags.freeze); return flags.freeze;}
    
    bool freeze_get() {return flags.freeze;}
    
    bool bypass_get(void) {return flags.bypass;}
    void bypass_set(bool state) 
    {
        flags.bypass = state;
        if (state) freeze(false);       // disable freeze in bypass mode
    }
    bool bypass_tgl(void) 
    {
        flags.bypass ^= 1; 
        if (flags.bypass) freeze(false);       // disable freeze in bypass mode
        return flags.bypass;
    }

	/**
	 * @brief controls the delay line modulation, higher values create chorus effect
	 * 
	 * @param c chorus depth, range 0.0f to 1.0f
	 */
	void chorus(float c)
	{
		c = map(c, 0.0f, 1.0f, 1.0f, 100.0f);
		LFO_AMPLset = (uint32_t)c; 
	}

	/**
	 * @brief 
	 * 
	 * @param s 
	 */
	void shimmer(float s)
	{
		if (flags.freeze) return; // do not update the shimmer if in freeze mode
		s = constrain(s, 0.0f, 1.0f);
		s = 2*s - s*s;	
		pitchShimL.setMix(s);
		pitchShimR.setMix(s);
		shimmerRatio = s;
	}
	void shimmerPitch(float ratio)
	{
		pitchShimL.setPitch(ratio);
		pitchShimR.setPitch(ratio);
	}
	void shimmerPitchSemitones(int8_t semitones)
	{
		pitchShimL.setPitchSemintone(semitones);
		pitchShimR.setPitchSemintone(semitones);
	}
	/**
	 * @brief set the reverb pitch.  Range -12 to +24 
	 * 
	 * @param semitones pitch shift in semitones
	 */
	void pitchSemitones(int8_t semitones)
	{
		pitchL.setPitchSemintone(semitones);
		pitchR.setPitchSemintone(semitones);
	}
	void pitchMix(float s)
	{
		s = constrain(s, 0.0f, 1.0f);
		pitchL.setMix(s);
		pitchR.setMix(s);
		pitchRatio = s;
	}

private:
    struct flags_t
    {
        unsigned bypass:            1;
        unsigned freeze:            1;
        unsigned shimmer:           1; // maybe will be added at some point
        unsigned cleanup_done:      1;
    }flags;
    audio_block_f32_t *inputQueueArray_f32[2];

	static const uint16_t IN_ALLP1_BUFL_LEN = 224u;
	static const uint16_t IN_ALLP2_BUFL_LEN = 420u;
	static const uint16_t IN_ALLP3_BUFL_LEN = 856u;
	static const uint16_t IN_ALLP4_BUFL_LEN = 1089u;

	static const uint16_t IN_ALLP1_BUFR_LEN = 156u;
	static const uint16_t IN_ALLP2_BUFR_LEN = 520u;
	static const uint16_t IN_ALLP3_BUFR_LEN = 956u;
	static const uint16_t IN_ALLP4_BUFR_LEN = 1289u;

	static const uint16_t LP_ALLP1_BUF_LEN  = 2303u;
	static const uint16_t LP_ALLP2_BUF_LEN  = 2905u;
	static const uint16_t LP_ALLP3_BUF_LEN  = 3175u;
	static const uint16_t LP_ALLP4_BUF_LEN  = 2398u;

	static const uint16_t LP_DLY1_BUF_LEN   = 3423u;
	static const uint16_t LP_DLY2_BUF_LEN   = 4589u;
	static const uint16_t LP_DLY3_BUF_LEN   = 4365u;
	static const uint16_t LP_DLY4_BUF_LEN   = 3698u;

    const uint16_t lp_dly1_offset_L = 201;
    const uint16_t lp_dly2_offset_L = 145;
    const uint16_t lp_dly3_offset_L = 1897;
    const uint16_t lp_dly4_offset_L = 280;

    const uint16_t lp_dly1_offset_R = 1897;
    const uint16_t lp_dly2_offset_R = 1245;
    const uint16_t lp_dly3_offset_R = 487;
    const uint16_t lp_dly4_offset_R = 780;  

	AudioFilterAllpass<IN_ALLP1_BUFL_LEN> in_allp_1L;
	AudioFilterAllpass<IN_ALLP2_BUFL_LEN> in_allp_2L;
	AudioFilterAllpass<IN_ALLP3_BUFL_LEN> in_allp_3L;
	AudioFilterAllpass<IN_ALLP4_BUFL_LEN> in_allp_4L;

	AudioFilterAllpass<IN_ALLP1_BUFR_LEN> in_allp_1R;
	AudioFilterAllpass<IN_ALLP2_BUFR_LEN> in_allp_2R;
	AudioFilterAllpass<IN_ALLP3_BUFR_LEN> in_allp_3R;
	AudioFilterAllpass<IN_ALLP4_BUFR_LEN> in_allp_4R;

	AudioFilterAllpass<LP_ALLP1_BUF_LEN> lp_allp_1;
	AudioFilterAllpass<LP_ALLP2_BUF_LEN> lp_allp_2;
	AudioFilterAllpass<LP_ALLP3_BUF_LEN> lp_allp_3;
	AudioFilterAllpass<LP_ALLP4_BUF_LEN> lp_allp_4;

	uint16_t LFO_AMPL = 20u;
	uint16_t LFO_AMPLset = 20u;
	AudioBasicLfo lfo1 = AudioBasicLfo(1.35f, LFO_AMPL);
	AudioBasicLfo lfo2 = AudioBasicLfo(1.57f, LFO_AMPL);

    float input_attn;
    float wet_gain;
    float dry_gain;

    float in_allp_k; // input allpass coeff (default 0.6)
    float in_allp_out_L;    // L allpass chain output
    float in_allp_out_R;    // R allpass chain output

    float loop_allp_k;         // loop allpass coeff (default 0.6)
    float lp_allp_out;

	AudioBasicDelay<LP_DLY1_BUF_LEN> lp_dly1;
	AudioBasicDelay<LP_DLY2_BUF_LEN> lp_dly2;
	AudioBasicDelay<LP_DLY3_BUF_LEN> lp_dly3;
	AudioBasicDelay<LP_DLY4_BUF_LEN> lp_dly4;

    float lp_hidamp_k, lp_hidamp_k_tmp;       // loop high band damping coeff
    float lp_lodamp_k, lp_lodamp_k_tmp;       // loop low band damping coeff

	AudioFilterShelvingLPHP flt1;
	AudioFilterShelvingLPHP flt2;
	AudioFilterShelvingLPHP flt3;
	AudioFilterShelvingLPHP flt4;

	float master_lp_k, master_hp_k;
	AudioFilterShelvingLPHP flt_masterL;
	AudioFilterShelvingLPHP flt_masterR;
	// Shimmer
	float pitchRatio = 0.0f;
	AudioBasicPitch	pitchL;
	AudioBasicPitch	pitchR;

	float shimmerRatio = 0.0f;
	AudioBasicPitch	pitchShimL;
	AudioBasicPitch	pitchShimR;

    const float rv_time_k_max = 0.97f;
    float rv_time_k, rv_time_k_tmp;         // reverb time coeff
    float rv_time_scaler;    // with high lodamp settings lower the max reverb time to avoid clipping

    const float freeze_rvtime_k = 1.0f;
    float freeze_ingain = 0.05f;
    const float freeze_lodamp_k = 0.0f;
    const float freeze_hidamp_k = 1.0f;

	bool initialised = false;
};

#endif // _EFFECT_PLATERVBSTEREO_20COPY_H_
