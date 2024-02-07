/**
 * @file basic_delay.h
 * @author Piotr Zapart www.hexefx.com
 * @brief basic delay line 
 * @version 1.0
 * @date 2024-01-09
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef _BASIC_DELAY_H_
#define _BASIC_DELAY_H_

#include "Arduino.h"


/**
 * @brief Basic delay line with buffer placed in PSRAM
 * 
 * @tparam N delay length in samples (float)
 */
//template <int N>
class AudioBasicDelay
{
public:
	~AudioBasicDelay()
	{
		if(bf) free(bf);
	}
	bool init(uint32_t size_samples,  bool psram=false)
	{
		if(bf) free(bf);
		size = size_samples;
		if (psram) bf = (float *)extmem_malloc(size * sizeof(float)); // allocate buffer
		else bf = (float *)malloc(size * sizeof(float)); // allocate buffer
		if (!bf) return false;
		idx = 0;
		reset();
		return true;
	}
	void reset()
	{
		memset(bf, 0, size * sizeof(float));
	}
	/**
	 * @brief get the tap from the delay buffer
	 * 
	 * @param offset 	delay time 
	 * @return float 
	 */
	inline float getTap(uint32_t offset, float frac=0.0f)
	{
		int32_t read_idx, read_idx_next; 
		read_idx = idx - offset;
		if (read_idx < 0) read_idx += size;
		if (frac == 0.0f) return bf[read_idx];
		read_idx_next = read_idx - 1;
		if (read_idx_next < 0) read_idx_next += size;
		return (bf[read_idx]*(1.0f-frac) + bf[read_idx_next]*frac);
	}

    inline const float getTapHermite(float delay) const
    {
        int32_t delay_integral   = static_cast<int32_t>(delay);
        float   delay_fractional = delay - static_cast<float>(delay_integral);

        int32_t     t     = (idx + delay_integral + size);
        const float     xm1   = bf[(t - 1) % size];
        const float     x0    = bf[(t) % size];
        const float     x1    = bf[(t + 1) % size];
        const float     x2    = bf[(t + 2) % size];
        const float c     = (x1 - xm1) * 0.5f;
        const float v     = x0 - x1;
        const float w     = c + v;
        const float a     = w + v + (x2 - x0) * 0.5f;
        const float b_neg = w + a;
        const float f     = delay_fractional;
        return (((a * f) - b_neg) * f + c) * f + x0;
    }

	/**
	 * @brief read last sample and write a new one
	 * 
	 * @param newSample new sample written to the start address
	 * @return float lase sample read from the end of the buffer
	 */
	inline float process(float newSample)
	{
		float out = bf[idx];
		bf[idx] = newSample;
		
		return out; 
	}
	inline void write_toOffset(float newSample, uint32_t offset)
	{
		int32_t write_idx;
		write_idx = idx - offset;
		if (write_idx < 0) write_idx += size;
		bf[write_idx] = newSample;
	}
	inline void updateIndex()
	{
		if (++idx >= size) idx = 0;
	}
private:
	int32_t size; 
	float *bf;
	int32_t idx;
};

#endif // _BASIC_DELAY_H_
