/*  Stereo spring reverb  for Teensy 4
 *
 *  Author: Piotr Zapart
 *          www.hexefx.com
 *
 * Copyright (c) 2024 by Piotr Zapart
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

#include <Arduino.h>
#include "effect_springreverb_F32.h"

#define INP_ALLP_COEFF      (0.6f)
#define CHIRP_ALLP_COEFF    (-0.6f)

#define TREBLE_LOSS_FREQ    (0.55f)
#define BASS_LOSS_FREQ      (0.36f)

AudioEffectSpringReverb_F32::AudioEffectSpringReverb_F32() : AudioStream_F32(2, inputQueueArray)
{
    inputGain = 0.5f;
	rv_time_k = 0.8f;
    in_allp_k = INP_ALLP_COEFF;
	bool memOK = true;
	if(!sp_lp_allp1a.init(&in_allp_k)) memOK = false;
	if(!sp_lp_allp1b.init(&in_allp_k)) memOK = false;
	if(!sp_lp_allp1c.init(&in_allp_k)) memOK = false;
	if(!sp_lp_allp1d.init(&in_allp_k)) memOK = false;

	if(!sp_lp_allp2a.init(&in_allp_k)) memOK = false;
	if(!sp_lp_allp2b.init(&in_allp_k)) memOK = false;
	if(!sp_lp_allp2c.init(&in_allp_k)) memOK = false;
	if(!sp_lp_allp2d.init(&in_allp_k)) memOK = false;	
	if(!lp_dly1.init(SPRVB_DLY1_LEN)) memOK = false;
	if(!lp_dly2.init(SPRVB_DLY2_LEN)) memOK = false;
	// chirp allpass chain
	sp_chrp_alp1_buf = (float *)malloc(SPRVB_CHIRP_AMNT*SPRVB_CHIRP1_LEN*sizeof(float));
	sp_chrp_alp2_buf = (float *)malloc(SPRVB_CHIRP_AMNT*SPRVB_CHIRP2_LEN*sizeof(float));
	sp_chrp_alp3_buf = (float *)malloc(SPRVB_CHIRP_AMNT*SPRVB_CHIRP3_LEN*sizeof(float));
	sp_chrp_alp4_buf = (float *)malloc(SPRVB_CHIRP_AMNT*SPRVB_CHIRP4_LEN*sizeof(float));
	if (!sp_chrp_alp1_buf) memOK = false;
	if (!sp_chrp_alp2_buf) memOK = false;
	if (!sp_chrp_alp3_buf) memOK = false;
	if (!sp_chrp_alp4_buf) memOK = false;
	memset(&sp_chrp_alp1_buf[0], 0, SPRVB_CHIRP_AMNT*SPRVB_CHIRP1_LEN*sizeof(float));
	memset(&sp_chrp_alp2_buf[0], 0, SPRVB_CHIRP_AMNT*SPRVB_CHIRP2_LEN*sizeof(float));
	memset(&sp_chrp_alp3_buf[0], 0, SPRVB_CHIRP_AMNT*SPRVB_CHIRP3_LEN*sizeof(float));
	memset(&sp_chrp_alp4_buf[0], 0, SPRVB_CHIRP_AMNT*SPRVB_CHIRP4_LEN*sizeof(float));
	in_BassCut_k = 0.0f;
	in_TrebleCut_k = 0.95f;
	lp_BassCut_k = 0.0f;
	lp_TrebleCut_k = 1.0f;
	flt_in.init(BASS_LOSS_FREQ, &in_BassCut_k, TREBLE_LOSS_FREQ, &in_TrebleCut_k);
	flt_lp1.init(BASS_LOSS_FREQ, &lp_BassCut_k, TREBLE_LOSS_FREQ, &lp_TrebleCut_k);
	flt_lp2.init(BASS_LOSS_FREQ, &lp_BassCut_k, TREBLE_LOSS_FREQ, &lp_TrebleCut_k);
	mix(0.5f);
	cleanup_done = true;
	if (memOK) initialized = true;
}

void AudioEffectSpringReverb_F32::update()
{   
#if defined(__IMXRT1062__)
	audio_block_f32_t *blockL, *blockR;
	int i, j;
	float32_t inL, inR, dryL, dryR;
	float32_t acc;
    float32_t lp_out1, lp_out2, mono_in, dry_in;
    float32_t rv_time;
	uint32_t allp_idx;
	uint32_t offset;
	float lfo_fr;	
    if (!initialized) return;
    if (bp)
    {
		if (!cleanup_done && bp_mode != BYPASS_MODE_TRAILS)
		{
			sp_lp_allp1a.reset();
			sp_lp_allp1b.reset();
			sp_lp_allp1c.reset();
			sp_lp_allp1d.reset();
			sp_lp_allp2a.reset();
			sp_lp_allp2b.reset();
			sp_lp_allp2c.reset();
			sp_lp_allp2d.reset();
			lp_dly1.reset();
			lp_dly2.reset();
			memset(&sp_chrp_alp1_buf[0], 0, SPRVB_CHIRP_AMNT*SPRVB_CHIRP1_LEN*sizeof(float));
			memset(&sp_chrp_alp2_buf[0], 0, SPRVB_CHIRP_AMNT*SPRVB_CHIRP2_LEN*sizeof(float));
			memset(&sp_chrp_alp3_buf[0], 0, SPRVB_CHIRP_AMNT*SPRVB_CHIRP3_LEN*sizeof(float));
			memset(&sp_chrp_alp4_buf[0], 0, SPRVB_CHIRP_AMNT*SPRVB_CHIRP4_LEN*sizeof(float));			
			cleanup_done = true;
		}

		switch(bp_mode)
		{
			case BYPASS_MODE_PASS:
				blockL = AudioStream_F32::receiveReadOnly_f32(0);
				blockR = AudioStream_F32::receiveReadOnly_f32(1);
				if (!blockL || !blockR) 
				{
					if (blockL) AudioStream_F32::release(blockL);
					if (blockR) AudioStream_F32::release(blockR);
					return;
				}
				AudioStream_F32::transmit(blockL, 0);	
				AudioStream_F32::transmit(blockR, 1);
				AudioStream_F32::release(blockL);
				AudioStream_F32::release(blockR);
				return;
				break;
			case BYPASS_MODE_OFF:
				blockL = AudioStream_F32::allocate_f32();
				if (!blockL) return;
				memset(&blockL->data[0], 0, blockL->length*sizeof(float32_t));
				AudioStream_F32::transmit(blockL, 0);	
				AudioStream_F32::transmit(blockL, 1);
				AudioStream_F32::release(blockL);	
				return;
				break;
			case BYPASS_MODE_TRAILS:
			default:
				break;
		}
    }
	cleanup_done = false;
	blockL = AudioStream_F32::receiveWritable_f32(0);
	blockR = AudioStream_F32::receiveWritable_f32(1);
	if (!blockL || !blockR)
	{
		if (blockL)
			AudioStream_F32::release(blockL);
		if (blockR)
			AudioStream_F32::release(blockR);
		return;
	}
    rv_time = rv_time_k;
	for (i=0; i < blockL->length; i++) 
    {  
		lfo.update();
		inputGain += (inputGainSet - inputGain) * 0.25f;
		dryL = blockL->data[i];
		dryR = blockR->data[i];
		dry_in = (dryL + dryR) * inputGain;

		mono_in = flt_in.process(dry_in)* (1.0f + in_BassCut_k*-2.5f);
		acc = lp_dly1.getTap(0) * rv_time;
		lp_out1 = flt_lp1.process(acc);

		acc = sp_lp_allp1a.process(lp_out1); 
		acc = sp_lp_allp1b.process(acc);
		acc = sp_lp_allp1c.process(acc);
		acc = sp_lp_allp1d.process(acc);
		acc = lp_dly2.process(acc + mono_in) * rv_time;
		lp_out2 = flt_lp2.process(acc);

		acc = sp_lp_allp2a.process(lp_out2); 
		acc = sp_lp_allp2b.process(acc);
		acc = sp_lp_allp2c.process(acc);
		acc = sp_lp_allp2d.process(acc);

		lp_dly1.write_toOffset(acc + mono_in, 0);
		lp_dly1.updateIndex();
		
		inL = inR = (lp_out1 + lp_out2);

		j = 0;
        while(j < SPRVB_CHIRP_AMNT)
        {
			// 1st 4 are left channel
			allp_idx = j*SPRVB_CHIRP1_LEN + chrp_alp1_idx[j];
			acc = sp_chrp_alp1_buf[allp_idx] + inL * chrp_allp_k[0];
			sp_chrp_alp1_buf[allp_idx] = inL - chrp_allp_k[0] * acc;
            inL = acc;
            if (++chrp_alp1_idx[j] >= SPRVB_CHIRP1_LEN) chrp_alp1_idx[j] = 0;   
            
			allp_idx = (j+1)*SPRVB_CHIRP1_LEN + chrp_alp1_idx[j+1];
            acc = sp_chrp_alp1_buf[allp_idx]  + inL * chrp_allp_k[1];  
            sp_chrp_alp1_buf[allp_idx] = inL - chrp_allp_k[1] * acc;			
            inL = acc;
            if (++chrp_alp1_idx[j+1] >= SPRVB_CHIRP1_LEN) chrp_alp1_idx[j+1] = 0; 
            
			allp_idx = (j+2)*SPRVB_CHIRP1_LEN + chrp_alp1_idx[j+2];
			acc = sp_chrp_alp1_buf[allp_idx]  + inL * chrp_allp_k[2];  
            sp_chrp_alp1_buf[allp_idx] = inL - chrp_allp_k[2] * acc;
            inL = acc;
            if (++chrp_alp1_idx[j+2] >= SPRVB_CHIRP1_LEN) chrp_alp1_idx[j+2] = 0; 

			allp_idx = (j+3)*SPRVB_CHIRP1_LEN + chrp_alp1_idx[j+3];
            acc = sp_chrp_alp1_buf[allp_idx]  + inL * chrp_allp_k[3];  
            sp_chrp_alp1_buf[allp_idx] = inL - chrp_allp_k[3] * acc;
            inL = acc;
            if (++chrp_alp1_idx[j+3] >= SPRVB_CHIRP1_LEN) chrp_alp1_idx[j+3] = 0; 
			// channel R
			allp_idx = (j+4)*SPRVB_CHIRP1_LEN + chrp_alp1_idx[j+4];
            acc = sp_chrp_alp1_buf[allp_idx]  + inR * chrp_allp_k[3];  
            sp_chrp_alp1_buf[allp_idx] = inR - chrp_allp_k[3] * acc;
            inR = acc;
            if (++chrp_alp1_idx[j+4] >= SPRVB_CHIRP1_LEN) chrp_alp1_idx[j+4] = 0; 

			allp_idx = (j+5)*SPRVB_CHIRP1_LEN + chrp_alp1_idx[j+5];
            acc = sp_chrp_alp1_buf[allp_idx]  + inR * chrp_allp_k[2];  
            sp_chrp_alp1_buf[allp_idx] = inR - chrp_allp_k[2] * acc;
            inR = acc;
            if (++chrp_alp1_idx[j+5] >= SPRVB_CHIRP1_LEN) chrp_alp1_idx[j+5] = 0; 

			allp_idx = (j+6)*SPRVB_CHIRP1_LEN + chrp_alp1_idx[j+6];
            acc = sp_chrp_alp1_buf[allp_idx]  + inR * chrp_allp_k[1];  
            sp_chrp_alp1_buf[allp_idx] = inR - chrp_allp_k[1] * acc;
            inR = acc;
            if (++chrp_alp1_idx[j+6] >= SPRVB_CHIRP1_LEN) chrp_alp1_idx[j+6] = 0; 

			allp_idx = (j+7)*SPRVB_CHIRP1_LEN + chrp_alp1_idx[j+7];
			acc = sp_chrp_alp1_buf[allp_idx]  + inR * chrp_allp_k[0];  
            sp_chrp_alp1_buf[allp_idx] = inR - chrp_allp_k[0] * acc;			
            inR = acc;
            if (++chrp_alp1_idx[j+7] >= SPRVB_CHIRP1_LEN) chrp_alp1_idx[j+7] = 0; 

            j = j + 8;
        }
        j = 0;
        while(j < SPRVB_CHIRP_AMNT)
        {	// channel L
			allp_idx = j*SPRVB_CHIRP2_LEN + chrp_alp2_idx[j];
            acc = sp_chrp_alp2_buf[allp_idx]  + inL * chrp_allp_k[0];  
            sp_chrp_alp2_buf[allp_idx] = inL - chrp_allp_k[0] * acc;
            inL = acc;
            if (++chrp_alp2_idx[j] >= SPRVB_CHIRP2_LEN) chrp_alp2_idx[j] = 0;   
            
			allp_idx = (j+1)*SPRVB_CHIRP2_LEN + chrp_alp2_idx[j+1];
            acc = sp_chrp_alp2_buf[allp_idx]  + inL * chrp_allp_k[1];  
            sp_chrp_alp2_buf[allp_idx] = inL - chrp_allp_k[1] * acc;			
            inL = acc;
            if (++chrp_alp2_idx[j+1] >= SPRVB_CHIRP2_LEN) chrp_alp2_idx[j+1] = 0; 
            
			allp_idx = (j+2)*SPRVB_CHIRP2_LEN + chrp_alp2_idx[j+2];
            acc = sp_chrp_alp2_buf[allp_idx]  + inL * chrp_allp_k[2];  
            sp_chrp_alp2_buf[allp_idx] = inL - chrp_allp_k[2] * acc;			
            inL = acc;
            if (++chrp_alp2_idx[j+2] >= SPRVB_CHIRP2_LEN) chrp_alp2_idx[j+2] = 0; 
            
			allp_idx = (j+3)*SPRVB_CHIRP2_LEN + chrp_alp2_idx[j+3];
            acc = sp_chrp_alp2_buf[allp_idx]  + inL * chrp_allp_k[3];  
            sp_chrp_alp2_buf[allp_idx] = inL - chrp_allp_k[3] * acc;			
            inL = acc;
            if (++chrp_alp2_idx[j+3] >= SPRVB_CHIRP2_LEN) chrp_alp2_idx[j+3] = 0; 
			// channel R
			allp_idx = (j+4)*SPRVB_CHIRP2_LEN + chrp_alp2_idx[j+4];
			acc = sp_chrp_alp2_buf[allp_idx]  + inR * chrp_allp_k[3];  
            sp_chrp_alp2_buf[allp_idx] = inR - chrp_allp_k[3] * acc;
            inR = acc;
            if (++chrp_alp2_idx[j+4] >= SPRVB_CHIRP2_LEN) chrp_alp2_idx[j+4] = 0; 
    
			allp_idx = (j+5)*SPRVB_CHIRP2_LEN + chrp_alp2_idx[j+5];
			acc = sp_chrp_alp2_buf[allp_idx]  + inR * chrp_allp_k[2];  
            sp_chrp_alp2_buf[allp_idx] = inR - chrp_allp_k[2] * acc;
            inR = acc;
            if (++chrp_alp2_idx[j+5] >= SPRVB_CHIRP2_LEN) chrp_alp2_idx[j+5] = 0; 

			allp_idx = (j+6)*SPRVB_CHIRP2_LEN + chrp_alp2_idx[j+6];
			acc = sp_chrp_alp2_buf[allp_idx]  + inR * chrp_allp_k[1];  
            sp_chrp_alp2_buf[allp_idx] = inR - chrp_allp_k[1] * acc;
            inR = acc;
            if (++chrp_alp2_idx[j+6] >= SPRVB_CHIRP2_LEN) chrp_alp2_idx[j+6] = 0; 

			allp_idx = (j+7)*SPRVB_CHIRP2_LEN + chrp_alp2_idx[j+7];
			acc = sp_chrp_alp2_buf[allp_idx]  + inR * chrp_allp_k[0];  
            sp_chrp_alp2_buf[allp_idx] = inR - chrp_allp_k[0] * acc;
            inR = acc;
            if (++chrp_alp2_idx[j+7] >= SPRVB_CHIRP2_LEN) chrp_alp2_idx[j+7] = 0; 
            j = j + 8;
        }
        j = 0;
        while(j < SPRVB_CHIRP_AMNT)
        {	// channel L
			allp_idx = j*SPRVB_CHIRP3_LEN + chrp_alp3_idx[j];
            acc = sp_chrp_alp3_buf[allp_idx]  + inL * chrp_allp_k[0];  
            sp_chrp_alp3_buf[allp_idx] = inL - chrp_allp_k[0] * acc;
            inL = acc;
            if (++chrp_alp3_idx[j] >= SPRVB_CHIRP3_LEN) chrp_alp3_idx[j] = 0;   
  
			allp_idx = (j+1)*SPRVB_CHIRP3_LEN + chrp_alp3_idx[j+1];
			acc = sp_chrp_alp3_buf[allp_idx]  + inL * chrp_allp_k[1];  
            sp_chrp_alp3_buf[allp_idx] = inL - chrp_allp_k[1] * acc;    
            inL = acc;
            if (++chrp_alp3_idx[j+1] >= SPRVB_CHIRP3_LEN) chrp_alp3_idx[j+1] = 0; 
            
			allp_idx = (j+2)*SPRVB_CHIRP3_LEN + chrp_alp3_idx[j+2];
            acc = sp_chrp_alp3_buf[allp_idx]  + inL * chrp_allp_k[2];  
            sp_chrp_alp3_buf[allp_idx] = inL - chrp_allp_k[2] * acc;			
            inL = acc;
            if (++chrp_alp3_idx[j+2] >= SPRVB_CHIRP3_LEN) chrp_alp3_idx[j+2] = 0; 

			allp_idx = (j+3)*SPRVB_CHIRP3_LEN + chrp_alp3_idx[j+3];
            acc = sp_chrp_alp3_buf[allp_idx]  + inL * chrp_allp_k[3];  
            sp_chrp_alp3_buf[allp_idx] = inL - chrp_allp_k[3] * acc;			
            inL = acc;
            if (++chrp_alp3_idx[j+3] >= SPRVB_CHIRP3_LEN) chrp_alp3_idx[j+3] = 0; 
			// channel R
			allp_idx = (j+4)*SPRVB_CHIRP3_LEN + chrp_alp3_idx[j+4];
            acc = sp_chrp_alp3_buf[allp_idx]  + inR * chrp_allp_k[3];  
            sp_chrp_alp3_buf[allp_idx] = inR - chrp_allp_k[3] * acc;			
            inR = acc;
            if (++chrp_alp3_idx[j+4] >= SPRVB_CHIRP3_LEN) chrp_alp3_idx[j+4] = 0; 
    
			allp_idx = (j+5)*SPRVB_CHIRP3_LEN + chrp_alp3_idx[j+5];
            acc = sp_chrp_alp3_buf[allp_idx]  + inR * chrp_allp_k[2];  
            sp_chrp_alp3_buf[allp_idx] = inR - chrp_allp_k[2] * acc;			
            inR = acc;
            if (++chrp_alp3_idx[j+5] >= SPRVB_CHIRP3_LEN) chrp_alp3_idx[j+5] = 0; 

			allp_idx = (j+6)*SPRVB_CHIRP3_LEN + chrp_alp3_idx[j+6];
            acc = sp_chrp_alp3_buf[allp_idx]  + inR * chrp_allp_k[1];  
            sp_chrp_alp3_buf[allp_idx] = inR - chrp_allp_k[1] * acc;			
            inR = acc;
            if (++chrp_alp3_idx[j+6] >= SPRVB_CHIRP3_LEN) chrp_alp3_idx[j+6] = 0; 

			allp_idx = (j+7)*SPRVB_CHIRP3_LEN + chrp_alp3_idx[j+7];
            acc = sp_chrp_alp3_buf[allp_idx]  + inR * chrp_allp_k[0];  
            sp_chrp_alp3_buf[allp_idx] = inR - chrp_allp_k[0] * acc;			
            inR = acc;
            if (++chrp_alp3_idx[j+7] >= SPRVB_CHIRP3_LEN) chrp_alp3_idx[j+7] = 0; 
            j = j + 8;
        }
		j = 0;
        while(j < SPRVB_CHIRP_AMNT)
        {	// channel L
			allp_idx = j*SPRVB_CHIRP4_LEN + chrp_alp4_idx[j];
            acc = sp_chrp_alp4_buf[allp_idx]  + inL * chrp_allp_k[0];  
            sp_chrp_alp4_buf[allp_idx] = inL - chrp_allp_k[0] * acc;
            inL = acc;
            if (++chrp_alp4_idx[j] >= SPRVB_CHIRP4_LEN) chrp_alp4_idx[j] = 0;   
            
			allp_idx = (j+1)*SPRVB_CHIRP4_LEN + chrp_alp4_idx[j+1];
            acc = sp_chrp_alp4_buf[allp_idx]  + inL * chrp_allp_k[1];  
            sp_chrp_alp4_buf[allp_idx] = inL - chrp_allp_k[1] * acc;			
            inL = acc;
            if (++chrp_alp4_idx[j+1] >= SPRVB_CHIRP4_LEN) chrp_alp4_idx[j+1] = 0; 

	        allp_idx = (j+2)*SPRVB_CHIRP4_LEN + chrp_alp4_idx[j+2];
            acc = sp_chrp_alp4_buf[allp_idx]  + inL * chrp_allp_k[1];  
            sp_chrp_alp4_buf[allp_idx] = inL - chrp_allp_k[1] * acc;			
            inL = acc;
            if (++chrp_alp4_idx[j+2] >= SPRVB_CHIRP4_LEN) chrp_alp4_idx[j+2] = 0; 

			allp_idx = (j+3)*SPRVB_CHIRP4_LEN + chrp_alp4_idx[j+3];
            acc = sp_chrp_alp4_buf[allp_idx]  + inL * chrp_allp_k[3];  
            sp_chrp_alp4_buf[allp_idx] = inL - chrp_allp_k[3] * acc;			
            inL = acc;
            if (++chrp_alp4_idx[j+3] >= SPRVB_CHIRP4_LEN) chrp_alp4_idx[j+3] = 0; 
			// channel R
			allp_idx = (j+4)*SPRVB_CHIRP4_LEN + chrp_alp4_idx[j+4];
            acc = sp_chrp_alp4_buf[allp_idx]  + inR * chrp_allp_k[3];  
            sp_chrp_alp4_buf[allp_idx] = inR - chrp_allp_k[3] * acc;			
            inR = acc;
            if (++chrp_alp4_idx[j+4] >= SPRVB_CHIRP4_LEN) chrp_alp4_idx[j+4] = 0; 

			allp_idx = (j+5)*SPRVB_CHIRP4_LEN + chrp_alp4_idx[j+5];
            acc = sp_chrp_alp4_buf[allp_idx]  + inR * chrp_allp_k[2];  
            sp_chrp_alp4_buf[allp_idx] = inR - chrp_allp_k[2] * acc;			
            inR = acc;
            if (++chrp_alp4_idx[j+5] >= SPRVB_CHIRP4_LEN) chrp_alp4_idx[j+5] = 0; 

			allp_idx = (j+6)*SPRVB_CHIRP4_LEN + chrp_alp4_idx[j+6];
            acc = sp_chrp_alp4_buf[allp_idx]  + inR * chrp_allp_k[1];  
            sp_chrp_alp4_buf[allp_idx] = inR - chrp_allp_k[1] * acc;			
            inR = acc;
            if (++chrp_alp4_idx[j+6] >= SPRVB_CHIRP4_LEN) chrp_alp4_idx[j+6] = 0; 

			allp_idx = (j+7)*SPRVB_CHIRP4_LEN + chrp_alp4_idx[j+7];
            acc = sp_chrp_alp4_buf[allp_idx]  + inR * chrp_allp_k[0];  
            sp_chrp_alp4_buf[allp_idx] = inR - chrp_allp_k[0] * acc;			
            inR = acc;
            if (++chrp_alp4_idx[j+7] >= SPRVB_CHIRP4_LEN) chrp_alp4_idx[j+7] = 0; 
            j = j + 8;
        }

		// modulate the allpass filters
		lfo.get(BASIC_LFO_PHASE_0, &offset, &lfo_fr); 
		acc = sp_lp_allp1d.getTap(offset+1, lfo_fr);
		sp_lp_allp1d.write_toOffset(acc, (lfo_ampl<<1)+1);
		lfo.get(BASIC_LFO_PHASE_90, &offset, &lfo_fr); 
		acc = sp_lp_allp2d.getTap(offset+1, lfo_fr);
		sp_lp_allp2d.write_toOffset(acc, (lfo_ampl<<1)+1);

        blockL->data[i] = inL * wet_gain + dryL * dry_gain; 
		blockR->data[i] = inR * wet_gain + dryR * dry_gain;
	}
    AudioStream_F32::transmit(blockL, 0);
	AudioStream_F32::transmit(blockR, 1);
	AudioStream_F32::release(blockL);
	AudioStream_F32::release(blockR);
#endif
}
