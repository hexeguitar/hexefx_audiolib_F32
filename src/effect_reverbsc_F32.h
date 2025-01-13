/*
 * ReverbSC
 * 8 delay line stereo FDN reverb, with feedback matrix based upon physical modeling 
 * scattering junction of 8 lossless waveguides of equal characteristic impedance. 
 * Based on Csound orchestra version by Sean Costello.
 * 
 * Original Author(s): Sean Costello, Istvan Varga
 * Year: 1999, 2005
 * Ported to soundpipe by:  Paul Batchelor
 * 
 * Ported/upgraded to Teensy4 and OpenAudio_ArduinoLibrary: 
 * 01.2024 Piotr Zapart www.hexefx.com 
 * 
 */

#ifndef _EFFECT_REVERBSC_F32_H_
#define _EFFECT_REVERBSC_F32_H_
	
#include <Arduino.h>
#include "AudioStream.h"
#include "AudioStream_F32.h"
#include "arm_math.h"
#include "basic_DSPutils.h"
#include "basic_bypassStereo_F32.h"

#define REVERBSC_DLYBUF_SIZE 98936

class AudioEffectReverbSc_F32 : public AudioStream_F32
{
public:
	AudioEffectReverbSc_F32(bool use_psram = false);
	~AudioEffectReverbSc_F32(){};
	virtual void update();

	typedef struct
	{
		int    write_pos;         /**< write position */
		int    buffer_size;       /**< buffer size */
		int    read_pos;          /**< read position */
		int    read_pos_frac;     /**< fractional component of read pos */
		int    read_pos_frac_inc; /**< increment for fractional */
		int    dummy;             /**<  dummy var */
		int    seed_val;          /**< randseed */
		int    rand_line_cnt;     /**< number of random lines */
		float32_t  filter_state;      /**< state of filter */
		float32_t *buf;               /**< buffer ptr */
	} ReverbScDl_t;

	inline void feedback(const float32_t &fb) 
	{
		if (flags.freeze) return;
		float32_t inGain;
		float32_t feedb = 2.0f * fb - fb*fb;
		feedb = map(feedb, 0.0f, 1.0f, 0.1f, feedb_max);
		feedback_tmp = feedb;
		inGain = map(feedb, 0.1f, feedb_max, 0.5f, 0.2f);
		__disable_irq();
		input_gain_set = inGain;
		feedback_ = feedb;
		__enable_irq();
	}
	inline void lowpass(float32_t val)
	{
		if (flags.freeze) return;
		val = constrain(val, 0.0f, 0.96f);
		val = val*val*val;
		if (damp_fact_ != val)
		{
			damp_fact_tmp = val;
			__disable_irq();
			damp_fact_ = val;
			__enable_irq();	
		}	
	}

    void mix(float32_t mix)
    {
		mix = constrain(mix, 0.0f, 1.0f);
		float dry, wet;
		mix_pwr(mix, &wet, &dry);

		__disable_irq();
		wet_gain = wet;
		dry_gain = dry;
		__enable_irq();
    }

    void wet_level(float32_t wet)
    {
		wet = constrain(wet, 0.0f, 1.0f);
		__disable_irq();
        wet_gain = wet;
		__enable_irq();
    }

    void dry_level(float32_t dry)
    {
		dry = constrain(dry, 0.0f, 1.0f);
		__disable_irq();
        dry_gain = dry;
		__enable_irq();
    }	
	void freeze(bool state);
    bool freeze_tgl() {freeze(flags.freeze^1); return flags.freeze;}
    bool freeze_get() {return flags.freeze;}
	void bypass_setMode(bypass_mode_t m)
	{
		if (m <= BYPASS_MODE_TRAILS) 
		{
			__disable_irq();
			bp_mode = m;
			__enable_irq();
		}
	}
	bypass_mode_t bypass_geMode() {return bp_mode;}
    bool bypass_get(void) {return flags.bypass;}
    void bypass_set(bool state) 
    {
		if (flags.mem_fail) return;
        flags.bypass = state;
        if (state) 
		{
			if (bp_mode == BYPASS_MODE_TRAILS) input_gain_set = 0.0f;
			freeze(false);       // disable freeze in bypass mode
			__disable_irq();
			memCleanupStart = 0;
			memCleanupEnd = memCleanupStep;
			__enable_irq();
		}
		else input_gain_set = input_gain_tmp;
    }
    bool bypass_tgl(void) 
    {
		bypass_set(flags.bypass^1);
        return flags.bypass;
    }
	uint32_t getBfAddr()
	{
		float32_t *addr = aux_;
		return (uint32_t)addr;
	}
private:
    struct flags_t
    {
        unsigned bypass:            1;
        unsigned freeze:            1;
		unsigned cleanup_done:		1;
		unsigned memsetup_done:		1;
		unsigned mem_fail:			1;
    }flags;
	bypass_mode_t bp_mode;
	audio_block_f32_t *inputQueueArray_f32[2];
    void NextRandomLineseg(ReverbScDl_t *lp, int n);
    void InitDelayLine(ReverbScDl_t *lp, int n);
	//void bypass_process();
    float32_t feedback_, feedback_tmp;
	float32_t lpfreq_;
	float32_t i_pitch_mod_;
    float32_t sample_rate_;
    float32_t damp_fact_, damp_fact_tmp;
    bool initialised = false;
    ReverbScDl_t delay_lines_[8];
    float32_t *aux_ = NULL; // main delay line storage buffer, placed either in RAM2 or PSRAM
	const uint32_t aux_size_bytes = REVERBSC_DLYBUF_SIZE*sizeof(float32_t);
	float32_t dry_gain = 0.5f;
	float32_t wet_gain = 0.5f;

	float32_t input_gain_set = 0.5f;
	float32_t input_gain = 0.5f;
	float32_t input_gain_tmp = 0.5f;
	float32_t freeze_ingain = 0.05f;
	static constexpr float32_t feedb_max = 0.99f;

	bool memCleanup(void);
	const uint32_t memCleanupStep = 512;
	uint32_t memCleanupStart = 0;
	uint32_t memCleanupEnd = memCleanupStep;


};
#endif // _EFFECT_REVERBSC_H_
