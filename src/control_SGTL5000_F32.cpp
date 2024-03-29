#include "control_SGTL5000_F32.h"
#include <Wire.h>

#define CHIP_I2S_CTRL			0x0006
#define CHIP_ADCDAC_CTRL		0x000E

void AudioControlSGTL5000_F32::set_bitDepth(bit_depth_t bits)
{
	uint16_t regTmp = read(CHIP_I2S_CTRL);
	regTmp &= ~(0x30);						// clear bit 5:4 (DLEN)
	regTmp |= ((uint8_t)bits << 4) & 0x30; 	// update DLEN

	write(CHIP_ADCDAC_CTRL, 0x000C); 		// mute DAC
	write(CHIP_I2S_CTRL, regTmp); 			// write new config
	write(CHIP_ADCDAC_CTRL, 0x0000); 		// unmute DAC
}
