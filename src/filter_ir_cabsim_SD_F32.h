/* filter_ir_cabsim_SD_F32.h
 *
 * Piotr Zapart 11.2024 www.hexefx.com
 *  - Combined into a stereo speaker simulator
 *  - Added IR wav file loading from an SD card
 *  - Added stereo enhancer for double tracking emulation
 * 
 * based on:
 *               A u d i o FilterConvolutionUP
 * Uniformly-Partitioned Convolution Filter for Teeny 4.0
 * Written by Brian Millier November 2019
 * adapted from routines written for Teensy 4.0 by Frank DD4WH 
 * that were based upon code/literature by Warren Pratt 
 *
 * This program is free software: you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. 
 * If not, see <https://www.gnu.org/licenses/>."
 */
#ifndef _FILTER_IR_CABSIM_SD_H
#define _FILTER_IR_CABSIM_SD_H

#include <Arduino.h>
#include "AudioStream_F32.h"
#include "arm_math.h"
#include <SD.h>
#include "basic_delay.h"
#include "basic_shelvFilter.h"
#include "basic_DSPutils.h"
#include <arm_const_structs.h>


#define TCAB_BUFFER_SIZE  		(128)
#define TCAB_IR_LEN_MAX_SAMPLES	(8192)
#define TCAB_NFORMAX      		(TCAB_IR_LEN_MAX_SAMPLES / TCAB_BUFFER_SIZE)
#define TCAB_FFT_LENGTH   		(2 * TCAB_BUFFER_SIZE)
#define TCAB_N_B          		(1)
#define TCAB_OFF_MSG			("OFF")
#define TCAB_DEFAULT_IR_PATH	("ir")
#define TCAB_DEFAULT_CONF_PATH	("config.txt")


class AudioFilterIRCabsim_SD_F32 : public AudioStream_F32
{
public:
    AudioFilterIRCabsim_SD_F32();
	void begin();
    virtual void update(void);
	
	typedef enum
	{
		IR_WAV_SUCCESS = 0,
		IR_WAV_ERR_NO_RIFF,
		IR_WAV_ERR_NO_WAV,
		IR_WAV_ERR_NO_HEADER,
		IR_WAV_ERR_TYPE_NOT_1,
		IR_WAV_ERR_BAD_CHANNELS,	// only 1 or 2 channel wav
		IR_WAV_ERR_BAD_FS,
		IR_WAV_ERR_BAD_BPS,
		IR_WAV_ERR_BAD_BITS,		// only 8, 16 or 24
		IR_WAV_ERR_NO_DATA,
		IR_WAV_ERR_NO_FMT,
		IR_WAV_ERR_FILE_NOT_FOUND,
		IR_WAV_LAST
	}ir_wav_result_t;

    ir_wav_result_t ir_load(const char *filePath);
	ir_wav_result_t ir_load(File &file);
	ir_wav_result_t ir_load(uint16_t fileIndex);
	bool ir_load(float32_t* dataPtr, size_t dataLength);
	ir_wav_result_t ir_load_next();
	ir_wav_result_t ir_load_prev();
	ir_wav_result_t ir_load_first();

	const char* get_err_msg(ir_wav_result_t res)
	{
		if (res < IR_WAV_LAST) return err_msg[res];
		return "";
	}
	char* ir_get_name()
	{
		return ir_file_name;
	}
	void ir_get_idx(uint16_t* idxPtr, uint16_t* idxTotalPtr)
	{
		if (idxPtr) *idxPtr = ir_file_idx + 1;
		if (idxTotalPtr) *idxTotalPtr = ir_file_total;
	}
    float32_t ir_get_len_ms(void)
    {
		return ir_length_ms;
    }
	void ir_get_params(	uint16_t* idxPtr, uint16_t* idxTotalPtr, 
						uint8_t* bitsPtr, float32_t* lenMsPtr, 
						char** namePtr)
	{

		if (idxPtr) *idxPtr = ir_file_idx + 1;
		if (idxTotalPtr) *idxTotalPtr = ir_file_total;
		if (bitsPtr) *bitsPtr = ir_bitdepth;
		if(lenMsPtr) *lenMsPtr = ir_length_ms;
		if(namePtr) *namePtr = ir_file_name;
	}
	void doubler_set(bool s)
	{
		__disable_irq();
		doubleTrack = s;
		if (doubleTrack) 
		{
			delay.reset();
		}
		__enable_irq();
	}	
	bool doubler_tgl()
	{
		__disable_irq();
		doubleTrack = doubleTrack ? false : true;
		if (doubleTrack) 
		{
			delay.reset();
		}
		__enable_irq();
		return doubleTrack;
	}
	bool doubler_get() {return doubleTrack;}
	
	void input_gain(float32_t g)
	{
		audio_gain = constrain(g, 0.0f, 1.0f);
	}

	bool init_done() 
	{
		return initialized;
	}
	bool bypass_get()
	{
		return !ir_loaded;
	}
	void factory_reset();
	const char* get_ir_path() { return default_ir_path; }
	const char* get_conf_path() {return default_conf_path; }


private:
    audio_block_f32_t *inputQueueArray_f32[2];
	uint16_t block_size = AUDIO_BLOCK_SAMPLES;
    float32_t audio_gain = 1.0f;
    int idx_t = 0;
    int16_t *sp_L;
    int16_t *sp_R;
    const uint32_t FFT_L = TCAB_FFT_LENGTH;
    uint8_t first_block = 1;
    uint8_t ir_loaded = 0;  

    uint32_t nfor = 0;
    const uint32_t nforMax = TCAB_NFORMAX;

    int buffidx = 0;
    int k = 0;
	uint8_t fmask_idx = 0;
	float32_t fmask[TCAB_NFORMAX][TCAB_FFT_LENGTH * 2];
	float32_t ac2[512];
	float32_t accum[TCAB_FFT_LENGTH * 2];
	float32_t fftin[TCAB_FFT_LENGTH * 2];
	float32_t* last_sample_buffer_R;
	float32_t* last_sample_buffer_L;
	float32_t* maskgen;
	float32_t* fftout;

	float32_t* wav_ir_data;
	static const float32_t* ir_default_guitar;

	float32_t* ptr_fftout;
	float32_t* ptr_fmask;
	float32_t* ptr1;
	float32_t* ptr2;
	int k512;
	int j512;

	const arm_cfft_instance_f32 *S = &arm_cfft_sR_f32_len256;
	const arm_cfft_instance_f32 *iS = &arm_cfft_sR_f32_len256;
	
	static const uint32_t delay_l = AUDIO_SAMPLE_RATE * 0.01277f; 	//12ms delay
	AudioBasicDelay delay;

	float32_t ir_length_ms = 0.0f;
	uint8_t ir_bitdepth = 24;
    void init_partitioned_filter_masks(const float32_t *irPtr);
	bool initialized = false;
	
	// stereo doubler
	static constexpr float32_t doubler_gainL = 0.55f;
	static constexpr float32_t doubler_gainR = 0.65f;
	bool doubleTrack = false;
	static const uint8_t nfir = 30;	// fir taps
	arm_fir_instance_f32 FIR_preL, FIR_preR, FIR_postL, FIR_postR;
	float32_t FIRstate[4][AUDIO_BLOCK_SAMPLES + nfir];
	float32_t FIRk_preL[30] = {
		 0.000894872763f,  0.00020902598f,  0.000285242248f,  0.000503875781f,  0.00207542209f,  0.0013392308f, 
		-0.00476867426f,  -0.0112718018f,  -0.00560652791f,   0.0158470348f,    0.0319586769f,   0.0108086104f, 
		-0.0470990688f,   -0.0834295526f,  -0.0208595414f,    0.154734746f,     0.35352844f,     0.441179603f, 
		 0.35352844f,      0.154734746f,   -0.0208595414f,   -0.0834295526f,   -0.0470990688f,   0.0108086104f, 
		 0.0319586769f,    0.0158470348f,  -0.00560652791f,  -0.0112718018f,   -0.00476867426f,  0.0013392308f };
	float32_t FIRk_preR[30] = {
		 0.00020902598f,   0.000285242248f, 0.000503875781f,  0.00207542209f,   0.0013392308f,  -0.00476867426f, 
		-0.0112718018f,   -0.00560652791f,  0.0158470348f,    0.0319586769f,    0.0108086104f,  -0.0470990688f,
		-0.0834295526f,   -0.0208595414f,   0.154734746f,     0.35352844f,      0.441179603f,    0.35352844f, 
		 0.154734746f,    -0.0208595414f,  -0.0834295526f,   -0.0470990688f,    0.0108086104f,   0.0319586769f, 
		 0.0158470348f,   -0.00560652791f, -0.0112718018f,   -0.00476867426f,   0.0013392308f,   0.00207542209f };
	float32_t FIRk_postL[30] = {
		 0.000285242248f,  0.000503875781f, 0.00207542209f,   0.0013392308f,   -0.00476867426f, -0.0112718018f,
		-0.00560652791f,   0.0158470348f,   0.0319586769f,    0.0108086104f,   -0.0470990688f,  -0.0834295526f,
		-0.0208595414f,    0.154734746f,    0.35352844f,      0.441179603f,     0.35352844f,     0.154734746f,
		-0.0208595414f,   -0.0834295526f,  -0.0470990688f,    0.0108086104f,    0.0319586769f,   0.0158470348f,
		-0.00560652791f,  -0.0112718018f,  -0.00476867426f,   0.0013392308f,    0.00207542209f,  0.000503875781f };
	float32_t FIRk_postR[30] = {
		 0.000503875781f,  0.00207542209f,  0.0013392308f,   -0.00476867426f,  -0.0112718018f,  -0.00560652791f, 
		 0.0158470348f,    0.0319586769f,   0.0108086104f,   -0.0470990688f,   -0.0834295526f,  -0.0208595414f, 
		 0.154734746f,     0.35352844f,     0.441179603f,     0.35352844f,      0.154734746f,   -0.0208595414f,
		-0.0834295526f,   -0.0470990688f,   0.0108086104f,    0.0319586769f,    0.0158470348f,  -0.00560652791f, 
		-0.0112718018f,   -0.00476867426f,  0.0013392308f,    0.00207542209f,   0.000503875781f, 0.0f };

	// ------------------- SD CARD -------------------------------------
	static const char* off_msg;
	static const char* default_ir_path;
	static const char* default_conf_path;
	static const char* const* err_msg;
	uint16_t ir_file_idx;	 	// index of the current file in the /ir directory
	uint16_t ir_file_total;	 	// total number of files in the /ir directory
	char* ir_file_name;			// buffer in RAM2 contating the name of the loaded IR file
	File ir_folder = NULL;

	uint16_t sd_read_u16(File &file)
	{
		uint16_t res = file.read();
		res |= (file.read() << 8);
		return res;
	}

	uint32_t sd_read_u32(File &file)
	{
		uint32_t res = file.read();
		res |= (file.read() << 8);
		res |= (file.read() << 16);
		res |= (file.read() << 24);
		return res;
	}
	/**
	 * @brief read a 16bit sample and convert it to float -1.0 ... 1.0 range
	 * 
	 * @param file file pointer
	 * @return float32_t read sample as float
	 */
	float32_t sd_rd_sample16(File &file)
	{
		int16_t res = file.read();
		res |= (file.read() << 8);
		return ((float32_t)res * I16_TO_F32_NORM_FACTOR);
	}
	/**
	 * @brief read a 24bit sample and convert it to float -1.0 ... 1.0 range
	 * 
	 * @param file file pointer
	 * @return float32_t read sample as float
	 */
	float32_t sd_rd_sample24(File &file)
	{
		char d[3];
		file.readBytes(d, 3);
		int32_t res = (int32_t)(0x00 | (d[0]<<8) | (d[1]<<16) | (d[2]<<24)) >> 8;
		float32_t out = (float32_t)res / 8388607.0f;
		return out;
	}

	ir_wav_result_t parse_wav_header(File &file, uint8_t &channels, uint32_t &sample_rate, uint8_t &bitdepth, uint32_t &count);
	bool parse_wav_header(File &file);
	bool scan_ir_dir(bool load);
	void load_builtin_ir();
	bool write_conf();
}; 

#endif // _FILTER_IR_CONVOLVER_H
