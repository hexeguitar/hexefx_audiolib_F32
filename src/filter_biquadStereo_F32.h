#ifndef _FILTER_BIQUADSTEREO_F32_H_
#define _FILTER_BIQUADSTEREO_F32_H_

#include "Arduino.h"
#include "AudioStream_F32.h"
#include "arm_math.h"
#include "basic_DSPutils.h"

// Changed Feb 2021
#define IIR_STEREO_MAX_STAGES 4

class AudioFilterBiquadStereo_F32 : public AudioStream_F32
{
	// GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
	// GUI: shortName:IIR2
public:
	AudioFilterBiquadStereo_F32(uint8_t stages=IIR_STEREO_MAX_STAGES) : AudioStream_F32(2, inputQueueArray), numStagesUsed(stages)
	{
		setSampleRate_Hz(AUDIO_SAMPLE_RATE_EXACT);
		doClassInit();
	}
	AudioFilterBiquadStereo_F32(const AudioSettings_F32 &settings, uint8_t stages=IIR_STEREO_MAX_STAGES) : AudioStream_F32(2, inputQueueArray), numStagesUsed(stages)
	{
		setSampleRate_Hz(settings.sample_rate_Hz);
		doClassInit();
	}

	void doClassInit(void)
	{
		memset(&coeff32[0], 0, 5 * IIR_STEREO_MAX_STAGES * sizeof(coeff32[0]));
		for (int ii = 0; ii < 4; ii++)
		{
			coeff32[5 * ii] = 1.0f; // b0 = 1 for pass through
		}
		arm_biquad_cascade_df1_init_f32(&iirL_inst, numStagesUsed, &coeff32[0], &stateL_F32[0]);
		arm_biquad_cascade_df1_init_f32(&iirR_inst, numStagesUsed, &coeff32[0], &stateR_F32[0]);
		doBiquad = false;  // This is the way to jump over the biquad
	}

	// Up to 4 stages are allowed.  Coefficients, either by design function
	// or from direct setCoefficients() need to be added to the double array
	// and also to the float
	void setCoefficients(int iStage, double *cf)
	{
		if (iStage > numStagesUsed)
		{
			if (Serial)
			{
				Serial.print("AudioFilterBiquad_F32: setCoefficients:");
				Serial.println(" *** MaxStages Error");
			}
			return;
		}
		
		for (int ii = 0; ii < 5; ii++)
		{
			coeff32[ii + 5 * iStage] = (float)cf[ii]; // and of floats
		}

		doBiquad = true;
	}

	void end(void)
	{
		doBiquad = false;
	}

	void setSampleRate_Hz(float _fs_Hz) { sampleRate_Hz = _fs_Hz; }

	//  Deprecated
	void setBlockDC(void)
	{
		// https://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html#ga8e73b69a788e681a61bccc8959d823c5
		// Use matlab to compute the coeff for HP at 40Hz: [b,a]=butter(2,40/(44100/2),'high'); %assumes fs_Hz = 44100
		double b[] = {8.173653471988667e-01, -1.634730694397733e+00, 8.173653471988667e-01}; // from Matlab
		double a[] = {1.000000000000000e+00, -1.601092394183619e+00, 6.683689946118476e-01}; // from Matlab
		setFilterCoeff_Matlab(b, a);
	}

	void setFilterCoeff_Matlab(double b[], double a[])
	{ // one stage of N=2 IIR
		double coeff[5];
		// https://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html#ga8e73b69a788e681a61bccc8959d823c5
		// Use matlab to compute the coeff, such as: [b,a]=butter(2,20/(44100/2),'high'); %assumes fs_Hz = 44100
		coeff[0] = b[0];
		coeff[1] = b[1];
		coeff[2] = b[2]; // here are the matlab "b" coefficients
		coeff[3] = -a[1];
		coeff[4] = -a[2]; // the DSP needs the "a" terms to have opposite sign vs Matlab
		setCoefficients(0, coeff);
	}
	// Compute common filter functions
	// http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
	// void setLowpass(uint32_t stage, float frequency, float q = 0.7071) {
	void setLowpass(int stage, float frequency, float q)
	{
		double coeff[5];
		double w0 = frequency * (2 * 3.141592654 / sampleRate_Hz);
		double sinW0 = sin(w0);
		double alpha = sinW0 / ((double)q * 2.0);
		double cosW0 = cos(w0);
		double scale = 1.0 / (1.0 + alpha); // which is equal to 1.0 / a0
		/* b0 */ coeff[0] = ((1.0 - cosW0) / 2.0) * scale;
		/* b1 */ coeff[1] = (1.0 - cosW0) * scale;
		/* b2 */ coeff[2] = coeff[0];
		/* a1 */ coeff[3] = -(-2.0 * cosW0) * scale;
		/* a2 */ coeff[4] = -(1.0 - alpha) * scale;
		setCoefficients(stage, coeff);
	}

	void setHighpass(uint32_t stage, float frequency, float q)
	{
		double coeff[5];
		double w0 = frequency * (2 * 3.141592654 / sampleRate_Hz);
		double sinW0 = sin(w0);
		double alpha = sinW0 / ((double)q * 2.0);
		double cosW0 = cos(w0);
		double scale = 1.0 / (1.0 + alpha);
		/* b0 */ coeff[0] = ((1.0 + cosW0) / 2.0) * scale;
		/* b1 */ coeff[1] = -(1.0 + cosW0) * scale;
		/* b2 */ coeff[2] = coeff[0];
		/* a1 */ coeff[3] = -(-2.0 * cosW0) * scale;
		/* a2 */ coeff[4] = -(1.0 - alpha) * scale;
		setCoefficients(stage, coeff);
	}

	void setBandpass(uint32_t stage, float frequency, float q)
	{
		double coeff[5];
		double w0 = frequency * (2 * 3.141592654 / sampleRate_Hz);
		double sinW0 = sin(w0);
		double alpha = sinW0 / ((double)q * 2.0);
		double cosW0 = cos(w0);
		double scale = 1.0 / (1.0 + alpha);
		/* b0 */ coeff[0] = alpha * scale;
		/* b1 */ coeff[1] = 0;
		/* b2 */ coeff[2] = (-alpha) * scale;
		/* a1 */ coeff[3] = -(-2.0 * cosW0) * scale;
		/* a2 */ coeff[4] = -(1.0 - alpha) * scale;
		setCoefficients(stage, coeff);
	}

	// frequency in Hz.  q makes the response stay close to 0.0dB until
	// close to the notch frequency.  q up to 100 or more seem stable.
	void setNotch(uint32_t stage, float frequency, float q)
	{
		double coeff[5];
		double w0 = frequency * (2 * 3.141592654 / sampleRate_Hz);
		double sinW0 = sin(w0);
		double alpha = sinW0 / ((double)q * 2.0);
		double cosW0 = cos(w0);
		double scale = 1.0 / (1.0 + alpha); // which is equal to 1.0 / a0
		/* b0 */ coeff[0] = scale;
		/* b1 */ coeff[1] = (-2.0 * cosW0) * scale;
		/* b2 */ coeff[2] = coeff[0];
		/* a1 */ coeff[3] = -(-2.0 * cosW0) * scale;
		/* a2 */ coeff[4] = -(1.0 - alpha) * scale;
		setCoefficients(stage, coeff);
	}

	void setLowShelf(uint32_t stage, float frequency, float gain, float slope)
	{
		double coeff[5];
		double a = pow(10.0, gain / 40.0);
		double w0 = frequency * (2 * 3.141592654 / sampleRate_Hz);
		double sinW0 = sin(w0);
		// double alpha = (sinW0 * sqrt((a+1/a)*(1/slope-1)+2) ) / 2.0;
		double cosW0 = cos(w0);
		// generate three helper-values (intermediate results):
		double sinsq = sinW0 * sqrt((pow(a, 2.0) + 1.0) * (1.0 / slope - 1.0) + 2.0 * a);
		double aMinus = (a - 1.0) * cosW0;
		double aPlus = (a + 1.0) * cosW0;
		double scale = 1.0 / ((a + 1.0) + aMinus + sinsq);
		/* b0 */ coeff[0] = a * ((a + 1.0) - aMinus + sinsq) * scale;
		/* b1 */ coeff[1] = 2.0 * a * ((a - 1.0) - aPlus) * scale;
		/* b2 */ coeff[2] = a * ((a + 1.0) - aMinus - sinsq) * scale;
		/* a1 */ coeff[3] = 2.0 * ((a - 1.0) + aPlus) * scale;
		/* a2 */ coeff[4] = -((a + 1.0) + aMinus - sinsq) * scale;
		setCoefficients(stage, coeff);
	}

	void setHighShelf(uint32_t stage, float frequency, float gain, float slope)
	{
		double coeff[5];
		double a = pow(10.0, gain / 40.0);
		double w0 = frequency * (2 * 3.141592654 / sampleRate_Hz);
		double sinW0 = sin(w0);
		// double alpha = (sinW0 * sqrt((a+1/a)*(1/slope-1)+2) ) / 2.0;
		double cosW0 = cos(w0);
		// generate three helper-values (intermediate results):
		double sinsq = sinW0 * sqrt((pow(a, 2.0) + 1.0) * (1.0 / slope - 1.0) + 2.0 * a);
		double aMinus = (a - 1.0) * cosW0;
		double aPlus = (a + 1.0) * cosW0;
		double scale = 1.0 / ((a + 1.0) - aMinus + sinsq);
		/* b0 */ coeff[0] = a * ((a + 1.0) + aMinus + sinsq) * scale;
		/* b1 */ coeff[1] = -2.0 * a * ((a - 1.0) + aPlus) * scale;
		/* b2 */ coeff[2] = a * ((a + 1.0) + aMinus - sinsq) * scale;
		/* a1 */ coeff[3] = -2.0 * ((a - 1.0) - aPlus) * scale;
		/* a2 */ coeff[4] = -((a + 1.0) - aMinus - sinsq) * scale;
		setCoefficients(stage, coeff);
	}
	void update(void);

	void mix(float m)
	{
		float g_wet, g_dry;
		m = constrain(m, 0.0f, 1.0f);
		mix_pwr(m, &g_wet, &g_dry);
		__disable_irq();
		gain_wet = g_wet;
		gain_dry = g_dry;
		__enable_irq();
	}
	void makeupGain(float g)
	{
		__disable_irq();
		makeup_gain = g;
		__enable_irq();		
	}

 	void bypass_set(bool state) 
	{
		__disable_irq();
		bp = state;
		__enable_irq();
	}
    bool bypass_tgl(void) 
    {
		bool bp_new = bp ^ 1;
		__disable_irq();
        bp  = bp_new; 
		__enable_irq();
        return bp;
    }
private:
	audio_block_f32_t *inputQueueArray[2];
	bool bp = false;
	float coeff32[5 * IIR_STEREO_MAX_STAGES]; // Local copies to be transferred with begin()
	float stateL_F32[4 * IIR_STEREO_MAX_STAGES];
	float stateR_F32[4 * IIR_STEREO_MAX_STAGES];
	float sampleRate_Hz = AUDIO_SAMPLE_RATE_EXACT; // default.  from AudioStream.h??
	const uint8_t numStagesUsed;
	bool doBiquad = false;
	float gain_dry = 0.0f;
	float gain_wet = 1.0f;
	float makeup_gain = 1.0f;

	/* Info - The structure from arm_biquad_casd_df1_inst_f32 consists of
	 *    uint32_t  numStages;
	 *    const float32_t *pCoeffs;  //Points to the array of coefficients, length 5*numStages.
	 *    float32_t *pState;         //Points to the array of state variables, length 4*numStages.
	 */
	// ARM DSP Math library filter instance.
	arm_biquad_casd_df1_inst_f32 iirL_inst;
	arm_biquad_casd_df1_inst_f32 iirR_inst;
};

#endif // _FILTER_BIQUADSTEREO_F32_H_
