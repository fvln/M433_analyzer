#ifndef ESP8266_H
#define ESP8266_H

#include "main.h"

uint8_t esp8266_init(void);
uint8_t esp8266_connect(void);
uint8_t esp8266_syslog(char *message);




#endif // ESP8266_H
