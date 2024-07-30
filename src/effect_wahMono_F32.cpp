/**
 * @file effect_wahMono_F32.cpp
 * @author Piotr Zapart
 * @brief Mono WAH effect
 * @version 0.1
 * @date 2024-07-09
 * 
 * @copyright Copyright (c) 2024 www.hexefx.com
 * 
 * Implementation is based on the work of Transmogrifox
 * https://cackleberrypines.net/transmogrifox/src/bela/inductor_wah_C_src/
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


#include "effect_wahMono_F32.h"

#define k *1e3
#define nF *1e-9
const AudioEffectWahMono_F32::wah_componentValues_t AudioEffectWahMono_F32::compValues[WAH_MODEL_LAST] = 
{//  Lp     Cf        Ci        Rpot      Ri       Rs      Rp,      Rc   	Rbias     Re      beta      name
	{0.50f, 10.0f nF, 10.0f nF, 100.0f k, 68.0f k, 1.5f k, 33.0f k, 22.0f k, 470.0f k, 390.0f, 250.0f}, // G1
	{0.50f, 10.0f nF, 10.0f nF, 100.0f k, 68.0f k, 1.5f k, 33.0f k, 22.0f k, 470.0f k, 510.0f, 650.0f}, // G2
	{0.66f, 10.0f nF, 10.0f nF, 100.0f k, 68.0f k, 1.5f k, 33.0f k, 22.0f k, 470.0f k, 390.0f, 250.0f}, // G3
	{0.50f, 10.0f nF, 10.0f nF, 100.0f k, 68.0f k, 1.5f k, 100.0f k,22.0f k, 470.0f k, 470.0f, 250.0f}, // G4
	{0.50f, 15.0f nF,  8.0f nF, 100.0f k, 220.0f k, 0.1f k, 47.0f k, 22.0f k, 470.0f k, 510.0f, 250.0f}, // VOCAL
	{0.50f, 10.0f nF, 47.0f nF, 100.0f k, 68.0f k, 1.5f k,150.0f k, 22.0f k, 470.0f k, 150.0f, 250.0f},  // EXTREME
	{0.66f, 10.0f nF, 10.0f nF, 100.0f k, 68.0f k, 1.5f k,33.0f k, 22.0f k, 470.0f k, 470.0f, 250.0f},	 // CUSTOM
	{0.50f, 100.0f nF, 10.0f nF, 100.0f k, 100.0f k, 1.5f k,100.0f k, 22.0f k, 470.0f k, 470.0f, 200.0f} // BASS
};
#undef k
#undef nF

void AudioEffectWahMono_F32::update()
{
	audio_block_f32_t *blockL, *blockR, *blockMod;
	float32_t a0, a1, a2, ax, drySig;
	uint16_t i;
	if (bp) // handle bypass
	{
		blockL = AudioStream_F32::receiveReadOnly_f32(0);
		blockR = AudioStream_F32::receiveReadOnly_f32(1);
		if (!blockL || !blockR)
		{
			if (blockL)
				AudioStream_F32::release(blockL);
			if (blockR)
				AudioStream_F32::release(blockR);
			return;
		}
		AudioStream_F32::transmit(blockL, 0);
		AudioStream_F32::transmit(blockR, 1);
		AudioStream_F32::release(blockL);
		AudioStream_F32::release(blockR);
		return;
	}
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
	blockMod = AudioStream_F32::receiveReadOnly_f32(2);

	arm_add_f32(blockL->data, blockR->data, blockL->data, blockL->length); // add two channels
	arm_scale_f32(blockL->data, input_gain, blockL->data, blockL->length);
	for (i=0; i<blockL->length; i++)
	{
		//variable gp is the pot gain, nominal range 0.0 to 1.0
		//although this can be abused for extended range.
		//A value less than zero would make the filter go unstable
		//and put a lot of NaNs in your audio output
		if (blockMod) 
		{
			// TODO: scale the range to 0.0-1.0
			gp = blockMod->data[i];
		}
		if(gp < 0.0f) gp = 0.0f;
		float32_t gp_scaled = map_sat(gp, 0.0f, 1.0f, gp_top, gp_btm);
		
		//The magic numbers below approximate frequency warping characteristic
		float32_t gw = 4.6f-18.4f/(4.0f + gp_scaled);

		//Update Biquad coefficients
		a0 = a0b + gw*a0c;
		a1 = -(a1b + gw*a1c);
		a2 = -(a2b + gw*a2c);
		ax = 1.0f/a0;
		
		drySig = blockL->data[i];

		//run it through the 1-pole HPF and gain first
		float32_t hpf = ghpf * (drySig - xh1) - a1p*yh1;
		xh1 = drySig;
		yh1 = hpf;

		//Apply modulated biquad
		float32_t y0 = b0*hpf + b1*x1 + b2*x2 + a1*y1 + a2*y2;
		y0 *= ax;
		float32_t  out = clip(y0);
		y0 = 0.95f*y0 + 0.05f*out; //Let a little harmonic distortion feed back into the filter loop
		x2 = x1;
		x1 = hpf;
		y2 = y1;
		y1 = y0;
		
		out = out * wet_gain + drySig * dry_gain;

		blockL->data[i] = out;
		blockR->data[i] = out;
	}
	AudioStream_F32::transmit(blockL, 0); // send blockL on both output channels
	AudioStream_F32::transmit(blockL, 1);
	AudioStream_F32::release(blockL);
	AudioStream_F32::release(blockR);
	if (blockMod) AudioStream_F32::release(blockMod);
}

FLASHMEM void AudioEffectWahMono_F32::setModel(wahModel_t model)
{
	if (model > WAH_MODEL_LAST) model = WAH_MODEL_G1;
	
	float32_t _b0, _b1, _b2;
	float32_t _a0c, _a1c, _a2c;
	float32_t _b0h, _b1h, _b2h;
	float32_t _b0b, _b2b;
	float32_t _a0b, _a1b, _a2b;
	float32_t _a1p, _ghpf;

	//helper variables 
	float32_t ro = 0.0f;
	float32_t re = 0.0f;
	float32_t Req = 0.0f;
	float32_t ic = 0.0f;

    float32_t RpRi, f0, w0, Q, c, s, alpha;

	//Equivalent output resistance seen from BJT collector
	ro = compValues[model].Rc * compValues[model].Rpot / (compValues[model].Rc + compValues[model].Rpot) ;
	ro = ro * compValues[model].Rbias / (ro + compValues[model].Rbias);
 	ic = 3.7f/compValues[model].Rc;  //Typical bias current
	re = 0.025f/ic; 		//BJT gain stage equivalent internal emitter resistance
							// gm = Ic/Vt, re = 1/gm
	Req = compValues[model].Re + re;
	gf = -ro/Req;  //forward gain of transistor stage
	re = compValues[model].beta*Req; //Resistance looking into BJT emitter
	Rp = compValues[model].Rp*re/(re + compValues[model].Rp);

	RpRi = Rp*compValues[model].Ri/(Rp + compValues[model].Ri);
	f0 = 1.0f/(2.0f*M_PI*sqrtf(compValues[model].Lp*compValues[model].Cf));
	w0 = 2.0f*M_PI*f0/fs;
	Q = RpRi*sqrtf(compValues[model].Cf/compValues[model].Lp);
	c = cosf(w0);
	s = sinf(w0);
	alpha = s/(2.0f*Q);

	//High Pass Biquad Coefficients
	_b0h = (1.0f + c)/2.0f;
	_b1h = -(1.0f + c);
	_b2h = (1.0f + c)/2.0f;

    //Band-pass biquad coefficients
	_b0b = Q*alpha;
	_b2b = -Q*alpha;
	_a0b = 1.0f + alpha;
	_a1b = -2.0f*c;
	_a2b = 1.0f - alpha;

    //1-pole high pass filter coefficients
    // H(z) = g * (1 - z^-1)/(1 - a1*z^-1)
    // Direct Form 1:
    //      y[n] = ghpf * ( x[n] - x[n-1] ) - a1p*y[n-1]

    _a1p = -expf(-1.0f/(compValues[model].Ri*compValues[model].Ci*fs));
    _ghpf = gf*(1.0f - a1p)*0.5f;   //BJT forward gain worked in here to
                                        // save extra multiplications in
                                        // updating biquad coefficients

    //Distill all down to final biquad coefficients
    float32_t Gi = compValues[model].Rs/(compValues[model].Ri + compValues[model].Rs);
    float32_t gbpf = 1.0f/(2.0f*M_PI*f0*compValues[model].Ri*compValues[model].Cf);  //band-pass component equivalent gain

    //Final Biquad numerator coefficients
    _b0 = gbpf*b0b + Gi*a0b;
    _b1 = Gi*a1b;
    _b2 = gbpf*b2b + Gi*a2b;

    //Constants to make denominator coefficients computation more efficient
    //in real-time
	_a0c = -gf*b0h;
	_a1c = -gf*b1h;
	_a2c = -gf*b2h;

	__disable_irq();
	b0h = _b0h;
	b1h = _b1h;
	b2h = _b2h;
	b0b = _b0b;
	b2b = _b2b;
	a0b = _a0b;
	a1b = _a1b;
	a2b = _a2b;
	a1p = _a1p;
	ghpf = _ghpf;
    b0 = _b0;
    b1 = _b1;
    b2 = _b2;
	a0c = _a0c;
	a1c = _a1c;
	a2c = _a2c;
	y1 = 0.0f;	//biquad state variables
	y2 = 0.0f;
	x1 = 0.0f;
	x2 = 0.0f;
	//First order high-pass filter state variables
	yh1 = 0.0f;
	xh1 = 0.0f;	
	__enable_irq();
}


float32_t AudioEffectWahMono_F32::clip(float32_t x)
{

    float32_t thrs = 0.8f;
    float32_t nthrs = -0.72f;
    float32_t f=1.25f;

    //Hard limiting
    if(x >= 1.2f) x = 1.2f;
    if(x <= -1.12f) x = -1.12f;
    
    //Soft clipping
    if(x > thrs)
	{
        x -= f*sqr(x - thrs);
    }
    if(x < nthrs){
        x += f*sqr(x - nthrs);
    }   
    return x;
}