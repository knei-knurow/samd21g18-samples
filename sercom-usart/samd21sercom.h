#ifndef SAMD21SERCOM_H
#define SAMD21SERCOM_H

#include "sam.h"

typedef struct SAMD21SercomUsartConfig
{
	char sampleRate;
	char txPos;
	char rxPo;
	char format;
	char comMode;
	char dataOrder;
	char charSize;
	char stopBits;
	char parityMode;
	char txEn;
	char rxEn;
	int baud;
	char intRxComplete;
} SAMD21SercomUsartConfig_t;

class SAMD21SercomUsart
{
	public:
		//Configuration of USART
		SAMD21SercomUsartConfig *config;

		//Initializing function
		//Note: this configures clock delivery, but user must configure I/O pins!
		void init(char sercomNum, char clkGen, SAMD21SercomUsartConfig_t *_config);

		//Send data
		void send(char data);

		//Send string
		//String must have 0 at end
		void sendStr(char *str);

		//This function checks is there are received data
		char available(void);

		//Read received data
		char read(void);
	private:
		Sercom *sercom;
};

#endif