#ifndef MAIN_H
#define MAIN_H

/**
  ******************************************************************************
  * @file    main.h 
  * @brief   Header for main.c module
  */

#include <stdio.h>
#include "stm32f4xx.h"
#include "defines.h"
#include "decoder.h"
#include "tm_stm32f4_disco.h"
#include "tm_stm32f4_usart.h"

/* Exported constants --------------------------------------------------------*/

// User-friendly LED names
#define LED_WIFI			LED_ORANGE
#define LED_HEARTBEAT 		LED_GREEN
#define LED_UART			LED_BLUE
#define LED_FAIL			LED_RED

//! Sentences shorter than this will not be processed (in µs)
#define MIN_SENTENCE_LEN	10000

//! Maximum number of pulses that can be recorded in a sentence
#define MAX_NUM_PULSES		1024

// Earn a joker every JOKER_PERIOD valid pulses
#define JOKER_PERIOD		48
// Set NUM_JOKERS initial jokers when a recording starts
#define NUM_JOKERS			2


/* Extern variables ------------------------------------------------------------*/

extern volatile uint32_t	sysTickTime;



//! Maximum string length
#define BUFFER_LEN	1024

//! Uart buffer and data length
extern char 		UartBuffer[BUFFER_LEN];
extern uint16_t		UartBufSz;


/* Exported macros ------------------------------------------------------------*/
// Define DEBUG_PRINTF() and PRINTF() depending on the user settings

#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

#ifdef DEBUG
	#define DEBUG_PRINTF(...) \
		UartBufSz = snprintf(UartBuffer, BUFFER_LEN, "# " __VA_ARGS__); \
		TM_DISCO_LedOn(LED_UART); \
		TM_USART_Puts(COMPUTER_UART, UartBuffer); \
		TM_DISCO_LedOff(LED_UART);
#else
	#define DEBUG_PRINTF(...)
#endif

#ifdef USE_ESP8266
	#define PRINTF(...) \
		UartBufSz = snprintf(UartBuffer, BUFFER_LEN, __VA_ARGS__); \
		TM_DISCO_LedOn(LED_WIFI); \
		esp8266_syslog(UartBuffer); \
		TM_DISCO_LedOff(LED_WIFI);
#else
	#define PRINTF(...) \
		UartBufSz = snprintf(UartBuffer, BUFFER_LEN, __VA_ARGS__); \
		TM_DISCO_LedOn(LED_UART); \
		TM_USART_Puts(COMPUTER_UART, UartBuffer); \
		TM_DISCO_LedOff(LED_UART);
#endif


/* Exported functions ------------------------------------------------------- */
void 	Error_Handler(void);

#endif // MAIN_H

