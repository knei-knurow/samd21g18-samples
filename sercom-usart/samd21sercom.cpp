#include "sam.h"
#include "samd21sercom.h"

void SAMD21SercomUsart::init(char sercomNum, char clkGen, SAMD21SercomUsartConfig_t *_config)
{
	config = _config;
	char sercomPmMask = 0;
	char sercomClk = 0;
	IRQn_Type sercom_irqn = (IRQn_Type)0;
	sercom = (Sercom *)0;
	switch(sercomNum)	//Select correct SERCOM
	{
		case 0:
			sercomPmMask = PM_APBCMASK_SERCOM0;
			sercomClk = GCLK_CLKCTRL_ID_SERCOM0_CORE;
			sercom_irqn = SERCOM0_IRQn;
			sercom = SERCOM0;
			break;
		case 1:
			sercomPmMask = PM_APBCMASK_SERCOM1;
			sercomClk = GCLK_CLKCTRL_ID_SERCOM1_CORE;
			sercom_irqn = SERCOM1_IRQn;
			sercom = SERCOM1;
			break;
		case 2:
			sercomPmMask = PM_APBCMASK_SERCOM2;
			sercomClk = GCLK_CLKCTRL_ID_SERCOM2_CORE;
			sercom_irqn = SERCOM2_IRQn;
			sercom = SERCOM2;
			break;
		case 3:
			sercomPmMask = PM_APBCMASK_SERCOM3;
			sercomClk = GCLK_CLKCTRL_ID_SERCOM3_CORE;
			sercom_irqn = SERCOM3_IRQn;
			sercom = SERCOM3;
			break;
		case 4:
			sercomPmMask = PM_APBCMASK_SERCOM4;
			sercomClk = GCLK_CLKCTRL_ID_SERCOM4_CORE;
			sercom_irqn = SERCOM4_IRQn;
			sercom = SERCOM4;
			break;
		case 5:
			sercomPmMask = PM_APBCMASK_SERCOM5;
			sercomClk = GCLK_CLKCTRL_ID_SERCOM5_CORE;
			sercom_irqn = SERCOM5_IRQn;
			sercom = SERCOM5;
			break;
	}
	PM->APBCMASK.reg			|=	sercomPmMask;											//Deliver clock to SERCOM
	GCLK->CLKCTRL.reg			=	sercomClk												//Clock for correct SERCOM
								|	(clkGen	<<	GCLK_CLKCTRL_GEN_Pos)						//Use correct GCLK Generator
								|	GCLK_CLKCTRL_CLKEN;										//Enable clock
	sercom->USART.CTRLA.reg		=	SERCOM_USART_CTRLA_MODE_USART_INT_CLK					//USART with internal clock
								|	(config->sampleRate	<<	SERCOM_USART_CTRLA_SAMPR_Pos)	//Given sample rate
								|	(config->txPos	<<	SERCOM_USART_CTRLA_TXPO_Pos)		//Given TX position
								|	(config->rxPo	<<	SERCOM_USART_CTRLA_RXPO_Pos)		//Given RX position
								|	(0	<<	SERCOM_USART_CTRLA_SAMPA_Pos)					//Middle sample adjustment
								|	(config->comMode	<<	SERCOM_USART_CTRLA_CMODE_Pos)	//Given communication mode
								|	(config->dataOrder	<<	SERCOM_USART_CTRLA_DORD_Pos);	//Given data order
	sercom->USART.CTRLB.reg		=	(config->txEn ? SERCOM_USART_CTRLB_TXEN : 0)			//Enable or disable TX
								|	(config->rxEn ? SERCOM_USART_CTRLB_RXEN : 0);			//Enable or disable RX
	sercom->USART.BAUD.reg		=	config->baud;											//Given baud rate
	sercom->USART.INTENSET.reg	=	(config->intRxComplete ? SERCOM_USART_INTENSET_RXC : 0);//Enable or disable receive complete interrupt
	sercom->USART.CTRLA.reg		|=	SERCOM_USART_CTRLA_ENABLE;								//Enable SERCOM

	if(config->intRxComplete) NVIC_EnableIRQ(sercom_irqn);									//Enable or not SERCOM interrupt
}

void SAMD21SercomUsart::send(char data)
{
	while(!(sercom->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_DRE));	//Wait for data buffer
	sercom->USART.DATA.reg	=	data;									//Send
}

void SAMD21SercomUsart::sendStr(char *str)
{
	while(*str != 0)	//If pointed char is not 0
	{
		send(*str);		//Send pointed char
		str++;			//Move pointer forward
	}
}

char SAMD21SercomUsart::available(void)
{
	if(sercom->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_RXC)	//If there are received data
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

char SAMD21SercomUsart::read(void)
{
	char data = 0;
	if(available())	//If data has been received
	{
		data = sercom->USART.DATA.reg;	//Read received data
	}
	return data;
}