/**
 * @file control_AK4452_F32.h
 * @author Piotr Zapart
 * @brief driver for the AK4452 DAC
 * @version 0.1
 * @date 2024-06-14
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
#ifndef _CONTROL_AK4558_F32_H_
#define _CONTROL_AK4558_F32_H_
#include <Arduino.h>
#include <Wire.h>
#include "AudioControl.h"

#define AK4558_ADDR00	(0x10)
#define AK4558_ADDR01	(0x11)
#define AK4558_ADDR10	(0x12)
#define AK4558_ADDR11	(0x13)


class AudioControlAK4558_F32 : public AudioControl
{
public:
	AudioControlAK4558_F32(int8_t pdown=-1, bool mclk_present=true)
	{
		pdown_pin = pdown;
		mclk_used = mclk_present;
	};
	~AudioControlAK4558_F32(){};
	bool enable()
	{
		return enable(&Wire, AK4558_ADDR00, true);
	}
	bool enable(TwoWire *i2cBus, uint8_t addr, bool mclk);
	bool volume(float volume);
	// not used but required by AudioControl
	bool disable() {return true;}
	bool inputLevel(float volume) {return true;}
	bool inputSelect(int n) {return true;}
	bool DCblocker_en(bool state);

private:
	static bool configured;
	TwoWire *ctrlBus;
	uint8_t i2cAddr;
	int8_t pdown_pin;
	bool mclk_used;
	bool writeReg(uint8_t addr, uint8_t val);
	bool writeRegSeq(uint8_t addr, uint8_t *data, uint8_t sz);
	bool readReg(uint8_t addr, uint8_t* valPtr);
	uint8_t modifyReg(uint8_t reg, uint8_t val, uint8_t iMask);
};

#endif // _CONTROL_AK4452_H_
