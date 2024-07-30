/**
 * @file effect_guitarBooster_F32.h
 * @author Piotr Zapart
 * @brief Oversampled Waveshaper based overdrive effect
 * 			Stereo IO and bypass, but the processing is mono
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024
 * 
 * This program is free software: you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. 
 * If not, see <https://www.gnu.org/licenses/>."
 */
#ifndef _EFFECT_GUITARBOOSTER_F32_H_
#define _EFFECT_GUITARBOOSTER_F32_H_

#include <AudioStream_F32.h>
#include "basic_DSPutils.h"
#include <arm_math.h>


#define GBOOST_TONE_MINF	(800.0f)
#define GBOOST_TONE_MAXF	(8000.0f)
#define GBOOST_LP2_F		(10000.0f)
#define GBOOST_BOTTOM_MINF	(50.0f)
#define GBOOST_BOTTOM_MAXF	(350.0f)

class AudioEffectGuitarBooster_F32 : public AudioStream_F32
{
public:
	AudioEffectGuitarBooster_F32(void) : AudioStream_F32(2, inputQueueArray)
	{
		fs_Hz = AUDIO_SAMPLE_RATE_EXACT;
		blockSize = AUDIO_BLOCK_SAMPLES;
		begin();
	}

	AudioEffectGuitarBooster_F32(const AudioSettings_F32 &settings) : AudioStream_F32(2, inputQueueArray)
	{
		fs_Hz = settings.sample_rate_Hz;
		blockSize = settings.audio_block_samples;
		begin();		
	}

	virtual void update();
	
	void begin()
	{
		arm_fir_interpolate_init_f32(&interpolator, upsample_k, FIR_taps, (float32_t*)FIR_coeffs, interpState, AUDIO_BLOCK_SAMPLES);
		arm_fir_decimate_init_f32(&decimator, FIR_taps, upsample_k, (float32_t*)FIR_coeffs, decimState, upsample_k * AUDIO_BLOCK_SAMPLES);
		bottom(1.0f);
		tone(1.0f);
		hpPost_k = omega(GBOOST_BOTTOM_MINF);
		lp2_k = omega(GBOOST_LP2_F);
		gainRange = 4.0f;
	}
	void drive(float32_t value)
	{
		value = fabs(value);
		value = 1.0f + value * upsample_k;
		__disable_irq()
		gainSet = value;
		__enable_irq();
	}
	/**
	 * @brief Normalized drive, scaled to 1.0 ... gainRange value
	 * 
	 * @param value 0.0f - 1.0f
	 */
	void drive_normalized(float32_t value)
	{
		value = fabs(value);
		value = constrain(value, 0.0f, 1.0f);
		// start with 0.5 - L+R are summed giving x2 gain
		value = 0.5f + value * upsample_k * gainRange;
		__disable_irq()
		gainSet = value;
		__enable_irq();
	}	
	void driveRange(float32_t value)
	{
		__disable_irq()
		gainRange = value;
		__enable_irq();		
	}

	void bottom(float32_t bottom);
	void tone(float32_t t);
	void bias(float32_t b)
	{
		b = constrain(b, -1.0f, 1.0f);
		__disable_irq();
		DCbias = b;
		__enable_irq();
	}

	void mix(float32_t m)
	{
		float32_t d, w;
		m = constrain(m, 0.0f, 1.0f);
		mix_pwr(m, &w, &d);
		__disable_irq();
		wetGain = w;
		dryGain = d;
		__enable_irq();
	}
	void volume(float32_t l)
	{
		l = constrain(l, 0.0f, 1.0f);
		__disable_irq();
		levelSet = l;
		__enable_irq();
	}
	// Bypass 
    bool bypass_get(void) {return bp;}
    void bypass_set(bool state) {bp = state;}
    bool bypass_tgl(void) 
    {
        bp ^= 1; 
        return bp;
    }

	bool octave_get(void) {return octave;}
    void octave_set(bool state) {octave = state;}
    bool octave_tgl(void) 
    {
        octave ^= 1; 
        return octave;
    }
private:
	audio_block_f32_t *inputQueueArray[2];
	float fs_Hz;
	uint16_t blockSize;
	static const uint8_t upsample_k = 5;
	static const uint8_t FIR_taps = 75;
	static constexpr float32_t FIR_coeffs[FIR_taps] =
	{
		-0.000033, 0.000112,-0.000100,-0.000103, 0.000361,-0.000331,-0.000181, 0.000822,-0.000824,-0.000205, 
		 0.001556,-0.001737,-0.000054, 0.002607,-0.003263, 0.000461, 0.003979,-0.005641, 0.001621, 0.005626, 
		-0.009170, 0.003841, 0.007448,-0.014287, 0.007776, 0.009301,-0.021812, 0.014667, 0.011009,-0.033775, 
		 0.027639, 0.012392,-0.057262, 0.058949, 0.013293,-0.142816, 0.268001, 0.680272, 0.268001,-0.142816, 
		 0.013293, 0.058949,-0.057262, 0.012392, 0.027639,-0.033775, 0.011009, 0.014667,-0.021812, 0.009301, 
		 0.007776,-0.014287, 0.007448, 0.003841,-0.009170, 0.005626, 0.001621,-0.005641, 0.003979, 0.000461, 
		-0.003263, 0.002607,-0.000054,-0.001737, 0.001556,-0.000205,-0.000824, 0.000822,-0.000181,-0.000331, 
		 0.000361,-0.000103,-0.000100, 0.000112,-0.000033
	};
	float32_t blockInterpolated[upsample_k * AUDIO_BLOCK_SAMPLES];

	float32_t interpState[(FIR_taps / upsample_k) + AUDIO_BLOCK_SAMPLES - 1];
	float32_t decimState[FIR_taps + (upsample_k * AUDIO_BLOCK_SAMPLES) - 1];

	arm_fir_interpolate_instance_f32 interpolator;
	arm_fir_decimate_instance_f32 decimator;
	arm_linear_interp_instance_f32 waveshaper = 
	{
		2000, -1.0f, 2.0f/2000.0f, &driveWaveform[0]
	};
	bool bp = true; // bypass flag

	bool octave = true;

	float32_t dryGain = 0.0f;
	float32_t wetGain = 1.0f;
	float32_t  DCbias = 0.175f;
	float32_t gainRange = 4.5f;
    float32_t gainSet = 1.0f; // gain is in range 0.0 to 1.0, scaled to 0.0 to gainRange
    float32_t gain = 0.0f;
	float32_t gain_hp = 1.0f;
    float32_t levelSet = 1.0f;
    float32_t level = 1.0f;
    float32_t lp1_k = 0.0f;
    float32_t lp1_reg = 0.0f;
    float32_t lp2_k = 0.0f;
    float32_t lp2_reg = 0.0f;
    float32_t hpPre1_k = 0.0f;
    float32_t hpPre1_reg = 0.0f;
    float32_t hpPre2_k = 0.0f;
    float32_t hpPre2_reg = 0.0f;	
    float32_t hpPost_k = 0.0f;
    float32_t hpPost_reg = 0.0f;	

	static float32_t driveWaveform[2001];

	inline float32_t omega(float f)
	{
		float32_t fs = fs_Hz * upsample_k;
		return 1.0f - expf(-TWO_PI * f / fs);
	}
};

#endif // _EFFECT_GUITARBOOSTER_F32_H_
