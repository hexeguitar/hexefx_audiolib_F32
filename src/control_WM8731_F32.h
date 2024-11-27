/**
 * @file control_WM8731_ext.h
 * @author Piotr Zapart
 * @brief Alternative WM8731 driver with configurable bit depth
 * @version 1.0
 * @date 2024-03-21
 * 
 * @copyright Copyright (c) 2024 www.hexefx.com
 * This program is free software: you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. 
 * If not, see <https://www.gnu.org/licenses/>."
 */
#ifndef _CONTROL_WM8731_F32_H_
#define _CONTROL_WM8731_F32_H_

#include <Arduino.h>
#include <Wire.h>
#include "AudioControl.h"

#define WM8731_I2C_ADDR_CSB0 0x1A
#define WM8731_I2C_ADDR_CSB1 0x1B

class AudioControlWM8731_F32 : public AudioControl
{
public:	
	AudioControlWM8731_F32() {}
	
	typedef enum
	{ 
		I2S_BITS_16 = 0,
		I2S_BITS_20,
		I2S_BITS_24,
		I2S_BITS_32
	}bit_depth_t;

	bool enable()
	{
		return enable(I2S_BITS_32, &Wire, WM8731_I2C_ADDR_CSB0);
	}
	bool enable(bit_depth_t bits, TwoWire *i2cBus, uint8_t addr);
	bool disable(void) { return false; }
	bool volume(float n) { return hp_volumeInteger(n * 80.0f + 47.499f); }
	bool inputLevel(float n); 												// range: 0.0f to 1.0f
	bool inputLevelRaw(uint8_t n); 											// direct value 0-31
	bool inputSelect(int n=AUDIO_INPUT_LINEIN);

	void lineIn_mute(bool m);
	void dry_enable(bool en); // bypass without muting the DAC
	void hp_filter(bool state);
	void dcbias_store(bool state);
	void dac_mute(bool m);
	void dac_enable(bool en);
	void bypass_set(bool b);
	bool bypass_get() {return bp;}
	bool bypass_tgl() {bp ^= 1; bypass_set(bp); return bp;}

protected:
	bool write(uint16_t regAddr, uint16_t val);
	uint16_t modify(uint16_t reg, uint16_t val, uint16_t iMask);
	bool hp_volumeInteger(uint16_t n); // range: 0x2F to 0x7F
private:
	uint16_t reg[16];
	uint8_t bit_depth = I2S_BITS_32;
	uint8_t i2c_addr;
	bool DACmute = false;
	TwoWire *_wire;
	bool dry_sig = false;
	bool bp = false; // used for analog bypass
};

class AudioControlWM8731_F32_master : public AudioControlWM8731_F32
{
public:
	bool enable(bit_depth_t bits = I2S_BITS_32, TwoWire *i2cBus = &Wire, uint8_t addr=WM8731_I2C_ADDR_CSB0);
private:
	
	uint8_t i2c_addr;
	TwoWire *_wire;
	bool dry_sig = false;
};
#endif // _CONTROL_WM8731_F32_H_
