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
 * Ported to Teensy4 and OpenAudio_ArduinoLibrary: 
 * 01.2024 Piotr Zapart www.hexefx.com 
 * 
 * Fixes, changes:
 * - In the original code the reverb level is affected by the feedback control, fixed
 * - Optional 
 * 
 */

#ifndef _EFFECT_REVERBSC_F32_H_
#define _EFFECT_REVERBSC_F32_H_
	
#include <Arduino.h>
#include "Audio.h"
#include "AudioStream.h"
#include "AudioStream_F32.h"
#include "arm_math.h"
#include "basic_DSPutils.h"

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
		input_gain = inGain;
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
        wet_gain = constrain(wet, 0.0f, 1.0f);
    }

    void dry_level(float32_t dry)
    {
        dry_gain = constrain(dry, 0.0f, 1.0f);
    }	
	void freeze(bool state);
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
		unsigned memsetup_done:		1;
    }flags = {0, 0, 0};

	audio_block_f32_t *inputQueueArray_f32[2];
    void NextRandomLineseg(ReverbScDl_t *lp, int n);
    void InitDelayLine(ReverbScDl_t *lp, int n);
    float32_t feedback_, feedback_tmp;
	float32_t lpfreq_;
	float32_t i_pitch_mod_;
    float32_t sample_rate_;
    float32_t damp_fact_, damp_fact_tmp;
    bool initialised = false;
    ReverbScDl_t delay_lines_[8];
    float32_t *aux_; // main delay line storage buffer, placed either in RAM2 or PSRAM
	float32_t dry_gain = 0.5f;
	float32_t wet_gain = 0.5f;

	float32_t input_gain = 0.5f;
	float32_t input_gain_tmp = 0.5f;
	float32_t freeze_ingain = 0.05f;
	static constexpr float32_t feedb_max = 0.99f;
};
#endif // _EFFECT_REVERBSC_H_
