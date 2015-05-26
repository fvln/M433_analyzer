#include "main.h"
#include "decoder.h"

#define MIN_HIGH_LEN	430
#define MAX_HIGH_LEN	600

#define MIN_SHORT_LEN	1800
#define MAX_SHORT_LEN	2100

#define MIN_LONG_LEN	3900
#define MAX_LONG_LEN	4200

#define MIN_SYNC_LEN	8500
#define MAX_SYNC_LEN	8700

#define RAW_DATA_LEN	32	// bits
#define MIN_NUM_PULSES	20	// at least 4 bytes * 2 pulses + 2 sync pulses

#define THIGH_MASK		0xFF000000
#define THIGH_SHIFT		24

#define TLOW_MASK		0x00FF0000
#define TLOW_SHIFT		16

#define TDEC_MASK		0x0000FF00
#define TDEC_SHIFT		8


static uint16_t decode_UnknownTemp(uint32_t *pulseLens, uint16_t nbPulses);

decoderDesc_t decoder_UnknownTemp = {
	.name 			= "UnknownTemp",
	.minPulseLen  	= MIN_SHORT_LEN,
	.maxPulseLen  	= MAX_SYNC_LEN,
	.minNumPulses 	= MIN_NUM_PULSES,
	.decoderFunc	= decode_UnknownTemp
};


static uint8_t interpret_UnknownTemp(uint32_t	rawData, uint8_t nbBits)
{
	uint8_t	dataBytes[4];
	memcpy(dataBytes, &rawData, 4);
	
	if (nbBits == 24)
	{
		if (BIN_VALUE(TDEC) <= 9 && ((rawData & 0xFF) == 0))
		{
			PRINTF("%s,%08X,Humid=%d.%d %%\n", decoder_UnknownTemp.name, rawData, BIN_VALUE(TLOW), BIN_VALUE(TDEC));
		}
		else
		{
			PRINTF("%s,%d,%08X\n", decoder_UnknownTemp.name, nbBits, rawData);
		}
		return 1;
	}
	
	return 0;
}

/*!
 * @param[in]	pulseLens	Durations of the pulses to decode
 * @param[in]	nbPulses	Number of pulses in pulseLens
 * @return		Number of pulses used, starting from offset 0
 */
static uint16_t decode_synced_UnknownTemp(uint32_t *pulseLens, uint16_t nbPulses, uint32_t revert)
{
	uint16_t	i;
	
	uint8_t		dataBitOffset	= RAW_DATA_LEN; // Bits to receive
	uint32_t	rawData 		= 0;
		
	// Decode the raw data
	// pulseLens should point to the first data pulse
	for (i = 0; i < nbPulses-1; i += 2)
	{
		if (!IS_HIGH_PULSE(pulseLens[i]))
		{
			// Invalid high pulse len
			break;
		}		
		else if (IS_SHORT(pulseLens[i+1]))
		{
			// Nothing to do
			rawData |= (revert << --dataBitOffset);
		}
		else if (IS_LONG(pulseLens[i+1]))
		{
			rawData |= ((1 - revert) << --dataBitOffset);
		}
		else
		{
			// Invalid low pulse len
			break;
		}
		
		// If the receive buffer is full => break
		if (!dataBitOffset)
		{
			break;
		}
	}
	
	if (interpret_UnknownTemp(rawData, RAW_DATA_LEN-dataBitOffset))
	{
		return i;
	}
	else
	{
		return 0;
	}
}

static uint16_t decode_UnknownTemp(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	result = 0;
	uint16_t	syncOffset = 0;
	
	while (nbPulses - syncOffset > MIN_NUM_PULSES)
	{	
		if (IS_HIGH_PULSE(pulseLens[syncOffset]) && IS_SYNC(pulseLens[syncOffset+1]))
		{
			// Valid sync pulses found - try and decode the sentence 
			syncOffset += 2;
			result = decode_synced_UnknownTemp(pulseLens + syncOffset, nbPulses - syncOffset, 0);
			syncOffset += result;
		}
		else
		{
			syncOffset += 2;
		}
		
		if (result > 0) {
			break;
		}
	}
	
	return (result > 0);
}

