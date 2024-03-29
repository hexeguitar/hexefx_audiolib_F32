//----------------------------------------------------------------------------
//                                3 Band EQ :)
//
// EQ.C - Main Source file for 3 band EQ
// (c) Neil C / Etanza Systems / 2K6
// Shouts / Loves / Moans = etanza at lycos dot co dot uk
//
// This work is hereby placed in the public domain for all purposes, including
// use in commercial applications.
// The author assumes NO RESPONSIBILITY for any problems caused by the use of
// this software.
//----------------------------------------------------------------------------
// NOTES :
// - Original filter code by Paul Kellet (musicdsp.pdf)
// - Uses 4 first order filters in series, should give 24dB per octave
// - Now with P4 Denormal fix :)
//----------------------------------------------------------------------------
// Teensy OpenAudio_ArduinoLibrary_F32 port: 01.2024 Piotr Zapart www.hexefx.com 

#ifndef _FILTER_3BANDEQ_H_
#define _FILTER_3BANDEQ_H_

#include "Arduino.h"
#include "AudioStream_F32.h"
#include "arm_math.h"
#include "mathDSP_F32.h"

class AudioFilterEqualizer3band_F32 : public AudioStream_F32
{
public:
	AudioFilterEqualizer3band_F32(void) : AudioStream_F32(1, inputQueueArray)
	{
		setBands(500.0f, 3000.0f);
	}
	void setBands(float32_t bassF, float32_t trebleF)
	{
		trebleF = 2.0f * sinf(M_PI * (trebleF / AUDIO_SAMPLE_RATE_EXACT));
		bassF = 2.0f * sinf(M_PI * (bassF / AUDIO_SAMPLE_RATE_EXACT));

		__disable_irq();
		lowpass_f = bassF;
		hipass_f = trebleF;
		__enable_irq();
	}
	void treble(float32_t t)
	{
		__disable_irq();
		treble_g = t;
		__enable_irq();
	}
	void mid(float32_t m)
	{
		__disable_irq();
		mid_g = m;
		__enable_irq();
	}

	void bass(float32_t b)
	{
		__disable_irq();
		bass_g = b;
		__enable_irq();
	}
	void set(float32_t t, float32_t m, float32_t b)
	{
		__disable_irq();
		treble_g = t;
		mid_g = m;
		bass_g = b;
		__enable_irq();
	}
	void update()
	{
		audio_block_f32_t *block;
		int i;
		block = AudioStream_F32::receiveWritable_f32();
		if (!block)
			return;
		if (bp) // bypass mode
		{
			AudioStream_F32::transmit(block);
			AudioStream_F32::release(block);
			return;
		}
		for (i = 0; i < block->length; i++)
		{
			float32_t lpOut, midOut, hpOut;	   // Low / Mid / High - Sample Values
			float32_t sample = block->data[i]; // give some headroom
			// Filter #1 (lowpass)
			f1p0 += (lowpass_f * (sample - f1p0)) + vsa;
			f1p1 += (lowpass_f * (f1p0 - f1p1));
			f1p2 += (lowpass_f * (f1p1 - f1p2));
			f1p3 += (lowpass_f * (f1p2 - f1p3));
			lpOut = f1p3;

			// // Filter #2 (highpass)
			f2p0 += (hipass_f * (sample - f2p0)) + vsa;
			f2p1 += (hipass_f * (f2p0 - f2p1));
			f2p2 += (hipass_f * (f2p1 - f2p2));
			f2p3 += (hipass_f * (f2p2 - f2p3));
			hpOut = sdm3 - f2p3;

			midOut = sdm3 - (lpOut + hpOut);
			// // Scale, Combine and store
			lpOut *= bass_g;
			midOut *= mid_g;
			hpOut *= treble_g;

			// // Shuffle history buffer
			sdm3 = sdm2;
			sdm2 = sdm1;
			sdm1 = sample;

			block->data[i] = (lpOut + midOut + hpOut);
		}
		AudioStream_F32::transmit(block);
		AudioStream_F32::release(block);
	}
	void bypass_set(bool s) { bp = s; }
	bool bypass_tgl()
	{
		bp ^= 1;
		return bp;
	}

private:
	audio_block_f32_t *inputQueueArray[1];
	bool bp = false;

	float32_t f1p0 = 0.0f;
	float32_t f1p1 = 0.0f;
	float32_t f1p2 = 0.0f;
	float32_t f1p3 = 0.0f;
	float32_t f2p0 = 0.0f;
	float32_t f2p1 = 0.0f;
	float32_t f2p2 = 0.0f;
	float32_t f2p3 = 0.0f;

	float32_t sdm1 = 0.0f;
	float32_t sdm2 = 0.0f; // 2
	float32_t sdm3 = 0.0f; // 3

	static constexpr float32_t vsa = (1.0 / 4294967295.0); // Very small amount (Denormal Fix)
	float32_t lpreg;
	float32_t hpreg;
	float32_t lowpass_f;
	float32_t bass_g = 1.0f;
	float32_t hipass_f;
	float32_t treble_g = 1.0f;
	float32_t mid_g = 1.0f;
};

class AudioFilterEqualizer3bandStereo_F32 : public AudioStream_F32
{
public:
	AudioFilterEqualizer3bandStereo_F32(void) : AudioStream_F32(2, inputQueueArray)
	{
		setBands(500.0f, 3000.0f);
	}
	void setBands(float32_t bassF, float32_t trebleF)
	{
		trebleF = 2.0f * sinf(M_PI * (trebleF / AUDIO_SAMPLE_RATE_EXACT));
		bassF = 2.0f * sinf(M_PI * (bassF / AUDIO_SAMPLE_RATE_EXACT));

		__disable_irq();
		lowpass_f = bassF;
		hipass_f = trebleF;
		__enable_irq();
	}
	void treble(float32_t t)
	{
		__disable_irq();
		treble_g = t;
		__enable_irq();
	}
	void mid(float32_t m)
	{
		__disable_irq();
		mid_g = m;
		__enable_irq();
	}

	void bass(float32_t b)
	{
		__disable_irq();
		bass_g = b;
		__enable_irq();
	}
	void set(float32_t t, float32_t m, float32_t b)
	{
		__disable_irq();
		treble_g = t;
		mid_g = m;
		bass_g = b;
		__enable_irq();
	}
	void update()
	{
		audio_block_f32_t *blockL, *blockR;
		int i;
		if (bp) // bypass mode
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

		for (i = 0; i < blockL->length; i++)
		{
			float32_t lpOut, midOut, hpOut;	   // Low / Mid / High - Sample Values
			
			// channel L
			float32_t sample = blockL->data[i]; // give some headroom
			// Filter #1 (lowpass)
			f1p0[0] += (lowpass_f * (sample - f1p0[0])) + vsa;
			f1p1[0] += (lowpass_f * (f1p0[0] - f1p1[0]));
			f1p2[0] += (lowpass_f * (f1p1[0] - f1p2[0]));
			f1p3[0] += (lowpass_f * (f1p2[0] - f1p3[0]));
			lpOut = f1p3[0];

			// // Filter #2 (highpass)
			f2p0[0] += (hipass_f * (sample - f2p0[0])) + vsa;
			f2p1[0] += (hipass_f * (f2p0[0] - f2p1[0]));
			f2p2[0] += (hipass_f * (f2p1[0] - f2p2[0]));
			f2p3[0] += (hipass_f * (f2p2[0] - f2p3[0]));
			hpOut = sdm3[0] - f2p3[0];

			midOut = sdm3[0] - (lpOut + hpOut);
			// // Scale, Combine and store
			lpOut *= bass_g;
			midOut *= mid_g;
			hpOut *= treble_g;

			// // Shuffle history buffer
			sdm3[0] = sdm2[0];
			sdm2[0] = sdm1[0];
			sdm1[0] = sample;

			blockL->data[i] = (lpOut + midOut + hpOut);

			// channel R
			sample = blockR->data[i]; // give some headroom
			// Filter #1 (lowpass)
			f1p0[1] += (lowpass_f * (sample - f1p0[1])) + vsa;
			f1p1[1] += (lowpass_f * (f1p0[1] - f1p1[1]));
			f1p2[1] += (lowpass_f * (f1p1[1] - f1p2[1]));
			f1p3[1] += (lowpass_f * (f1p2[1] - f1p3[1]));
			lpOut = f1p3[1];

			// // Filter #2 (highpass)
			f2p0[1] += (hipass_f * (sample - f2p0[1])) + vsa;
			f2p1[1] += (hipass_f * (f2p0[1] - f2p1[1]));
			f2p2[1] += (hipass_f * (f2p1[1] - f2p2[1]));
			f2p3[1] += (hipass_f * (f2p2[1] - f2p3[1]));
			hpOut = sdm3[1] - f2p3[1];

			midOut = sdm3[1] - (lpOut + hpOut);
			// // Scale, Combine and store
			lpOut *= bass_g;
			midOut *= mid_g;
			hpOut *= treble_g;

			// // Shuffle history buffer
			sdm3[1] = sdm2[1];
			sdm2[1] = sdm1[1];
			sdm1[1] = sample;

			blockR->data[i] = (lpOut + midOut + hpOut);			

		}
		AudioStream_F32::transmit(blockL, 0);
		AudioStream_F32::transmit(blockR, 1);
		AudioStream_F32::release(blockL);
		AudioStream_F32::release(blockR);
	}
	void bypass_set(bool s) { bp = s; }
	bool bypass_tgl()
	{
		bp ^= 1;
		return bp;
	}

private:
	audio_block_f32_t *inputQueueArray[2];
	bool bp = false;

	float32_t f1p0[2] = {0.0f, 0.0f};
	float32_t f1p1[2] = {0.0f, 0.0f};
	float32_t f1p2[2] = {0.0f, 0.0f};
	float32_t f1p3[2] = {0.0f, 0.0f};
	float32_t f2p0[2] = {0.0f, 0.0f};
	float32_t f2p1[2] = {0.0f, 0.0f};
	float32_t f2p2[2] = {0.0f, 0.0f};
	float32_t f2p3[2] = {0.0f, 0.0f};

	float32_t sdm1[2] = {0.0f, 0.0f};
	float32_t sdm2[2] = {0.0f, 0.0f};
	float32_t sdm3[2] = {0.0f, 0.0f};

	static constexpr float32_t vsa = (1.0 / 4294967295.0); // Very small amount (Denormal Fix)
	float32_t lpreg[2];
	float32_t hpreg[2];
	float32_t lowpass_f;
	float32_t bass_g = 1.0f;
	float32_t hipass_f;
	float32_t treble_g = 1.0f;
	float32_t mid_g = 1.0f;
};

#endif