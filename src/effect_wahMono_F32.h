/**
 * @file effect_wahMono_F32.h
 * @author Piotr Zapart
 * @brief Mono WAH effect
 * @version 0.1
 * @date 2024-07-09
 * 
 * @copyright Copyright (c) 2024 www.hexefx.com
 * 
 * Implementation is based on the work of Transmogrifox
 * https://cackleberrypines.net/transmogrifox/src/bela/inductor_wah_C_src/
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
#ifndef _EFFECT_WAHMONO_F32_H_
#define _EFFECT_WAHMONO_F32_H_

#include <Arduino.h>
#include "AudioStream_F32.h"
#include "basic_DSPutils.h"

typedef enum
{
	WAH_MODEL_G1 = 0,
	WAH_MODEL_G2,
	WAH_MODEL_G3,
	WAH_MODEL_G4,
	WAH_MODEL_VOCAL,
	WAH_MODEL_EXTREME,
	WAH_MODEL_CUSTOM,
	WAH_MODEL_BASS,
	WAH_MODEL_LAST
}wahModel_t;

class AudioEffectWahMono_F32 : public AudioStream_F32
{
public:
	AudioEffectWahMono_F32(void) : AudioStream_F32(3, inputQueueArray_f32) 
	{
		setModel(WAH_MODEL_G3);
	}
	// Alternate specification of block size.  Sample rate does not apply for analyze_rms
	AudioEffectWahMono_F32(const AudioSettings_F32 &settings) : AudioStream_F32(3, inputQueueArray_f32)
	{
		block_size = settings.audio_block_samples;
		fs = settings.sample_rate_Hz;
		setModel(WAH_MODEL_G3);
	}
	virtual void update();

	void setModel(wahModel_t model);
	void setFreq(float32_t val)
	{
		val = constrain(val, 0.0f, 1.0f);
		val = 1.0f - val;
		__disable_irq();
		gp = val;
		__enable_irq();
	}
	void setRange(float32_t heel, float32_t toe)
	{
		gp_top = 1.0f - constrain(heel, 0.0f, 1.0f);
		gp_btm = 1.0f - constrain(toe, 0.0f, 1.0f);
	}

	void setMix(float32_t mix)
	{
		mix = constrain(mix, 0.0f, 1.0f);
		float32_t dry, wet;
		mix_pwr(mix, &wet, &dry);
		__disable_irq();
		dry_gain = dry;
		wet_gain = wet;
		__enable_irq();
	}
    bool bypass_get(void) {return bp;}
    void bypass_set(bool state) {bp = state;}
    bool bypass_tgl(void) 
    {
        bp ^= 1; 
        return bp;
    }	
private:
	bool bp = true; // bypass flag
	audio_block_f32_t *inputQueueArray_f32[3];
	uint16_t block_size = AUDIO_BLOCK_SAMPLES;
	float32_t fs = AUDIO_SAMPLE_RATE_EXACT;
	typedef struct
	{
		//Circuit parameters
		//Using these makes it straight-forward to model other 
		//variants of the circuit
		float32_t Lp; //RLC tank inductor
		float32_t Cf; //feedback capacitor
		float32_t Ci; //input capacitor
		
		float32_t Rpot; //Pot resistance value
		float32_t Ri; //input feed resistor
		float32_t Rs; //RLC tank to BJT base resistor (dry mix)
		float32_t Rp; //resistor placed parallel with the inductor
		
		//Gain-setting components
		float32_t Rc; //BJT gain stage collector resistor
		
		float32_t Rbias; //Typically 470k bias resistor shows up in parallel with output
		float32_t Re; //BJT gain stage emitter resistor
		
		float32_t beta;	//BJT forward gain
	}wah_componentValues_t;

	static const wah_componentValues_t compValues[WAH_MODEL_LAST];

	float32_t gp = 0.0f;	// pot gain
	float32_t input_gain = 0.5f;
	float32_t dry_gain = 0.0f;
	float32_t wet_gain = 1.0f;
	float32_t gp_top = 0.0f;
	float32_t gp_btm = 1.0f;

	float32_t re; //equivalent resistance looking into input BJT base
	float32_t Rp; //resistor placed parallel with the inductor
	float32_t gf; 	//forward gain of BJT amplifier
	//High-Pass biquad coefficients
	float32_t b0h, b1h, b2h;

	//Band-Pass biquad coefficients
	float32_t b0b, b2b;
	float32_t a0b, a1b, a2b;

	//Final combined biquad coefficients used by run_filter()
	float32_t b0;
	float32_t b1;
	float32_t b2;

	float32_t a0c;
	float32_t a1c;
	float32_t a2c;
	//First order high-pass filter coefficients
	//y[n] = ghpf * ( x[n] - x[n-1] ) - a1p*y[n-1]
	float32_t a1p;
	float32_t ghpf;

	//biquad state variables
	float32_t y1;
	float32_t y2;
	float32_t x1;
	float32_t x2;

	//First order high-pass filter state variables
	float32_t yh1;
	float32_t xh1;
	float32_t clip(float32_t x);
	float32_t sqr(float32_t x)
	{
		return x*x;
	}
};


#endif // _EFFECT_WAHMONO_F32_H_
