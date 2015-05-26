#include "main.h"
#include "generic_rcswitch.h"

/*******************************************************************************
 * CAME432 DECODER                                                             *
 *******************************************************************************
 * SENTENCE ENCODING *
 *********************
 * SYNC:
 * 		Low pulse with "sync" len
 * DATA:
 *  	Similar to RCSwitch (fixed pair length with a 1/3-2/3 ratio)
 *      The pair begins with a low pulse, then a high pulse (instead of high-then-low),
 *      this is why we use decode_2ndPulse_rcswitch_sentence() instead of the default
 *      rcswitch decoder
 *
 ******************
 * INTERPRETATION *
 ******************
 * Expected: 12 or 13 data bits (in the latter case, the MSB is '1' and can be ignored).
 * I couldn't guess why this bit sometimes appears, and sometimes doesn't.
 */
 

#define RAW_DATA_LEN	32		// bits

#define MIN_SHORT_LEN	300		// Measured: 345µs
#define MAX_SHORT_LEN	400

#define MIN_LONG_LEN	600
#define MAX_LONG_LEN	750

#define MIN_SYNC_LEN	14600	// Measured: 15.6ms
#define MAX_SYNC_LEN	16600

// #define MIN_PAIR_LEN	900		// Measured: 1.009ms
// #define MAX_PAIR_LEN	1100

#define MIN_NUM_PULSES	26		// 2 * 12

#define PROLOGUE		(rawData & 0x80000000)


static uint16_t decode_came432(uint32_t *pulseLens, uint16_t nbPulses);

decoderDesc_t decoder_Came432Na = 
{
	.name 			= "Came432Na",
	.minPulseLen  	= MIN_SHORT_LEN,
	.maxPulseLen  	= MAX_SYNC_LEN,
	.minNumPulses 	= MIN_NUM_PULSES,
	.decoderFunc	= decode_came432
};


static uint8_t interpret_came432(uint32_t rawData, uint8_t nbBits)
{
	if (nbBits == 13 && PROLOGUE)
	{
		PRINTF("%s,%db,0x%08X\n", decoder_Came432Na.name, nbBits-1, rawData << 1);
		return 1;
	}
	else if (nbBits == 12)
	{
		PRINTF("%s,%db,0x%08X\n", decoder_Came432Na.name, nbBits, rawData);
		return 1;
	}
	else if (nbBits >= 8)
	{
		PRINTF("%s,%db,x%08X\n", decoder_Came432Na.name, nbBits, rawData);
		return 1;
	}
	else
	{
		return 0;
	}
}

static uint16_t decode_2ndPulse_rcswitch_sentence(uint32_t *pulseLens, uint16_t nbPulses, ui32InterpreterFunc_t dataHandler, uint32_t revert)
{
	uint16_t	i,
				dataBitOffset	= RAW_DATA_LEN; // Bits to receive
	uint32_t	rawData 		= 0;
		
	
	// Each bit can be:
	// short low+long high (1)
	// long low+short high (0)
	for (i = 0; i < nbPulses-1; i += 2)
	{
		if (IS_SHORT(pulseLens[i+1]))
		{
			rawData |= (revert << --dataBitOffset);
		}
		else if (IS_LONG(pulseLens[i+1]))
		{
			rawData |= ((1 - revert) << --dataBitOffset);
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


static uint16_t decode_came432(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	result = 0;
	uint16_t	syncOffset = 0;
	
	while (nbPulses - syncOffset > MIN_NUM_PULSES)
	{	
		if (IS_SYNC(pulseLens[syncOffset+1]))
		{
			// Valid sync pulses found - try and decode the sentence 
			// start @syncOffset+2)
			syncOffset += 2;			
			result = decode_2ndPulse_rcswitch_sentence(pulseLens + syncOffset, nbPulses - syncOffset, interpret_came432, 0);
			syncOffset += result;
		}
		else
		{
			syncOffset += 2;
		}
	}
	
	return (result > 0);
}


