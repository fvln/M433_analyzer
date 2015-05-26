#include "decoder.h"
#include "generic_rcswitch.h"

/*******************************************************************************
 * DIPSWITCH DECODER                                                           *
 *******************************************************************************
 * SENTENCE ENCODING *
 *********************
 * SYNC:
 * 		Short high, sync low
 * DATA:
 *  	Similar to RCSwitch (fixed pair length with a 1/3-2/3 ratio)
 *      The pair begins with a low pulse, then a high pulse (instead of high-then-low),
 *      this is why we use decode_2ndPulse_rcswitch_sentence() instead of the default
 *      rcswitch decoder
 *
 ******************
 * INTERPRETATION *
 ******************
 * Expected: 12 data bits, including a prologue (MSB set to 0) and an epilogue
 * (LSB set to 1). The 10 remaining bits correspond to the value of the dip switch
 */

#define MIN_SHORT_LEN	610		// Measured: 710µs
#define MAX_SHORT_LEN	810

#define MIN_LONG_LEN	1320
#define MAX_LONG_LEN	1520

#define MIN_SYNC_LEN	25000	// Measured: 26ms
#define MAX_SYNC_LEN	27000

#define AVG_PAIR_LEN	2130	// Measured: 2,135ms

#define MIN_NUM_PULSES	28		// 2 sync + 2 * 12
#define RAW_DATA_LEN	32

#define PROLOGUE		(rawData & 0x80000000)
#define	EPILOGUE		(rawData & 0x00100000)

#define DIPCODE_MASK	0x7FE00000
#define DIPCODE_SHIFT	21


static uint16_t decode_dipswitch(uint32_t *pulseLens, uint16_t nbPulses);

decoderDesc_t decoder_dipSwitch =
{
	.name			= "DIPswitch",
	.minPulseLen  	= MIN_SHORT_LEN,
	.maxPulseLen  	= MAX_SYNC_LEN,
	.minNumPulses 	= MIN_NUM_PULSES,
	.decoderFunc	= decode_dipswitch
};


static uint8_t interpret_dipswitch(uint32_t rawData, uint8_t nbBits)
{
	// Decode the raw data
	if (nbBits == 12 && !PROLOGUE && EPILOGUE)
	{
		// None of the odd bits is set (we don't check the last 2 since the state value may be 0b00 or 0b11)
		PRINTF("%s: DIPcode value=0x%3X ", decoder_dipSwitch.name, BIN_VALUE(DIPCODE));
		return 1;
	}
	else if (nbBits >= 10)
	{
		// Decode the raw data
		PRINTF("%s: Valid code (%db): 0x%08X\n", decoder_dipSwitch.name, nbBits, rawData);
		return 1;
	}
	
	return 0;
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

static uint16_t decode_dipswitch(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	result = 0;
	uint16_t	syncOffset = 0;
	
	while (nbPulses - syncOffset > MIN_NUM_PULSES)
	{	
		if (IS_SHORT(pulseLens[syncOffset]) && IS_SYNC(pulseLens[syncOffset+1]))
		{
			// Valid sync pulses found - try and decode the sentence 
			
			// start @syncOffset+2
			syncOffset += 2;			
			result = decode_2ndPulse_rcswitch_sentence(pulseLens + syncOffset, nbPulses - syncOffset, interpret_dipswitch, 1);
			// syncOffset must be aligned back to a high pulse
			syncOffset += result;
		}
		else
		{
			syncOffset += 2;
		}
	}
	
	return (result > 0);
}

