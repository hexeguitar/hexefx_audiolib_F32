#ifndef _BASIC_BYPASSSTEREO_F32_H_
#define _BASIC_BYPASSSTEREO_F32_H_

#include "Arduino.h"
#include "AudioStream_F32.h"


typedef enum
{
	BYPASS_MODE_PASS,		// pass the input signal to the output
	BYPASS_MODE_OFF,		// mute the output
	BYPASS_MODE_TRAILS		// mutes the input only
}bypass_mode_t;

/**
 * @brief Stereo bypass handling and validating the input audio blocks
 * 			The main component has to provide the pointers to audio blocks received inside the "update" function
 * 			This function checks whether they are avaiable and genereates silence where needed (null block received)
 * 			With the bypass OFF (state=false) it can be used to validate the input blocks before processing.
 * 
 * @param p_blockL 	pointer to an audio_block_f32_t pointer received/allocated in the main compontnt, channel L
 * @param p_blockR  pointer to an audio_block_f32_t pointer received/allocated in the main compontnt, channel R
 * @param mode 		one of the bypass models defined in bypass_mode_t enum
 * @param state 	bypass state, true = effect OFF, false = effect ON
 * @return true 	operation succesful, blocks are available for further processing
 * @return false 	fail, some of the blocks are not available, do not process the data
 */
bool bypass_process(audio_block_f32_t** p_blockL, audio_block_f32_t** p_blockR, bypass_mode_t mode, bool state);


#endif // _BASIC_BYPASSSTEREO_F32_H_
