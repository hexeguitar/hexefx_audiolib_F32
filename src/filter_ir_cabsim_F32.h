/* filter_ir_cabsim.h
 *
 * Piotr Zapart 01.2024 www.hexefx.com
 *  - Combined into a stereo speaker simulator with included set of IRs.
 *  - Added stereo enhancer for double tracking emulation
 * 
 * based on:
 *               A u d i o FilterConvolutionUP
 * Uniformly-Partitioned Convolution Filter for Teeny 4.0
 * Written by Brian Millier November 2019
 * adapted from routines written for Teensy 4.0 by Frank DD4WH 
 * that were based upon code/literature by Warren Pratt 
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
#ifndef _FILTER_IR_CABSIM_H
#define _FILTER_IR_CABSIM_H

#include <Arduino.h>
#include <Audio.h>
#include "AudioStream.h"
#include "AudioStream_F32.h"
#include "arm_math.h"
#include "filter_ir_cabsim_irs.h"
#include "basic_allpass.h"
#include "basic_delay.h"
#include "basic_shelvFilter.h"

#define IR_BUFFER_SIZE  128
#define IR_NFORMAX      (8192 / IR_BUFFER_SIZE)
#define IR_FFT_LENGTH   (2 * IR_BUFFER_SIZE)
#define IR_N_B          (1)
#define IR_MAX_REG_NUM  11       // max number of registered IRs

class AudioFilterIRCabsim_F32 : public AudioStream_F32
{
public:
    AudioFilterIRCabsim_F32();
    virtual void update(void);
    void ir_register(const float32_t *irPtr, uint8_t position);
    void ir_load(uint8_t idx);
    uint8_t ir_get(void) {return ir_idx;} 
    float ir_get_len_ms(void)
    {
        float32_t slen = 0.0f;
        if (irPtrTable[ir_idx])      slen = irPtrTable[ir_idx][0];
        return (slen / AUDIO_SAMPLE_RATE_EXACT)*1000.0f;
    }
	bool doubler_tgl()
	{
		__disable_irq();
		doubleTrack = doubleTrack ? false : true;
		if (doubleTrack) 
		{
			delay.reset();
		}
		__enable_irq();
		return doubleTrack;
	}

private:
    audio_block_f32_t *inputQueueArray_f32[2];
    float32_t audio_gain = 0.3f;
    int idx_t = 0;
    int16_t *sp_L;
    int16_t *sp_R;
    const uint32_t FFT_L = IR_FFT_LENGTH;
    uint8_t first_block = 1;
    uint8_t ir_loaded = 0;  
    uint8_t ir_idx = 0xFF;
    uint32_t nfor = 0;
    const uint32_t nforMax = IR_NFORMAX;

    int buffidx = 0;
    int k = 0;

	float32_t *ptr_fftout;
	float32_t *ptr_fmask;
	float32_t* ptr1;
	float32_t* ptr2;
	int k512;
	int j512;
	
    uint32_t N_BLOCKS = IR_N_B;

	static const uint32_t delay_l = AUDIO_SAMPLE_RATE * 0.01277f; 	//15ms delay
	AudioBasicDelay<delay_l> delay;

	// default IR table, use NULL for bypass
    const float32_t *irPtrTable[IR_MAX_REG_NUM] = 
    {
        ir_1_guitar, ir_2_guitar, ir_3_guitar, ir_4_guitar, ir_10_guitar, ir_11_guitar, ir_6_guitar, ir_7_bass,  ir_8_bass, ir_9_bass, NULL
    };
    void init_partitioned_filter_masks(const float32_t *irPtr);
	bool initialized = false;
	bool doubleTrack = true;
}; 


#endif // _FILTER_IR_CONVOLVER_H
