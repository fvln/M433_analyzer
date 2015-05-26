/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "decoder.h"
#include "esp8266.h"
#include "defines.h"
#include "main.h"

/* Include core modules */
#include "stm32f4xx.h"
#include "defines.h"
#include "tm_stm32f4_delay.h"
#include "tm_stm32f4_gpio.h"
#include "tm_stm32f4_disco.h"
#include "tm_stm32f4_exti.h"
#include "tm_stm32f4_usart.h"

/* External functions --------------------------------------------------------*/
void 		SystemClock_Config(void);
uint16_t 	decode_default(uint32_t *pulseLens, uint16_t nbPulses);


/* Global variables ----------------------------------------------------------*/
//! Decoder descriptions
static decoderDesc_t 	*decoders[16];

//! Number of registered decoders
static uint8_t			numDecoders = 0;

//! Global pulse filter
static decoderDesc_t	globalFilter;

/*!
 * Array of recorded pulse lens
 * @remark The first pulse is always a HIGH pulse
 */
static uint32_t		pulseLens[MAX_NUM_PULSES];

//! Number of pulses stored in pulseLens[]
static uint16_t 	numPulses;

//! Total length of the sentence stored in pulseLens
static uint32_t		sentenceLen;

//! UART transmission buffer
char 				UartBuffer[BUFFER_LEN];
uint16_t			UartBufSz;

//! Systick counter
volatile uint32_t	sysTickTime = 0;


/*----------------------------------------------------------------------------*/
/*!
 * @brief Register a decoder to the analyzer. Its min and max ranges are merged
 *        with the global filter
 */
int registerDecoder(decoderDesc_t *decoder)
{
	if (decoder == NULL || numDecoders >= MAX_DECODERS) {
		return 0;
	}
	
	decoders[numDecoders++] = decoder;
	if (decoder->minNumPulses < globalFilter.minNumPulses) {
		globalFilter.minNumPulses = decoder->minNumPulses;
	}
	if (decoder->minPulseLen < globalFilter.minPulseLen) {
		globalFilter.minPulseLen = decoder->minPulseLen;
	}
	if (decoder->maxPulseLen > globalFilter.maxPulseLen) {
		globalFilter.maxPulseLen = decoder->maxPulseLen;
	}
	
	DEBUG_PRINTF("* Registered decoder #%d: %s\n", numDecoders, decoder->name);	
	return 1;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Setup the interrupts triggered when the 433MHz receiver's data value changes
 * @remark The GPIO port and pin number must be defined in defines.h
 */
static void RadioInterrupt_Config(void)
{
	TM_GPIO_Init(RECEIVER_PORT, RECEIVER_PIN, TM_GPIO_Mode_IN, TM_GPIO_OType_OD, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_Medium);
	
	if (TM_EXTI_Attach(RECEIVER_PORT, RECEIVER_PIN, TM_EXTI_Trigger_Rising_Falling) != TM_EXTI_Result_Ok)
	{
		TM_DISCO_LedOn(LED_FAIL);
		while (1);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Run all the decoders on a sentence once the recording ends
 */
static void processSentence()
{
	uint8_t			i, result = 0;
	decoderDesc_t	*dec;
	
	
	// Disable the upcoming IT
	NVIC_DisableIRQ(EXTI0_IRQn);
	
	for (i=0; i<numDecoders; i++)
	{
		dec = decoders[i];
		if (numPulses > dec->minNumPulses) {
			result += dec->decoderFunc(pulseLens, numPulses);
		}
	}
	
	if (result == 0)
	{
		// No decoder matched the sentence - call the default decoder
		decode_default(pulseLens, numPulses);
	}
		
	// Enable the interrupts again
	NVIC_EnableIRQ(EXTI0_IRQn);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Callback function run every time the value of the receiver GPIO changes
 * 
 * Check if this pulse has a suitable length and append it to the recorded
 * sentence. If the sentence appears to be over, hand it to the registered decoders.
 *
 * @param GPIO_Pin GPIO whose value just changed
 */

void TM_EXTI_Handler(uint16_t GPIO_Pin)
{
	// Date of the previous interrupt
	static uint32_t	lastTime = 0;
	static uint8_t	jokers;

	uint32_t 	pulseLen, pinValue;
	uint8_t		validPulse = 0;		
		
	if (GPIO_Pin != RECEIVER_PIN) {
		return;
	}
	
	TM_DISCO_LedToggle(LED_WIFI);
	
	// Compute pulse len and save current date for the next interrupt
	pulseLen = 10 * (sysTickTime - lastTime);
	lastTime = sysTickTime;
	
	pinValue = TM_GPIO_GetInputPinValue(RECEIVER_PORT, RECEIVER_PIN);
	
	
	if (numPulses > 0)
	{
		// We had already started recording -> validate pulse len or use a joker
		if ((pulseLen < globalFilter.maxPulseLen && pulseLen > globalFilter.minPulseLen) ||
			jokers-- > 0)
		{
			pulseLens[numPulses++] = (uint16_t)pulseLen;
			sentenceLen += pulseLen;
			validPulse = 1;
			if (numPulses % JOKER_PERIOD == 0) {
				// Enough pulses have been recorded, we earned a joker!
				jokers += NUM_JOKERS;
			}
		}
	}
	else if (pinValue == RESET && pulseLen < globalFilter.maxPulseLen && pulseLen > globalFilter.minPulseLen)
	{
		// we just received a suitable HIGH pulse -> start recording
		jokers 		= NUM_JOKERS;
		sentenceLen = 0;
		
		pulseLens[numPulses++] = (uint16_t)pulseLen;
		sentenceLen += pulseLen;

		validPulse = 1;
	}
	
	if (!validPulse || (numPulses == MAX_NUM_PULSES))
	{
		// Recording may stop if an invalid pulse is received
		// or if the record buffer is full
		if (sentenceLen > MIN_SENTENCE_LEN || numPulses > globalFilter.minNumPulses)
		{
			processSentence();
		}
		
		// Reset the pulse counter
		numPulses = 0;
	}
}


/*----------------------------------------------------------------------------*/
/* MAIN ----------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

int main(void)
{	
	/* Initialize system */
	SystemInit();
	
	/* Set Systick interrupt every 10µs */
	if (SysTick_Config(SystemCoreClock / 100000)) {
		/* Capture error */
		while (1);
	}
	
	/* Initialize delay */
	TM_DELAY_Init();
		
	/* Initialize leds on board */
	TM_DISCO_LedInit();
	
	/* Initialize the computer USART */
	TM_USART_Init(COMPUTER_UART, COMPUTER_UART_PINSPACK, COMPUTER_UART_BAUDRATE);
	
	DEBUG_PRINTF("* Decoder starting!\n\n");	
	
	
#ifdef USE_ESP8266
	/* Initialize the wifi UART, connected to the ESP8266 module */
	TM_USART_Init(ESP8266_UART, ESP8266_UART_PINSPACK, ESP8266_UART_BAUDRATE);
  	
	// Initialize the ESP8266 module
	if (!esp8266_init() || !esp8266_connect())
	{
		DEBUG_PRINTF("Unable to connect ESP8266 module\n");
		Error_Handler();
	}
#endif
	
	// Initialize the global pulse filter
	globalFilter.minNumPulses = 0xFFFF;
	globalFilter.minPulseLen = 0xFFFFFFFF;
	globalFilter.maxPulseLen = 0;
	
	// Register the available decoders
	// Sensors
	REGISTER_OREGON_EW91;
	REGISTER_OREGON_V2;
	
	// Home automation
	REGISTER_RCSWITCH;
	REGISTER_HOME_EASY;
	
	// Garage doors
	REGISTER_CAME_432NA;
	REGISTER_DIP_SWITCH;
	
	// Car key fobs
	REGISTER_CARKEY_1;
	REGISTER_SIEMENS_VDO;
	
	// Initialize the record variables
	numPulses = sentenceLen = 0;
	
	// Initialize the 433MHz receiver interrupts
	RadioInterrupt_Config();
	
	// GO!
	DEBUG_PRINTF("Main globalFilter: minNumPulses=%d, minPulseLen=%d, maxPulseLen=%d\n", globalFilter.minNumPulses, globalFilter.minPulseLen, globalFilter.maxPulseLen);
	
	/* Infinite loop */
	while (1)
	{
		Delayms(500);
		TM_DISCO_LedToggle(LED_HEARTBEAT);
	}
}

/*----------------------------------------------------------------------------*/
/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{	
	TM_DISCO_LedOn(LED_FAIL); 
	
	/* Infinite loop */
	while (1);
}
