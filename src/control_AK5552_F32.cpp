/**
 * @file control_AK5552_F32.cpp
 * @author Piotr Zapart
 * @brief driver for the AK5552 ADC
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
#include "control_AK5552_F32.h"

#define AK5552_REG_PWR_MAN1			(0x00)
#define AK5552_REG_PWR_MAN1_DFLT	(0xFF)
	#define AK5552_BIT_PW1			(1<<0)
	#define AK5552_BIT_PW2			(1<<1)

#define AK5552_REG_PWR_MAN2			(0x01)
#define AK5552_REG_PWR_MAN2_DFLT	(0x01)
	#define AK5552_BIT_RSTN			(1<<0)
	#define AK5552_BIT_MONO1		(1<<1)
	#define AK5552_BIT_MONO2		(1<<2)

#define AK5552_REG_PWR_CTRL1		(0x02)
#define AK5552_REG_PWR_CTRL1_DFLT	(0x01)
	#define AK5552_BIT_HPFE			(1<<0)
	#define AK5552_BIT_DIF0			(1<<1)
	#define AK5552_BIT_DIF1			(1<<2)
	#define AK5552_BIT_CKS0			(1<<3)
	#define AK5552_BIT_CKS1			(1<<4)
	#define AK5552_BIT_CKS2			(1<<5)
	#define AK5552_BIT_CKS3			(1<<6)

#define AK5552_REG_PWR_CTRL2		(0x03)
#define AK5552_REG_PWR_CTRL2_DFLT	(0x00)
	#define AK5552_BIT_TDM0			(1<<5)
	#define AK5552_BIT_TDM1			(1<<6)

#define AK5552_REG_PWR_CTRL3		(0x04)
#define AK5552_REG_PWR_CTRL3_DFLT	(0x00)
	#define AK5552_BIT_SLOW			(1<<0)
	#define AK5552_BIT_SD			(1<<1)

#define AK5552_REG_PWR_DSD			(0x04)
#define AK5552_REG_PWR_DSD_DFLT		(0x00)
	#define AK5552_DSDSEL_64FS		(0x00)
	#define AK5552_DSDSEL_128FS		(0x01)
	#define AK5552_DSDSEL_256FS		(0x10)
	#define AK5552_DSDSEL(x)		((x)&0x03)
	#define AK5552_BIT_DCKB			(1<<2)
	#define AK5552_BIT_PMOD			(1<<3)
	#define AK5552_BIT_DCKS			(1<<5)

bool AudioControlAK5552_F32::configured = false;


bool AudioControlAK5552_F32::enable(TwoWire *i2cBus, uint8_t addr)
{
	ctrlBus = i2cBus;
	i2cAddr = addr;
	ctrlBus->begin();
	ctrlBus->setClock(400000);

	if (!writeReg(AK5552_REG_PWR_MAN2, 0x00))	// put the registers in reset mode
	{
		return false; // codec not found
	}	
	// Normal Speed 256fs, table 5, page 34
	// CKS3=0, CKS2=0, CKS1=1, CKS0=0
	// 32bit I2S, Slave mode, table 8, page 62
	// TDM1=0, TDM0=0, MSN=0, DIF1=1, DIF0=1 
	writeReg(	AK5552_REG_PWR_CTRL1, 	AK5552_BIT_CKS1 | 
										AK5552_BIT_DIF1 | 
										AK5552_BIT_DIF0);
	// enable short delay
	writeReg(AK5552_REG_PWR_CTRL3, AK5552_BIT_SD);
	writeReg(AK5552_REG_PWR_MAN2, AK5552_BIT_RSTN); // register in normal operation
	configured = true;
	return true;
}

bool AudioControlAK5552_F32::writeReg(uint8_t addr, uint8_t val)
{
	ctrlBus->beginTransmission(i2cAddr);
	ctrlBus->write(addr);
	ctrlBus->write(val);
	return ctrlBus->endTransmission() == 0;
}
bool AudioControlAK5552_F32::readReg(uint8_t addr, uint8_t *valPtr)
{
	ctrlBus->beginTransmission(i2cAddr);
	ctrlBus->write(addr);
	if (ctrlBus->endTransmission(false) != 0)
		return false;	
	if (ctrlBus->requestFrom((int)i2cAddr, 1) < 1)  return false;
	*valPtr = ctrlBus->read();
	return true;
}

uint8_t AudioControlAK5552_F32::modifyReg(uint8_t reg, uint8_t val, uint8_t iMask)
{
	uint8_t val1;
	val1 = (readReg(reg, &val1) & (~iMask)) | val;
	if (!writeReg(reg, val1))
		return 0;
	return val1;
}
