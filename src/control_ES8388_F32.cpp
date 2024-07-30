#include "control_ES8388_F32.h"

#define ES8388_REG_CHIP_CTRL1		(0x00)	// Default 0000 0110
#define ES8388_REG_CHIP_CTRL1_DFLT	(0x06)
	#define ES8388_BIT_SCPRESET		(1<<7) 	// 1=reset registers to default
	#define ES8388_BIT_LRCM			(1<<6)	
	#define ES8388_BIT_DACMCLK		(1<<5)
	#define ES8388_BIT_SAMEFS		(1<<4)
	#define ES8388_BIT_SEQEN		(1<<3)
	#define ES8388_BIT_ENREF		(1<<2)
	#define ES8388_VMIDSEL_DIS		(0x00)
	#define ES8388_VMIDSEL_50K		(0x01)
	#define ES8388_VMIDSEL_500K		(0x02)
	#define ES8388_VMIDSEL_5K		(0x03)
#define ES8388_REG_CHIP_CTRL2		(0x01)	// Default 0101 1100
#define ES8388_REG_CHIP_CTRL2_DFLT	(0x5C)
	#define ES8388_BIT_LPVCMMOD		(1<<5)
	#define ES8388_BIT_LPVREFBUF	(1<<4)
	#define ES8388_BIT_PDNANA		(1<<3)
	#define ES8388_BIT_PDBIBIASGEN	(1<<2)
	#define ES8388_BIT_VREFLO		(1<<1)
	#define ES8388_BIT_PDBVREFBUF	(1<<0)
#define ES8388_REG_CHIP_PWR_MAN		(0x02)	// Default 1100 0011
#define ES8388_REG_CHIP_PWR_MAN_DFLT (0xC3)
	#define ES8388_BIT_ADC_DIGPDN	(1<<7)
	#define ES8388_BIT_DAC_DIGPDN	(1<<6)
	#define ES8388_BIT_ADCSTMRST	(1<<5)
	#define ES8388_BIT_DACSTMRST	(1<<4)
	#define ES8388_BIT_ADCDLL_PDN	(1<<3)
	#define ES8388_BIT_DACDLL_PDN	(1<<2)
	#define ES8388_BIT_ADCVREFPDN	(1<<1)
	#define ES8388_BIT_DACVREFPDN	(1<<0)
#define ES8388_REG_ADC_PWR_MAN		(0x03)	// Default 1111 1100
#define ES8388_REG_ADC_PWR_MAN_DFLT	(0xFC)
	#define ES8388_BIT_PDNAINL		(1<<7)
	#define ES8388_BIT_PDNAINR		(1<<6)
	#define ES8388_BIT_PDNADCL		(1<<5)
	#define ES8388_BIT_PDNADCR		(1<<4)
	#define ES8388_BIT_PDNMICB		(1<<3)
	#define ES8388_BIT_PDNADCBIASG	(1<<2)
	#define ES8388_BIT_FLASHLP		(1<<1)
	#define ES8388_BIT_INT1LP		(1<<0)
#define ES8388_REG_DAC_PWR_MAN		(0x04)	// Default 1100 0000
#define ES8388_REG_DAC_PWR_MAN_DFLT	(0xC0)	
	#define ES8388_BIT_PDNDACL		(1<<7)
	#define ES8388_BIT_PDNDACR		(1<<6)
	#define ES8388_BIT_LOUT1_EN		(1<<5)
	#define ES8388_BIT_ROUT1_EN		(1<<4)
	#define ES8388_BIT_LOUT2_EN		(1<<3)
	#define ES8388_BIT_ROUT2_EN		(1<<2)
#define ES8388_REG_CHIP_LOWPWR1		(0x05)	// Default 0000 0000
#define ES8388_REG_CHIP_LOWPWR1_DFLT (0x00)
	#define ES8388_BIT_LPDACL		(1<<7)
	#define ES8388_BIT_LPDACR		(1<<6)
	#define ES8388_BIT_LPOUT1		(1<<5)
	#define ES8388_BIT_LPOUT2		(1<<3)
#define ES8388_REG_CHIP_LOWPWR2		(0x06)	// Default 0000 0000
#define ES8388_REG_CHIP_LOWPWR2_DFLT (0x00)
	#define ES8388_BIT_LPPGA		(1<<7)
	#define ES8388_BIT_LPMIX		(1<<6)
	#define ES8388_BIT_LPADCVRP		(1<<1)
	#define ES8388_BIT_LPDACVRP		(1<<0)
#define ES8388_REG_ANALOG_VOLT_MAN	(0x07)	// Default 0111 1100
#define ES8388_REG_ANALOG_VOLT_MAN_DFLT (0x7C)
	#define ES8388_BIT_VSEL_MASK	(0x7C)
#define ES8388_REG_MASTER_MODE_CTRL	(0x08)	// Default 1000 0000
#define ES8388_REG_MASTER_MODE_CTRL_DFLT (0x80)
	#define ES8388_BIT_MSC			(1<<7)
	#define ES8388_BIT_MCLKDIV2		(1<<6)
	#define ES8388_BIT_BCLKINV		(1<<5)
	#define ES8388_BCLKDIVAUTO		(0x00)
	#define ES8388_BCLKDIV1			(0x01)
	#define ES8388_BCLKDIV2			(0x02)
	#define ES8388_BCLKDIV3			(0x03)
	#define ES8388_BCLKDIV4			(0x04)
	#define ES8388_BCLKDIV6			(0x05)
	#define ES8388_BCLKDIV8			(0x06)
	#define ES8388_BCLKDIV9			(0x07)
	#define ES8388_BCLKDIV11		(0x08)
	#define ES8388_BCLKDIV12		(0x09)
	#define ES8388_BCLKDIV16		(0x0A)
	#define ES8388_BCLKDIV18		(0x0B)
	#define ES8388_BCLKDIV22		(0x0C)
	#define ES8388_BCLKDIV24		(0x0D)
	#define ES8388_BCLKDIV33		(0x0E)
	#define ES8388_BCLKDIV36		(0x0F)
	#define ES8388_BCLKDIV44		(0x10)
	#define ES8388_BCLKDIV48		(0x11)
	#define ES8388_BCLKDIV66		(0x12)
	#define ES8388_BCLKDIV72		(0x13)
	#define ES8388_BCLKDIV5			(0x14)
	#define ES8388_BCLKDIV10		(0x15)
	#define ES8388_BCLKDIV15		(0x16)
	#define ES8388_BCLKDIV17		(0x17)
	#define ES8388_BCLKDIV20		(0x18)
	#define ES8388_BCLKDIV25		(0x19)
	#define ES8388_BCLKDIV30		(0x1A)
	#define ES8388_BCLKDIV32		(0x1B)
	#define ES8388_BCLKDIV34		(0x1C)
	#define ES8388_BCLKDIV(x)		((x)&0x1F)
#define ES8388_REG_ADC_CTRL1		(0x09)	// Default 0000 0000
#define ES8388_REG_ADC_CTRL1_DFLT	(0x00)
	#define ES8388_MICAMPL_MASK		(0xF0)
	#define ES8388_MICAMPL_SHIFT	(0x04)
	#define ES8388_MICAMPR_MASK		(0x0F)
	#define ES8388_MICAMPR_SHIFT	(0x00)		
#define ES8388_REG_ADC_CTRL2		(0x0A)	// Default 0000 0000
#define ES8388_REG_ADC_CTRL2_DFLT	(0x00)	
	#define ES8388_LINSEL_MASK		(0xC0)
	#define ES8388_LINSEL_SHIFT		(0x06)
	#define ES8388_INPUT1			(0x00)
	#define ES8388_INPUT2			(0x01)
	#define ES8388_INPUTDIFF		(0x03)
	#define ES8388_LINSEL(x)		(((x)<<ES8388_LINSEL_SHIFT)&ES8388_LINSEL_MASK)
	#define ES8388_RINSEL_MASK		(0x30)
	#define ES8388_RINSEL_SHIFT		(0x04)
	#define ES8388_RINSEL(x)		(((x)<<ES8388_RINSEL_SHIFT)&ES8388_RINSEL_MASK)
	#define ES8388_BIT_DSSEL		(1<<3)
	#define ES8388_BIT_DSR 			(1<<2)
#define ES8388_REG_ADC_CTRL3		(0x0B)	// Default 0000 0010
#define ES8388_REG_ADC_CTRL3_DFLT	(0x02)
	#define ES8388_BIT_DS 			(1<<7)
	#define ES8388_MONOMIX_MASK		(0x18)
	#define ES8388_MONOMIX_SHIFT	(0x03)
	#define ES8388_MONOMIX_STEREO	(0x00)
	#define ES8388_MONOMIX_ADCL		(0x01)
	#define ES8388_MONOMIX_ADCR		(0x02)
	#define ED8388_MONOMIX(x)		(((x)<<ES8388_MONOMIX_SHIFT)&ES8388_MONOMIX_MASK)
#define ES8388_REG_ADC_CTRL4		(0x0C)	// Default 0000 0000
#define ES8388_REG_ADC_CTRL4_DFLT	(0x00)
	#define ES8388_DATSEL_MASK		(0xC0)
	#define ES8388_DATSEL_SHIFT		(0x06)
	#define ES8388_DATSEL_LL_RR		(0x00)
	#define ES8388_DATSEL_LL_LR		(0x01)
	#define ES8388_DATSEL_RL_RR		(0x02)
	#define ES8388_DATSEL_LR_RL		(0x03)
	#define ES8388_BIT_ADCLRP		(1<<5)
	#define ES8388_ADCWL_MASK		(0x1C)
	#define ES8388_ADCWL_SHIFT		(0x1C)
	#define ES8388_ADCWL_24BIT		(0x00)
	#define ES8388_ADCWL_20BIT		(0x01)
	#define ES8388_ADCWL_18BIT		(0x02)
	#define ES8388_ADCWL_16BIT		(0x03)
	#define ES8388_ADCWL_32BIT		(0x04)
	#define ES8388_ADCWL(x)			(((x)<<ES8388_ADCWL_SHIFT)&ES8388_ADCWL_MASK)
	#define ES8388_ADCFORMAT_MASK	(0x03)
	#define ES8388_ADCFORMAT_SHIFT	(0x00)
	#define ES8388_ADCFORMAT_I2S	(0x00)	
	#define ES8388_ADCFORMAT_LJUST	(0x01)
	#define ES8388_ADCFORMAT_RJUST	(0x02)
	#define ES8388_ADCFORMAT_DSPPCM	(0x03)
	#define ES8388_ADCFORMAT(x)		(((x)<<ES8388_ADCFORMAT_SHIFT)&ES8388_ADCFORMAT_MASK)
#define ES8388_REG_ADC_CTRL5		(0x0D)	// Default 0000 0110
#define ES8388_REG_ADC_CTRL5_DFLT	(0x06)
	#define ES8388_BIT_ADCFSMODE	(1<<5)
	#define ES8388_ADCFSRATIO_MASK	(0x1F)
	#define ES8388_ADCFSRATIO_SHIFT	(0x00)
	#define ES8388_FSRATIO_128		(0x00)
	#define ES8388_FSRATIO_192		(0x01)
	#define ES8388_FSRATIO_256		(0x02)
	#define ES8388_FSRATIO_384		(0x03)
	#define ES8388_FSRATIO_512		(0x04)
	#define ES8388_FSRATIO_576		(0x05)
	#define ES8388_FSRATIO_768		(0x06) // default
	#define ES8388_FSRATIO_1024		(0x07)
	#define ES8388_FSRATIO_1152		(0x08)
	#define ES8388_FSRATIO_1408		(0x09)
	#define ES8388_FSRATIO_1536		(0x0A)
	#define ES8388_FSRATIO_2112		(0x0B)
	#define ES8388_FSRATIO_2304		(0x0C)
	#define ES8388_FSRATIO_125		(0x10)
	#define ES8388_FSRATIO_136 		(0x11)
	#define ES8388_FSRATIO_250 		(0x12)
	#define ES8388_FSRATIO_272 		(0x13)
	#define ES8388_FSRATIO_375 		(0x14)
	#define ES8388_FSRATIO_500		(0x15)
	#define ES8388_FSRATIO_544		(0x16)
	#define ES8388_FSRATIO_750 		(0x17)
	#define ES8388_FSRATIO_1000		(0x18)
	#define ES8388_FSRATIO_1088		(0x19)
	#define ES8388_FSRATIO_1496		(0x1A)	
	#define ES8388_FSRATIO_1500		(0x1B)
	#define ES8388_ADCFSRATIO(x)	(((x)<<ES8388_ADCFSRATIO_SHIFT)&ES8388_ADCFSRATIO_MASK)
#define ES8388_REG_ADC_CTRL6		(0x0E)	// Default 0011 0000
#define ES8388_REG_ADC_CTRL6_DFLT	(0x30)
	#define ES8388_BIT_ADCINVL		(1<<7)
	#define ES8388_BIT_ADCINVR		(1<<6)
	#define ES8388_BIT_ADCHPFL		(1<<5)
	#define ES8388_BIT_ADCHPFR		(1<<4)
#define ES8388_REG_ADC_CTRL7		(0x0F)	// Default 0010 0000
#define ES8388_REG_ADC_CTRL7_DFLT	(0x20)
	#define ES8388_ADCRAMPRATE_MASK	(0xC0)
	#define ES8388_ADCRAMPRATE_SHIFT (0x06)
	#define ES8388_ADCRAMPRATE_05DB_4LRCLK	(0x00)
	#define ES8388_ADCRAMPRATE_05DB_8LRCLK	(0x01)
	#define ES8388_ADCRAMPRATE_05DB_16LRCLK (0x02)
	#define ES8388_ADCRAMPRATE_05DB_32LRCLK (0x03)
	#define ES8388_ADCRAMPRATE(x)	(((x)<<ES8388_ADCRAMPRATE_SHIFT)&ES8388_ADCRAMPRATE_MASK)
	#define ES8388_BIT_ADCSOFTRAMP	(1<<5)
	#define ES8388_BIT_ADCLER		(1<<3)
	#define ES8388_BIT_ADCMUTE		(1<<2)
#define ES8388_REG_ADC_CTRL8		(0x10)	// Default 1100 0000
#define ES8388_REG_ADC_CTRL8_DFLT	(0xC0)
	#define ESP8388_LADCVOL			(ES8388_REG_ADC_CTRL8)
#define ES8388_REG_ADC_CTRL9		(0x11)	// Default 1100 0000
#define ES8388_REG_ADC_CTRL9_DFLT	(0xC0)
	#define ESP8388_RADCVOL			(ES8388_REG_ADC_CTRL9)
#define ES8388_REG_ADC_CTRL10		(0x12)	// Default 0011 1000
#define ES8388_REG_ADC_CTRL10_DFLT	(0x38)
	#define ES8388_ALCSEL_MASK		(0xC0)
	#define ES8388_ALCSEL_SHIFT		(0x06)
	#define ES8388_ALCSEL_OFF		(0x00)
	#define ES8388_ALCSEL_R			(0x01)
	#define ES8388_ALCSEL_L			(0x02)
	#define ES8388_ALCSEL_LR		(0x03)
	#define ES8388_ALCSEL(x)		(((x)<<ES8388_ALCSEL_SHIFT)&ES8388_ALCSEL_MASK)
	#define ES8388_MAXGAIN_MASK		(0x38)
	#define ES8388_MAXGAIN_SHIFT	(0x03)
	#define ES8388_MAXGAIN_M6_5DB	(0x00)
	#define ES8388_MAXGAIN_M0_5DB	(0x01)
	#define ES8388_MAXGAIN_5_5DB	(0x02)
	#define ES8388_MAXGAIN_11_5DB	(0x03)
	#define ES8388_MAXGAIN_17_5DB	(0x04)
	#define ES8388_MAXGAIN_23_5DB	(0x05)
	#define ES8388_MAXGAIN_29_5DB	(0x06)
	#define ES8388_MAXGAIN_35_5DB	(0x07)
	#define ES8388_MAXGAIN(x)		(((x)<<ES8388_MAXGAIN_SHIFT)&ES8388_MAXGAIN_MASK)
	#define ES8388_MINGAIN_MASK		(0x07)
	#define ES8388_MINGAIN_SHIFT	(0x00)
	#define ES8388_MINGAIN_M12DB	(0x00)
	#define ES8388_MINGAIN_M6DB		(0x01)
	#define ES8388_MINGAIN_0DB		(0x02)
	#define ES8388_MINGAIN_6DB		(0x03)
	#define ES8388_MINGAIN_12DB		(0x04)
	#define ES8388_MINGAIN_18DB		(0x05)
	#define ES8388_MINGAIN_24DB		(0x06)
	#define ES8388_MINGAIN_30DB		(0x07)
	#define ES8388_MINGAIN(x)		(((x)<<ES8388_MINGAIN_SHIFT)&ES8388_MINGAIN_MASK)
#define ES8388_REG_ADC_CTRL11		(0x13)	// Default 1011 0000
#define ES8388_REG_ADC_CTRL11_DFLT	(0xB0)
	#define ES8388_ALCLVL_MASK		(0xF0)
	#define ES8388_ALCLVL_SHIFT		(0x04)
	#define ES8388_ALCLVL(x)		(((x)<<ES8388_ALCLVL_SHIFT)&ES8388_ALCLVL_MASK)
	#define ES8388_ALCHLD_MASK		(0x0F)
	#define ES8388_ALCHLD_SHIFT		(0x00)
	#define ES8388_ALCHLD(x)		(((x)<<ES8388_ALCHLD_SHIFT)&ES8388_ALCHLD_MASK)	
#define ES8388_REG_ADC_CTRL12		(0x14)	// Default 0011 0010
#define ES8388_REG_ADC_CTRL12_DFLT	(0x32)
	#define ES8388_ALCDCY_MASK		(0xF0)
	#define ES8388_ALCDCY_SHIFT		(0x04)
	#define ES8388_ALCDCY(x)		(((x)<<ES8388_ALCDCY_SHIFT)&ES8388_ALCDCY_MASK)
	#define ES8388_ALCATK_MASK		(0x0F)
	#define ES8388_ALCATK_SHIFT		(0x00)
	#define ES8388_ALCATK(x)		(((x)<<ES8388_ALCATK_SHIFT)&ES8388_ALCATK_MASK)	
#define ES8388_REG_ADC_CTRL13		(0x15)	// Default 0000 0110
#define ES8388_REG_ADC_CTRL13_DFLT	(0x06)
	#define ES8388_BIT_ALCMODE		(1<<7)
	#define ES8388_BIT_ALCZC		(1<<6)
	#define ES8388_BIT_TIMEOUT		(1<<5)
	#define ES8388_WINSIZE_MASK		(0x1F)
	#define ES8388_WINSIZE_SHIFT	(0x00)
	#define ES8388_WINSIZE(x)		(((x)<<ES8388_WINSIZE_SHIFT)&ES8388_WINSIZE_MASK)	
#define ES8388_REG_ADC_CTRL14		(0x16)	// Default 0000 0000
#define ES8388_REG_ADC_CTRL14_DFLT	(0x00)
	#define ES8388_NGTH_MASK		(0xF8)
	#define ES8388_NGTH_SHIFT		(0x03)
	#define ES8388_NGTH(x)			(((x)<<ES8388_NGTH_SHIFT)&ES8388_NGTH_MASK)	
	#define ES8388_NGG_MASK			(0x06)
	#define ES8388_NGG_SHIFT		(0x01)
	#define ES8388_NGG_PGA_CONST	(0x00)
	#define ES8388_NGG_ADCMUTE		(0x01)	
	#define ES8388_NGG(x)			(((x)<<ES8388_NGG_SHIFT)&ES8388_NGG_MASK)	
	#define ES8388_BIT_NGAT_EN		(1<<0)
#define ES8388_REG_DAC_CTRL1		(0x17)	// Default 0000 0000
#define ES8388_REG_DAC_CTRL1_DFLT	(0x00)
	#define ES8388_BIT_DACLRSWAP	(1<<7)
	#define ES8388_BIT_DACLRP		(1<<6)
	#define ES8388_DACWL_MASK		(0x38)
	#define ES8388_DACWL_SHIFT		(0x03)
	#define ES8388_DACWL_24BIT		(0x00)
	#define ES8388_DACWL_20BIT		(0x01)
	#define ES8388_DACWL_18BIT		(0x02)
	#define ES8388_DACWL_16BIT		(0x03)
	#define ES8388_DACWL_32BIT		(0x04)
	#define ES8388_DACWL(x)			(((x)<<ES8388_DACWL_SHIFT)&ES8388_DACWL_MASK)
	#define ES8388_DACFORMAT_MASK	(0x06)
	#define ES8388_DACFORMAT_SHIFT	(0x01)
	#define ES8388_DACFORMAT_I2S	(0x00)
	#define ES8388_DACFORMAT_LJUST	(0x01)
	#define ES8388_DACFORMAT_RJUST	(0x02)
	#define ES8388_DACFORMAT_DSPPCM	(0x03)
	#define ES8388_DACFORMAT(x)		(((x)<<ES8388_DACFORMAT_SHIFT)&ES8388_DACFORMAT_MASK)
#define ES8388_REG_DAC_CTRL2		(0x18)	// Default 0000 0110
#define ES8388_REG_DAC_CTRL2_DFLT	(0x06)
	#define ES8388_BIT_DACFSMODE	(1<<5)
	#define ES8388_DACFSRATIO_MASK	(0x1F)
	#define ES8388_DACFSRATIO_SHIFT	(0x00)	// values define in ADCFSRATIO
	#define ES8388_DACFSRATIO(x)	(((x)<<ES8388_DACFSRATIO_SHIFT)&ES8388_DACFSRATIO_MASK)
#define ES8388_REG_DAC_CTRL3		(0x19)	// Default 0010 0010
#define ES8388_REG_DAC_CTRL3_DFLT	(0x22)
	#define ES8388_DACRAMPRATE_MASK	(0xC0)
	#define ES8388_DACRAMPRATE_SHIFT (0x06)
	#define ES8388_DACRAMPRATE_05DB_4LRCLK	(0x00)
	#define ES8388_DACRAMPRATE_05DB_32LRCLK	(0x01)
	#define ES8388_DACRAMPRATE_05DB_64LRCLK (0x02)
	#define ES8388_DACRAMPRATE_05DB_128LRCLK (0x03)
	#define ES8388_DACRAMPRATE(x)	(((x)<<ES8388_DACRAMPRATE_SHIFT)&ES8388_DACRAMPRATE_MASK)
	#define ES8388_BIT_DACSOFTRAMP	(1<<5)
	#define ES8388_BIT_DACLER		(1<<3)
	#define ES8388_BIT_DACMUTE		(1<<2)
#define ES8388_REG_DAC_CTRL4		(0x1A)	// Default 1100 0000
#define ES8388_REG_DAC_CTRL4_DFLT	(0xC0)
	#define ES8388_LDACVOL			(ES8388_REG_DAC_CTRL4)
#define ES8388_REG_DAC_CTRL5		(0x1B)	// Default 1100 0000
#define ES8388_REG_DAC_CTRL5_DFLT	(0xC0)
	#define ES8388_RDACVOL			(ES8388_REG_DAC_CTRL5)
#define ES8388_REG_DAC_CTRL6		(0x1C)	// Default 0000 1000
#define ES8388_REG_DAC_CTRL6_DFLT	(0x08)
	#define ES8388_DACDEEMP_MASK	(0xC0)
	#define ES8388_DACDEEMP_SHIFT 	(0x06)
	#define ES8388_DACDEEMP_OFF		(0x00)
	#define ES8388_DACDEEMP_32KHZ	(0x01)
	#define ES8388_DACDEEMP_44_1KHZ	(0x02)
	#define ES8388_DACDEEMP_48KHZ	(0x03)
	#define ES8388_DACDEEMP(x)		(((x)<<ES8388_DACDEEMP_SHIFT)&ES8388_DACDEEMP_MASK)
	#define ES8388_BIT_DACINVL		(1<<5)
	#define ES8388_BIT_DACINVR		(1<<4)
	#define ES8388_BIT_CLICKFREE	(1<<3)
#define ES8388_REG_DAC_CTRL7		(0x1D)	// Default 0000 0000
	#define ES8388_BIT_DACZEROL		(1<<7)
	#define ES8388_BIT_DACZEROR		(1<<6)
	#define ES8388_BIT_DACMONO		(1<<5)
	#define ES8388_DACSE_MASK		(0x1C)
	#define ES8388_DACSE_SHIFT		(0x02)
	#define ES8388_DACSE(x)			(((x)<<ES8388_DACSE_SHIFT)&ES8388_DACSE_MASK)
	#define ES8388_VPP_MASK			(0x03)
	#define ES8388_VPP_SHIFT		(0x00)
	#define ES8388_VPP_3_5V			(0x00)
	#define ES8388_VPP_4V			(0x01)
	#define ES8388_VPP_3V			(0x02)
	#define ES8388_VPP_2_5V			(0x03)	
	#define ES8388_VPP(x)			(((x)<<ES8388_VPP_SHIFT)&ES8388_VPP_MASK)
#define ES8388_REG_DAC_CTRL8		(0x1E)	// DAC shelving filter coeff a [29:24]
#define ES8388_REG_DAC_CTRL9		(0x1F)	// DAC shelving filter coeff a  [23:16]
#define ES8388_REG_DAC_CTRL10		(0x20)	// DAC shelving filter coeff a [15:08]
#define ES8388_REG_DAC_CTRL11		(0x21)	// DAC shelving filter coeff a [07:00]
#define ES8388_REG_DAC_CTRL12		(0x22)	// DAC shelving filter coeff b [29:24]
#define ES8388_REG_DAC_CTRL13		(0x23)	// DAC shelving filter coeff b  [23:16]
#define ES8388_REG_DAC_CTRL14		(0x24)	// DAC shelving filter coeff b [15:08]
#define ES8388_REG_DAC_CTRL15		(0x25)	// DAC shelving filter coeff b [07:00]
#define ES8388_REG_DAC_CTRL16		(0x26)	// Default 0000 0000
#define ES8388_REG_DAC_CTRL16_DFLT	(0x00)
	#define ES8388_LMIXSEL_MASK		(0x38)
	#define ES8388_LMIXSEL_SHIFT	(0x03)
	#define ES8388_LMIXSEL_LIN1		(0x00)
	#define ES8388_LMIXSEL_LIN2		(0x01)
	#define ES8388_LMIXSEL_ADCL_P	(0x03)
	#define ES8388_LMIXSEL_ADCL_N	(0x04)
	#define ES8388_LMIXSEL(x)		(((x)<<ES8388_LMIXSEL_SHIFT)&ES8388_LMIXSEL_MASK)
	#define ES8388_RMIXSEL_MASK		(0x03)
	#define ES8388_RMIXSEL_SHIFT	(0x00)
	#define ES8388_RMIXSEL_RIN1		(0x00)
	#define ES8388_RMIXSEL_RIN2		(0x01)
	#define ES8388_RMIXSEL_ADCR_P	(0x03)
	#define ES8388_RMIXSEL_ADCR_N	(0x04)
	#define ES8388_RMIXSEL(x)		(((x)<<ES8388_RMIXSEL_SHIFT)&ES8388_RMIXSEL_MASK)
#define ES8388_REG_DAC_CTRL17		(0x27)	// Default 0011 1000
#define ES8388_REG_DAC_CTRL17_DFLT	(0x38)
	#define ES8388_BIT_LD2LO		(1<<7)
	#define ES8388_BIT_LI2LO		(1<<3)
	#define ES8388_LI2LOVOL_MASK	(0x38)
	#define ES8388_LI2LOVOL_SHIFT	(0x03)
	#define ES8388_VOL_6DB			(0x00)
	#define ES8388_VOL_3DB			(0x01)
	#define ES8388_VOL_0DB			(0x02)
	#define ES8388_VOL_M3DB			(0x03)
	#define ES8388_VOL_M6DB			(0x04)
	#define ES8388_VOL_M9DB			(0x05)
	#define ES8388_VOL_M12DB		(0x06)
	#define ES8388_VOL_M15DB		(0x07)
	#define ES8388_LI2LOVOL(x)		(((x)<<ES8388_LI2LOVOL_SHIFT)&ES8388_LI2LOVOL_MASK)
#define ES8388_REG_DAC_CTRL18		(0x28)	// No data? Default 0010 1000
#define ES8388_REG_DAC_CTRL19		(0x29)	// No data? Default 0010 1000
#define ES8388_REG_DAC_CTRL20		(0x2A)	// Default 0011 1000
#define ES8388_REG_DAC_CTRL20_DFLT	(0x38)
	#define ES8388_BIT_RD2RO		(1<<7) 	// DACR to outmixer R
	#define ES8388_BIT_RI2RO		(1<<6)	// RIN to outmixer R
	#define ES8388_RI2ROVOL_MASK	(0x38)
	#define ES8388_RI2ROVOL_SHIFT	(0x03)
	#define ES8388_RI2ROVOL(x)		(((x)<<ES8388_RI2ROVOL_SHIFT)&ES8388_RI2ROVOL_MASK)
#define ES8388_REG_DAC_CTRL21		(0x2B)	// Default 0000 0000
#define ES8388_REG_DAC_CTRL21_DFLT	(0x00)
	#define ES8388_BIT_SLRCK		(1<<7)
	#define ES8388_BIT_LRCK_SEL		(1<<6)
	#define ES8388_BIT_OFFSET_DIS	(1<<5)
	#define ES8388_BIT_MCLK_DIS		(1<<4)
	#define ES8388_BIT_ADC_DLL_PWD	(1<<3)
	#define ES8388_BIT_DAC_DLL_PWD	(1<<2)
#define ES8388_REG_DAC_CTRL22		(0x2C)	// Default 0000 0000
	#define ES8388_REG_DAC_OFFSET	(ES8388_REG_DAC_CTRL21)
#define ES8388_REG_DAC_CTRL23		(0x2D)	// Default 0000 0000
	#define ES8388_BIT_VROI			(1<<4)
#define ES8388_REG_DAC_CTRL24		(0x2E)	// Default 0000 0000
	#define ES8388_LOUT1VOL			(ES8388_REG_DAC_CTRL24)
#define ES8388_REG_DAC_CTRL25		(0x2F)	// Default 0000 0000
	#define ES8388_ROUT1VOL			(ES8388_REG_DAC_CTRL25)
#define ES8388_REG_DAC_CTRL26		(0x30)	// Default 0000 0000
	#define ES8388_LOUT2VOL			(ES8388_REG_DAC_CTRL26)
#define ES8388_REG_DAC_CTRL27		(0x31)	// Default 0000 0000
	#define ES8388_ROUT2VOL			(ES8388_REG_DAC_CTRL27)
#define ES8388_REG_DAC_CTRL28		(0x32)	// No data? Default 0000 1000

bool AudioControlES8388_F32::configured = false;

bool AudioControlES8388_F32::enable(TwoWire *i2cBus, uint8_t addr, config_t cfg)
{
	configured = false;
	ctrlBus = i2cBus;
	i2cAddr = addr;
	ctrlBus->begin();
	ctrlBus->setClock(100000);				 
	if (!writeReg(ES8388_REG_MASTER_MODE_CTRL, 0x00))	// set to slave mode
	{
		return false; // codec not found
	}	
	writeReg(ES8388_REG_CHIP_PWR_MAN, 0xF3);									 // power down
	writeReg(ES8388_REG_DAC_CTRL21, ES8388_BIT_SLRCK);							 // DACLRC = ADCLRC
	writeReg(ES8388_REG_CHIP_CTRL1, ES8388_VMIDSEL_5K | ES8388_BIT_ENREF);		 // 50k divider,
	writeReg(ES8388_REG_CHIP_CTRL2, 0x40);										 // low power modes off, bit6 not defined? based on default value
	writeReg(ES8388_REG_ADC_PWR_MAN, 0x00);										 // power up ADC, turn off the PDNMICB?
	writeReg(ES8388_REG_DAC_PWR_MAN, ES8388_BIT_LOUT1_EN | ES8388_BIT_ROUT1_EN); // enable LR1

	switch (cfg)
	{
	case ES8388_CFG_LINEIN_DIFF:
		writeReg(ES8388_REG_ADC_CTRL2, ES8388_LINSEL(ES8388_INPUTDIFF) |	 // LIN=LIN1-RIN1 (ADCCTRL3[7] DS = 0)
										   ES8388_RINSEL(ES8388_INPUTDIFF) | // RIN=LIN2-RIN2 (DSR = 1)
										   ES8388_BIT_DSSEL |				 // use different DSR settings for L and R
										   ES8388_BIT_DSR);					 // DS setting for channel R
		break;
	case ES8388_CFG_LINEIN_SINGLE_ENDED:
		writeReg(ES8388_REG_ADC_CTRL2, ES8388_LINSEL(ES8388_INPUT1) |	  // LIN=LIN1-RIN1 (ADCCTRL3[7] DS = 0)
										   ES8388_RINSEL(ES8388_INPUT1)); // RIN=LIN2-RIN2 (DSR = 1)
		break;
	default:
		writeReg(ES8388_REG_ADC_CTRL2, ES8388_LINSEL(ES8388_INPUT1) |	  // LIN=LIN1-RIN1 (ADCCTRL3[7] DS = 0)
										   ES8388_RINSEL(ES8388_INPUT1)); // RIN=LIN2-RIN2 (DSR = 1)
		break;
	}
	writeReg(ES8388_REG_ADC_CTRL6, 0x00); // disable HPF
							   // 0dB
	writeReg(ES8388_REG_ADC_CTRL4, ES8388_ADCWL(ES8388_ADCWL_32BIT));			// 24bit
	writeReg(ES8388_REG_ADC_CTRL5, ES8388_ADCFSRATIO(ES8388_FSRATIO_256)); 		// 256*Fs, single speed
	// ADC digital volume
	writeReg(ESP8388_LADCVOL, 0x00); // 0dB
	writeReg(ESP8388_RADCVOL, 0x00); // 0dB
	// DAC setup
	writeReg(ES8388_REG_DAC_CTRL1, ES8388_DACWL(ES8388_DACWL_32BIT));	   		// 24bit
	writeReg(ES8388_REG_DAC_CTRL2, ES8388_DACFSRATIO(ES8388_FSRATIO_256)); 		// 256*Fs single speed
	// DAC digital volume
	writeReg(ES8388_REG_DAC_CTRL4, 0x00); // 0dB
	writeReg(ES8388_REG_DAC_CTRL5, 0x00); // 0dB
	// Mixer Setup
	writeReg(ES8388_REG_DAC_CTRL16, ES8388_LMIXSEL(ES8388_LMIXSEL_ADCL_P) |
										ES8388_RMIXSEL(ES8388_RMIXSEL_ADCR_P));
	writeReg(ES8388_REG_DAC_CTRL17, ES8388_BIT_LD2LO); // LDAC to left mixer enable, gain 0dB
	writeReg(ES8388_REG_DAC_CTRL20, ES8388_BIT_RD2RO); // RDAC to right mixer enable, gain 0dB

	// R L OUT volume
	dacGain = 0x1E;
	writeReg(ES8388_LOUT1VOL, dacGain); // L1 0dB
	writeReg(ES8388_ROUT1VOL, dacGain); // R1 0dB
	// optimize A/D conversion for 1/4 Vrms range
	optimizeConversion(0);
	writeReg(ES8388_REG_CHIP_PWR_MAN, 0x00); // Power up DEM and STM

	configured = true;
	return true;
}

void AudioControlES8388_F32::optimizeConversion(uint8_t range)
{
	uint8_t ingain[] = {0, 2, 4, 6, 8};		 // 0db, 6dB, 12dB, 18dB, 24dB
	uint8_t outvol[] = {30, 26, 22, 18, 14}; // 0db, -6dB, -12dB, -18dB, -24dB
	if (range < 0) range = 0;
	if (range > 4) range = 4;
	volume(outvol[range]);
	setInGain(ingain[range]);
}

// get and set the output level (analog gain)
// vol = 0-31
void AudioControlES8388_F32::volume(uint8_t vol)
{
	if (vol > 30)
		vol = 30;
	writeReg(ES8388_REG_DAC_CTRL24, vol); // LOUT1VOL
	writeReg(ES8388_REG_DAC_CTRL25, vol); // ROUT1VOL
}
bool AudioControlES8388_F32::volume(float n)
{
	n = constrain(n, 0.0f, 1.0f);
	uint8_t vol = n * 30.99f;
	if (vol > 30)
		vol = 30;
	writeReg(ES8388_REG_DAC_CTRL24, vol); // LOUT1VOL
	writeReg(ES8388_REG_DAC_CTRL25, vol); // ROUT1VOL
	return true;
}


uint8_t AudioControlES8388_F32::getOutVol()
{
	uint8_t vol;
	readReg(ES8388_REG_DAC_CTRL24, &vol);
	return vol;
}

bool AudioControlES8388_F32::setInGain(uint8_t gain)
{
	if (gain > 8)
		gain = 8;
	uint8_t temp;
	temp = gain << 4;
	temp = temp | gain;

	return writeReg(ES8388_REG_ADC_CTRL1, temp);
}

uint8_t AudioControlES8388_F32::getInGain()
{
	uint8_t temp;
	readReg(ES8388_REG_ADC_CTRL1, &temp);
	temp = (temp & 0xF0) >> 4;
	return temp;
}
void AudioControlES8388_F32::set_noiseGate(float thres)
{
	uint8_t thres_val = constrain(thres, 0.0f, 1.0f) * 31.99f;
	writeReg(ES8388_REG_ADC_CTRL14, ES8388_NGTH(thres_val) | ES8388_NGG(ES8388_NGG_ADCMUTE)| ES8388_BIT_NGAT_EN);	
}


bool AudioControlES8388_F32::analogBypass(bool bypass)
{
	bool res = true;
	if (bypass)
	{
		res = writeReg(ES8388_REG_DAC_CTRL17, ES8388_BIT_LI2LO | ES8388_LI2LOVOL(ES8388_VOL_0DB));
		res &= writeReg(ES8388_REG_DAC_CTRL20, ES8388_BIT_RI2RO | ES8388_RI2ROVOL(ES8388_VOL_0DB));
	}
	else
	{
		res = writeReg(ES8388_REG_DAC_CTRL17, ES8388_BIT_LD2LO | ES8388_LI2LOVOL(ES8388_VOL_0DB));
		res &= writeReg(ES8388_REG_DAC_CTRL20, ES8388_BIT_RD2RO | ES8388_RI2ROVOL(ES8388_VOL_0DB));
	}
	return res;
}

bool AudioControlES8388_F32::analogSoftBypass(bool bypass)
{
	bool res = true;
	if (bypass)
	{
		res &= writeReg(ES8388_REG_DAC_CTRL17, 	ES8388_BIT_LI2LO | 					// Lin on
												ES8388_BIT_LD2LO |					// L Dac on
												ES8388_LI2LOVOL(ES8388_VOL_0DB)); 	// Lin gain 0dB
		res &= writeReg(ES8388_REG_DAC_CTRL20,  ES8388_BIT_RI2RO | 					// Rin on
												ES8388_BIT_RD2RO |					// R Dac on
												ES8388_RI2ROVOL(ES8388_VOL_0DB)); 	// Rin gain 0dB
	}
	else
	{
		res = writeReg(ES8388_REG_DAC_CTRL17, ES8388_BIT_LD2LO | ES8388_LI2LOVOL(ES8388_VOL_0DB));
		res &= writeReg(ES8388_REG_DAC_CTRL20, ES8388_BIT_RD2RO | ES8388_RI2ROVOL(ES8388_VOL_0DB));
	}
	return res;
}

bool AudioControlES8388_F32::writeReg(uint8_t addr, uint8_t val)
{
	ctrlBus->beginTransmission(i2cAddr);
	ctrlBus->write(addr);
	ctrlBus->write(val);
	return ctrlBus->endTransmission() == 0;
}
bool AudioControlES8388_F32::readReg(uint8_t addr, uint8_t *valPtr)
{
	ctrlBus->beginTransmission(i2cAddr);
	ctrlBus->write(addr);
	if (ctrlBus->endTransmission(false) != 0)
		return false;	
	if (ctrlBus->requestFrom((int)i2cAddr, 1) < 1)  return false;
	*valPtr = ctrlBus->read();
	return true;
}

uint8_t AudioControlES8388_F32::modifyReg(uint8_t reg, uint8_t val, uint8_t iMask)
{
	uint8_t val1;
	val1 = (readReg(reg, &val1) & (~iMask)) | val;
	if (!writeReg(reg, val1))
		return 0;
	return val1;
}