#include "decoder.h"
#include "generic_rcswitch.h"


#define LONG_SHORT_MIN_RATIO	2.0
#define PULSE_TOLERANCE			0.10	// Pulse len tolerance
#define RAW_DATA_LEN			32		// bits

// #define	MIN_SYNC_LEN			1000
// #define	MAX_SYNC_LEN			100000

// Local macro redefinition
#undef 	IS_PAIR
#define	IS_PAIR(n)		((n) > minPairLen && (n) < maxPairLen)


uint16_t decode_generic_rcswitch_sentence(uint32_t *pulseLens, uint16_t nbPulses, uint32_t pairLen, ui32InterpreterFunc_t dataHandler, uint32_t revert)
{
	uint16_t	i,
				dataBitOffset	= RAW_DATA_LEN; // Bits to receive
	uint32_t	rawData 		= 0;
	uint32_t	minPairLen,
				maxPairLen;
	
	
	if (pairLen > 0)
	{
		minPairLen 		= (uint32_t)(pairLen * (1.0 - PULSE_TOLERANCE)),
		maxPairLen 		= (uint32_t)(pairLen * (1.0 + PULSE_TOLERANCE));
	}
	else
	{
		// Get the first pair length and use it as the reference
		minPairLen 	= (uint32_t)((pulseLens[0] + pulseLens[1]) * (1.0 - PULSE_TOLERANCE));
		maxPairLen 	= (uint32_t)((pulseLens[0] + pulseLens[1]) * (1.0 + PULSE_TOLERANCE));
	}
	
	
	// Each bit can be:
	// short low+long high (1)
	// long low+short high (0)
	for (i = 0; i < nbPulses-1; i += 2)
	{
		if (IS_PAIR(pulseLens[i]+pulseLens[i+1]))
		{
			if ((double)pulseLens[i] / (double)pulseLens[i+1] > LONG_SHORT_MIN_RATIO)
			{
				rawData |= ((1 - revert) << --dataBitOffset);
			}
			else if ((double)pulseLens[i+1] / (double)pulseLens[i] > LONG_SHORT_MIN_RATIO)
			{
				rawData |= (revert << --dataBitOffset);
			}
			else
			{
				// Pulse len is invalid
				break;
			}
		}
		else
		{
			// Pulse len is invalid
			break;
		}
		
		if (!dataBitOffset)
		{
			i += 2;
			break;
		}
	}
	
	if (dataHandler(rawData, RAW_DATA_LEN - dataBitOffset))
	{
		return i;
	}
	
	return 0;
}
