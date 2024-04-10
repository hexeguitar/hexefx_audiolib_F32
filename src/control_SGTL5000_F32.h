#ifndef _CONTROL_SGTL5000_F32_H_
#define _CONTROL_SGTL5000_F32_H_
/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
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

/**
 * @file control_SGTL5000_F32.h
 * @author Piotr Zapart
 * @brief enables the bit depth setting for the SGTL5000 codec chip + configurable Wire interface
 * @version 0.1
 * @date 2024-03-20
 */

#include <Arduino.h>
#include <Wire.h>
#include <AudioStream_F32.h>
#include "AudioControl.h"


class AudioControlSGTL5000_F32 : public AudioControl
{
public:
	AudioControlSGTL5000_F32(){};
	void setAddress(uint8_t gpioLevel);
	bool enable();
	bool enable(TwoWire *i2cBus, uint8_t addr=0x0A, const uint32_t extMCLK=0, const uint32_t pllFreq = (4096.0l * AUDIO_SAMPLE_RATE_EXACT)); 
	bool disable(void) { return false; }
	bool volume(float n) { return volumeInteger(n * 129 + 0.499f); }
	bool inputLevel(float volume) {return false;}
	bool muteHeadphone(void) { return write(0x0024, ana_ctrl | (1 << 4)); }
	bool unmuteHeadphone(void) { return write(0x0024, ana_ctrl & ~(1 << 4)); }
	bool muteLineout(void) { return write(0x0024, ana_ctrl | (1 << 8)); }
	bool unmuteLineout(void) { return write(0x0024, ana_ctrl & ~(1 << 8)); }
	bool inputSelect(int n);
	bool headphoneSelect(int n);

	bool volume(float left, float right);
	bool micGain(uint16_t dB);
	bool lineInLevel(uint8_t n) { return lineInLevel(n, n); }
	bool lineInLevel(uint8_t left, uint8_t right);
	uint16_t lineOutLevel(uint8_t n);
	uint16_t lineOutLevel(uint8_t left, uint8_t right);
	uint16_t dacVolume(float n);
	uint16_t dacVolume(float left, float right);
	bool dacVolumeRamp();
	bool dacVolumeRampLinear();
	bool dacVolumeRampDisable();
	uint16_t adcHighPassFilterEnable(void);
	uint16_t adcHighPassFilterFreeze(void);
	uint16_t adcHighPassFilterDisable(void);
	uint16_t audioPreProcessorEnable(void);
	uint16_t audioPostProcessorEnable(void);
	uint16_t audioProcessorDisable(void);
	uint16_t eqFilterCount(uint8_t n);
	uint16_t eqSelect(uint8_t n);
	uint16_t eqBand(uint8_t bandNum, float n);
	void eqBands(float bass, float mid_bass, float midrange, float mid_treble, float treble);
	void eqBands(float bass, float treble);
	void eqFilter(uint8_t filterNum, int *filterParameters);
	uint16_t autoVolumeControl(uint8_t maxGain, uint8_t lbiResponse, uint8_t hardLimit, float threshold, float attack, float decay);
	uint16_t autoVolumeEnable(void);
	uint16_t autoVolumeDisable(void);
	uint16_t enhanceBass(float lr_lev, float bass_lev);
	uint16_t enhanceBass(float lr_lev, float bass_lev, uint8_t hpf_bypass, uint8_t cutoff);
	uint16_t enhanceBassEnable(void);
	uint16_t enhanceBassDisable(void);
	uint16_t surroundSound(uint8_t width);
	uint16_t surroundSound(uint8_t width, uint8_t select);
	uint16_t surroundSoundEnable(void);
	uint16_t surroundSoundDisable(void);
	void killAutomation(void) { semi_automated = false; }
	void setMasterMode(uint32_t freqMCLK_in);
	typedef enum
	{
		I2S_BITS_32 = 0,
		I2S_BITS_24,
		I2S_BITS_20,
		I2S_BITS_16
	}bit_depth_t;
	void set_bitDepth(bit_depth_t bits);

protected:
	bool muted;
	bool volumeInteger(uint16_t n); // range: 0x00 to 0x80
	uint16_t ana_ctrl;
	uint8_t i2c_addr;
	unsigned char calcVol(float n, unsigned char range);
	uint16_t read(uint16_t reg);
	bool write(uint16_t reg, uint16_t val);
	uint16_t modify(uint16_t reg, uint16_t val, uint16_t iMask);
	uint16_t dap_audio_eq_band(uint8_t bandNum, float n);

private:
	bool semi_automated;
	void automate(uint8_t dap, uint8_t eq);
	void automate(uint8_t dap, uint8_t eq, uint8_t filterCount);
	TwoWire *_wire;
};

#endif // _CONTROL_SGTL5000_F32_H_
