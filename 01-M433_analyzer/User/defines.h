#ifndef DEFINES_H
#define DEFINES_H

/*******************************************************************************
 * Hardware and wiring
 ******************************************************************************/
 
//! Port and PIN where the 433MHz receiver is connected
#define RECEIVER_PORT			GPIOB
#define RECEIVER_PIN			GPIO_PIN_0
#define RECEIVER_CLK_ENABLE		__GPIOB_CLK_ENABLE

/*! 
 * Define to use the ESP8266 module (otherwise, the commputer UART will be
 * used to print out the results). THIS MODULE HAS NOT BEEN TESTED EXTENSIVELY. USE WITH CARE
 */
#undef USE_ESP8266

//! Port and TX PIN where the computer UART and the ESP8266 UART are connected
/*
 * DEFAULT VALUES:
 *
 * -Computer UART: TX=PA2 --> to be connected to the RX pin of your UART module
 * -Computer UART: RX is not used (i.e. you cannot send character from your computer)
 *
 * -ESP8266 UART: TX=PB10 --> to be connected to the RX pin of the ESP8266 module
 * -ESP8266 UART: RX=PB11 --> to be connected to the TX pin of the ESP8266 module
 */
#define COMPUTER_UART			USART2
#define COMPUTER_UART_PINSPACK	TM_USART_PinsPack_1
#define COMPUTER_UART_BAUDRATE	57600

#define ESP8266_UART			USART3
#define ESP8266_UART_PINSPACK	TM_USART_PinsPack_1
#define ESP8266_UART_BAUDRATE	9600

//! When using the ESP8266 module, output is sent as syslog messages to this UDP IP+Port
#define	SYSLOG_IP	"192.168.1.30"
#define SYSLOG_PORT	32000

//! Enable debug messages with DEBUG_PRINTF (will be prefixed with a #)
#define DEBUG 1


/*******************************************************************************
 * Decoders to enable
 ******************************************************************************/

//! Decoders to enable
#define USE_OREGON_EW91		1
#define USE_OREGON_V2		1
#define USE_RCSWITCH		1
#define USE_HOME_EASY		1
#define USE_CAME_432NA		1
#define USE_DIP_SWITCH		1
#define USE_CARKEY_1		1
#undef USE_SIEMENS_VDO		


/*******************************************************************************
 * Recorder settings
 ******************************************************************************/
 
//! Sentences shorter than this will not be processed (in µs)
#define MIN_SENTENCE_LEN	10000

//! Maximum number of pulses that can be recorded in a sentence
#define MAX_NUM_PULSES		1024

/*
 * Jokers allow the analyzer to record a pulse even though it is
 * too short or too long. When a recording starts, NUM_JOKERS are available.
 * This number is decreased every time a pulse with an invalid pulse is
 * recorded, and increased every JOKER_PERIOD recorded pulses.
 */

// Set NUM_JOKERS initial jokers when a recording starts
#define NUM_JOKERS			2

// Earn a joker every JOKER_PERIOD valid pulses
#define JOKER_PERIOD		48


/*******************************************************************************
 * Internal settings of the TM libraries
 ******************************************************************************/

/* TIM4 is used for delay functions */
#define TM_DELAY_TIM				TIM4
#define TM_DELAY_TIM_IRQ			TIM4_IRQn
#define TM_DELAY_TIM_IRQ_HANDLER	TIM4_IRQHandler


#endif // DEFINES_H
