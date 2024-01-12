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
		free(bf);
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
private:
	float *kPtr;
	float *bf;
	uint32_t idx;
	const uint32_t len = N*sizeof(float);
};


#endif // _FILTER_ALLPASS_H_
