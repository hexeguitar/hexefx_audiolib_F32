#ifndef _BASIC_DSPUTILS_H_
#define _BASIC_DSPUTILS_H_

#include <arm_math.h>

static inline void mix_pwr(float32_t mix, float32_t *wetMix, float32_t *dryMix);
static inline void mix_pwr(float32_t mix, float32_t *wetMix, float32_t *dryMix)
{
    // Calculate mix parameters
    //    A cheap mostly energy constant crossfade from SignalSmith Blog
    //    https://signalsmith-audio.co.uk/writing/2021/cheap-energy-crossfade/
    float32_t x2 = 1.0f - mix;
    float32_t A = mix*x2;
    float32_t B = A * (1.0f + 1.4186f * A);
    float32_t C = B + mix;
    float32_t D = B + x2;

    *wetMix = C * C;
    *dryMix = D * D;
}

#endif // _BASIC_DSPUTILS_H_
