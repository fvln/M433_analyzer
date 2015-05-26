#ifndef DECODER_H
#define DECODER_H

#include <string.h>
#include <stm32f4xx.h>

// This file defines which decoders are to be enabled
#include "defines.h"

/*
 * These macros are provided here for convenience and can be used by any decoder
 * Length-related macros:
 */
#define IS_SYNC(n) 			((n) > MIN_SYNC_LEN && (n) < MAX_SYNC_LEN)
#define IS_PAIR(n)			((n) > MIN_PAIR_LEN && (n) < MAX_PAIR_LEN)

#define IS_SHORT(n) 		((n) > MIN_SHORT_LEN && (n) < MAX_SHORT_LEN)
#define IS_LONG(n) 			((n) > MIN_LONG_LEN && (n) < MAX_LONG_LEN)

#define IS_VALID_PULSE(n)	((n) > MIN_SHORT_LEN && (n) < MAX_LONG_LEN)

#define IS_HIGH_PULSE(n)	((n) > MIN_HIGH_LEN && (n) < MAX_HIGH_LEN)
#define IS_LONG_LOW(n)		((n) > MIN_LONG_LOW_LEN)

/*
 * Data decoding macros:
 * This macro applies a mask on an integer value and shifts the useful bits
 * so that thay are aligned on the LSB
 */
#define BIN_VALUE(x) 		((rawData & (x ## _MASK)) >> (x ## _SHIFT))

/*
 * This macro extracts a value the same way, on a byte array (therefore, the
 * extracted value cannot be longer than 8 bits).
 */
#define BIN_VALUE_MB(x) 	((rawData[x ## _BYTE] & (x ## _MASK)) >> (x ## _SHIFT))

//! Maximum length of the decoder "friendly name"
#define DEC_MAX_NAME_LEN	16

//! Maximum count of decoders handled by the analyzer
#define	MAX_DECODERS		16

//! Recurrent function prototypes
typedef uint16_t (*decoderFunc_t)(uint32_t *pulseLens, uint16_t nbPulses);
typedef uint8_t (*ui32InterpreterFunc_t)(uint32_t rawData, uint8_t nbBits);

//! Decoder description structure
typedef struct {
	uint8_t			name[DEC_MAX_NAME_LEN]; //! Decoder name
	uint32_t		minPulseLen;            // Min length of the pulses
	uint32_t		maxPulseLen;            // Max length of the pulses
	uint16_t		minNumPulses;           // Min pulse count to have a valid sentence
	decoderFunc_t	decoderFunc;            // Function called to decode a sentence
} decoderDesc_t;


// Known decoders
#ifdef USE_OREGON_EW91
	extern decoderDesc_t decoder_OregonEW91;
	#define REGISTER_OREGON_EW91	registerDecoder(&decoder_OregonEW91);
#else
	#define REGISTER_OREGON_EW91
#endif

#ifdef USE_OREGON_V2
	extern decoderDesc_t decoder_OregonV2;
	#define REGISTER_OREGON_V2	registerDecoder(&decoder_OregonV2);
#else
	#define REGISTER_OREGON_V2
#endif

#ifdef USE_RCSWITCH
	extern decoderDesc_t decoder_RCSwitch;
	#define REGISTER_RCSWITCH	registerDecoder(&decoder_RCSwitch);
#else
	#define REGISTER_RCSWITCH
#endif

#ifdef USE_HOME_EASY
	extern decoderDesc_t decoder_HomeEasy;
	#define REGISTER_HOME_EASY	registerDecoder(&decoder_HomeEasy);
#else
	#define REGISTER_HOME_EASY
#endif

#ifdef USE_CAME_432NA
	extern decoderDesc_t decoder_Came432Na;
	#define REGISTER_CAME_432NA	registerDecoder(&decoder_Came432Na);
#else
	#define REGISTER_CAME_432NA
#endif

#ifdef USE_CARKEY_1
	extern decoderDesc_t decoder_CarKey1;
	#define REGISTER_CARKEY_1	registerDecoder(&decoder_CarKey1);
#else
	#define REGISTER_CARKEY_1
#endif

#ifdef USE_DIP_SWITCH
	extern decoderDesc_t decoder_dipSwitch;
	#define REGISTER_DIP_SWITCH	registerDecoder(&decoder_dipSwitch);
#else
	#define REGISTER_DIP_SWITCH
#endif

#ifdef USE_SIEMENS_VDO
	extern decoderDesc_t decoder_siemensVdo;
	#define REGISTER_SIEMENS_VDO	registerDecoder(&decoder_siemensVdo);
#else
	#define REGISTER_SIEMENS_VDO
#endif


#endif // DECODER_H
