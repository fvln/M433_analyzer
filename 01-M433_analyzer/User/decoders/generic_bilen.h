#ifndef GENERIC_BILEN_H
#define GENERIC_BILEN_H

#include "main.h"


uint16_t decode_generic_b_sentence(uint32_t *pulseLens, uint16_t nbPulses, uint32_t pairLen, ui32InterpreterFunc_t dataHandler, uint32_t revert);


#endif // GENERIC_RCSWITCH_H