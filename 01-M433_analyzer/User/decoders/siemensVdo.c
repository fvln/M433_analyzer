#include "decoder.h"
#include "main.h"

/*******************************************************************************
 * SIEMENSVDO DECODER (Car key fob)                                            *
 *******************************************************************************
 * SENTENCE ENCODING *
 *********************
 * SYNC:
 * 		High sync, Low sync
 * DATA:
 *  	Each high and low pulse may be short or long
 *
 ******************
 * INTERPRETATION *
 ******************
 * Expected: >= 64 data bits
 *
 * This decoder has not been tested extensively and must be improved
 *
 */

#define MIN_SHORT_LEN	150
#define MAX_SHORT_LEN	350
#define MIN_LONG_LEN	400
#define MAX_LONG_LEN	600

#define MIN_SYNC_LEN	1700
#define MAX_SYNC_LEN	4000

#define MIN_NUM_PULSES	32

#define RAW_DATA_LEN	8

static uint16_t decode_siemens(uint32_t	*pulseLens, uint16_t nbPulses);

decoderDesc_t decoder_siemensVdo = 
{
	.name 	= "SiemensVdo",
	.minPulseLen  	= MIN_SHORT_LEN,
	.maxPulseLen  	= MAX_SYNC_LEN,
	.minNumPulses 	= MIN_NUM_PULSES,
	.decoderFunc	= decode_siemens
};

static uint16_t decode_siemens(uint32_t	*pulseLens, uint16_t nbPulses)
{
	uint16_t	i				= 0,
				dataOffset 		= 0,
				dataByteOffset 	= 0,
				parseOffset		= 1,	// each bit is a (low,high) couple
				result			= 0;

	uint8_t		rawData[RAW_DATA_LEN],
				dataByte 		= 0,
				dataBitOffset	= 0;
	
	
	// We may try to parse the sentence from different offsets if we find valid sync pulses,
	// until valid data can be decoded
	while (nbPulses - parseOffset > 42)
	{
		// Skip the preamble, look for the first pause
		// Sync: pause low, pause high
		
		for (i = parseOffset; i < nbPulses-1; i+=2)
		{
			// Skip the preamble, just look for the next pause
			if (IS_SYNC(pulseLens[i]) && IS_SYNC(pulseLens[i+1]))
			{
				i += 2;
				break;
			}
		}
		
		if (i == (nbPulses-1)) {
			break;
		}

		// PRINTF("Sync len: %d (%d%%)\n", nbSync, nbSync*100/128);
		
		memset(rawData, 0, RAW_DATA_LEN);
		dataByte = dataByteOffset = 0;
		dataBitOffset = 8;
		
		for (i = dataOffset; i < nbPulses-1; i++)
		{
			if (IS_SHORT(pulseLens[i]))
			{
				// Nothing to do
				dataBitOffset--;
			}
			else if (IS_LONG(pulseLens[i]))
			{
				dataByte |= (1 << --dataBitOffset);
			}
			else
			{
				break;
			}
			
			// If this byte is complete, switch to the next one
			if (!dataBitOffset)
			{
				rawData[dataByteOffset++] = dataByte;
				dataBitOffset = 8;
				dataByte = 0;
			}
			
			if (dataByteOffset == RAW_DATA_LEN)
			{
				break;
			}
		}
		
		if (dataByteOffset >= 5)
		{
			// We decoded at least 40 bits. Display the code
			PRINTF("Siemens: message: %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x\n", 
				rawData[0], rawData[1], rawData[2], rawData[3], rawData[4], rawData[5], rawData[6], rawData[7]);
			result = i;
			
			memset(rawData, 0, RAW_DATA_LEN);
			dataByte = dataByteOffset = 0;
			dataBitOffset = 8;
		}
		
		parseOffset = i;
	}
	
	if (result > 0) {
		return result;
	}
	
	return 0;
}
