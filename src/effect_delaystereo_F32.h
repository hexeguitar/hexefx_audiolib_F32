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
#ifndef _EFFECT_DELAYSTEREO_H_
#define _EFFECT_DELAYSTEREO_H_

#include <Arduino.h>
#include "Audio.h"
#include "AudioStream.h"
#include "AudioStream_F32.h"
#include "arm_math.h"
#include "basic_components.h"

class AudioEffectDelayStereo_F32 : public AudioStream_F32
{
public:
	AudioEffectDelayStereo_F32(uint32_t dly_range_ms=400, bool use_psram=false);
	~AudioEffectDelayStereo_F32(){};
	virtual void update();
	/**
	 * @brief set the delay time
	 * 
	 * @param t delay time scaled to range 0.0 to 1.0
	 * @param force bypass the smoothing, immediate change
	 */
	void time(float t, bool force = false)
	{
		t = constrain(t, 0.0f, 1.0f);
		t = t * t;
		t = map(t, 0.0f, 1.0f, (float32_t)(dly_length-dly_time_min), 0.0f);
		__disable_irq();
		if (force) dly_time = t;
		dly_time_set = t;
		__enable_irq();
	}
	/**
	 * @brief delay time set in samples
	 * 
	 * @param samples range is from 0 to delay buffer lengts - minimum delay
	 */
	void delay(uint32_t samples)
	{
		samples = constrain(samples, 0u, dly_length-dly_time_min);
		samples = dly_length-dly_time_min - samples;
		__disable_irq();
		dly_time_set = samples;
		__enable_irq();	
	}
	/**
	 * @brief Amount of repeats
	 * 
	 * @param n 0.0f-1.0f range
	 */
	void feedback(float n)
    {
		if (infinite) return;
		float32_t fb, attn;
        n = constrain(n, 0.0f, 1.0f);
	    fb = map(n, 0.0f, 1.0f, 0.0f, feedb_max) * hp_feedb_limit;
        attn = map(n*n*n, 0.0f, 1.0f, 1.0f, 0.4f);
		inputGain_tmp = attn;
        __disable_irq();
        feedb = fb;
        inputGainSet = attn;
        __enable_irq();
    }
	/**
	 * @brief How fast the delay time is updated
	 * 		emulates analog tape machines
	 * 
	 * @param n 0.0f-1.0f range, 0 - fastest update
	 */
	void inertia(float n)
	{
		n = constrain(n, 0.0f, 1.0f);
		n = 2.0f * n - (n*n);
        n = map (n, 0.0f, 1.0f, 10.0f, 0.3f);
        __disable_irq();
        dly_time_step = n;
        __enable_irq();	
	}
	/**
	 * @brief Output treble control
	 * 
	 * @param n 0.0f-1.0f range
	 */
    void treble(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
		__disable_irq();
        treble_k = n;
		__enable_irq();
    }
	/**
	 * @brief Treble loss control (darkens the repeats)
	 * 
	 * @param n 0.0f-1.0f range
	 */
    void treble_cut(float n)
    {
		if (infinite) return;
        n = 1.0f - constrain(n, 0.0f, 1.0f);
		trebleCut_k_tmp = n;
		__disable_irq();
        trebleCut_k = n;
		__enable_irq();
    }
	/**
	 * @brief Output bass control
	 * 
	 * @param n 0.0f-1.0f range
	 */
    void bass(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
		n = 1.0f - 2.0f*n + (n*n);
        __disable_irq();
        bass_k = -n;
        __enable_irq();
    }    
	/**
	 * @brief Bass loss (repeats will loose low end)
	 * 
	 * @param n 0.0f-1.0f range
	 */
    void bass_cut(float n)
    {
		if (infinite) return;
        n = constrain(n, 0.0f, 1.0f);
        n = 2.0f * n - (n*n);
		bassCut_k_tmp = -n;
        __disable_irq();
        bassCut_k = -n;
        __enable_irq();
    }
	/**
	 * @brief dry/wet mixer
	 * 		0 = dry only, 1=wet only
	 * 
	 * @param m 0.0f-1-0f range
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
	 * @brief Modulation frequency in Hz
	 * 
	 * @param f range 0.0f - 16.0f
	 */
	void mod_rateHz(float32_t f)
	{
		f = constrain(f, 0.0f, 16.0f);
		__disable_irq();
		lfo.setRate(f);
		__enable_irq();
	}
	/**
	 * @brief modulation frequency scaled to 0.0f-1.0f range
	 * 
	 * @param r rate
	 */
	void mod_rate(float32_t r)
	{
		r = constrain(r*r*r, 0.0f, 1.0f);
		r = map(r, 0.0f, 1.0f, 0.0f, lfo_fmax);
		__disable_irq();
		lfo.setRate(r);
		__enable_irq();	
	}
	/**
	 * @brief Modulation depth
	 * 
	 * @param d 0.0f-1-0f range
	 */
	void mod_depth(float32_t d)
	{
		d = constrain(d, 0.0f, 1.0f);
		d = map(d, 0.0f, 1.0f, 0.0f, lfo_ampl_max);
		__disable_irq();
		lfo.setDepth(d);
		__enable_irq();	
	}
	typedef enum
	{
		BYPASS_MODE_PASS,		// pass the input signal to the output
		BYPASS_MODE_OFF,		// mute the output
		BYPASS_MODE_TRAILS		// mutes the input only
	}bypass_mode_t;
	void bypass_setMode(bypass_mode_t m)
	{
		if (m <= BYPASS_MODE_TRAILS) bp_mode = m;
	}
	bypass_mode_t bypass_geMode() {return bp_mode;}
	bool bypass_get(void) {return bp;}
    void bypass_set(bool state) 
	{
		if (bp == state) return;
		bp = state;
		if (bp)
		{
			__disable_irq();
			memCleanupStart = 0;
			memCleanupEnd = memCleanupStep;
			__enable_irq();
			freeze(false);
		}
		else
		{
			__disable_irq();
			inputGainSet = inputGain_tmp;
			__enable_irq();
		}
	}
    bool bypass_tgl(void) 
    {
		bypass_set(bp ^ 1);
        return bp;
    }
	void freeze(bool state);
    bool freeze_tgl() {freeze(infinite^1); return infinite;}
    bool freeze_get() {return infinite;}
	uint32_t tap_tempo(bool avg=true)
	{
		int32_t delta;
		uint32_t tempo_ticks = 0;
		if (!tap_active)
		{
			tap_counter = 0;
			tap_active = true;
		} 
		else
		{
			__disable_irq();
			tap_counter_new = tap_counter;
			tap_counter = 0;
			__enable_irq();
			delta = tap_counter_new - tap_counter_last;
			if (abs(delta) > tap_counter_deltamax || !avg) // new tempo?
			{
				tempo_ticks = tap_counter_new;
			}
			else 
			{
				tempo_ticks = (tap_counter_new>>1) + (tap_counter_last>>1);
			}
			while (tempo_ticks > dly_length - dly_time_min)
			{
				tempo_ticks >>= 1;
			}
			tap_counter_last = tempo_ticks;
			delay(tempo_ticks);
		}
		return tempo_ticks;
	} 
private:
	audio_block_f32_t *inputQueueArray[2];

	uint32_t dly_length;
	AudioBasicDelay dly0a;
	AudioBasicDelay dly0b;
	AudioBasicDelay dly1a;
	AudioBasicDelay dly1b;
	
	AudioFilterShelvingLPHP flt0L;
	AudioFilterShelvingLPHP flt1L;
	AudioFilterShelvingLPHP flt0R;
	AudioFilterShelvingLPHP flt1R;

	static constexpr float32_t lfo_fmax = 16.0f;
	static constexpr float32_t lfo_ampl_max = 127.0f;
	float32_t lfo_ampl = 0.0f;
	AudioBasicLfo lfo = AudioBasicLfo(0.0f, lfo_ampl);
	bool psram_mode;
	bool memsetup_done = false;
	bool bp = true;
	bypass_mode_t bp_mode = BYPASS_MODE_TRAILS;
	bool cleanup_done = false;
	bool infinite = false;
	bool extInputMode = false; // external input via pointers passed to constructor

	static constexpr float32_t feedb_max = 0.96f;
	float32_t feedb = 0;
	float32_t hp_feedb_limit = 1.0f;
    float32_t wet_gain;
    float32_t dry_gain;
	float32_t inputGainSet = 1.0f;
	float32_t inputGain = 1.0f;
	float32_t trebleCut_k = 1.0f;
	float32_t bassCut_k = 0.0f;
	float32_t treble_k = 1.0f;
	float32_t bass_k = 0.0f;
	float32_t dly_time, dly_time_set;
	float32_t dly_time_step = 10.0f;
	static const uint32_t dly_time_min = 128;
	bool initialized = false;
	
	// freeze variables
	float32_t freeze_ingain = 0.00f;
	float32_t inputGain_tmp = 1.0f;
	float32_t bassCut_k_tmp = 0.0f;
	float32_t trebleCut_k_tmp = 1.0f;
	float32_t feedb_tmp = 0;

	bool tap_active = false;
	uint32_t tap_counter = 0;
	uint32_t tap_counter_last=0, tap_counter_new=0;
	static const uint32_t tap_counter_max = 3000*AUDIO_SAMPLE_RATE; // 3 sec
	static const int32_t tap_counter_deltamax = 0.3f*AUDIO_SAMPLE_RATE_EXACT;

	bool memCleanup(void);
	void begin(uint32_t dly_range_ms, bool use_psram);
	const uint32_t memCleanupStep = 2048;
	uint32_t memCleanupStart = 0;
	uint32_t memCleanupEnd = memCleanupStep;
};

#endif // _EFFECT_DELAYSTEREO_H_
