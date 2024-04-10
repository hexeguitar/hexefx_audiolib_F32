#ifndef _CONTROL_ES8388_F32_H_
#define _CONTROL_ES8388_F32_H_

#include <Arduino.h>
#include <Wire.h>
#include "AudioControl.h"

#define ES8388_I2C_ADDR_L	(0x10) 		// CS/ADD pin low
#define ES8388_I2C_ADDR_H	(0x11) 		// CS/ADD pin high

class AudioControlES8388_F32 //: public AudioControl
{
public:
	AudioControlES8388_F32(void){};
	~AudioControlES8388_F32(void){};
	typedef enum
	{
		ES8388_CFG_LINEIN_SINGLE_ENDED = 0,
		ES8388_CFG_LINEIN_DIFF,
	}config_t;	


	bool enable()
	{
		return enable(&Wire, ES8388_I2C_ADDR_L, ES8388_CFG_LINEIN_SINGLE_ENDED);
	}
	bool enable(TwoWire *i2cBus, uint8_t addr,  config_t cfg);
	bool disable(void) { return false; }
	bool volume(float n);
	bool inputLevel(float n); // range: 0.0f to 1.0f


	void set_noiseGate(float thres);
	
	void volume(uint8_t vol);
	uint8_t getOutVol();

	bool setInGain(uint8_t gain);
	uint8_t getInGain();

	bool analogBypass(bool bypass);
	bool analogSoftBypass(bool bypass);
private:
	static bool configured;
	TwoWire *ctrlBus;
	uint8_t i2cAddr;
	uint8_t dacGain;

	bool writeReg(uint8_t addr, uint8_t val);
	bool readReg(uint8_t addr, uint8_t* valPtr);
	uint8_t modifyReg(uint8_t reg, uint8_t val, uint8_t iMask);
	void optimizeConversion(uint8_t range);
};

#endif // _CONTROL_ES8388_F32_H_
