#include "control_WM8731_F32.h"
#include <Wire.h>

#define WM8731_I2C_ADDR_A 0x1A
#define WM8731_I2C_ADDR_B 0x1B

#define WM8731_MUTE_ON						(1)
#define WM8731_MUTE_OFF						(0)

#define WM8731_REG_LLINEIN					(0)
	#define WM8731_BITS_LINVOL_SHIFT		(0)
	#define WM8731_BITS_LINVOL_MASK			(0x1F)
	#define WM8731_BITS_LINVOL(x)			(((x)<<WM8731_BITS_LINVOL_SHIFT)&WM8731_BITS_LINVOL_MASK)

	#define WM8731_BITS_LINMUTE_SHIFT		(7)
	#define WM8731_BITS_LINMUTE_MASK		(1<<WM8731_BITS_LINMUTE_SHIFT)
	#define WM8731_BITS_LINMUTE(x)			(((x)<<WM8731_BITS_LINMUTE_SHIFT)&WM8731_BITS_LINMUTE_MASK)

	#define WM8731_BITS_LRINBOTH_SHIFT		(8)
	#define WM8731_BITS_LRINBOTH_MASK		(1<<WM8731_BITS_LRINBOTH_SHIFT)
	#define WM8731_BITS_LRINBOTH(x)			(((x)<<WM8731_BITS_LRINBOTH_SHIFT)&WM8731_BITS_LRINBOTH_MASK)

#define WM8731_REG_RLINEIN	(1)
	#define WM8731_BITS_RINVOL_SHIFT		(0)
	#define WM8731_BITS_RINVOL_MASK			(0x1F)
	#define WM8731_BITS_RINVOL(x)			(((x)<<WM8731_BITS_RINVOL_SHIFT)&WM8731_BITS_RINVOL_MASK)

	#define WM8731_BITS_RINMUTE_SHIFT		(7)
	#define WM8731_BITS_RINMUTE_MASK		(1<<WM8731_BITS_RINMUTE_SHIFT)
	#define WM8731_BITS_RINMUTE(x)			(((x)<<WM8731_BITS_RINMUTE_SHIFT)&WM8731_BITS_RINMUTE_MASK)

	#define WM8731_BITS_RLINBOTH_SHIFT		(8)
	#define WM8731_BITS_RLINBOTH_MASK		(1<<WM8731_BITS_RLINBOTH_SHIFT)
	#define WM8731_BITS_RLINBOTH(x)			(((x)<<WM8731_BITS_RLINBOTH_SHIFT)&WM8731_BITS_RLINBOTH_MASK)

#define WM8731_REG_LHEADOUT					(2)
	#define WM8731_BITS_LHPVOL_SHIFT		(0)
	#define WM8731_BITS_LHPVOL_MASK			(0x7F)
	#define WM8731_BITS_LHPVOL(x)			(((x))<<WM8731_BITS_LHPVOL_SHIFT)&WM8731_BITS_LHPVOL_MASK)

	#define WM8731_BITS_LZCEN_SHIFT			(7)
	#define WM8731_BITS_LZCEN_MASK			(1<<WM8731_BITS_LZCEN_SHIFT)
	#define WM8731_BITS_LZCEN(x)			(((x)<<WM8731_BITS_LZCEN_SHIFT)&WM8731_BITS_LZCEN_MASK)

	#define WM8731_BITS_LRHPBOTH_SHIFT		(8)
	#define WM8731_BITS_LRHPBOTH_MASK		(1<<WM8731_BITS_LRHPBOTH_SHIFT)
	#define WM8731_BITS_LRHPBOTH(x)			(((x)<<WM8731_BITS_LRHPBOTH_SHIFT)&WM8731_BITS_LRHPBOTH_MASK)

#define WM8731_REG_RHEADOUT					(3)
	#define WM8731_BITS_RHPVOL_SHIFT		(0)
	#define WM8731_BITS_RHPVOL_MASK			(0x7F)
	#define WM8731_BITS_RHPVOL(x)			(((x))<<WM8731_BITS_RHPVOL_SHIFT)&WM8731_BITS_RHPVOL_MASK)

	#define WM8731_BITS_RZCEN_SHIFT			(7)
	#define WM8731_BITS_RZCEN_MASK			(1<<WM8731_BITS_RZCEN_SHIFT)
	#define WM8731_BITS_RZCEN(x)			(((x)<<WM8731_BITS_RZCEN_SHIFT)&WM8731_BITS_RZCEN_MASK)

	#define WM8731_BITS_RLHPBOTH_SHIFT		(8)
	#define WM8731_BITS_RLHPBOTH_MASK		(1<<WM8731_BITS_RLHPBOTH_SHIFT)
	#define WM8731_BITS_RLHPBOTH(x)			(((x)<<WM8731_BITS_RLHPBOTH_SHIFT)&WM8731_BITS_RLHPBOTH_MASK)

#define WM8731_REG_ANALOG					(4)
	#define WM8731_BITS_MICBOOST_SHIFT		(0)
	#define WM8731_BITS_MICBOOST_MASK		(1<<WM8731_BITS_MICBOOST_SHIFT)
	#define WM8731_BITS_MICBOOST(x)			(((x)<<WM8731_BITS_MICBOOST_SHIFT)&WM8731_BITS_MICBOOST_MASK)

	#define WM8731_BITS_MUTEMIC_SHIFT		(1)
	#define WM8731_BITS_MUTEMIC_MASK		(1<<WM8731_BITS_MUTEMIC_SHIFT)
	#define WM8731_BITS_MUTEMIC(x)			(((x)<<WM8731_BITS_MUTEMIC_SHIFT)&WM8731_BITS_MUTEMIC_MASK)

	#define WM8731_BITS_INSEL_SHIFT			(2)
	#define WM8731_BITS_INSEL_MASK			(1<<WM8731_BITS_INSEL_SHIFT)
	#define WM8731_BITS_INSEL(x)			(((x)<<WM8731_BITS_INSEL_SHIFT)&WM8731_BITS_INSEL_MASK)

	#define WM8731_BITS_BYPASS_SHIFT		(3)
	#define WM8731_BITS_BYPASS_MASK			(1<<WM8731_BITS_BYPASS_SHIFT)
	#define WM8731_BITS_BYPASS(x)			(((x)<<WM8731_BITS_BYPASS_SHIFT)&WM8731_BITS_BYPASS_MASK)

	#define WM8731_BITS_DACSEL_SHIFT		(4)
	#define WM8731_BITS_DACSEL_MASK			(1<<WM8731_BITS_DACSEL_SHIFT)
	#define WM8731_BITS_DACSEL(x)			(((x)<<WM8731_BITS_DACSEL_SHIFT)&WM8731_BITS_DACSEL_MASK)

	#define WM8731_BITS_SIDETONE_SHIFT		(5)
	#define WM8731_BITS_SIDETONE_MASK		(1<<WM8731_BITS_SIDETONE_SHIFT)
	#define WM8731_BITS_SIDETONE(x)			(((x)<<WM8731_BITS_SIDETONE_SHIFT)&WM8731_BITS_SIDETONE_MASK)

	#define WM8731_BITS_SIDEATT_SHIFT		(6)
	#define WM8731_BITS_SIDEATT_MASK		(0xC0)
	#define WM8731_BITS_SIDEATT(x)			(((x)<<WM8731_BITS_SIDEATT_SHIFT)&WM8731_BITS_SIDEATT_MASK)	
	#define WM8731_SIDEATT_M6_DB			(0x00)
	#define WM8731_SIDEATT_M9_DB			(0x01)
	#define WM8731_SIDEATT_M12_DB			(0x02)
	#define WM8731_SIDEATT_M15_DB			(0x03)

#define WM8731_REG_DIGITAL					(5)
	#define WM8731_BITS_ADCHPD_SHIFT		(0)
	#define WM8731_BITS_ADCHPD_MASK			(1<<WM8731_BITS_ADCHPD_SHIFT)
	#define WM8731_BITS_ADCHPD(x)			(((x)<<WM8731_BITS_ADCHPD_SHIFT)&WM8731_BITS_ADCHPD_MASK)

	#define WM8731_BITS_DEEMP_SHIFT			(6)
	#define WM8731_BITS_DEEMP_MASK			(0x06)
	#define WM8731_BITS_DEEMP(x)			(((x)<<WM8731_BITS_DEEMP_SHIFT)&WM8731_BITS_DEEMP_MASK)	
	#define WM8731_DEEMP_OFF				(0x00)
	#define WM8731_DEEMP_32KHZ				(0x01)
	#define WM8731_DEEMP_44_1KHZ			(0x02)
	#define WM8731_DEEMP_48KHZ				(0x03)

	#define WM8731_BITS_DACMU_SHIFT			(3)
	#define WM8731_BITS_DACMU_MASK			(1<<WM8731_BITS_DACMU_SHIFT)
	#define WM8731_BITS_DACMU(x)			(((x)<<WM8731_BITS_DACMU_SHIFT)&WM8731_BITS_DACMU_MASK)

	#define WM8731_BITS_HPOR_SHIFT			(4)
	#define WM8731_BITS_HPOR_MASK			(1<<WM8731_BITS_HPOR_SHIFT)
	#define WM8731_BITS_HPOR(x)				(((x)<<WM8731_BITS_HPOR_SHIFT)&WM8731_BITS_HPOR_MASK)

#define WM8731_REG_POWERDOWN				(6)
	#define WM8731_BITS_LINEINPD_SHIFT		(0)
	#define WM8731_BITS_LINEINPD_MASK		(1<<WM8731_BITS_LINEINPD_SHIFT)
	#define WM8731_BITS_LINEINPD(x)			(((x)<<WM8731_BITS_LINEINPD_SHIFT)&WM8731_BITS_LINEINPD_MASK)

	#define WM8731_BITS_MICPD_SHIFT			(1)
	#define WM8731_BITS_MICPD_MASK			(1<<WM8731_BITS_MICPD_SHIFT)
	#define WM8731_BITS_MICPD(x)			(((x)<<WM8731_BITS_MICPD_SHIFT)&WM8731_BITS_MICPD_MASK)

	#define WM8731_BITS_ADCPD_SHIFT			(2)
	#define WM8731_BITS_ADCPD_MASK			(1<<WM8731_BITS_ADCPD_SHIFT)
	#define WM8731_BITS_ADCPD(x)			(((x)<<WM8731_BITS_ADCPD_SHIFT)&WM8731_BITS_ADCPD_MASK)

	#define WM8731_BITS_DACPD_SHIFT			(3)
	#define WM8731_BITS_DACPD_MASK			(1<<WM8731_BITS_DACPD_SHIFT)
	#define WM8731_BITS_DACPD(x)			(((x)<<WM8731_BITS_DACPD_SHIFT)&WM8731_BITS_DACPD_MASK)

	#define WM8731_BITS_OUTPD_SHIFT			(4)
	#define WM8731_BITS_OUTPD_MASK			(1<<WM8731_BITS_OUTPD_SHIFT)
	#define WM8731_BITS_OUTPD(x)			(((x)<<WM8731_BITS_OUTPD_SHIFT)&WM8731_BITS_OUTPD_MASK)

	#define WM8731_BITS_OSCPD_SHIFT			(5)
	#define WM8731_BITS_OSCPD_MASK			(1<<WM8731_BITS_OSCPD_SHIFT)
	#define WM8731_BITS_OSCPD(x)			(((x)<<WM8731_BITS_OSCPD_SHIFT)&WM8731_BITS_OSCPD_MASK)

	#define WM8731_BITS_CLKOUTPD_SHIFT		(6)
	#define WM8731_BITS_CLKOUTPD_MASK		(1<<WM8731_BITS_CLKOUTPD_SHIFT)
	#define WM8731_BITS_CLKOUTPD(x)			(((x)<<WM8731_BITS_CLKOUTPD_SHIFT)&WM8731_BITS_CLKOUTPD_MASK)

	#define WM8731_BITS_POWEROFF_SHIFT		(7)
	#define WM8731_BITS_POWEROFF_MASK		(1<<WM8731_BITS_POWEROFF_SHIFT)
	#define WM8731_BITS_POWEROFF(x)			(((x)<<WM8731_BITS_POWEROFF_SHIFT)&WM8731_BITS_POWEROFF_MASK)

#define WM8731_REG_INTERFACE				(7)
	#define WM8731_BITS_FORMAT_SHIFT		(0)
	#define WM8731_BITS_FORMAT_MASK			(0x03)
	#define WM8731_BITS_FORMAT(x)			(((x)<<WM8731_BITS_FORMAT_SHIFT)&WM8731_BITS_FORMAT_MASK)	
	#define WM8731_FORMAT_DSP_MODE			(0x03)
	#define WM8731_FORMAT_I2S_MSB_LEFT		(0x02)
	#define WM8731_FORMAT_MSB_LEFT			(0x01)
	#define WM8731_FORMAT_MSB_RIGHT			(0x00)

	#define WM8731_BITS_IWL_SHIFT			(2)
	#define WM8731_BITS_IWL_MASK			(0x0C)
	#define WM8731_BITS_IWL(x)				((x<<WM8731_BITS_IWL_SHIFT)&WM8731_BITS_IWL_MASK)

	#define WM8731_BITS_LRP_SHIFT			(4)
	#define WM8731_BITS_LRP_MASK			(1<<WM8731_BITS_LRP_SHIFT)
	#define WM8731_BITS_LRP(x)				(((x)<<WM8731_BITS_LRP_SHIFT)&WM8731_BITS_LRP_MASK)

	#define WM8731_BITS_LRSWAP_SHIFT		(5)
	#define WM8731_BITS_LRSWAP_MASK			(1<<WM8731_BITS_LRSWAP_SHIFT)
	#define WM8731_BITS_LRSWAP(x)			(((x)<<WM8731_BITS_LRSWAP_SHIFT)&WM8731_BITS_LRSWAP_MASK)

	#define WM8731_BITS_MS_SHIFT			(6)
	#define WM8731_BITS_MS_MASK				(1<<WM8731_BITS_MS_SHIFT)
	#define WM8731_BITS_MS(x)				(((x)<<WM8731_BITS_MS_SHIFT)&WM8731_BITS_MS_MASK)	

	#define WM8731_BITS_BCLKINV_SHIFT		(7)
	#define WM8731_BITS_BCLKINV_MASK		(1<<WM8731_BITS_BCLKINV_SHIFT)
	#define WM8731_BITS_BCLKINV(x)			(((x)<<WM8731_BITS_BCLKINV_SHIFT)&WM8731_BITS_BCLKINV_MASK)	

#define WM8731_REG_SAMPLING					(8)
	#define WM8731_BITS_USB_NORMAL_SHIFT	(0)
	#define WM8731_BITS_USB_NORMAL_MASK		(1<<WM8731_BITS_USB_NORMAL_SHIFT)
	#define WM8731_BITS_USB_NORMAL(x)		(((x)<<WM8731_BITS_USB_NORMAL_SHIFT)&WM8731_BITS_USB_NORMAL_MASK)

	#define WM8731_BITS_BOSR_SHIFT			(1)
	#define WM8731_BITS_BOSR_MASK			(1<<WM8731_BITS_BOSR_SHIFT)
	#define WM8731_BITS_BOSR(x)				(((x)<<WM8731_BITS_BOSR_SHIFT)&WM8731_BITS_BOSR_MASK)

	#define WM8731_BITS_SR_SHIFT			(2)
	#define WM8731_BITS_SR_MASK				(0x3C)
	#define WM8731_BITS_SR(x)				(((x)<<WM8731_BITS_SR_SHIFT)&WM8731_BITS_SR_MASK)

	#define WM8731_BITS_CLKIDIV2_SHIFT		(6)
	#define WM8731_BITS_CLKIDIV2_MASK		(1<<WM8731_BITS_CLKIDIV2_SHIFT)
	#define WM8731_BITS_CLKIDIV2(x)			(((x)<<WM8731_BITS_CLKIDIV2_SHIFT)&WM8731_BITS_CLKIDIV2_MASK)	

	#define WM8731_BITS_CLKODIV2_SHIFT		(7)
	#define WM8731_BITS_CLKODIV2_MASK		(1<<WM8731_BITS_CLKODIV2_SHIFT)
	#define WM8731_BITS_CLKODIV2(x)			(((x)<<WM8731_BITS_CLKODIV2_SHIFT)&WM8731_BITS_CLKODIV2_MASK)		

#define WM8731_REG_ACTIVE					(9)
#define WM8731_REG_RESET					(15)
// ----------------------------------------------------------------------------------
bool AudioControlWM8731_F32::enable(bit_depth_t bits, TwoWire *i2cBus, uint8_t addr)
{
	_wire = i2cBus;
	i2c_addr = addr;

	_wire->begin();
	delay(5);
	if (!write(WM8731_REG_RESET, 0))
	{
		return false; // no WM8731 chip responding
	}
		
	write(WM8731_REG_INTERFACE, WM8731_BITS_FORMAT(WM8731_FORMAT_I2S_MSB_LEFT) | 
								WM8731_BITS_IWL(bits)); // I2S, x bit, MCLK slave
	
	write(WM8731_REG_SAMPLING, 	WM8731_BITS_USB_NORMAL(0)	|	// normal mode
								WM8731_BITS_BOSR(0)			|	// 256*fs
								WM8731_BITS_SR(8)			|	// 44.1kHz
								WM8731_BITS_CLKIDIV2(0)		|	// MCLK/1
								WM8731_BITS_CLKODIV2(0));

	write(WM8731_REG_DIGITAL, WM8731_BITS_DACMU(1)); 	// Soft mute DAC
	write(WM8731_REG_ANALOG, 0x00);	 // disable all
	write(WM8731_REG_POWERDOWN, 0x00); // codec powerdown
	write(WM8731_REG_LHEADOUT, 0x80); // volume off
	write(WM8731_REG_RHEADOUT, 0x80);
	delay(300);
	write(WM8731_REG_ACTIVE, 1);
	delay(5);
	write(WM8731_REG_DIGITAL, WM8731_BITS_DACMU(0)); 	// DAC unmuted
	write(WM8731_REG_ANALOG, WM8731_BITS_DACSEL(1));	 // DAC selected
	return true;
}
// ----------------------------------------------------------------------------------
void AudioControlWM8731_F32::dac_mute(bool m)
{
	modify(WM8731_REG_DIGITAL, m ? WM8731_BITS_DACMU(1) : WM8731_BITS_DACMU(0), WM8731_BITS_DACMU_MASK);
	DACmute = m;
}
// ----------------------------------------------------------------------------------
void AudioControlWM8731_F32::hp_filter(bool state)
{
	modify(WM8731_REG_DIGITAL, WM8731_BITS_ADCHPD(state^1), WM8731_BITS_ADCHPD_MASK);
}
// ----------------------------------------------------------------------------------
// Freeze the HP filter
void AudioControlWM8731_F32::dcbias_store(bool state)
{
	modify(WM8731_REG_DIGITAL, WM8731_BITS_HPOR(state), WM8731_BITS_HPOR_MASK);
}
// ----------------------------------------------------------------------------------
// Mute both Line inputs
void AudioControlWM8731_F32::lineIn_mute(bool m)
{
	modify(WM8731_REG_LLINEIN, WM8731_BITS_LINMUTE(m), WM8731_BITS_LINMUTE_MASK);
	modify(WM8731_REG_RLINEIN, WM8731_BITS_RINMUTE(m), WM8731_BITS_RINMUTE_MASK);
}
// ----------------------------------------------------------------------------------
// Enable/Disable DAC output mixer switch
void AudioControlWM8731_F32::dac_enable(bool en)
{
	modify(WM8731_REG_ANALOG, WM8731_BITS_DACSEL(en), WM8731_BITS_DACSEL_MASK);
}
// ----------------------------------------------------------------------------------
// Enable Dry (Bypass) output mixer switch
void AudioControlWM8731_F32::dry_enable(bool en)
{
	modify(WM8731_REG_ANALOG, WM8731_BITS_BYPASS(en), WM8731_BITS_BYPASS_MASK);
	dry_sig = en;
}
// ----------------------------------------------------------------------------------
// analog bypass switch
void AudioControlWM8731_F32::bypass_set(bool b)
{
	uint8_t bp_state = ((((uint8_t)dry_sig)<<1) & 0x01) | b;
	switch(bp_state)
	{
		case 0b00:	// Dry Off, Wet on -> pass Wet only
		case 0b11:
			dry_enable(false);
			dac_enable(true);
			break;
		case 0b01:	// dry OFF, bypass ON -> pass Dry only
			dry_enable(false);
			dac_enable(true);
			break;
		case 0b10:	// Dry on, Wet on
			dry_enable(true);
			dac_enable(true);
			break;
		default: break;
	}
}
// ----------------------------------------------------------------------------------
bool AudioControlWM8731_F32::write(uint16_t regAddr, uint16_t val)
{
	reg[regAddr] = val;
	int attempt = 0;
	while (1)
	{
		attempt++;
		_wire->beginTransmission(i2c_addr);
		_wire->write((regAddr << 1) | ((val >> 8) & 1));
		_wire->write(val & 0xFF);
		int status = _wire->endTransmission();
		if (status == 0) return true;
		if (attempt >= 12)  return false;
		delayMicroseconds(80);
	}
}
// ----------------------------------------------------------------------------------
uint16_t AudioControlWM8731_F32::modify(uint16_t regAddr, uint16_t val, uint16_t iMask)
{
	reg[regAddr]  = (reg[regAddr] & (~iMask)) | val;
	if (!write(regAddr, reg[regAddr])) return 0;
	return reg[regAddr];
}
// ----------------------------------------------------------------------------------
// Set the headphone volume
bool AudioControlWM8731_F32::hp_volumeInteger(uint16_t n)
{
	// n = 127 for max volume (+6 dB)
	// n = 48 for min volume (-73 dB)
	// n = 0 to 47 for mute
	if (n > 127) n = 127;
	write(WM8731_REG_LHEADOUT, n | 0x180);
	write(WM8731_REG_RHEADOUT, n | 0x80);
	return true;
}
// ----------------------------------------------------------------------------------
bool AudioControlWM8731_F32::inputLevel(float n)
{
	// range is 0x00 (min) - 0x1F (max)
	int _level = int(n * 31.f);
	_level = _level > 0x1F ? 0x1F : _level;
	write(WM8731_REG_LLINEIN, _level);
	write(WM8731_REG_RLINEIN, _level);
	return true;
}
// ----------------------------------------------------------------------------------
bool AudioControlWM8731_F32::inputLevelRaw(uint8_t n)
{
	// range is 0x00 (min) - 0x1F (max)
	n = n > 0x1F ? 0x1F : n;
	write(WM8731_REG_LLINEIN, n);
	write(WM8731_REG_RLINEIN, n);
	return true;
}
// ----------------------------------------------------------------------------------
bool AudioControlWM8731_F32::inputSelect(int n)
{
	if (n == AUDIO_INPUT_LINEIN) modify(WM8731_REG_ANALOG, WM8731_BITS_INSEL(0), WM8731_BITS_INSEL_MASK); 	
	else if (n == AUDIO_INPUT_MIC)	modify(WM8731_REG_ANALOG, WM8731_BITS_INSEL(1), WM8731_BITS_INSEL_MASK); 
	else return false;
	return true;
}

/******************************************************************/

bool AudioControlWM8731_F32_master::enable(bit_depth_t bits, TwoWire *i2cBus, uint8_t addr)
{
	_wire = i2cBus;
	i2c_addr = addr;
	_wire->begin();
	delay(5);
	// write(WM8731_REG_RESET, 0);
	write(WM8731_REG_INTERFACE, 
			WM8731_BITS_FORMAT(WM8731_FORMAT_I2S_MSB_LEFT) | 
			WM8731_BITS_IWL(bits)|
			WM8731_BITS_MS(1)); 		// I2S, x bit, MCLK master
	write(WM8731_REG_SAMPLING, 0x20);  // 256*Fs, 44.1 kHz, MCLK/1

	// In order to prevent pops, the DAC should first be soft-muted (DACMU),
	// the output should then be de-selected from the line and headphone output
	// (DACSEL), then the DAC powered down (DACPD).

	write(WM8731_REG_DIGITAL, 0x08); // DAC soft mute
	write(WM8731_REG_ANALOG, 0x00);	 // disable all

	write(WM8731_REG_POWERDOWN, 0x00); // codec powerdown

	write(WM8731_REG_LHEADOUT, 0x80); // volume off
	write(WM8731_REG_RHEADOUT, 0x80);

	delay(100); // how long to power up?

	write(WM8731_REG_ACTIVE, 1);
	delay(5);
	write(WM8731_REG_DIGITAL, 0x00); // DAC unmuted
	write(WM8731_REG_ANALOG, 0x10);	 // DAC selected

	return true;
}
