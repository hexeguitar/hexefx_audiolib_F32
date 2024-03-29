#include "filter_biquadStereo_F32.h"

void AudioFilterBiquadStereo_F32::update(void)
{
	audio_block_f32_t *blockL, *blockR, *blockOutL, *blockOutR;
	blockL = AudioStream_F32::receiveWritable_f32(0);
	blockR = AudioStream_F32::receiveWritable_f32(1);

	// no input signal
	if (!blockL || !blockR)
	{
		if (blockL) AudioStream_F32::release(blockL);
		if (blockR) AudioStream_F32::release(blockR);
		return;
	}
	if (bp || gain_dry == 1.0f || !doBiquad)
	{
		AudioStream_F32::transmit(blockL, 0);	
		AudioStream_F32::transmit(blockR, 1);
		AudioStream_F32::release(blockL);
		AudioStream_F32::release(blockR);
		return;
	}

	blockOutL = AudioStream_F32::allocate_f32();
	blockOutR = AudioStream_F32::allocate_f32();
	if (!blockOutL || !blockOutR)
	{
		if (blockOutL) AudioStream_F32::release(blockOutL);
		if (blockOutR) AudioStream_F32::release(blockOutR);
		AudioStream_F32::release(blockL);
		AudioStream_F32::release(blockR);		
		return;
	}
	arm_biquad_cascade_df1_f32(&iirL_inst, blockL->data, blockOutL->data, blockOutL->length);
	arm_biquad_cascade_df1_f32(&iirR_inst, blockR->data, blockOutR->data, blockOutR->length);
	if (gain_wet != 1.0f)	// transmit wet only
	{
		arm_scale_f32(blockL->data, gain_dry, blockL->data, blockL->length); 				// dryL * gain_dry
		arm_scale_f32(blockR->data, gain_dry, blockR->data, blockR->length); 				// dryR * gain_dry
		arm_scale_f32(blockOutL->data, gain_wet, blockOutL->data, blockOutL->length); 		// wetL * gain_wet
		arm_scale_f32(blockOutR->data, gain_wet, blockOutR->data, blockOutR->length); 		// wetR * gain_wet	
		arm_add_f32(blockL->data, blockOutL->data, blockOutL->data, blockOutL->length);	 	// dryL+wetL
		arm_add_f32(blockR->data, blockOutR->data, blockOutR->data, blockOutR->length);	 	// dryR+wetR
	}
	if (makeup_gain != 1.0f)
	{
		arm_scale_f32(blockOutL->data, makeup_gain, blockOutL->data, blockOutL->length); 		// wetL * makeup gain
		arm_scale_f32(blockOutR->data, makeup_gain, blockOutR->data, blockOutR->length); 		// wetR * makeup gain		
	}
	AudioStream_F32::transmit(blockOutL, 0);
	AudioStream_F32::transmit(blockOutR, 1);
	AudioStream_F32::release(blockOutL);
	AudioStream_F32::release(blockOutR);
	AudioStream_F32::release(blockL);
	AudioStream_F32::release(blockR);

	
}
