# hexefx_audiolib_F32
Audio effects library for Teensy4.x (extension to OpenAudio_ArduinoLibrary)  
### [GUI available HERE](https://hexeguitar.github.io/hexefx_audiolib_F32/gui/index.html "hexefx_audiolib_F32 GUI")

## Effects  
**AudioEffectInfinitePhaser_F32**  
Infinite/barberpole phaser (mono version). Creates acoustic illusion of infinite move forwards or backwards.  

**AudioEffectPhaserStereo_F32**  
Stereo phaser with up to 12 stages.  

**AudioEffectMonoToStereo_F32**  
Mono to stereo converter.  

**AudioEffectPlateReverb_F32**  
Stereo plate reverb.  

**AudioEffectSpringReverb_F32**  
Stereo spring reverb emulation.  

**AudioEffectReverbSc_F32**  
8 delay line stereo FDN reverb, based on work by Sean Costello.  
Optional PSRAM use for the delay buffers.  

**AudioEffectDelayStereo_F32**  
Versatile stereo ping-pong delay with modulation.  

**AudioEffectNoiseGateStereo_F32**  
Stereo noise gate with external SideChain input.  

**AudioEffectGuitarBooster_F32**  
Overdrive emulation using oversampled wave shaper, switchable octave up.  

**AudioEffectWahMono_F32**  
WAH pedal emulation including 8 models and versatile range handling.  

**AudioFilterToneStackStereo_F32**  
Stereo guitar tone stack (EQ) emulator.  

**AudioFilterEqualizer_HX_F32**  
Slightly modified original equalizer component, added bypass system.  

**AudioFilterIRCabsim_F32**  
Stereo guitar/bass cabinet emulator using low latency uniformly partitioned convolution.  
10 cabinet impulse responses built in.  

**AudioFilterIRCabsim_SD_F32**  
Stereo guitar/bass cabinet emulator using low latency uniformly partitioned convolution.  
Uses IR wav files (16/24bit 44.1kHz, up to 8K samples) stored on an SD card.  

**AudioFilterEqualizer3band_F32**  
Simple 3 band (Treble, Mid, Bass) equalizer.  

**AudioEffectCompressorStereo_F32**  
Stereo compressor with bypass.  

**AudioEffectGainStereo_F32**  
Stereo gain control (volume, panorama)  

**AudioSwitchSelectorStereo**  
Stereo/mono signal selector. Routes either L+R, L+L or R+R to the L+R outputs.  

**AudioEffectXfaderStereo_F32**  
Stereo crossfader for 2 input channels.  

**AudioFilterDCblockerStereo_F32**
IIR based DC blocking filter.  


## I/O  
**AudioInputI2S2_F32**  
**AudioOutputI2S2_F32**  
Input and output for the I2S2 interface, Teensy 4.1 only.  

**AudioInputI2S_ext_F32**  
**AudioOutputI2S_ext_F32**  
Custom input and output for the I2S interface, including a few extra options (ie. channel swap)  

## Control  
**AudioControlAK4452_F32**  
AK4452 32bit DAC driver.  

**AudioControlAK5552_F32**  
AK5552 32bit ADC driver.  

**AudioControlAK4558_F32**  
AK4558 32bit codec driver.  

**AudioControlES8388_F32**  
ES8388 24bit codec driver.  

**AudioControlSGTL5000_F32**
SGTL5000 24bit codec driver, configurable I2C bus.  

**AudioControlWM8731_F32**  
WM8731 24bit codec driver, configurable I2C bus.  


## Basic  
Single header basic building blocks for various DSP components:  
- allpass filter  
- lfo  
- delay line  
- delay line based pitch shifter  
- shelving lowpass and hipass filter
- lowpass filter  
- stereo bypass system  

## Example projects  
* https://github.com/hexeguitar/hexefx_audiolib_F32_examples  
* https://github.com/hexeguitar/tgx4

---  
Copyright 07.2024 by Piotr Zapart  
www.hexefx.com

