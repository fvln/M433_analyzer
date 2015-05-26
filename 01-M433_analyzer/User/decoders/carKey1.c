#include "main.h"
#include "decoder.h"

/*******************************************************************************
 * CARKEY1 DECODER (Unknown model, seems to be a rolling code or keeloq)       *
 *******************************************************************************
 * SENTENCE ENCODING *
 *********************
 * SYNC:
 * 		A pair of pulses with an expected total len
 * DATA:
 *  	Similar to RCSwitch
 *
 ******************
 * INTERPRETATION *
 ******************
 * Expected: 64 data bits
 */


#define MIN_SHORT_LEN	400
#define MAX_SHORT_LEN	600

#define MIN_LONG_LEN	1100
#define MAX_LONG_LEN	1300

#define MIN_SYNC_LEN	2900
#define MAX_SYNC_LEN	3100

#define RAW_DATA_LEN	64	// bits
#define MIN_NUM_PULSES	130	// at least 64b + 2 sync pulses


static uint16_t decode_CarKey1(uint32_t *pulseLens, uint16_t nbPulses);

decoderDesc_t decoder_CarKey1 =
{
	.name			= "CarKey1",
	.minPulseLen  	= MIN_SHORT_LEN,
	.maxPulseLen  	= MAX_SYNC_LEN,
	.minNumPulses 	= MIN_NUM_PULSES,
	.decoderFunc	= decode_CarKey1
};


static uint8_t interpret_CarKey1(uint32_t rawData, uint32_t rawData2, uint8_t nbBits)
{
	if (nbBits < 12)
	{
		return 0;
	}
	
	if (nbBits == 64)
	{
		PRINTF("%s,%08X%08X\n", decoder_CarKey1.name, rawData, rawData2);
	}
	else
	{
		PRINTF("%s,%d,%08X%08X\n", decoder_CarKey1.name, nbBits, rawData, rawData2);
	}
	return 1;
}

/*!
 * @param[in]	pulseLens	Durations of the pulses to decode
 * @param[in]	nbPulses	Number of pulses in pulseLens
 * @return		Number of pulses used, starting from offset 0
 */
static uint16_t decode_synced_CarKey1(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	i,
				dataByteOffset  = 0,
				dataBitOffset	= 32; // Bits to receive
	uint32_t	rawData[2]		= {0, 0};

	
	// Each bit can be:
	// short low+long high (1)
	// long low+short high (0)
	for (i = 0; i < nbPulses-1; i += 2)
	{
		if (IS_SHORT(pulseLens[i]) && IS_LONG(pulseLens[i+1]))
		{
			--dataBitOffset;
		}
		else if (IS_LONG(pulseLens[i]) && IS_SHORT(pulseLens[i+1]))
		{
			rawData[dataByteOffset] |= (1 << --dataBitOffset);
		}
		else
		{
			// Pulse len is invalid
			break;
		}
		
		if (dataBitOffset == 0)
		{
			dataBitOffset = 32;
			dataByteOffset++;
		}
		
		if (dataByteOffset == 2)
		{
			i += 2;
			break;
		}
	}
	
	if (interpret_CarKey1(rawData[0], rawData[1], (32*dataByteOffset)+(32-dataBitOffset)))
	{
		return i;
	}
	
	return 0;
}

static uint16_t decode_CarKey1(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	result = 0;
	uint16_t	syncOffset = 0;
	
	while (nbPulses - syncOffset > MIN_NUM_PULSES)
	{	
		if (IS_SYNC(pulseLens[syncOffset] + pulseLens[syncOffset+1]))
		{
			// Valid sync pulses found - try and decode the sentence 
			syncOffset += 2;
			result = decode_synced_CarKey1(pulseLens + syncOffset, nbPulses - syncOffset);
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

