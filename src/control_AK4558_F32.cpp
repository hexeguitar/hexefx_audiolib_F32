/**
 * @file control_AK4558_F32.cpp
 * @author Piotr Zapart
 * @brief driver for the AK4558 codec 
 * @version 0.1
 * @date 2024-09-08
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
#include "control_AK4558_F32.h"

#define AK45582_REG_PWR_MNGT					(0x00)
#define AK45582_REG_PWR_MNGT_DFLT				(0x01)
		#define AK4558_BIT_RSTN					(1<<0)
		#define AK4558_BIT_PMDAL				(1<<1)
		#define AK4558_BIT_PMDAR				(1<<2)
		#define AK4558_BIT_PMADL				(1<<3)
		#define AK4558_BIT_PMADR				(1<<4)
		#define AK4558_BIT_PMALL				(0x1F)

#define AK45582_REG_PLL_CTRL					(0x01)
#define AK45582_REG_PLL_CTRL_DFLT				(0x04)
	#define AK4558_BIT_PMPLL					(1<<0)
	#define AK4558_PLLCLK_MASK					(0x1E)	//PLL Reference Clock Select
	#define AK4558_PLLCLK_SHIFT					(0x01)
	#define AK4558_PLLCLK(x)					(((x)<<AK4558_PLLCLK_SHIFT)&AK4558_PLLCLK_MASK)
		#define AK4558_PLLCLK_BCK_256FS			(0x00)
		#define AK4558_PLLCLK_BCK_128FS			(0x01)
		#define AK4558_PLLCLK_BCK_64FS			(0x02)  	
		#define AK4558_PLLCLK_BCK_32FS			(0x03)
		#define AK4558_PLLCLK_MCK_11_2896MHZ	(0x04)
		#define AK4558_PLLCLK_MCK_12_288MHZ		(0x05)
		#define AK4558_PLLCLK_MCK_12MHZ			(0x06)
		#define AK4558_PLLCLK_MCK_24MHZ			(0x07)
		#define AK4558_PLLCLK_MCK_19_2MHZ		(0x08)
		#define AK4558_PLLCLK_MCK_13MHZ			(0x0A)
		#define AK4558_PLLCLK_MCK_26MHZ			(0x0B)
		#define AK4558_PLLCLK_MCK_13_5MHZ		(0x0C)
		#define AK4558_PLLCLK_MCK_27MHZ			(0x0D)
		#define AK4558_PLLCLK_LRCLK_1FS			(0x0F)

#define AK45582_REG_DAC_TDM						(0x02)
#define AK45582_REG_DAC_TDM_DFLT				(0x00)
	#define AK45582_BIT_SDS0					(1<<0)
	#define AK45582_BIT_SDS1					(1<<1)	

#define AK45582_REG_CTRL1						(0x03)
#define AK45582_REG_CTRL1_DFLT					(0x38)
	#define AK45582_BIT_SMUTE					(1<<0)
	#define AK45582_ATS_MASK					(0x06)	
	#define AK45582_ATS_SHIFT					(0x01)
	#define AK4558_ATS(x)						(((x)<<AK45582_ATS_SHIFT)&AK45582_ATS_MASK)
		#define AK4558_ATS_4080_DIV_FS			(0x00)
		#define AK4558_ATS_2040_DIV_FS			(0x01)
		#define AK4558_ATS_510_DIV_FS			(0x02)
		#define AK4558_ATS_255_DIV_FS			(0x03)
	#define AK45582_DIF_MASK					(0x18)	
	#define AK45582_DIF_SHIFT					(0x03)
	#define AK4558_DIF(x)						(((x)<<AK45582_DIF_SHIFT)&AK45582_DIF_MASK)
	#define AK45582_TDM_MASK					(0xC0)	
	#define AK45582_TDM_SHIFT					(0x06)
	#define AK4558_TDM(x)						(((x)<<AK45582_TDM_SHIFT)&AK45582_TDM_MASK)

#define AK45582_REG_CTRL2						(0x04)
#define AK45582_REG_CTRL2_DFLT					(0x10)
	#define AK45582_BIT_ACKS					(1<<0)
	#define AK45582_DFS_MASK					(0x06)	
	#define AK45582_DFS_SHIFT					(0x01)
	#define AK4558_DFS(x)						(((x)<<AK45582_DFS_SHIFT)&AK45582_DFS_MASK)
		#define AK4558_DFS_NORMAL				(0x00)
		#define AK4558_DFS_DOUBLE				(0x01)
		#define AK4558_DFS_QUAD					(0x02)
	#define AK45582_MCKS_MASK					(0x18)	
	#define AK45582_MCKS_SHIFT					(0x03)
	#define AK4558_MCKS(x)						(((x)<<AK45582_MCKS_SHIFT)&AK45582_MCKS_MASK)
		#define AK4558_MCKS_256_256_128FS		(0x00)
		#define AK4558_MCKS_384_256_128FS		(0x01)
		#define AK4558_MCKS_512_256_128FS		(0x02)
		#define AK4558_MCKS_768_256_128FS		(0x03)	

#define AK45582_REG_MODECTRL					(0x05)
#define AK45582_REG_MODECTRL_DFLT				(0x2A)
	#define AK45582_BIT_LOPS					(1<<0)
	#define AK45582_BCKO_MASK					(0x06)	
	#define AK45582_BCKO_SHIFT					(0x01)
	#define AK4558_BCKO(x)						(((x)<<AK45582_BCKO_SHIFT)&AK45582_BCKO_MASK)
	#define AK45582_FS_MASK						(0x78)	
	#define AK45582_FS_SHIFT					(0x03)
	#define AK4558_FS(x)						(((x)<<AK45582_FS_SHIFT)&AK45582_FS_MASK)
		#define AK4558_FS_8_13KHZ				(0x00)
		#define AK4558_FS_12_27KHZ				(0x01)
		#define AK4558_FS_24_54KHZ				(0x02)
		#define AK4558_FS_48_108KHZ				(0x03)
		#define AK4558_FS_96_216KHZ				(0x04)

#define AK45582_REG_FILTER_SETT					(0x06)
#define AK45582_REG_FILTER_SETT_DFLT			(0x29)
	#define AK45582_DEM_MASK					(0x03)	
	#define AK45582_DEM_SHIFT					(0x00)
	#define AK4558_DEM(x)						(((x)<<AK45582_DEM_SHIFT)&AK45582_DEM_MASK)
		#define AK4558_DEM_44_1KHZ				(0x00)
		#define AK4558_DEM_OFF					(0x01)
		#define AK4558_DEM_48KHZ				(0x02)
		#define AK4558_DEM_32KHZ				(0x03)
	#define AK45582_BIT_SSLOW					(1<<2)
	#define AK45582_BIT_SDDA					(1<<3)
	#define AK45582_BIT_SLDA					(1<<4)
	#define AK45582_FIRDA_MASK					(0xE0)	
	#define AK45582_FIRDA_SHIFT					(0x05)
	#define AK4558_FIRDA(x)						(((x)<<AK45582_FIRDA_SHIFT)&AK45582_FIRDA_MASK)

#define AK45582_REG_HPF_EN_FILTER_SETT			(0x07)
#define AK45582_REG_HPF_EN_FILTER_SETT_DFLT		(0x03)
	#define AK45582_BIT_HPFEL					(1<<0)	
	#define AK45582_BIT_HPFER					(1<<1)
	#define AK45582_BIT_HPFENABLE				(0x03)
	#define AK45582_BIT_SDAD					(1<<2)
	#define AK45582_BIT_SLAD					(1<<3)

#define AK45582_REG_LOUT_VOL					(0x08)
#define AK45582_REG_LOUT_VOL_DFLT				(0xFF)

#define AK45582_REG_ROUT_VOL					(0x09)
#define AK45582_REG_ROUT_VOL_DFLT				(0xFF)


bool AudioControlAK4558_F32::configured = false;

/**
 * @brief Configure the codec. The default startup register values
 * 		match the desired configuration: 
 * 		MCLK = 256Fs
 * 		BCK = 64Fs
 * 		32bit I2S format, no TDM, not using PLL, slave mode
 * 
 * @param i2cBus bus the codec chip is on (Wire, Wire1 etc)
 * @param addr 	slave address
 * @return true success
 * @return false fail (codec not found/i2c error)
 */
FLASHMEM bool AudioControlAK4558_F32::enable(TwoWire *i2cBus, uint8_t addr, bool mclk)
{
	configured = false;
	ctrlBus = i2cBus;
	i2cAddr = addr;
	ctrlBus->begin();
	ctrlBus->setClock(400000);
	if (pdown_pin >= 0)
	{
		pinMode(pdown_pin, OUTPUT);
		digitalWrite(pdown_pin, LOW);
		delay(1);
		digitalWrite(pdown_pin, HIGH);
		delay(20);
	}
	/**
	 * @brief if MCLK is not present, lets configure the PLL to generate it
	 * 		from the supplied BCK
	 * 		PMPLL register @ 0x01
	 * 		PMPLL = 1 (PLL Mode and Power up)
	 * 		PLL0-3 = 0b0010 (default) BICK = 64Fs
	 */
	if (!mclk)
	{
		if (!writeReg(AK45582_REG_PLL_CTRL, AK45582_REG_PLL_CTRL_DFLT | AK4558_BIT_PMPLL))
			return false;
	}
	// CKS = 0b0101 - 256Fs, 32bit I2S, HPF on
	// CKS = 0b0111 - 256Fs. 32bit I2S, HPF off
	// TDM = 0b111 = 32bit I2s
	// Power up the ADC+DAC
	if (!writeReg(AK45582_REG_PWR_MNGT, AK45582_REG_PWR_MNGT_DFLT | AK4558_BIT_PMALL))
		return false;
	configured = true;
	return true;
}
/**
 * @brief Switch the ADC DC blocking HP filter on or off
 * 
 * @param state true = filter on
 * @return true success
 * @return false fail (i2c error)
 */
FLASHMEM bool AudioControlAK4558_F32::DCblocker_en(bool state)
{

	return true;
}
 
FLASHMEM bool AudioControlAK4558_F32::volume(float volume)  
{
	uint8_t vol[2];
	vol[0] = vol[1] = uint8_t(volume * 255.0f); 
	return writeRegSeq(AK45582_REG_LOUT_VOL, vol, 2);;
};

FLASHMEM bool AudioControlAK4558_F32::writeReg(uint8_t addr, uint8_t val)
{
	ctrlBus->beginTransmission(i2cAddr);
	ctrlBus->write(addr);
	ctrlBus->write(val);
	return ctrlBus->endTransmission() == 0;
}

FLASHMEM bool AudioControlAK4558_F32::writeRegSeq(uint8_t addr, uint8_t *data, uint8_t sz)
{
	ctrlBus->beginTransmission(i2cAddr);
	ctrlBus->write(addr);
	while (sz)
	{
		ctrlBus->write(*data++);
		sz--;
	}
	return ctrlBus->endTransmission() == 0;
}



FLASHMEM bool AudioControlAK4558_F32::readReg(uint8_t addr, uint8_t *valPtr)
{
	ctrlBus->beginTransmission(i2cAddr);
	ctrlBus->write(addr);
	if (ctrlBus->endTransmission(false) != 0)
		return false;	
	if (ctrlBus->requestFrom((int)i2cAddr, 1) < 1)  return false;
	*valPtr = ctrlBus->read();
	return true;
}

FLASHMEM uint8_t AudioControlAK4558_F32::modifyReg(uint8_t reg, uint8_t val, uint8_t iMask)
{
	uint8_t val1;
	val1 = (readReg(reg, &val1) & (~iMask)) | val;
	if (!writeReg(reg, val1))
		return 0;
	return val1;
}
