/**
 * @file basic_shelvFilter.h
 * @author Piotr Zapart www.hexefx.com
 * @brief basic hp/lp filter class
 * @version 1.0
 * @date 2024-01-09
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef _BASIC_SHELVFILTER_H_
#define _BASIC_SHELVFILTER_H_

#include <Arduino.h>

class AudioFilterShelvingLPHP
{
public:
	void init(float hp_freq, float *hp_damp, float lp_freq, float *lp_damp)
	{
		hidampPtr = lp_damp;
		hidamp = *hidampPtr;
		hp_f = hp_freq;
		lodampPtr = hp_damp;
		lodamp = *lodampPtr;
		lp_f = lp_freq;
		lpreg = 0.0f;
		hpreg = 0.0f;
	}
	float process(float input)
	{
		float tmp1, tmp2;
		// smoothly update params
		if (hidamp < (*hidampPtr))
		{
			hidamp += upd_step;
			if (hidamp >(*hidampPtr)) hidamp = *hidampPtr;
		}
		if (hidamp > (*hidampPtr))
		{
			hidamp -= upd_step;
			if (hidamp < (*hidampPtr)) hidamp = *hidampPtr;
		}
		if (lodamp < (*lodampPtr))
		{
			lodamp += upd_step;
			if (lodamp >(*lodampPtr)) lodamp = *lodampPtr;
		}
		if (lodamp > (*lodampPtr))
		{
			lodamp -= upd_step;
			if (lodamp < (*lodampPtr)) lodamp = *lodampPtr;
		}

		tmp1 = input - lpreg;
        lpreg += tmp1 * lp_f;
        tmp2 = input - lpreg;
        tmp1 = lpreg - hpreg;
        hpreg += tmp1 * hp_f;
		return (lpreg + hidamp*tmp2 + lodamp * hpreg);
	}
private:
	float lpreg;
	float hpreg;
	float *lodampPtr;
	float *hidampPtr;
	float hidamp; 
	float lodamp;
	float hp_f;
	float lp_f;
	static constexpr float upd_step = 0.02f;
};


class AudioFilterLP
{
public:	
	void init(float *lp_freq)
	{
		lp_fPtr = lp_freq;
		lpreg = 0.0f;
	}
	float process(float input)
	{
		float tmp;
        tmp = input - lpreg;
        lpreg += (*lp_fPtr) *tmp;
		return lpreg;
	}
private:
	float lpreg;
	float *lp_fPtr;
};



#endif // _BASIC_SHELVFILTER_H_
