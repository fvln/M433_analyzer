#include "main.h"
#include "generic_rcswitch.h"


#define MIN_SYNC_LEN	1000
#define MAX_SYNC_LEN	100000

#define PULSE_TOLERANCE	0.15	// Pulse len tolerance

#undef 	IS_PAIR
#define	IS_PAIR(n)		((n) > minPairLen && (n) < maxPairLen)

#define SIMILAR(v,ref)	(((double)v / (double)ref) > (1-PULSE_TOLERANCE) && ((double)v / (double)ref) < (1+PULSE_TOLERANCE))

#define MIN_NUM_PAIRS	10


static uint32_t rawData;
static uint8_t 	nbBits;


static uint8_t interpret_default(uint32_t r, uint8_t b)
{
	if (b > MIN_NUM_PAIRS) {
		rawData = r;
		nbBits = b;
		return 1;
	}
	return 0;
}


uint16_t check_manchester_sentence(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	i;
	uint32_t	ReferenceLen[2] 	= {0, 0};
		
	
	for (i = 0; i < nbPulses; i++)
	{
		if (ReferenceLen[0] == 0)
		{
			ReferenceLen[0] = pulseLens[i];
		}
		else if (SIMILAR(pulseLens[i], ReferenceLen[0]))
		{
			continue;
		}
		else if (ReferenceLen[1] == 0 && (SIMILAR(2*pulseLens[i], ReferenceLen[0]) || SIMILAR(pulseLens[i]/2, ReferenceLen[0])))
		{
			ReferenceLen[1] = pulseLens[i];
		}
		else if (SIMILAR(pulseLens[i], ReferenceLen[1]))
		{
			continue;
		}
		else
		{
			// Invalid length
			break;
		}
	}
	
	if (i >= (2 * MIN_NUM_PAIRS))
	{
		PRINTF("DefaultManchester,NbPairs=%d,length0=%d,length1=%d\n", i/2, ReferenceLen[0], ReferenceLen[1]);
		// Make sure the result is even
		return 2 * (i/2);
	}
	
	return 0;
}


uint16_t check_samePulse_sentence(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	i;
	uint32_t	ReferenceLen[2];
	uint8_t		sameHigh = 1,
				sameLow = 1;
	
	
	ReferenceLen[0] = pulseLens[0];
	ReferenceLen[1] = pulseLens[1];
	
	
	for (i = 2; i < nbPulses - 1; i += 2)
	{
		if (sameHigh && !SIMILAR(pulseLens[i], ReferenceLen[0]))
		{
			if (!sameLow) {
				break;
			}
			sameHigh = 0;
		}
		if (sameLow && !SIMILAR(pulseLens[i+1], ReferenceLen[1]))
		{
			if (!sameHigh) {
				break;
			}
			sameLow = 0;
		}
		
		if (!sameHigh && !sameLow)
		{
			break;
		}
	}
	
	if (i >= (2 * MIN_NUM_PAIRS))
	{
		PRINTF("DefaultSamePulse,NbPairs=%d,SameHigh=%d,HighLen=%d,SameLow=%d,LowLen=%d\n", i/2, sameHigh, ReferenceLen[0], sameLow, ReferenceLen[1]);
		// Make sure the result is even
		return i;
	}
	
	return 0;
}


uint16_t decode_default(uint32_t *pulseLens, uint16_t nbPulses)
{
	uint16_t	usedPulses = 0;
	uint16_t	syncOffset = 0;
	uint16_t	i = 0;
	
	while (nbPulses - syncOffset > (2*MIN_NUM_PAIRS))
	{	
		// Try and decode the sentence by checking all the pairs have the same duration
		
		
		if ((usedPulses = decode_generic_rcswitch_sentence(
				pulseLens + syncOffset,		// uint32_t *pulseLens
				nbPulses - syncOffset, 		// uint16_t nbPulses
				0,							// pairLen
				interpret_default,			// DataHandler
				0							// Revert bits
			 )) >= 2*MIN_NUM_PAIRS)
		{
			
			PRINTF("DefaultRCS,Prologue=%d+%d,Epilogue=%d+%d,PairLen=%d,Length=%d,Data=0x%08x\nRaw,",
				(syncOffset > 2 ? pulseLens[syncOffset-2] : 0),
				(syncOffset > 1 ? pulseLens[syncOffset-1] : 0),
				(syncOffset + usedPulses + 1 < nbPulses ? pulseLens[syncOffset+usedPulses+1] : 0),
				(syncOffset + usedPulses + 2 < nbPulses ? pulseLens[syncOffset+usedPulses+2] : 0),
				pulseLens[syncOffset] + pulseLens[syncOffset+1],
				nbBits,
				rawData
			);
			for (i=0; i<nbPulses; i++) {
				PRINTF("%d,", pulseLens[i]);
			}
			PRINTF("0\n");
			syncOffset += usedPulses;
		}
		/*
		else if ((usedPulses = check_manchester_sentence(
				pulseLens + syncOffset,		// uint32_t *pulseLens
				nbPulses - syncOffset 		// uint16_t nbPulses
			 )) >= 2*MIN_NUM_PAIRS)
		{
			syncOffset += usedPulses;
		}
		else if ((usedPulses = check_samePulse_sentence(
				pulseLens + syncOffset,		// uint32_t *pulseLens
				nbPulses - syncOffset 		// uint16_t nbPulses
			 )) >= 2*MIN_NUM_PAIRS)
		{
			syncOffset += usedPulses;
		}
		*/
		else
		{
			syncOffset += 2;
		}
	}
	
	return 1;
}
