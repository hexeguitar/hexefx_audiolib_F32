#ifndef _BASIC_DSPUTILS_H_
#define _BASIC_DSPUTILS_H_

#include <Arduino.h>
#include <arm_math.h>

#define F32_TO_I32_NORM_FACTOR (2147483647) // which is 2^31-1
#define I32_TO_F32_NORM_FACTOR (4.656612875245797e-10)   //which is 1/(2^31 - 1)

static inline void mix_pwr(float32_t mix, float32_t *wetMix, float32_t *dryMix);
static inline void mix_pwr(float32_t mix, float32_t *wetMix, float32_t *dryMix)
{
    // Calculate mix parameters
    //    A cheap mostly energy constant crossfade from SignalSmith Blog
    //    https://signalsmith-audio.co.uk/writing/2021/cheap-energy-crossfade/
	mix = constrain(mix, 0.0f, 1.0f);
    float32_t x2 = 1.0f - mix;
    float32_t A = mix*x2;
    float32_t B = A * (1.0f + 1.4186f * A);
    float32_t C = B + mix;
    float32_t D = B + x2;

    *wetMix = C * C;
    *dryMix = D * D;
}

void scale_float_to_int32range(const float32_t *pSrc, float32_t *pDst, uint32_t blockSize);

/**
  * @brief  combine two separate buffers into interleaved one
  * @param  sz -  samples per output buffer (divisible by 2)
  * @param  dst - pointer to source buffer
  * @param  srcA - pointer to A source buffer (even samples)
  * @param  srcB - pointer to B source buffer (odd samples)
  * @retval none
  */
inline void memcpyInterleave_f32(float32_t *srcA, float32_t *srcB, float32_t *dst, int16_t sz)
{
	while(sz)
	{
		    *dst++ = *srcA++;
		    *dst++ = *srcB++;
			sz--;
 		    *dst++ = *srcA++;
		    *dst++ = *srcB++;
			sz--;           
	}
}
inline void memcpyInterleave_f32(float32_t *srcA, float32_t *srcB, float32_t *dst, int16_t sz);

inline void memcpyDeinterleave_f32(float32_t *src, float32_t *dstA, float32_t *dstB, int16_t sz)
{
	while(sz)
	{
		*dstA++ = *src++;
		*dstB++ = *src++;
		sz--;
		*dstA++ = *src++;
		*dstB++ = *src++;
		sz--;		
	}
}
inline void memcpyDeinterleave_f32(float32_t *src, float32_t *dstA, float32_t *dstB, int16_t sz);
#endif // _BASIC_DSPUTILS_H_
