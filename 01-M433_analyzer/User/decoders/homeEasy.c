#include "main.h"
#include "decoder.h"

/*******************************************************************************
 * HOMEEASY DECODER                                                            *
 *******************************************************************************
 * SENTENCE ENCODING *
 *********************
 * SYNC:
 * 		Low pulse with "sync" len
 * DATA:
 *  	Similar to RCSwitch. See:
 *      - http://homeeasyhacking.wikia.com/wiki/Simple_Protocol
 *      - http://homeeasyhacking.wikia.com/wiki/Advanced_Protocol
 *
 ******************
 * INTERPRETATION *
 ******************
 * Expected: 24 or 32 data bits. See the links above.
 */

// Low sync should be 2675탎
#define MIN_SYNC_LEN		2500
#define MAX_SYNC_LEN		2800

// High pulse should be 275탎
#define MIN_HIGH_LEN		150
#define MAX_HIGH_LEN		310

// Short low should be 240탎, long low should be 1300탎
#define MIN_LONG_LOW_LEN	800
#define MAX_LONG_LOW_LEN	1400

#define	MIN_NUM_PULSES		48

#define RAW_DATA_LEN		32	// bits

#define TID_MASK			0xFFFFFFC0	// 11111111 11111111 11111111 11000000
#define TID_SHIFT			6

#define GROUP_MASK			0x20	// 100000
#define GROUP_SHIFT			5

#define STATE_MASK			0x10 	// 10000
#define STATE_SHIFT			4

#define CODE_MASK			0xF		// 1111
#define CODE_SHIFT			0


static uint16_t decode_homeEasy(uint32_t *pulseLens, uint16_t nbPulses);

decoderDesc_t decoder_HomeEasy = {
	.name			= "HomeEasy",
	.minPulseLen  	= MIN_HIGH_LEN,
	.maxPulseLen  	= MAX_LONG_LOW_LEN,
	.minNumPulses	= MIN_NUM_PULSES,
	.decoderFunc	= decode_homeEasy
};


static uint16_t decode_synced_sentence_homeEasy(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	i;
	uint8_t		dataBitOffset,
	
				// Usage: manchesterBit == 0 if we are receiving the first half of a bit
				// manchesterBit == 1 if the first half of the bit was 1
				// manchesterBit == 2 if the first half of the bit was 0
				manchesterBit;	
	
	uint32_t	rawData			= 0;
	
	
	// Prepare the data buffer
	rawData			= 0;
	dataBitOffset	= RAW_DATA_LEN;
	manchesterBit	= 0;
	
	// Loop on the remaining pulses
	// i is already pointing to the first data pulse
	for (i = 0; i < nbPulses - 1; i += 2)
	{
		if (dataBitOffset && IS_HIGH_PULSE(pulseLens[i]))
		{
			if (manchesterBit == 0)
			{
				manchesterBit = (IS_LONG_LOW(pulseLens[i+1]) ? 1 : 2);
			}
			else if (manchesterBit == 1 && !IS_LONG_LOW(pulseLens[i+1]))
			{
				// Received manchester 10 => codes a 1
				rawData |= (1 << --dataBitOffset);
				manchesterBit = 0;
			}
			else if (manchesterBit == 2 && IS_LONG_LOW(pulseLens[i+1]))
			{
				// Received manchester 01 => codes a 0
				dataBitOffset--;
				manchesterBit = 0;
			}
			else
			{
				// Invalid bit
				break;
			}
		}
		else
		{
			break;				
		}
	}
	
	if (dataBitOffset == 0)
	{
		// Decode the raw data
		PRINTF("%s,Transmitter=0x%04X,Device=%d,State=%d,IsGroup=%d\n", decoder_HomeEasy.name, BIN_VALUE(TID), BIN_VALUE(CODE), BIN_VALUE(STATE), BIN_VALUE(GROUP));
		return i;
	}
	else if (dataBitOffset <= 8)
	{
		// Display the raw data
		PRINTF("%s,%d,0x%08X\n", decoder_HomeEasy.name, RAW_DATA_LEN - dataBitOffset, rawData);
		return i;
	}
	
	return 0;
}

static uint16_t decode_homeEasy(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	result = 0;
	uint16_t	syncOffset = 0;
	
	while (nbPulses - syncOffset > MIN_NUM_PULSES)
	{	
		// Stop at the first low sync pulse
		if (IS_SYNC(pulseLens[syncOffset+1]))
		{
			// Valid sync pulses found - try and decode the sentence 
			syncOffset += 2;
			result = decode_synced_sentence_homeEasy(pulseLens + syncOffset, nbPulses - syncOffset);
			syncOffset += result;
		}
		else
		{
			syncOffset += 2;
		}
	}
	
	return (result > 0);
}

