#include "decoder.h"
#include "generic_rcswitch.h"

/*******************************************************************************
 * RCSWITCH DECODER                                                            *
 *******************************************************************************
 * SENTENCE ENCODING *
 *********************
 * SYNC:
 * 		Short high, Sync low
 * DATA:
 *  	RCSwitch:
 *      short high + long low is a 0 bit,
 *      long high + short low is a 1 bit
 * 
 * Reference: https://code.google.com/p/rc-switch/wiki/KnowHow_LineCoding
 *
 ******************
 * INTERPRETATION *
 ******************
 * Expected: 12 valid Tri-state (==24 Two-state) data bits
 */


#define MIN_SHORT_LEN	300
#define MAX_SHORT_LEN	500
#define MIN_SYNC_LEN	(31 * MIN_SHORT_LEN)
#define MAX_SYNC_LEN	(31 * MAX_SHORT_LEN)
#define MIN_NUM_PULSES	50		// 2 sync + 2 * 24


#define CHANNEL_MASK	0xFF000000
#define CHANNEL_SHIFT	24

#define ADDR_MASK		0xFC0000	// 11111100 00000000 00000000
#define ADDR_SHIFT		18

#define PAD_MASK		0x3FC00		// 11 11111100 00000000
#define PAD_SHIFT		10

#define STATE_MASK		0x300 		// 11 00000000
#define STATE_SHIFT		8

#define EVEN_BITS_MASK	0x55555700	// 01010101 01010101 01010111 00000000

static uint32_t	pairLen;
static uint16_t decode_rcswitch(uint32_t *pulseLens, uint16_t nbPulses);

decoderDesc_t decoder_RCSwitch = 
{
	.name			= "RCswitch",
	.minPulseLen  	= MIN_SHORT_LEN,
	.maxPulseLen  	= MAX_SYNC_LEN,
	.minNumPulses 	= MIN_NUM_PULSES,
	.decoderFunc	= decode_rcswitch
};


static uint8_t interpret_rcswitch(uint32_t rawData, uint8_t nbBits)
{
	// Decode the raw data
	if (nbBits == 24 && ((rawData & EVEN_BITS_MASK) == rawData))
	{
		// None of the odd bits is set (we don't check the last 2 since the state value may be 0b00 or 0b11)
		PRINTF("%s,Channel=%d,Addr=%d,Padd85=%d,Data=%d,PairLen=%d\n", decoder_RCSwitch.name, BIN_VALUE(CHANNEL), BIN_VALUE(ADDR), BIN_VALUE(PAD), BIN_VALUE(STATE), pairLen);
		return 1;
	}
	else if (nbBits >= 10)
	{
		// Decode the raw data
		PRINTF("%s,Length=%d,Data=0x%08x,PairLen=%d\n", decoder_RCSwitch.name, nbBits, rawData, pairLen);
		return 1;
	}
	
	return 0;
}

static uint16_t decode_rcswitch(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	result = 0;
	uint16_t	syncOffset = 0;
	double		syncRatio;
	
	while (nbPulses - syncOffset > MIN_NUM_PULSES)
	{	
		syncRatio = (double)pulseLens[syncOffset+1] / (double)pulseLens[syncOffset];
		if (20.0 < syncRatio)
		{
			// Valid sync pulses found - try and decode the sentence 
			// Each pair of pulses must last (pulseLens[syncOffset] + pulseLens[syncOffset+1]) / 32 * 4
			pairLen = (pulseLens[syncOffset] + pulseLens[syncOffset+1]) / 8;
			
			// Shift to the first data pair
			syncOffset += 2;
			result = decode_generic_rcswitch_sentence(pulseLens + syncOffset, nbPulses - syncOffset, pairLen, interpret_rcswitch, 0);
			syncOffset += result;
		}
		else
		{
			syncOffset += 2;
		}
	}
	
	return (result > 0);
}

