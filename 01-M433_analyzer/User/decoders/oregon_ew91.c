#include "main.h"

/*******************************************************************************
 * OREGON EW91 DECODER                                                         *
 *******************************************************************************
 * SENTENCE ENCODING *
 *********************
 * SYNC:
 * 		Preamble: short high, short low, short high, short low
 *      Then sync: long high, long low
 * DATA:
 *  	High pulses are always short, then:
 *      - Short low => bit = 0
 *      - Long low => bit = 1
 *
 * Reference: http://gcrnet.net/node/24
 *
 ******************
 * INTERPRETATION *
 ******************
 * Expected: 8 significant bytes, last 4 are complements of first 4.
 * Found a better byte alignment by inserting a first 0 in the burst.
 * 4 Bytes meaning
 * XXXX-XXXX  XXCC-SXXX  XXXX-RHHH  LLLL-DDDD
 * CC = Channel
 * S = Sign 0:Positive, 1:Negative
 * R = Reset 1 on first frame after reset
 * HHH, LLLL, DDDD High & low temp digit, decimal all in BCD
 */

#define MIN_SHORT_LEN	1300
#define MAX_SHORT_LEN	2500

#define MIN_LONG_LEN	3500
#define MAX_LONG_LEN	4500

#define RAW_DATA_BYTES	8	// bytes
#define MIN_NUM_PULSES	66	// at least 4 bytes * 2 pulses + 2 sync pulses

#define CHANNEL_BYTE	1
#define CHANNEL_MASK	48 //0b110000
#define CHANNEL_SHIFT	4

#define SIGN_BYTE		1
#define SIGN_MASK		8 //0b1000
#define SIGN_SHIFT		3

#define THIGH_BYTE		2
#define THIGH_MASK		7 //0b111
#define THIGH_SHIFT		0

#define TLOW_BYTE		3
#define TLOW_MASK		240 //0b11110000
#define TLOW_SHIFT		4

#define TDEC_BYTE		3
#define TDEC_MASK		15 //0b1111
#define TDEC_SHIFT		0

static uint16_t decode_oregon_ew91(uint32_t *pulseLens, uint16_t nbPulses);

decoderDesc_t decoder_OregonEW91 = {
	.name 			= "OregonEW91",
	.minPulseLen  	= MIN_SHORT_LEN,
	.maxPulseLen  	= MAX_LONG_LEN,
	.minNumPulses 	= MIN_NUM_PULSES,
	.decoderFunc	= decode_oregon_ew91
};


static uint8_t interpret_oregon_ew91(uint8_t rawData[RAW_DATA_BYTES], uint8_t nbBytes)
{
	if (nbBytes < 4) {
		return 0;
	}
	
	// Run the parity checks
	if ((rawData[0] ^ rawData[4]) != 255 ||
		(rawData[1] ^ rawData[5]) != 255 ||
		(rawData[2] ^ rawData[6]) != 255 ||
		(rawData[3] ^ rawData[7]) != 255)
	{
		if (BIN_VALUE_MB(THIGH) <= 9 && BIN_VALUE_MB(TLOW) <= 9 && BIN_VALUE_MB(TDEC) <= 9)
		{
			// Normal sentence seems good! Use this one anyway
			PRINTF("%s,%d,%c%d%d.%d\n", decoder_OregonEW91.name, BIN_VALUE_MB(CHANNEL), (BIN_VALUE_MB(SIGN) ? '-' : '0'), BIN_VALUE_MB(THIGH), BIN_VALUE_MB(TLOW), BIN_VALUE_MB(TDEC));
			return 1;
		}
		
		if (nbBytes == RAW_DATA_BYTES)
		{
			// Otherwise, replace the normal sentence with the second sentence inverted
			rawData[0] = (rawData[4] ^ 0xff);
			rawData[1] = (rawData[5] ^ 0xff);
			rawData[2] = (rawData[6] ^ 0xff);
			rawData[3] = (rawData[7] ^ 0xff);
			
			if (BIN_VALUE_MB(THIGH) <= 9 && BIN_VALUE_MB(TLOW) <= 9 && BIN_VALUE_MB(TDEC) <= 9 && (BIN_VALUE_MB(CHANNEL) == 1 || BIN_VALUE_MB(CHANNEL) == 2))
			{
				// Normal sentence seems good! Use this one
				PRINTF("%s,%d,%c%d%d.%d\n", decoder_OregonEW91.name, BIN_VALUE_MB(CHANNEL), (BIN_VALUE_MB(SIGN) ? '-' : '0'), BIN_VALUE_MB(THIGH), BIN_VALUE_MB(TLOW), BIN_VALUE_MB(TDEC));
				return 1;
			}
		}
		
		// Neither sentence was valid => discard the data
		return 0;
	}
	
	// Parity check succeeded
	PRINTF("%s,%d,%c%d%d.%d\n", decoder_OregonEW91.name, BIN_VALUE_MB(CHANNEL), (BIN_VALUE_MB(SIGN) ? '-' : '0'), BIN_VALUE_MB(THIGH), BIN_VALUE_MB(TLOW), BIN_VALUE_MB(TDEC));
	return 1;
}

/*!
 * @param[in]	pulseLens	Durations of the pulses to decode
 * @param[in]	nbPulses	Number of pulses in pulseLens
 * @return		Number of pulses used, starting from offset 0
 */
static uint16_t decode_synced_sentence(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	i,
				dataByteOffset	= 0;
	
	uint8_t		dataByte 		= 0,
				dataBitOffset	= 7; // First bit of first offset must be left to 0 for alignment
	
	uint8_t		rawData[RAW_DATA_BYTES];
		
	// Decode the raw data
	// pulseLens should point to the first data pulse
	for (i = 0; i < nbPulses-1; i += 2)
	{
		if (!IS_SHORT(pulseLens[i]))
		{
			if (dataByteOffset < 4)
			{
				// PRINTF("Oregon: Invalid pulse @%d, len=%dus\n", i, pulseLens[i]);
				return 0;
			}
			else
			{
				// We have decoded enough data to try and decode the sentence, without error correction
				break;
			}
		}
		if (IS_SHORT(pulseLens[i+1]))
		{
			// Nothing to do
			dataBitOffset--;
		}
		else if (IS_LONG(pulseLens[i+1]))
		{
			dataByte |= (1 << --dataBitOffset);
		}
		
		// If this byte is complete, switch to the next one
		if (!dataBitOffset)
		{
			rawData[dataByteOffset++] = dataByte;
			dataBitOffset = 8;
			dataByte = 0;
		}
		
		if (dataByteOffset == RAW_DATA_BYTES)
		{
			break;
		}
	}
	
	if (interpret_oregon_ew91(rawData, dataByteOffset+1))
	{
		return i;
	}
	else
	{
		return 0;
	}
}

static uint16_t decode_oregon_ew91(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	result = 0;
	uint16_t	syncOffset = 0;
	
	while (nbPulses - syncOffset > MIN_NUM_PULSES)
	{	
		if (IS_LONG(pulseLens[syncOffset]) && IS_LONG(pulseLens[syncOffset+1]))
		{
			// Valid sync pulses found - try and decode the sentence 
			syncOffset += 2;
			result = decode_synced_sentence(pulseLens + syncOffset, nbPulses - syncOffset);
			syncOffset += result;
		}
		else
		{
			syncOffset += 2;
		}
	}
	
	return (result > 0);
}

