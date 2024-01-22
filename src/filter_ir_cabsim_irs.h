/**
 * @file filter_ir_cabsim_irs.h
 * @author Piotr Zapart www.hexefx.com
 * @brief Guitar / Bass cabinet impulse responses
 * @version 0.1
 * @date 2024-01-22
 * 
 * @copyright Copyright (c) 2024
 *
 * This program is free software: you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. 
 * If not, see <https://www.gnu.org/licenses/>."
 */
#ifndef _FILTER_IR_CABSIM_IRS_H
#define _FILTER_IR_CABSIM_IRS_H

#include <arm_math.h>

extern const float32_t ir_1_guitar[];  // guitar cabs
extern const float32_t ir_2_guitar[];  //  
extern const float32_t ir_3_guitar[];
extern const float32_t ir_4_guitar[];
extern const float32_t ir_5_guitar[];
extern const float32_t ir_6_guitar[];
extern const float32_t ir_7_bass[];  // bass cab
extern const float32_t ir_8_bass[];
extern const float32_t ir_9_bass[];
extern const float32_t ir_10_guitar[];
extern const float32_t ir_11_guitar[];


#endif // _FILTER_IR_CABSIM_IRS_H
