/* filter_ir_cabsim.cpp
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
#include "filter_ir_cabsim_F32.h"
#include <arm_const_structs.h>
#include "HxFx_memcpy.h"

float32_t DMAMEM maskgen[IR_FFT_LENGTH * 2];
float32_t DMAMEM fmask[IR_NFORMAX][IR_FFT_LENGTH * 2]; //
float32_t DMAMEM fftin[IR_FFT_LENGTH * 2];
float32_t DMAMEM accum[IR_FFT_LENGTH * 2];

float32_t DMAMEM last_sample_buffer_L[IR_BUFFER_SIZE * IR_N_B];
float32_t DMAMEM last_sample_buffer_R[IR_BUFFER_SIZE * IR_N_B];
float32_t DMAMEM fftout[IR_NFORMAX][IR_FFT_LENGTH * 2];
float32_t DMAMEM ac2[512];

const static arm_cfft_instance_f32 *S;
// complex iFFT with the new library CMSIS V4.5
const static arm_cfft_instance_f32 *iS;
// FFT instance for direct calculation of the filter mask
// from the impulse response of the FIR - the coefficients
const static arm_cfft_instance_f32 *maskS;

AudioFilterIRCabsim_F32::AudioFilterIRCabsim_F32() : AudioStream_F32(2, inputQueueArray_f32)
{
	if (!delay.init()) return;
	initialized = true;
}

void AudioFilterIRCabsim_F32::update()
{
#if defined(__ARM_ARCH_7EM__)
	if (!initialized) return;
	audio_block_f32_t *blockL, *blockR;

	blockL = AudioStream_F32::receiveWritable_f32(0);
	blockR = AudioStream_F32::receiveWritable_f32(1);
	if (!blockL || !blockR)
	{
		if (blockL) AudioStream_F32::release(blockL);
		if (blockR) AudioStream_F32::release(blockR);
		return;
	}

	if (!ir_loaded) // ir not loaded yet or bypass mode
	{
		// bypass clean signal
		// TODO: Add bypass signal gain stage to match the processed signal volume
		AudioStream_F32::transmit(blockL, 0);
		AudioStream_F32::release(blockL);
		AudioStream_F32::transmit(blockR, 1);
		AudioStream_F32::release(blockR);
		return;
	}

	if (first_block) // fill real & imaginaries with zeros for the first BLOCKSIZE samples
	{
		arm_fill_f32(0.0f, fftin, blockL->length * 4);
		first_block = 0;
	}
	else
	{
		memcpyInterleave_f32(last_sample_buffer_L, last_sample_buffer_R, fftin, blockL->length);
	}
	if (doubleTrack)
	{
		// invert phase for channel R
		arm_scale_f32(blockR->data, -1.0f, blockR->data, blockR->length);
		// run channelR allpass
		for (int i=0; i<blockR->length; i++)
		{
			blockR->data[i] = delay.process(blockR->data[i]);
			delay.updateIndex();
		}
	}
	arm_copy_f32(blockL->data, last_sample_buffer_L, blockL->length);
	arm_copy_f32(blockR->data, last_sample_buffer_R, blockR->length);

	memcpyInterleave_f32(last_sample_buffer_L, last_sample_buffer_R, fftin + FFT_L, blockL->length); // interleave copy it to fftin at offset FFT_L
	arm_cfft_f32(S, fftin, 0, 1);

	uint32_t buffidx512 = buffidx * 512;
	ptr1 = ptr_fftout + (buffidx512); // set pointer to proper segment of fftout array
	memcpy(ptr1, fftin, 2048);			 // copy 512 samples from fftin to fftout (at proper segment)
	k = buffidx;
	memset(accum, 0, IR_BUFFER_SIZE * 16); // clear accum array
	k512 = k * 512;						   // save 7 k*512 multiplications per inner loop
	j512 = 0;
	for (uint32_t j = 0; j < nfor; j++) // BM np was nfor
	{
		ptr1 = ptr_fftout + k512;
		ptr2 = ptr_fmask + j512;
		// do a complex MAC (multiply/accumulate)
		arm_cmplx_mult_cmplx_f32(ptr1, ptr2, ac2, 256); // This is the complex multiply
		for (int q = 0; q < 512; q = q + 8)
		{ // this is the accumulate
			accum[q] += ac2[q];
			accum[q + 1] += ac2[q + 1];
			accum[q + 2] += ac2[q + 2];
			accum[q + 3] += ac2[q + 3];
			accum[q + 4] += ac2[q + 4];
			accum[q + 5] += ac2[q + 5];
			accum[q + 6] += ac2[q + 6];
			accum[q + 7] += ac2[q + 7];
		}
		k--;
		if (k < 0)
		{
			k = nfor - 1;
		}
		k512 = k * 512;
		j512 += 512;
	} // end np loop
	buffidx++;
	buffidx = buffidx % nfor;

	arm_cfft_f32(iS, accum, 1, 1);

	for (int i = 0; i < blockL->length; i++)
	{
		blockL->data[i] = accum[i * 2 + 0];
		blockR->data[i] = accum[i * 2 + 1];
	}
	// restore the channel R phase
	if (doubleTrack)  arm_scale_f32(blockR->data, -1.0f, blockR->data, blockR->length);
	AudioStream_F32::transmit(blockL, 0);
	AudioStream_F32::release(blockL);
	AudioStream_F32::transmit(blockR, 1);
	AudioStream_F32::release(blockR);
#endif
}

void AudioFilterIRCabsim_F32::ir_register(const float32_t *irPtr, uint8_t position)
{
	if (position >= IR_MAX_REG_NUM)
		return;
	irPtrTable[position] = irPtr;
}

void AudioFilterIRCabsim_F32::ir_load(uint8_t idx)
{
	
	const float32_t *newIrPtr = NULL;
	uint32_t nc = 0;

	if (idx >= IR_MAX_REG_NUM)
		return;
	if (idx == ir_idx)
		return; // load only once
	ir_idx = idx;
	newIrPtr = irPtrTable[idx];
	ir_loaded = 0;
	
	if (newIrPtr == NULL) // bypass
	{
		return;
	}
	AudioNoInterrupts();
	nc = newIrPtr[0];
	nfor = nc / IR_BUFFER_SIZE;
	if (nfor > nforMax) nfor = nforMax;
	ptr_fmask = &fmask[0][0];
	ptr_fftout = &fftout[0][0];
	memset(ptr_fftout, 0, nfor*512*4);  // clear fftout array
	memset(fftin, 0,  512 * 4);  // clear fftin array

	S = &arm_cfft_sR_f32_len256;
	iS = &arm_cfft_sR_f32_len256;
	maskS = &arm_cfft_sR_f32_len256;
	init_partitioned_filter_masks(newIrPtr);	

	delay.reset();
	ir_loaded = 1;
	AudioInterrupts();

	//Serial.printf("Loaded IR+ %d, part count = %d\r\n", ir_idx, nfor);
}

void AudioFilterIRCabsim_F32::init_partitioned_filter_masks(const float32_t *irPtr)
{
	for (uint32_t j = 0; j < nfor; j++)
	{
		arm_fill_f32(0.0f, maskgen, IR_BUFFER_SIZE * 4);
		for (unsigned i = 0; i < IR_BUFFER_SIZE; i++)
		{
			maskgen[i * 2 + IR_BUFFER_SIZE * 2] = irPtr[2 + i + j * IR_BUFFER_SIZE] * irPtr[1]; // added gain!
		}
		arm_cfft_f32(maskS, maskgen, 0, 1);
		for (unsigned i = 0; i < IR_BUFFER_SIZE * 4; i++)
		{
			fmask[j][i] = maskgen[i];
		}
	}
}
