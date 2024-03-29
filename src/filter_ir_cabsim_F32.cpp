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
#include "HxFx_memcpy.h"

AudioFilterIRCabsim_F32::AudioFilterIRCabsim_F32() : AudioStream_F32(2, inputQueueArray_f32)
{
	if (!delay.init(delay_l)) return;
	last_sample_buffer_L = (float32_t*)malloc(IR_BUFFER_SIZE * IR_N_B * sizeof(float32_t));
	last_sample_buffer_R = (float32_t*)malloc(IR_BUFFER_SIZE * IR_N_B * sizeof(float32_t));
	maskgen = (float32_t*)malloc(IR_FFT_LENGTH * 2 * sizeof(float32_t));
	fftout = (float32_t*)malloc(IR_NFORMAX * IR_FFT_LENGTH * 2 * sizeof(float32_t));
	if (!last_sample_buffer_L || !last_sample_buffer_R || !maskgen || !fftout) return;

	memset(maskgen, 0, IR_FFT_LENGTH * 2 * sizeof(float32_t));
	memset(fftout, 0, IR_NFORMAX * IR_FFT_LENGTH * 2 * sizeof(float32_t));

	arm_fir_init_f32(&FIR_preL, nfir, (float32_t *)FIRk_preL, &FIRstate[0][0], (uint32_t)block_size);
	arm_fir_init_f32(&FIR_preR, nfir, (float32_t *)FIRk_preR, &FIRstate[1][0], (uint32_t)block_size);
	arm_fir_init_f32(&FIR_postL, nfir, (float32_t *)FIRk_postL, &FIRstate[2][0], (uint32_t)block_size);
	arm_fir_init_f32(&FIR_postR, nfir, (float32_t *)FIRk_postR, &FIRstate[3][0], (uint32_t)block_size);
	initialized = true;
}

void AudioFilterIRCabsim_F32::update()
{
#if defined(__IMXRT1062__)
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
#ifdef USE_IR_ISR_LOAD
	switch(ir_loadState)
	{
		case IR_LOAD_START:
			ptr_fmask = &fmask[0][0];
			ptr_fftout = &fftout[0];
			memset(ptr_fftout, 0, nfor*512*4);  // clear fftout array
			memset(fftin, 0,  512 * 4);  // clear fftin array
			ir_loadState = IR_LOAD_STEP1;	
			break;
		case IR_LOAD_STEP1:
			init_partitioned_filter_masks(irPtrTable[ir_idx]);
			ir_loadState = IR_LOAD_STEP2;
			break;
		case IR_LOAD_STEP2:
			delay.reset();
			ir_loaded = 1;
			ir_loadState = IR_LOAD_FINISHED;
			break;
		case IR_LOAD_FINISHED:
		default: break;
	}

#endif
	if (!ir_loaded) // ir not loaded yet or bypass mode
	{
		// bypass clean signal
		AudioStream_F32::transmit(blockL, 0);
		AudioStream_F32::release(blockL);
		AudioStream_F32::transmit(blockR, 1);
		AudioStream_F32::release(blockR);
		return;
	}

	if (first_block) // fill real & imaginaries with zeros for the first BLOCKSIZE samples
	{
		memset(&fftin[0], 0, blockL->length*sizeof(float32_t)*4);
		first_block = 0;
	}
	else
	{
		memcpyInterleave_f32(last_sample_buffer_L, last_sample_buffer_R, fftin, blockL->length);
	}
	if (doubleTrack)
	{
		arm_fir_f32(&FIR_preL, blockL->data, blockL->data, blockL->length);
		arm_fir_f32(&FIR_preR, blockR->data, blockR->data, blockR->length);
		// invert phase for channel R
		arm_scale_f32(blockR->data, -1.0f, blockR->data, blockR->length);
		// run channelR delay
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
		{ 
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
		if (k < 0)  k = nfor - 1;
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
	// apply post EQ, restore the channel R phase, reduce the gain a bit
	if (doubleTrack)  
	{
		arm_fir_f32(&FIR_postL, blockL->data, blockL->data, blockL->length);
		arm_fir_f32(&FIR_postR, blockR->data, blockR->data, blockR->length);
		arm_scale_f32(blockR->data, -doubler_gainR, blockR->data, blockR->length);
		arm_scale_f32(blockL->data, doubler_gainL, blockL->data, blockL->length);		
	}
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
#ifdef USE_IR_ISR_LOAD
	nc = newIrPtr[0];
	uint32_t _nfor = nc / IR_BUFFER_SIZE;
	if (_nfor > nforMax) _nfor = nforMax;
	__disable_irq()
	nfor = _nfor;
	ir_loadState = IR_LOAD_START;
	__enable_irq();
	ir_length_ms =  (1000.0f * nfor * (float32_t)AUDIO_BLOCK_SAMPLES) / AUDIO_SAMPLE_RATE_EXACT;
#else
	
	AudioNoInterrupts();
	nc = newIrPtr[0];
	nfor = nc / IR_BUFFER_SIZE;
	if (nfor > nforMax) nfor = nforMax;
	ir_length_ms =  (1000.0f * nfor * (float32_t)AUDIO_BLOCK_SAMPLES) / AUDIO_SAMPLE_RATE_EXACT;
	ptr_fmask = &fmask[0][0];
	ptr_fftout = &fftout[0];
	memset(ptr_fftout, 0, nfor*512*4);  // clear fftout array
	memset(fftin, 0,  512 * 4);  // clear fftin array

	init_partitioned_filter_masks(newIrPtr);	

	delay.reset();
	ir_loaded = 1;
	AudioInterrupts();
	#endif

}


void AudioFilterIRCabsim_F32::init_partitioned_filter_masks(const float32_t *irPtr)
{
	const static arm_cfft_instance_f32 *maskS;
	maskS = &arm_cfft_sR_f32_len256;
	for (uint32_t j = 0; j < nfor; j++)
	{
		memset(&maskgen[0], 0, IR_BUFFER_SIZE * 4 *sizeof(float32_t));
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
