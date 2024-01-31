/**
 * @file basic_allpass.h
 * @author Piotr Zapart
 * @brief basic allpass filter
 * @version 1.0
 * @date 2024-01-09
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef _FILTER_ALLPASS_H_
#define _FILTER_ALLPASS_H_

#include "Arduino.h"
template <int N>
class AudioFilterAllpass
{
public:
	~AudioFilterAllpass()
	{
		if (bf) free(bf);
	}
	/**
	 * @brief Allocate the filter buffer in RAM
	 * 			set the pointer to the allpass coeff
	 * 
	 * @param coeffPtr pointer to the allpas coeff variable
	 */
	bool init(float* coeffPtr)
	{
		bf = (float *)malloc(N*sizeof(float)); // allocate buffer
		if (!bf) return false;
		kPtr = coeffPtr;
		reset();
		return true;
	}
	/**
	 * @brief zero the allpass buffer
	 * 
	 */
	void reset()
	{
		memset(bf, 0, N*sizeof(float));
		idx = 0;
	}
	/**
	 * @brief process new sample
	 * 
	 * @param in input sample
	 * @return float output sample
	 */
	float process(float in)
	{
		float out = bf[idx] + (*kPtr) * in;
		bf[idx] = in - (*kPtr) * out;
		if (++idx >= N) idx = 0;
		return out;
	}
	/**
	 * @brief Set new coeff pointer
	 * 
	 * @param coeffPtr 
	 */
	void coeff(float* coeffPtr)
	{
		kPtr = coeffPtr;
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
		if (read_idx < 0) read_idx += N;
		if (frac == 0.0f) return bf[read_idx];
		read_idx_next = read_idx - 1;
		if (read_idx_next < 0) read_idx_next += N;
		return (bf[read_idx]*(1.0f-frac) + bf[read_idx_next]*frac);
	}
	inline void write_toOffset(float newSample, uint32_t offset)
	{
		int32_t write_idx;
		write_idx = idx - offset;
		if (write_idx < 0) write_idx += N;
		bf[write_idx] = newSample;
	}	
private:
	float *kPtr;
	float *bf;
	uint32_t idx;
};


#endif // _FILTER_ALLPASS_H_
