/*
 *  ***** output_i2s_f32.cpp  *****
 *
 * Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/*
 *  Extended by Chip Audette, OpenAudio, May 2019
 *  Converted to F32 and to variable audio block length
 *  The F32 conversion is under the MIT License.  Use at your own risk.
 */
// Updated OpenAudio F32 with this version from Chip Audette's Tympan Library Jan 2021 RSL
// Removed old commented out code.  RSL 30 May 2022

#include "output_i2s_ext_F32.h"
#include "basic_DSPutils.h"
#include <arm_math.h>
#include <Audio.h> //to get access to Audio/utlity/imxrt_hw.h...do we really need this??? WEA 2020-10-31

audio_block_f32_t *AudioOutputI2S_ext_F32::block_left_1st = NULL;
audio_block_f32_t *AudioOutputI2S_ext_F32::block_right_1st = NULL;
audio_block_f32_t *AudioOutputI2S_ext_F32::block_left_2nd = NULL;
audio_block_f32_t *AudioOutputI2S_ext_F32::block_right_2nd = NULL;
uint16_t AudioOutputI2S_ext_F32::block_left_offset = 0;
uint16_t AudioOutputI2S_ext_F32::block_right_offset = 0;
bool AudioOutputI2S_ext_F32::update_responsibility = false;
DMAChannel AudioOutputI2S_ext_F32::dma(false);
DMAMEM __attribute__((aligned(32))) static uint64_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];

float AudioOutputI2S_ext_F32::sample_rate_Hz = AUDIO_SAMPLE_RATE;
int AudioOutputI2S_ext_F32::audio_block_samples = AUDIO_BLOCK_SAMPLES;

#if defined(__IMXRT1062__)
#include <utility/imxrt_hw.h> //from Teensy Audio library.  For set_audioClock()
#endif

#define I2S_BUFFER_TO_USE_BYTES (AudioOutputI2S_ext_F32::audio_block_samples * sizeof(i2s_tx_buffer[0]))


void AudioOutputI2S_ext_F32::begin()
{

	dma.begin(true); // Allocate the DMA channel first

	block_left_1st = NULL;
	block_right_1st = NULL;

	AudioOutputI2S_ext_F32::config_i2s(sample_rate_Hz);

#if defined(__IMXRT1062__)
	CORE_PIN7_CONFIG = 3; // 1:TX_DATA0
	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = 4;
	dma.TCD->SLAST = -I2S_BUFFER_TO_USE_BYTES;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = I2S_BUFFER_TO_USE_BYTES / 4;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = I2S_BUFFER_TO_USE_BYTES / 4;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.TCD->DADDR = (void *)((uint32_t)&I2S1_TDR0 + 0);
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX);
	dma.enable(); // newer location of this line in Teensy Audio library
	I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE;
	I2S1_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
#endif
	update_responsibility = update_setup();
	dma.attachInterrupt(AudioOutputI2S_ext_F32::isr);
	enabled = 1;
}

void AudioOutputI2S_ext_F32::isr(void)
{
#if defined(__IMXRT1062__)
	int32_t *dest;
	audio_block_f32_t *blockL, *blockR;
	uint32_t saddr, offsetL, offsetR;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)i2s_tx_buffer + I2S_BUFFER_TO_USE_BYTES / 2)
	{ // are we transmitting the first half or second half of the buffer?
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = (int32_t *)&i2s_tx_buffer[audio_block_samples / 2];
		if (AudioOutputI2S_ext_F32::update_responsibility)
			AudioStream_F32::update_all();
	}
	else
	{
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (int32_t *)i2s_tx_buffer;
	}

	blockL = AudioOutputI2S_ext_F32::block_left_1st;
	blockR = AudioOutputI2S_ext_F32::block_right_1st;
	offsetL = AudioOutputI2S_ext_F32::block_left_offset;
	offsetR = AudioOutputI2S_ext_F32::block_right_offset;

	int32_t *d = dest;
	if (blockL && blockR)
	{
		float32_t *pL = blockL->data + offsetL;
		float32_t *pR = blockR->data + offsetR;
		for (int i = 0; i < audio_block_samples / 2; i++)
		{
			*d++ = (int32_t)*pL++;
			*d++ = (int32_t)*pR++; // interleave
		}
		offsetL += audio_block_samples / 2;
		offsetR += audio_block_samples / 2;
	}
	else if (blockL)
	{
		float32_t *pL = blockL->data + offsetL;
		for (int i = 0; i < audio_block_samples / 2 * 2; i += 2)
		{
			*(d + i) = (int32_t)*pL++;
		} // interleave
		offsetL += audio_block_samples / 2;
	}
	else if (blockR)
	{
		float32_t *pR = blockR->data + offsetR;
		for (int i = 0; i < audio_block_samples / 2 * 2; i += 2)
		{
			*(d + i) = (int32_t)*pR++;
		} // interleave
		offsetR += audio_block_samples / 2;
	}
	else
	{
		memset(dest, 0, audio_block_samples * 4);
		return;
	}

	arm_dcache_flush_delete(dest, sizeof(i2s_tx_buffer) / 2);

	if (offsetL < (uint16_t)audio_block_samples)
	{
		AudioOutputI2S_ext_F32::block_left_offset = offsetL;
	}
	else
	{
		AudioOutputI2S_ext_F32::block_left_offset = 0;
		AudioStream_F32::release(blockL);
		AudioOutputI2S_ext_F32::block_left_1st = AudioOutputI2S_ext_F32::block_left_2nd;
		AudioOutputI2S_ext_F32::block_left_2nd = NULL;
	}
	if (offsetR < (uint16_t)audio_block_samples)
	{
		AudioOutputI2S_ext_F32::block_right_offset = offsetR;
	}
	else
	{
		AudioOutputI2S_ext_F32::block_right_offset = 0;
		AudioStream_F32::release(blockR);
		AudioOutputI2S_ext_F32::block_right_1st = AudioOutputI2S_ext_F32::block_right_2nd;
		AudioOutputI2S_ext_F32::block_right_2nd = NULL;
	}
#endif
}

// update has to be carefully coded so that, if audio_blocks are not available, the code exits
// gracefully and won't hang.  That'll cause the whole system to hang, which would be very bad.
// static int count = 0;
void AudioOutputI2S_ext_F32::update(void)
{
	audio_block_f32_t *block_f32;
	audio_block_f32_t *block_f32_scaled = AudioStream_F32::allocate_f32();
	audio_block_f32_t *block2_f32_scaled = AudioStream_F32::allocate_f32();
	if ((!block_f32_scaled) || (!block2_f32_scaled))
	{
		// couldn't get some working memory.  Return.
		if (block_f32_scaled)
			AudioStream_F32::release(block_f32_scaled);
		if (block2_f32_scaled)
			AudioStream_F32::release(block2_f32_scaled);
		return;
	}
	// now that we have our working memory, proceed with getting the audio data and processing
	block_f32 = receiveReadOnly_f32(0); // input 0 = left channel
	if (block_f32)
	{
		if (block_f32->length != audio_block_samples)
		{
			Serial.print("AudioOutputI2S_ext_F32: *** WARNING ***: audio_block says len = ");
			Serial.print(block_f32->length);
			Serial.print(", but I2S settings want it to be = ");
			Serial.println(audio_block_samples);
		}
		// Optional scaling for easy volume control.  Leave outputScale==1.0f for default
		if (outputScale < 1.0f || outputScale > 1.0f)
			arm_scale_f32(block_f32->data, outputScale, block_f32->data, block_f32->length);

		scale_float_to_int32range(block_f32->data, block_f32_scaled->data, audio_block_samples);
		// now process the data blocks
		__disable_irq();
		if (block_left_1st == NULL)
		{
			block_left_1st = block_f32_scaled;
			block_left_offset = 0;
			__enable_irq();
		}
		else if (block_left_2nd == NULL)
		{
			block_left_2nd = block_f32_scaled;
			__enable_irq();
		}
		else
		{
			audio_block_f32_t *tmp = block_left_1st;
			block_left_1st = block_left_2nd;
			block_left_2nd = block_f32_scaled;
			block_left_offset = 0;
			__enable_irq();
			AudioStream_F32::release(tmp);
		}
		AudioStream_F32::transmit(block_f32, 0);
		AudioStream_F32::release(block_f32); // echo the incoming audio out the outputs
	}
	else
	{
		// this branch should never get called, but if it does, let's release the buffer that was never used
		AudioStream_F32::release(block_f32_scaled);
	}

	block_f32_scaled = block2_f32_scaled; // this is simply renaming the pre-allocated buffer

	block_f32 = receiveReadOnly_f32(1); // input 1 = right channel
	if (block_f32)
	{
		// Optional scaling for easy volume control.  Leave outputScale==1.0f for default
		if (outputScale < 1.0f || outputScale > 1.0f)
			arm_scale_f32(block_f32->data, outputScale, block_f32->data, block_f32->length);
		// scale F32 to Int32
		scale_float_to_int32range(block_f32->data, block_f32_scaled->data, audio_block_samples);
		__disable_irq();
		if (block_right_1st == NULL)
		{
			block_right_1st = block_f32_scaled;
			block_right_offset = 0;
			__enable_irq();
		}
		else if (block_right_2nd == NULL)
		{
			block_right_2nd = block_f32_scaled;
			__enable_irq();
		}
		else
		{
			audio_block_f32_t *tmp = block_right_1st;
			block_right_1st = block_right_2nd;
			block_right_2nd = block_f32_scaled;
			block_right_offset = 0;
			__enable_irq();
			AudioStream_F32::release(tmp);
		}
		AudioStream_F32::transmit(block_f32, 1);
		AudioStream_F32::release(block_f32); // echo the incoming audio out the outputs
	}
	else
	{
		// this branch should never get called, but if it does, let's release the buffer that was never used
		AudioStream_F32::release(block_f32_scaled);
	}
}

void AudioOutputI2S_ext_F32::config_i2s(void) { config_i2s(AudioOutputI2S_ext_F32::sample_rate_Hz); }
void AudioOutputI2S_ext_F32::config_i2s(float fs_Hz)
{
#if defined(__IMXRT1062__)
	CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);

	// if either transmitter or receiver is enabled, do nothing
	if (I2S1_TCSR & I2S_TCSR_TE)
		return;
	if (I2S1_RCSR & I2S_RCSR_RE)
		return;
	// PLL:
	int fs = fs_Hz;

	// PLL between 27*24 = 648MHz und 54*24=1296MHz
	int n1 = 4; // SAI prescaler 4 => (n1*n2) = multiple of 4
	int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);

	double C = ((double)fs * 256 * n1 * n2) / 24000000;
	int c0 = C;
	int c2 = 10000;
	int c1 = C * c2 - (c0 * c2);
	set_audioClock(c0, c1, c2);

	// clear SAI1_CLK register locations
	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK)) | CCM_CSCMR1_SAI1_CLK_SEL(2);										// &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4
	CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK)) | CCM_CS1CDR_SAI1_CLK_PRED(n1 - 1) // &0x07
				 | CCM_CS1CDR_SAI1_CLK_PODF(n2 - 1);																				// &0x3f

	// Select MCLK
	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK)) | (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));

	CORE_PIN23_CONFIG = 3; // 1:MCLK
	CORE_PIN21_CONFIG = 3; // 1:RX_BCLK
	CORE_PIN20_CONFIG = 3; // 1:RX_SYNC

	int rsync = 0;
	int tsync = 1;

	I2S1_TMR = 0;

	I2S1_TCR1 = I2S_TCR1_RFW(1);
	I2S1_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP // sync=0; tx is async;
				| (I2S_TCR2_BCD | I2S_TCR2_DIV((1)) | I2S_TCR2_MSEL(1));
	I2S1_TCR3 = I2S_TCR3_TCE;

	I2S1_TCR4 = I2S_TCR4_FRSZ((2 - 1)) | I2S_TCR4_SYWD((32 - 1)) | I2S_TCR4_MF | I2S_TCR4_FSD | I2S_TCR4_FSE | I2S_TCR4_FSP;
	I2S1_TCR5 = I2S_TCR5_WNW((32 - 1)) | I2S_TCR5_W0W((32 - 1)) | I2S_TCR5_FBT((32 - 1));

	I2S1_RMR = 0;

	I2S1_RCR1 = I2S_RCR1_RFW(1);
	I2S1_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_RCR2_BCP // sync=0; rx is async;
				| (I2S_RCR2_BCD | I2S_RCR2_DIV((1)) | I2S_RCR2_MSEL(1));
	I2S1_RCR3 = I2S_RCR3_RCE;
	I2S1_RCR4 = I2S_RCR4_FRSZ((2 - 1)) | I2S_RCR4_SYWD((32 - 1)) | I2S_RCR4_MF | I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
	I2S1_RCR5 = I2S_RCR5_WNW((32 - 1)) | I2S_RCR5_W0W((32 - 1)) | I2S_RCR5_FBT((32 - 1));

#endif
}

/******************************************************************/

// From Chip: The I2SSlave functionality has NOT been extended to
// allow for different block sizes or sample rates (2020-10-31)

void AudioOutputI2Sslave_ext_F32::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	// pinMode(2, OUTPUT);
	block_left_1st = NULL;
	block_right_1st = NULL;

	AudioOutputI2Sslave_ext_F32::config_i2s();
#if defined(__IMXRT1062__)
	CORE_PIN7_CONFIG = 3; // 1:TX_DATA0
	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = -sizeof(i2s_tx_buffer);
	// dma.TCD->DADDR = (void *)((uint32_t)&I2S1_TDR1 + 2);
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	// dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI2_TX);
	dma.TCD->DADDR = (void *)((uint32_t)&I2S1_TDR0 + 2);
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX);
	dma.enable();

	I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE;
	I2S1_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;

#endif

	update_responsibility = update_setup();
	// dma.enable();
	dma.attachInterrupt(AudioOutputI2S_ext_F32::isr);
}

void AudioOutputI2Sslave_ext_F32::config_i2s(void)
{
#if defined(__IMXRT1062__)

	CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);

	// if either transmitter or receiver is enabled, do nothing
	if (I2S1_TCSR & I2S_TCSR_TE)
		return;
	if (I2S1_RCSR & I2S_RCSR_RE)
		return;

	// not using MCLK in slave mode - hope that's ok?
	// CORE_PIN23_CONFIG = 3;  // AD_B1_09  ALT3=SAI1_MCLK
	CORE_PIN21_CONFIG = 3;				  // AD_B1_11  ALT3=SAI1_RX_BCLK
	CORE_PIN20_CONFIG = 3;				  // AD_B1_10  ALT3=SAI1_RX_SYNC
	IOMUXC_SAI1_RX_BCLK_SELECT_INPUT = 1; // 1=GPIO_AD_B1_11_ALT3, page 868
	IOMUXC_SAI1_RX_SYNC_SELECT_INPUT = 1; // 1=GPIO_AD_B1_10_ALT3, page 872

	// configure transmitter
	I2S1_TMR = 0;
	I2S1_TCR1 = I2S_TCR1_RFW(1); // watermark at half fifo size
	I2S1_TCR2 = I2S_TCR2_SYNC(1) | I2S_TCR2_BCP;
	I2S1_TCR3 = I2S_TCR3_TCE;
	I2S1_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(31) | I2S_TCR4_MF | I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_RCR4_FSD;
	I2S1_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

	// configure receiver
	I2S1_RMR = 0;
	I2S1_RCR1 = I2S_RCR1_RFW(1);
	I2S1_RCR2 = I2S_RCR2_SYNC(0) | I2S_TCR2_BCP;
	I2S1_RCR3 = I2S_RCR3_RCE;
	I2S1_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(31) | I2S_RCR4_MF | I2S_RCR4_FSE | I2S_RCR4_FSP;
	I2S1_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);

#endif
}
