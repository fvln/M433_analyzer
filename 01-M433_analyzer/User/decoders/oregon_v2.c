#include "main.h"

/*******************************************************************************
 * OREGON V2 DECODER (UNTESTED)                                                *
 *******************************************************************************
 * SENTENCE ENCODING *
 *********************
 * SYNC:
 * 		To be documented
 * DATA:
 *  	Manchester encoding (To be documented)
 *
 * Implementation from: http://connectingstuff.net/blog/decodage-protocole-oregon-arduino-1/
 *
 ******************
 * INTERPRETATION *
 ******************
 * Nothing has been done at the moment.
 */
#define MIN_SHORT_LEN	200

#define MIN_LONG_LEN	700
#define MAX_LONG_LEN	1200

#define MIN_NUM_PULSES	150


static uint16_t decode_oregon_v2(uint32_t *pulseLens, uint16_t nbPulses);
	
decoderDesc_t decoder_OregonV2 = {
	.name			= "OregonV2",
	.minPulseLen  	= MIN_SHORT_LEN,
	.maxPulseLen  	= MAX_LONG_LEN,
	.minNumPulses 	= MIN_NUM_PULSES,
	.decoderFunc	= decode_oregon_v2
};



////////////////////////////////////////////////////////////////////////////////
//
// http://connectingstuff.net/blog/decodage-protocole-oregon-arduino-1/
//
////////////////////////////////////////////////////////////////////////////////

// Oregon V2 decoder modfied - Olivier Lebrun
// Oregon V2 decoder added - Dominique Pierre
// New code to decode OOK signals from weather sensors, etc.
// 2010-04-11 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id: ookDecoder.pde 5331 2010-04-17 10:45:17Z jcw $

static uint8_t total_bits, flip, state, pos;
static char data[32];
enum { UNKNOWN, T0, OK, DONE }; 
 

static uint8_t isDone ()
{
	return state == DONE;
}

static void resetDecoder ()
{
	total_bits = pos = flip = 0;
	memset(data, 0, 32);
	state = UNKNOWN;
}


// add one bit to the packet data buffer
static void gotBit (char value)
{
	if(!(total_bits & 0x01))
	{
		data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
	}
	total_bits++;
	pos = total_bits >> 4;
	if (pos >= sizeof data)
	{
		resetDecoder();
		return;
	}
	state = OK;
}

static void done ()
{
	state = DONE;
}

// store a bit using Manchester encoding
static void manchester (char value)
{
	flip ^= value; // manchester code, long pulse flips the bit
	gotBit(flip);
}

static int8_t decode(uint16_t width)
{
	if (200 <= width && width < 1200)
	{
		//Serial.println(width);
		uint8_t isLongPulse = width >= 700;

		switch (state)
		{
		case UNKNOWN:
			if (isLongPulse) {
				// Long pulse
				++flip;
			} else if (!isLongPulse && 24 <= flip) {
				// Short pulse, start bit
				flip = 0;
				state = T0;
			} else {
				// Reset decoder
				return -1;
			}
			break;
		case OK:
			if (!isLongPulse) {
				// Short pulse
				state = T0;
			} else {
				// Long pulse
				manchester(1);
			}
			break;
		case T0:
			if (!isLongPulse) {
			  // Second short pulse
				manchester(0);
			} else {
				// Reset decoder
				return -1;
			}
			break;
		  }
	}
	else if (width >= 2500  && pos >= 8)
	{
		return 1;
	}
	else
	{
		return -1;
	}
	return 0;
}

static uint8_t nextPulse (uint16_t width)
{
	if (state != DONE)
	{
		switch (decode(width))
		{
			case -1: resetDecoder(); break;
			case 1:  done(); break;
		}
	}
	return isDone();
}

////////////////////////////////////////////////////////////////////////////////

static uint16_t decode_oregon_v2(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	i, j;
	char		printBuffer[(2*32)+1];
	
	resetDecoder();
	
	for (i=0; i<nbPulses; i++)
	{
		if (nextPulse(pulseLens[i]))
		{
			for (j=0; j<32; j++)
			{
				sprintf(printBuffer + (2*i), "%02x", data[j]);
			}
			printBuffer[2*32] = '\0';
			PRINTF("%s,%d,%s\n", decoder_OregonV2.name, total_bits, printBuffer);
			return total_bits;
		}
	}
	
	return 0;
}
