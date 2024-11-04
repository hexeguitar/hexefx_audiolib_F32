/*  Stereo plate reverb for Teensy 4
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
    AudioEffectPlateReverb_F32() : AudioStream_F32(2, inputQueueArray_f32) { begin();}
	AudioEffectPlateReverb_F32(const AudioSettings_F32 &settings) : AudioStream_F32(2, inputQueueArray_f32)
	{
		block_size = settings.audio_block_samples;
		begin();			
	}
	~AudioEffectPlateReverb_F32(){};
    virtual void update();

    bool begin(void);

	/**
	 * @brief sets the reverb time
	 * 
	 * @param n range 0.0f - 1.0f
	 */
    void size(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
		n = 2*n - n*n;
        n = map(n, 0.0f, 1.0f, 0.2f, rv_time_k_max);
		rv_time_k_tmp = n;
		inputGain_tmp = 0.5f;
		__disable_irq();
        rv_time_k = n;
        inputGainSet = 0.5f;
		__enable_irq();
    }

	void time(float n)
	{
		size(n);
	}
	/**
	 * @brief returns the set reverb time
	 * 
	 * @return float reverb time value
	 */
	float size_get(void) {return rv_time_k;}

	/**
	 * @brief Treble loss in reverb tail
	 * 
	 * @param n 0.0f to 1.0f
	 */
    void hidamp(float n)
    {
        n = 1.0f - constrain(n, 0.0f, 1.0f);
		lp_hidamp_k_tmp = n;
		__disable_irq();
        lp_hidamp_k = n;
		__enable_irq();
    }
    /**
     * @brief Bass loss in reverb tails
     * 
     * @param n 0.0f to 1.0f
     */
    void lodamp(float n)
    {
        n = -constrain(n, 0.0f, 1.0f);
		float32_t tscal = 1.0f + n*0.12f; //n is negativbe here
		lp_lodamp_k_tmp = n;
		__disable_irq();
        lp_lodamp_k = n;
        rv_time_scaler = tscal;        // limit the max reverb time, otherwise it will clip
		__enable_irq();
	}
	/**
	 * @brief Output lowpass filter
	 * 
	 * @param n 0.0f to 1.0f
	 */
    void lowpass(float n)
    {
        n = 1.0f - constrain(n, 0.0f, 1.0f);
		__disable_irq();
		master_lp_k = n;
		__enable_irq();
    }
	/**
	 * @brief Output highpass filter
	 * 
	 * @param n 0.0f 1.0f
	 */
    void hipass(float n)
	{
		n = -constrain(n, 0.0f, 1.0f);
		__disable_irq();
		master_hp_k = n;
		__enable_irq();
	}
	/**
	 * @brief reverb tail diffusion, 
	 * 	lower values produce more single repeats, echos
	 * 
	 * @param n 0.0f - 1.0f
	 */
    void diffusion(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
        n = map(n, 0.0f, 1.0f, 0.005f, 0.65f);
		__disable_irq();
        in_allp_k = n;
        loop_allp_k = n;
		__enable_irq();
    }
	/**
	 * @brief Freeze option On/Off. Freeze sets the reverb
	 * 	time to infinity and mutes (almost) the input signal
	 * 
	 * @param state 
	 */
    void freeze(bool state)
    {
		if (flags.freeze == state || flags.bypass) return;
        flags.freeze = state;
        if (state)
        {
            rv_time_k_tmp = rv_time_k;      // store the settings
            lp_lodamp_k_tmp = lp_lodamp_k;
            lp_hidamp_k_tmp = lp_hidamp_k;
            __disable_irq();
            rv_time_k = freeze_rvtime_k;                                      
            inputGainSet = freeze_ingain;
            rv_time_scaler = 1.0f;
            lp_lodamp_k = freeze_lodamp_k;
            lp_hidamp_k = freeze_hidamp_k;
			pitchShimL.setMix(0.0f);	// shimmer off
			pitchShimR.setMix(0.0f);
			__enable_irq();
        }
        else
        {
            float sc = 1.0f - lp_lodamp_k_tmp * 0.12f;									// scale up the reverb time due to bass loss
			__disable_irq();
            rv_time_k = rv_time_k_tmp;                                      // restore the value
            if (!flags.bypass)
			{
				inputGainSet = 0.5f;
				inputGain_tmp = 0.5f;
			}
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
		if (flags.freeze) inputGainSet = b; // update input gain if freeze is enabled
	}
	/**
	 * @brief Internal Dry / Wet mixer
	 * 
	 * @param m 0.0f (full dry) - 1.0f (full wet)
	 */
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
	/**
	 * @brief wet signal volume
	 * 
	 * @param wet 0.0f - 1.0f
	 */
    void wet_level(float wet)
    {
		wet = constrain(wet, 0.0f, 6.0f);
		__disable_irq();
		wet_gain = wet;
		__enable_irq();
    }
	/**
	 * @brief dry signal volume
	 * 
	 * @param dry 0.0f - 1.0f
	 */
    void dry_level(float dry)
    {
        dry = constrain(dry, 0.0f, 1.0f);
		__disable_irq();
		dry_gain = dry;
		__enable_irq();
    }
	/**
	 * @brief toogle the Freeze mode
	 * 
	 * @return true 
	 * @return false 
	 */
    bool freeze_tgl() {freeze(flags.freeze^1); return flags.freeze;}
    /**
     * @brief return the Freeze mode state
     * 
     * @return true 
     * @return false 
     */
    bool freeze_get() {return flags.freeze;}
 	
	/**
	 * @brief sets the bypass mode (see above)
	 * 
	 * @param m 
	 */
	void bypass_setMode(bypass_mode_t m)
	{
		if (m <= BYPASS_MODE_TRAILS) bp_mode = m;
	}
	bypass_mode_t bypass_geMode() {return bp_mode;}   
    bool bypass_get(void) {return flags.bypass;}
    void bypass_set(bool state) 
    {
        flags.bypass = state;
        if (state) 
		{
			if (bp_mode == BYPASS_MODE_TRAILS) inputGainSet = 0.0f;
			freeze(false);       // disable freeze in bypass mode
		}
		else inputGainSet = inputGain_tmp;
    }
    bool bypass_tgl(void) 
    {
		bypass_set(flags.bypass^1);
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
		__disable_irq();
		LFO_AMPLset = (uint32_t)c; 
		__enable_irq();
	}

	/**
	 * @brief Contriols the amount of shimmer effect
	 * 
	 * @param s 0.0f - 1.0f
	 */
	void shimmer(float s)
	{
		if (flags.freeze) return; // do not update the shimmer if in freeze mode
		s = constrain(s, 0.0f, 1.0f);
		s = 2*s - s*s;
		__disable_irq();
		pitchShimL.setMix(s);
		pitchShimR.setMix(s);
		shimmerRatio = s;
		__enable_irq();
	}
	/**
	 * @brief Sets the pitch of the shimmer effect
	 * 
	 * @param ratio pitch up (>1.0f)  or down (<1.0f) ratio
	 */
	void shimmerPitch(float ratio)
	{
		__disable_irq();
		pitchShimL.setPitch(ratio);
		pitchShimR.setPitch(ratio);
		__enable_irq();
	}
	/**
	 * @brief Sets the shimmer effect pitch in semitones
	 * 
	 * @param semitones 
	 */
	void shimmerPitchSemitones(int8_t semitones)
	{
		__disable_irq();
		pitchShimL.setPitchSemintone(semitones);
		pitchShimR.setPitchSemintone(semitones);
		__enable_irq();
	}
	/**
	 * @brief shimemr pitch set using built in semitzone table
	 * 
	 * 
	 * @param value float 0.0f to 1.0f
	 */
	void shimmerPitchNormalized(float32_t value)
	{
		value = constrain(value, 0.0f, 1.0f);
		float32_t idx = map(value, 0.0f, 1.0f, 0.0f, (float32_t)sizeof(semitoneTable)+0.499f);
		pitchShim_semit = semitoneTable[(uint8_t)idx];
		__disable_irq();
		pitchShimL.setPitchSemintone(pitchShim_semit);
		pitchShimR.setPitchSemintone(pitchShim_semit);
		__enable_irq();
	}
	int8_t shimmerPitch_get() {return pitchShim_semit;}
	/**
	 * @brief set the reverb pitch.  Range -12 to +24 
	 * 
	 * @param semitones pitch shift in semitones
	 */
	void pitchSemitones(int8_t semitones)
	{
		__disable_irq();
		pitchL.setPitchSemintone(semitones);
		pitchR.setPitchSemintone(semitones);
		__enable_irq();
	}
	/**
	 * @brief sets the reverb pitch using the built in table
	 * 			input range is float 0.0 to 1.0
	 * 
	 * @param value  
	 */
	void pitchNormalized(float32_t value)
	{
		value = constrain(value, 0.0f, 1.0f);
		float32_t idx = map(value, 0.0f, 1.0f, 0.0f, (float32_t)sizeof(semitoneTable)+0.499f);
		pitch_semit = semitoneTable[(uint8_t)idx];
		__disable_irq();
		pitchL.setPitchSemintone(pitch_semit);
		pitchR.setPitchSemintone(pitch_semit);
		__enable_irq();
	}
	int8_t pitch_get() {return pitch_semit;}
	/**
	 * @brief Reverb pitch shifter dry/wet mixer
	 * 
	 * @param s 0.0f(dry reverb) - 1.0f (100% pitch shifter out)
	 */
	void pitchMix(float s)
	{
		s = constrain(s, 0.0f, 1.0f);
		__disable_irq();
		pitchL.setMix(s);
		pitchR.setMix(s);
		pitchRatio = s;
		__enable_irq();
	}

private:
    struct flags_t
    {
        unsigned bypass:            1;
        unsigned freeze:            1;
        unsigned shimmer:           1;
        unsigned cleanup_done:      1;
    }flags;
	bypass_mode_t bp_mode = BYPASS_MODE_PASS;
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

    float inputGain;
	float inputGainSet;
	float inputGain_tmp;
    float wet_gain;
    float dry_gain;

    float in_allp_k; // input allpass coeff (default 0.6)
    float in_allp_out_L;    // L allpass chain output
    float in_allp_out_R;    // R allpass chain output

    float loop_allp_k;         // loop allpass coeff (default 0.6)
    float lp_allp_out;

	AudioBasicDelay lp_dly1;
	AudioBasicDelay lp_dly2;
	AudioBasicDelay lp_dly3;
	AudioBasicDelay lp_dly4;

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

	const int8_t semitoneTable[9] = {-12, -7, -5, -3, 0, 3, 5, 7, 12};
	int8_t pitch_semit;
	int8_t pitchShim_semit;
    const float rv_time_k_max = 0.97f;
    float rv_time_k, rv_time_k_tmp;         // reverb time coeff
    float rv_time_scaler;    // with high lodamp settings lower the max reverb time to avoid clipping

    const float freeze_rvtime_k = 1.0f;
    float freeze_ingain = 0.05f;
    const float freeze_lodamp_k = 0.0f;
    const float freeze_hidamp_k = 1.0f;

	bool initialised = false;
	uint16_t block_size = AUDIO_BLOCK_SAMPLES;

};

#endif // _EFFECT_PLATERVBSTEREO_20COPY_H_
