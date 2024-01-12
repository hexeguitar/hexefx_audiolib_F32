#ifndef _HX_MEMCPY_H
#define _HX_MEMCPY_H

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


#endif // _HX_MEMCPY_H
