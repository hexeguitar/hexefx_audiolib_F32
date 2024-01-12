/*  Stereo Phaser/Vibrato effect for Teensy Audio library
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

#ifndef _EFFECT_PHASERSTEREO_F32_H
#define _EFFECT_PHASERSTEREO_F32_H

#include <Arduino.h>
#include "Audio.h"
#include "AudioStream.h"
#include "AudioStream_F32.h"
#include "arm_math.h"

#define PHASER_STEREO_STAGES	12

class AudioEffectPhaserStereo_F32 : public AudioStream_F32
{
    public:
    AudioEffectPhaserStereo_F32();
    ~AudioEffectPhaserStereo_F32();
    virtual void update();

    /**
     * @brief Scale and offset the modulation signal. It can be the internal LFO
     *          or the incomig routed modulation AudioSignal.
     *          LFO will oscillate between these two max and min values. 
     * 
     * @param top       top level of the LFO
     * @param bottom     bottom level of the LFO
     */
    void depth(float32_t top, float32_t bottom)
    {
        float32_t a, b;
        lfo_top = constrain(top, 0.0f, 1.0f);
        lfo_btm = constrain(bottom, 0.0f, 1.0f);
        
        a = abs(lfo_top - lfo_btm); // scaler
        b = min(lfo_top, lfo_btm);  // bias
        __disable_irq();
        lfo_bias = b;
        lfo_scaler = a;
        __enable_irq();
    }
    /**
     * @brief classic way of setting the depth: LFO centered around 0.5
     * 
     * @param dpth modulation wave amplitude
     */
    void depth(float32_t value)
    {
        value = constrain(value, 0.0f, 1.0f);
        value *= 0.5f;
		lfo_top = 0.5f + value;
		lfo_btm = 0.5f - value;
        __disable_irq();
        lfo_bias = 0.5f;
        lfo_scaler = value;
        __enable_irq();        
    }
    void top(float32_t value)
    {
        lfo_top = constrain(value, 0.0f, 1.0f);
		depth(lfo_top, lfo_btm);       
    }
    void btm(float32_t value)
    {
        lfo_btm = constrain(value, 0.0f, 1.0f);
		depth(lfo_top, lfo_btm);       
    }
    /**
     * @brief Controls the internal LFO, or if a control signal is used, scales it
     *          Use this function to update all lfo parameteres at once
     * 
     * @param f_Hz  lfo frequency, use 0.0f for manual phaser control
     * @param phase phase shift between the LFOs L and R waveforms, 0.0-1.0 range
     * @param top   lfo top level 
     * @param btm   lfo bottm level
     */
    void lfo(float32_t f_Hz, float32_t phase, float32_t top, float32_t btm)
    {
        float32_t a, b, c;
        uint32_t add;
        uint8_t bs;

        a = constrain(top, 0.0f, 1.0f);
        b = constrain(btm, 0.0f, 1.0f);
        c = abs(a - b); // scaler
        a = min(a, b);  // bias
        f_Hz = constrain(f_Hz, 0.0f, AUDIO_SAMPLE_RATE_EXACT/2);
        phase = constrain(phase, 0.0f, 1.0f);
        add = f_Hz * (4294967296.0f / AUDIO_SAMPLE_RATE_EXACT);
        bs = (uint8_t)(phase * 128.0f);
        __disable_irq();
        lfo_scaler = c;
        lfo_bias = a;
        lfo_add = add;
        lfo_lroffset = bs;
        __enable_irq();
    }
    void stereo(float32_t phase)
    {
        uint8_t bs;
        phase = constrain(phase, 0.0f, 1.0f);
        bs = (uint8_t)(phase * 128.0f);
        __disable_irq();
        lfo_lroffset = bs;
        __enable_irq();
    }

    /**
     * @brief Set the rate of the internal LFO
     * 
     * @param f_Hz lfo frequency, use 0.0f for manual phaser control
     */
    void lfo_rate(float32_t f_Hz)
    {
        float32_t c;
        uint32_t add;
        c = constrain(f_Hz, 0.0f, AUDIO_SAMPLE_RATE_EXACT/2);
        add = c * (4294967296.0 / AUDIO_SAMPLE_RATE_EXACT);
        __disable_irq();
        lfo_add = add;
        __enable_irq();
    }
    /**
     * @brief Controls the feedback parameter
     * 
     * @param fdb ffedback value in range 0.0f to 1.0f
     */
    void feedback(float32_t fdb)
    {
        feedb = constrain(fdb, -1.0f, 1.0f);
    }
    /**
     * @brief Dry / Wet mixer ratio. Classic Phaser sound uses 0.5f for 50% dry and 50%Wet
     *        1.0f will produce 100% wet signal craeting a vibrato effect
     * 
     * @param ratio mixing ratio, range 0.0f (full dry) to 1.0f (full wet)
     */
    void mix(float32_t ratio)
    {
        mix_ratio = constrain(ratio, 0.0f, 1.0f);
    }
    /**
     * @brief Sets the number of stages used in the phaser
     *        Allowed values are: 2, 4, 6, 8, 10, 12
     * 
     * @param st number of stages, even value <= 12
     */
    void stages(uint8_t st)
    {
        if (st && st == ((st >> 1) << 1) && st <= PHASER_STEREO_STAGES) // only 2, 4, 6, 8, 12 allowed
        {
            stg = st;
        }
    }
    /**
     * @brief Use to bypass the effect (true)
     * 
     * @param state true = bypass on, false = phaser on
     */
    void bypass_set(bool state) {bps = state;}
    bool bypass_tgl(void) {bps ^= 1; return bps;}

private:
    uint8_t stg;                                    // number of stages
    bool bps;                                       // bypass
    audio_block_f32_t *inputQueueArray_f32[3];      
    float32_t allpass_x[2][PHASER_STEREO_STAGES];      // allpass inputs
	float32_t allpass_y[2][PHASER_STEREO_STAGES];      // allpass outputs
	float32_t mix_ratio;                            // 0 = dry. 1.0 = wet
    float32_t feedb;                                // feedback 
    float32_t last_sampleL;
    float32_t last_sampleR;
    uint32_t lfo_phase_acc;                         // interfnal lfo 
    uint32_t lfo_add;
    float32_t lfo_lrphase;
    uint32_t lfo_lroffset;
    float32_t lfo_scaler;
    float32_t lfo_bias;
	float32_t lfo_top;
	float32_t lfo_btm;
};

#endif // _EFFECT_PHASER_H
