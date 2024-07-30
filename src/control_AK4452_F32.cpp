/**
 * @file control_AK4452_F32.cpp
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
#include "control_AK4452_F32.h"

#define AK4452_REG_CTRL1			(0x00)
#define AK4452_REG_CTRL1_DFLT		(0x0C)
	#define AK4452_BIT_RSTN			(1<<0)
	#define AK4452_DIF_MASK			(0x0E)
	#define AK4452_DIF_SHIFT		(0x01)
	#define AK4452_DIF(x)			(((x)<<AK4452_DIF_SHIFT)&AK4452_DIF_MASK)
		#define AK4452_DIF_16B_LSB	(0x00)
		#define AK4452_DIF_20B_LSB	(0x01)
		#define AK4452_DIF_24B_MSB	(0x02)
		#define AK4452_DIF_24B_I2S	(0x03)
		#define AK4452_DIF_24B_LSB	(0x04)
		#define AK4452_DIF_32B_LSB	(0x05)
		#define AK4452_DIF_32B_MSB	(0x06)
		#define AK4452_DIF_32B_I2S	(0x07)
	#define AK4452_BIT_ACKS			(1<<7)

#define AK4452_REG_CTRL2			(0x01)
#define AK4452_REG_CTRL2_DFLT		(0x22)
	#define AK4452_BIT_SMUTE		(1<<0)
	#define AK4452_DEM_MASK			(0x06)
	#define AK4452_DEM_SHIFT		(0x01)
	#define AK4452_DEM(x)			(((x)<<AK4452_DEM_SHIFT)&AK4452_DEM_MASK)
		#define AK4452_DEM_44_1K	(0x00)
		#define AK4452_DEM_OFF		(0x01)
		#define AK4452_DEM_48K		(0x02)
		#define AK4452_DEM_32K		(0x03)
	#define AK4452_DFS10_MASK		(0x18)
	#define AK4452_DFS10_SHIFT		(0x03)
	#define AK4452_DFS10(x)			(((x)<<AK4452_DFS10_SHIFT)&AK4452_DFS10_MASK)
		#define AK4452_DFS_NORMAL	(0x00)
		#define AK4452_DFS_DOUBLE	(0x01)
		#define AK4452_DFS_QUAD		(0x02)
		#define AK4452_DFS_OCT		(0x04)
		#define AK4452_DFS_HEX		(0x05)
	#define AK4452_BIT_SD			(1<<5)	

#define AK4452_REG_CTRL3			(0x02)
#define AK4452_REG_CTRL3_DFLT		(0x00)	
	#define AK4452_BIT_SLOW			(1<<0)
	#define AK4452_BIT_SELLR1		(1<<1)
	#define AK4452_BIT_DZFB			(1<<2)
	#define AK4452_BIT_MONO1		(1<<3)
	#define AK4452_BIT_DCKB			(1<<4)
	#define AK4452_BIT_DCKS			(1<<5)
	#define AK4452_BIT_DP			(1<<7)

#define AK4452_REG_L1CH_ATT			(0x03)
#define AK4452_REG_L1CH_ATT_DFLT	(0xFF)
#define AK4452_REG_R1CH_ATT			(0x04)
#define AK4452_REG_R1CH_ATT_DFLT	(0xFF)

#define AK4452_REG_CTRL4			(0x05)
#define AK4452_REG_CTRL4_DFLT		(0x00)
	#define AK4452_BIT_SSLOW		(1<<0)
	#define AK4452_DFS2_MASK		(1<<1)
	#define AK4452_DFS2_SHIFT		(0x01)
	#define AK4452_DFS2(x)			(((x)<<AK4452_DFS2_SHIFT)&AK4452_DFS2_MASK) // bits10 in CTRL2
	#define AK4452_BIT_INVL1		(1<<6)
	#define AK4452_BIT_INVR1		(1<<7)

#define AK4452_REG_DSD1				(0x06)
#define AK4452_REG_DSD1_DFLT		(0x00)
	#define AK4452_BIT_DSDSEL0		(1<<0)
	#define AK4452_BIT_DSDD			(1<<1)
	#define AK4452_BIT_DMRE			(1<<3)
	#define AK4452_BIT_DMC			(1<<4)
	#define AK4452_BIT_DMR1			(1<<5)
	#define AK4452_BIT_DML1			(1<<6)
	#define AK4452_BIT_DDM			(1<<7)

#define AK4452_REG_CTRL5			(0x07)
#define AK4452_REG_CTRL5_DFLT		(0x03)
	#define AK4452_BIT_SYNCE		(1<<0)

#define AK4452_REG_SND_CTRL			(0x08)
#define AK4452_REG_SND_CTRL_DFLT	(0x00)
	#define AK4452_SC_MASK			(0x03)
	#define AK4452_SC_SHIFT			(0x00)
	#define AK4452_SC(x)			(((x)<<AK4452_SC_SHIFT)&AK4452_SC_MASK) 
		#define AK4452_SC_NORMAL	(0x00)
		#define AK4452_SC_MAX		(0x01)
		#define AK4452_SC_MIN		(0x02)
	#define AK4452_BIT_DZF_L1		(1<<6)
	#define AK4452_BIT_DZF_R1		(1<<7)

#define AK4452_REG_DSD2				(0x09)
#define AK4452_REG_DSD2_DFLT		(0x00)
	#define AK4452_BIT_DSDSEL1		(1<<0)
	#define AK4452_BIT_DSDF			(1<<1)

#define AK4452_REG_CTRL6			(0x0A)
#define AK4452_REG_CTRL6_DFLT		(0x0D)
	#define AK4452_BIT_PW1			(1<<2)
	#define AK4452_SDS21_MASK		(0x30)
	#define AK4452_SDS21_SHIFT		(0x04)
	#define AK4452_SDS21(x)			(((x)<<AK4452_SDS21_SHIFT)&AK4452_SDS21_MASK) 
		#define AK4452_SDS_TDM128_L1R1	(0x00)
		#define AK4452_SDS_TDM128_L2R2	(0x01)
		#define AK4452_SDS_TDM256_L1R1	(0x00)
		#define AK4452_SDS_TDM256_L2R2	(0x01)
		#define AK4452_SDS_TDM256_L3R3	(0x02)
		#define AK4452_SDS_TDM256_L4R4	(0x03)
		#define AK4452_SDS_TDM512_L1R1	(0x00)
		#define AK4452_SDS_TDM512_L2R2	(0x01)
		#define AK4452_SDS_TDM512_L3R3	(0x02)
		#define AK4452_SDS_TDM512_L4R4	(0x03)
		#define AK4452_SDS_TDM512_L5R5	(0x04)
		#define AK4452_SDS_TDM512_L6R6	(0x05)
		#define AK4452_SDS_TDM512_L7R7	(0x06)
		#define AK4452_SDS_TDM512_L8R8	(0x07)
	#define AK4452_TDM_MASK			(0xC0)
	#define AK4452_TDM_SHIFT		(0x06)
	#define AK4452_TDM(x)			(((x)<<AK4452_TDM_SHIFT)&AK4452_TDM_MASK) 
		#define AK4452_TDM_NORMAL	(0x00)
		#define AK4452_TDM_128		(0x01)
		#define AK4452_TDM_246		(0x02)
		#define AK4452_TDM_512		(0x03)

#define AK4452_REG_CTRL7			(0x0B)
#define AK4452_REG_CTRL7_DFLT		(0x0C)
	#define AK4452_BIT_DCHAIN		(1<<2)
	#define AK4452_SDS0_MASK		(1<<4)
	#define AK4452_SDS0_SHIFT		(0x04)
	#define AK4452_SDS0(x)			(((x)<<AK4452_SDS0_SHIFT)&AK4452_SDS0_MASK) 
	#define AK4452_ATS_MASK			(0xC0)
	#define AK4452_ATS_SHIFT		(0x06)
	#define AK4452_ATS(x)			(((x)<<AK4452_ATS_SHIFT)&AK4452_ATS_MASK)
		#define AK4452_ATS_4080		(0x00)
		#define AK4452_ATS_2040		(0x01)
		#define AK4452_ATS_510		(0x02)
		#define AK4452_ATS_255		(0x03)

#define AK4452_REG_CTRL8			(0x0C)
#define AK4452_REG_CTRL8_DFLT		(0x00)
	#define AK4452_FIR(x)			((x)&0x07)

bool AudioControlAK4452_F32::configured = false;

bool AudioControlAK4452_F32::enable(TwoWire *i2cBus, uint8_t addr)
{
	ctrlBus = i2cBus;
	i2cAddr = addr;
	ctrlBus->begin();
	ctrlBus->setClock(400000);

	if (!writeReg(AK4452_REG_CTRL1, 0x00))	// put the registers in reset mode
	{
		return false; // codec not found
	}	
	// DIF[2:0] = 0b111 - 32bit I2S, Normal mode 
	// DFS[2:0] = 0b000 (default) Normal speed mode
	// DSDSEL[1:0] = 0b10 256fs
	writeReg(AK4452_REG_DSD2, AK4452_BIT_DSDSEL1);
	writeReg(AK4452_REG_CTRL1, AK4452_DIF(AK4452_DIF_32B_I2S) | AK4452_BIT_RSTN);

	configured = true;
	return true;
}

bool AudioControlAK4452_F32::writeReg(uint8_t addr, uint8_t val)
{
	ctrlBus->beginTransmission(i2cAddr);
	ctrlBus->write(addr);
	ctrlBus->write(val);
	return ctrlBus->endTransmission() == 0;
}
bool AudioControlAK4452_F32::readReg(uint8_t addr, uint8_t *valPtr)
{
	ctrlBus->beginTransmission(i2cAddr);
	ctrlBus->write(addr);
	if (ctrlBus->endTransmission(false) != 0)
		return false;	
	if (ctrlBus->requestFrom((int)i2cAddr, 1) < 1)  return false;
	*valPtr = ctrlBus->read();
	return true;
}

uint8_t AudioControlAK4452_F32::modifyReg(uint8_t reg, uint8_t val, uint8_t iMask)
{
	uint8_t val1;
	val1 = (readReg(reg, &val1) & (~iMask)) | val;
	if (!writeReg(reg, val1))
		return 0;
	return val1;
}
