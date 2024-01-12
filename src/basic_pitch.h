#ifndef _BASIC_PITCH_H_
#define _BASIC_PITCH_H_

#include <Arduino.h>
#include "Audio.h"

#define BASIC_PITCH_BUF_BITS 		(12)
#define BASIC_PITCH_BUF_SIZE 		(1<<BASIC_PITCH_BUF_BITS)
#define BASIC_PITCH_BUF_SIZE_HALF	(1<<(BASIC_PITCH_BUF_BITS-1))
#define BASIC_PITCH_BUF_MASK 		(BASIC_PITCH_BUF_SIZE-1)
#define BASIC_PITCH_BUF_FRAC_MASK	((1<<(32-BASIC_PITCH_BUF_BITS))-1) 

#define BASIC_PITCH_XFADE_BITS		(10)
#define BASIC_PITCH_XFADE_LEN		(1<<BASIC_PITCH_XFADE_BITS)
#define BASIC_PITCH_XFADE_LEN_HALF	(BASIC_PITCH_XFADE_LEN>>1)
#define BASIC_PITCH_XFADE_MASK		(BASIC_PITCH_XFADE_LEN-1)

extern "C" {
extern const float AudioWaveformFader_f32[]; // crossfade waveform
extern const float music_intevals[];		// semitone intervals -1oct to +2oct
}

class AudioBasicPitch
{
public:
	bool init()
	{
		outFilter.init(hp_f, (float *)&hp_gain, lp_f, &lp_gain);
		bf = (float *)malloc(BASIC_PITCH_BUF_SIZE*sizeof(float)); // allocate buffer
		if (!bf) return false;
		reset();
		return true;
	}

	void setPitch(float ratio)
	{
		readAdder = (float)pitchDelta0 * ratio;
	}
	void setPitchSemintone(int8_t s)
	{
		s = constrain(s, -12, +24); // limit to the predefined range
		setPitch(music_intevals[s + 12]);
	}
	void setTone(float t)
	{
		//lp_f = constrain(t, 0.01f, 1.0f);
		lp_gain = constrain(t, 0.0f, 1.0f);
	}

	float process(float newSample)
	{
		uint32_t idx1, idx2;
		uint32_t delta, delta_acc;
		float k_frac, delta_frac, s_n, s_half, xf0, xf1;

		bf[writeAddr] = newSample;				// write new sample
		readAddr = readAddr + readAdder;		// update read pointer, readAdder controls the pitch
		// bypass mode is at mix = 0 or if no pitch change
		if (mix == 0.0f || readAdder == pitchDelta0) 
		{
			writeAddr = (writeAddr + 1) & BASIC_PITCH_BUF_MASK;
			return newSample;
		}
		// sample end
		idx1 = (readAddr >> (32-BASIC_PITCH_BUF_BITS)) & BASIC_PITCH_BUF_MASK;						// index of the last sample 
		k_frac = (float)(readAddr & BASIC_PITCH_BUF_FRAC_MASK) / (float)BASIC_PITCH_BUF_FRAC_MASK;	// fractional part
	 	s_n = bf[idx1] * (1.0f-k_frac);			
		s_n += bf[(idx1 + 1) & BASIC_PITCH_BUF_MASK] * k_frac;										// interpolated sample
		// sample half
		idx2 = ((readAddr + 0x80000000) >> (32-BASIC_PITCH_BUF_BITS)) & BASIC_PITCH_BUF_MASK;
		k_frac = (float)((readAddr+0x80000000) & BASIC_PITCH_BUF_FRAC_MASK) / (float)BASIC_PITCH_BUF_FRAC_MASK;
		s_half = bf[idx2] * (1.0f - k_frac);
		s_half += bf[(idx2 + 1) & BASIC_PITCH_BUF_MASK] * k_frac;

		delta_acc = readAddr - (writeAddr<<(32-BASIC_PITCH_BUF_BITS));	// distance between the write and read pointer
		
		delta = (delta_acc >> (32-9)) & 0x1FF;								// 9 bit value = 2x fade table length (fade in + fade out)
		delta_frac = (float)(delta_acc & ((1<<23)-1)) / (float)((1<<23)-1);	// fractional part for the xfade curve
		idx2 = delta&0xFF;
		xf0 = AudioWaveformFader_f32[idx2];	
		xf1 = AudioWaveformFader_f32[idx2+1];
		k_frac = xf0 * (1.0f-delta_frac) + xf1 * delta_frac;	// interpolated smooth crossfade coeff.

		if (delta > 0xFF) k_frac = 1.0f-k_frac;					// invert the curve for the fade out part
		
		s_n = s_n * k_frac + s_half * (1.0f - k_frac);			// crossfade the last and mid sample
		
		writeAddr = (writeAddr + 1) & BASIC_PITCH_BUF_MASK;		// update the write pointer
		s_n = outFilter.process(s_n);						// apply output lowpass
		return (s_n * mix + newSample * (1.0f-mix));			// do dry/wet mix
	}
	void setMix(float mixRatio)
	{
		mix = constrain(mixRatio, 0.0f, 1.0f);
	}
	void reset()
	{
		memset(bf, 0, BASIC_PITCH_BUF_SIZE*sizeof(float));

		readAddr = 0;
		writeAddr = 0;
		readAdder = pitchDelta0;
		mix = 1.0f;
	}
private:
	float *bf;
	float mix;
	uint32_t readAddr;
	uint32_t readAdder;
	uint16_t writeAddr;
	static const uint32_t pitchDelta0 = BASIC_PITCH_BUF_FRAC_MASK+1;

	AudioFilterShelvingLPHP outFilter;
	static constexpr float hp_f = 0.003f;
	const float hp_gain = 0.0f;
	static constexpr float lp_f = 0.26f;
	float lp_gain = 1.0f;
};


#endif // _BASIC_PITCH_H_
