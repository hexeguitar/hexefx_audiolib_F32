# hexefx_audiolib_F32
Audio effects library for Teensy4.x (extension to OpenAudio_ArduinoLibrary)  

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

**AudioFilterEqualizer3band_F32**  
Simple 3 band (Treble, Mid, Bass) equalizer.  

**AudioEffectGainStereo_F32**  
Stereo gain control (volume etc.)  

**AudioSwitchSelectorStereo**  
Stereo/mono signal selector.  


## I/O  
**AudioInputI2S2_F32**  
**AudioOutputI2S2_F32**  
Input and output for the I2S2 interface, Teensy 4.1 only.  

**AudioInputI2S_ext_F32**  
**AudioOutputI2S_ext_F32**  
Custom input and output for the I2S interface, including a few extra options (ie. channel swap)  


## Basic  
Single header basic building blocks for various DSP components:  
- allpass filter  
- lfo  
- delay line  
- delay line based pitch shifter  
- shelving lowpass and hipass filter
- lowpass filter  

## Example projects  
https://github.com/hexeguitar/hexefx_audiolib_F32_examples
https://github.com/hexeguitar/tgx4

---  
Copyright 07.2024 by Piotr Zapart  
www.hexefx.com

