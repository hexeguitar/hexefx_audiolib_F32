#ifndef _CONTROL_SGTL5000_F32_H_
#define _CONTROL_SGTL5000_F32_H_
/**
 * @file control_SGTL5000_ext.h
 * @author Piotr Zapart 
 * @brief enables the bit depth setting for the SGTL5000 codec chip
 * @version 0.1
 * @date 2024-03-20
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

#include <Arduino.h>
#include <control_sgtl5000.h>

class AudioControlSGTL5000_F32 : public AudioControlSGTL5000
{
  //GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node
  public:
    AudioControlSGTL5000_F32(void) {};
	typedef enum
	{ 
		I2S_BITS_32 = 0,
		I2S_BITS_24,
		I2S_BITS_20,
		I2S_BITS_16
	}bit_depth_t;

	void set_bitDepth(bit_depth_t bits);
};

#endif // _CONTROL_SGTL5000_EXT_H_
