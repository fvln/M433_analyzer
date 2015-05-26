#include <string.h>
#include <stdio.h>
#include "main.h"
#include "defines.h"
#include "tm_stm32f4_usart.h"
#include "tm_stm32f4_delay.h"

/*******************************************************************************
                                BIG FAT WARNING
            THIS MODULE HAS NOT BEEN TESTED EXTENSIVELY. USE WITH CARE
*******************************************************************************/

static char 				WifiRxBuffer[BUFFER_LEN];
static char					WifiTxBuffer[BUFFER_LEN];

static uint8_t				connectOk	= 0;


#define CHECK_OK_RESPONSE \
	if (checkResponse("OK\r\n")) { \
		Delayms(1000); \
	} else { \
		Delayms(1000); \
		return 0; \
	} \


//------------------------------------------------------------------------------
static uint16_t getLineFromUart(void)
{	
	uint16_t WifiRxBufLen = 0;
	memset(WifiRxBuffer, 0, BUFFER_LEN);
	
	// Read from UART
	WifiRxBufLen = TM_USART_Gets(ESP8266_UART, WifiRxBuffer, BUFFER_LEN);
	
	if (WifiRxBufLen) {
		DEBUG_PRINTF("< %s", WifiRxBuffer);
	}
	return WifiRxBufLen;
}

//------------------------------------------------------------------------------
static void sendLineToUart(void)
{	
	DEBUG_PRINTF("> %s", WifiTxBuffer);
	
	TM_USART_Puts(ESP8266_UART, WifiTxBuffer);
}

//------------------------------------------------------------------------------
static uint8_t checkResponse(const char *token)
{	
	getLineFromUart();
	return (strstr(WifiRxBuffer, token) != NULL);
}

#define checkOkResponse() checkResponse("OK\r\n")
#define checkFailedResponse() checkResponse("ERROR\r\n")

//------------------------------------------------------------------------------
static void flushRxBuffer(void)
{
	while (getLineFromUart() > 0);
}


//------------------------------------------------------------------------------
/*!
 * @brief Initialize the ESP8266 module
 * The module is reset and we check its boot is completed with the AT command
*/
uint8_t esp8266_init()
{	
	uint16_t i;
	
	
	DEBUG_PRINTF("esp8266_init starting\n");
	
	// Send bullshit data in order to force the module to reset its command interpreter
	// (for example, it may be waiting for data after a previous AT+CIPSEND command has been issued)
	for (i=0; i< (128); i++) {
		WifiTxBuffer[i] = 'A';
	}
	WifiTxBuffer[i++] = '\r';
	WifiTxBuffer[i++] = '\n';
	WifiTxBuffer[i] = '\0';
	sendLineToUart();
	
	
	// Flush the RX buffer for bogus response from ESP8266
	flushRxBuffer();
	
	// RESET the module
	snprintf(WifiTxBuffer, BUFFER_LEN, "AT+RST\r\n");
	sendLineToUart();
	while (getLineFromUart() > 0 && strstr(WifiRxBuffer, "ready\r\n") == NULL);
	
	// Disable echo
	snprintf(WifiTxBuffer, BUFFER_LEN, "ATE0\r\n");
	sendLineToUart();
	CHECK_OK_RESPONSE
	
	// Test the AT Command
	snprintf(WifiTxBuffer, BUFFER_LEN, "AT\r\n");
	sendLineToUart();
	CHECK_OK_RESPONSE
	
	// Init is complete
	return 1;
}

//------------------------------------------------------------------------------
/*!
 * @brief Open a (virtual) socket to the UDP syslog port defined by SYSLOG_IP and SYSLOG_PORT
 */
uint8_t esp8266_connect(void)
{	
	// Disable MUX
	snprintf(WifiTxBuffer, BUFFER_LEN, "AT+CIPMUX=0\r\n");
	sendLineToUart();
	CHECK_OK_RESPONSE
	
	// Establish a connection to the remote syslog host
	snprintf(WifiTxBuffer, BUFFER_LEN, "AT+CIPSTART=\"UDP\",\"%s\",%d\r\n", SYSLOG_IP, SYSLOG_PORT);
	sendLineToUart();
	if (checkOkResponse() || strstr(WifiRxBuffer, "ALREAY CONNECT\r\n") != NULL)
	{	
		connectOk = 1;
		Delayms(1000);
		return 1;
	}
		
	return 0;
}

//------------------------------------------------------------------------------
/*!
 * @brief Send the provided message on the socket previously opened
 */
uint8_t esp8266_syslog(char *message)
{
	uint16_t	MessageLen;
	
	
	MessageLen = strlen(message);
	if (connectOk == 0 || message == NULL || MessageLen == 0) {
		return 0;
	}
		
	// Initiate wifi tx
	snprintf(WifiTxBuffer, BUFFER_LEN, "AT+CIPSEND=%d\r\n", MessageLen);
	sendLineToUart();
	if (!getLineFromUart() > 0 || strncmp(WifiRxBuffer, "> ", BUFFER_LEN) != 0)
	{
		DEBUG_PRINTF("Failed: received '%s'\n", WifiRxBuffer);
		return 0;
	}
	
	Delayms(1000);
	
	// Send data
	strncpy(WifiTxBuffer, message, BUFFER_LEN);
	sendLineToUart();
	CHECK_OK_RESPONSE
		
	return 1;
}
