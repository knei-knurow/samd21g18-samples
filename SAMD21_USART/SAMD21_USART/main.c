/*
 * SAMD21_USART.c
 *
 * Created: 02.01.2021 08:18:27
 * Author : USER
 */ 

/*
LED: PA17 (INV)

Generic Clocks:
GEN1: OSC8M (8MHz)
GEN2: OSC32K (32.768kHz)

SERCOM4:
USART
9600bps
TX:PAD0
RX:PAD1
*/

#include "sam.h"


#define USART_SENDING_FIFO_SIZE 16
char usart_sending_fifo[USART_SENDING_FIFO_SIZE];
char* usart_sending_fifo_w_ptr;
char* usart_sending_fifo_r_ptr;

void usart_sending_fifo_send()	//Wyœlij bajt z kolejki FIFO
{
	if(usart_sending_fifo_r_ptr != usart_sending_fifo_w_ptr)	//Je¿eli dodano coœ nowego do FIFO
	{
		usart_sending_fifo_r_ptr++;	//Zwiêksz wskaŸnik odczytu
		if((usart_sending_fifo_r_ptr - usart_sending_fifo) >= USART_SENDING_FIFO_SIZE) usart_sending_fifo_r_ptr = usart_sending_fifo; //Je¿eli wskaŸnik odczytu doszed³ do koñca, to daj go na pocz¹tek
		SERCOM4->USART.DATA.reg = *usart_sending_fifo_r_ptr;	//Wyœlij
	}
}
void usart_sending_fifo_add(char data)	//Dodaj do FIFO
{
	usart_sending_fifo_w_ptr++;	//Zwiêksz wskaŸnik zapisu
	if((usart_sending_fifo_w_ptr - usart_sending_fifo) >= USART_SENDING_FIFO_SIZE) usart_sending_fifo_w_ptr = usart_sending_fifo;	//Je¿eli wskaŸnik zapisu doszed³ do koñca, to daj go na pocz¹tek
	*usart_sending_fifo_w_ptr = data;	//Zapisz do FIFO
}
void usart_sending_fifo_add_str(char *str)	//Dodaj ci¹g znaków (zakoñczony zerem) do FIFO i rozpocznij wysy³anie
{
	usart_sending_fifo_add(*str);	//Dodaj pierwszy bajt
	if(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_DRE)	//Je¿eli ju¿ jest gotowy do wysy³ania
	{
		usart_sending_fifo_send();	//Wyœlij
	}
	str++;	//Zwiêksz wskaŸnik
	while(*str != 0)	//A¿ do zera
	{
		usart_sending_fifo_add(*str);	//Dodawaj kolejne bajty
		str++;	//Zwiêksz wskaŸnik
	}
}


int main(void)
{
	SystemInit();
	
	NVIC_EnableIRQ(TC3_IRQn);		//Enable interrupts from TC3
	NVIC_EnableIRQ(SERCOM4_IRQn);	//Enable interrupts from SERCOM4
	
	//GCLK Generators
	GCLK->GENCTRL.reg		=	(1	<<	GCLK_GENCTRL_ID_Pos)	//Generator 1
							|	GCLK_GENCTRL_SRC_OSC8M			//OSC8M is source
							|	GCLK_GENCTRL_GENEN;				//Enable generator
	GCLK->GENCTRL.reg		=	(2	<<	GCLK_GENCTRL_ID_Pos)	//Generator 2
							|	GCLK_GENCTRL_SRC_OSC32K			//OSC32K is source
							|	GCLK_GENCTRL_GENEN;				//Enable generator
	
	//OSC8M
	SYSCTRL->OSC8M.reg		=	SYSCTRL_OSC8M_ENABLE	//Enable OSC8M (8MHz)
							|	SYSCTRL_OSC8M_ONDEMAND	//OSC8M will run on demand
							|	SYSCTRL_OSC8M_PRESC_0	//Without prescaler
							|	(SYSCTRL->OSC8M.reg	&	SYSCTRL_OSC8M_CALIB_Msk)	//Keep default calibration
							|	SYSCTRL_OSC8M_FRANGE_2;	//Frequency range 8-11MHz
	
	//OSC32K
	long long calib			= *((long long *)0x806020);
	long long osc32k_calib	= (calib >> 38) & 0b1111111;						//Read factory calibration from Flash memory
	SYSCTRL->OSC32K.reg		=	SYSCTRL_OSC32K_ENABLE							//Enable OSC32K (32kHz)
							|	SYSCTRL_OSC32K_EN32K							//Output frequency is 32kHz
							|	(osc32k_calib	<<	SYSCTRL_OSC32K_CALIB_Pos);	//Use factory calibration
	
	//LED
	PORT->Group[0].DIRSET.reg		=	(1	<<	17);		//Output
	PORT->Group[0].OUTCLR.reg		=	(1	<<	17);		//Low
	PORT->Group[0].PINCFG[17].reg	=	PORT_PINCFG_DRVSTR;	//Stronger output
	
	//TC3
	PM->APBCMASK.reg		|=	PM_APBCMASK_TC3;			//Deliver clock to timer
	GCLK->CLKCTRL.reg		=	GCLK_CLKCTRL_ID_TCC2_TC3	//Clock for TCC2 and TC3
							|	GCLK_CLKCTRL_GEN_GCLK2		//Use GCLK Generator 2
							|	GCLK_CLKCTRL_CLKEN;			//Enable this clock
	TC3->COUNT8.CTRLA.reg	=	TC_CTRLA_MODE_COUNT8		//Use 8bit mode
							|	TC_CTRLA_PRESCALER_DIV256;	//Prescaler 256
	TC3->COUNT8.INTENSET.reg=	TC_INTENSET_OVF;			//Turn on overflow interrupt
	TC3->COUNT8.PER.reg		=	128;						//Period 128
	//TC3->COUNT8.PER.reg	=	2;
	TC3->COUNT8.CTRLA.reg	|=	TC_CTRLA_ENABLE;			//Enable timer
	
	//SERCOM4
	PM->APBCMASK.reg			|=	PM_APBCMASK_SERCOM4;					//Deliver clock to SERCOM
	GCLK->CLKCTRL.reg			=	GCLK_CLKCTRL_ID_SERCOM4_CORE			//Clock for SERCOM4
								|	GCLK_CLKCTRL_GEN_GCLK1					//Use GCLK Generator 1
								|	GCLK_CLKCTRL_CLKEN;						//Enable this clock
	SERCOM4->USART.CTRLA.reg	=	SERCOM_USART_CTRLA_MODE_USART_INT_CLK	//Mode: USART with internal clock
								|	(2	<<	SERCOM_USART_CTRLA_SAMPR_Pos)	//8x oversampling using arithmetic baud rate generation
								|	(0	<<	SERCOM_USART_CTRLA_TXPO_Pos)	//TX is at PAD0
								|	(1	<<	SERCOM_USART_CTRLA_RXPO_Pos)	//RX is at PAD1
								|	(0	<<	SERCOM_USART_CTRLA_SAMPA_Pos)	//Use middle three samples
								|	(0	<<	SERCOM_USART_CTRLA_CMODE_Pos)	//Asynchronous
								|	(1	<<	SERCOM_USART_CTRLA_DORD_Pos);	//LSB is transmitted first
	SERCOM4->USART.CTRLB.reg	=	SERCOM_USART_CTRLB_TXEN					//Enable transmitter
								|	SERCOM_USART_CTRLB_RXEN;				//Enable receiver
	SERCOM4->USART.BAUD.reg		=	64907;									//Baud rate 9600bps
	SERCOM4->USART.INTENSET.reg	=	SERCOM_USART_INTENSET_RXC				//Enable receive complete interrupt
								|	SERCOM_USART_INTENSET_TXC;				//Enable empty data buffer interrupt
	SERCOM4->USART.CTRLA.reg	|=	SERCOM_USART_CTRLA_ENABLE;				//Enable SERCOM
	PORT->Group[1].DIRSET.reg	=	(1	<<	8);								//Set TX pin as output
	PORT->Group[1].OUTSET.reg	=	(1	<<	9);								//Pull-up on RX
	PORT->Group[1].PMUX[4].reg	|=	PORT_PMUX_PMUXO_D						//SERCOM4PAD1
								|	PORT_PMUX_PMUXE_D;						//SERCOM4PAD0
	PORT->Group[1].PINCFG[8].reg=	PORT_PINCFG_PMUXEN;						//Enable peripheral multiplexing on TX pin
	PORT->Group[1].PINCFG[9].reg=	PORT_PINCFG_PMUXEN						//Enable peripheral multiplexing on RX pin
								|	PORT_PINCFG_INEN						//Set RX as input
								|	PORT_PINCFG_PULLEN;						//Turn on pull resistor
	
	//USART sending FIFO
	for(int i = 0 ; i < USART_SENDING_FIFO_SIZE ; i++)
	{
		usart_sending_fifo[i] = 0;
	}
	usart_sending_fifo_w_ptr = usart_sending_fifo;
	usart_sending_fifo_r_ptr = usart_sending_fifo;
	
	while (1)
	{
		for(int i = 0 ; i < 60000 ; i++)
		{
			for(int j = 0 ; j < 100 ; j++)
			{
				asm("nop;");
			}
		}
		usart_sending_fifo_add_str((char *)(char [5]){5,7,6,8,0});
	}
}

void TC3_Handler()
{
	if(TC3->COUNT8.INTFLAG.reg	&	TC_INTFLAG_OVF)							//If this is overflow interrupt
	{
		TC3->COUNT8.INTFLAG.reg		=	TC_INTFLAG_OVF;						//Clear interrupt flag
		PORT->Group[0].OUTTGL.reg	=	(1	<<	17);						//Toggle LED
		//while(!(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_DRE));	//Wait for USART data buffer
		//SERCOM4->USART.DATA.reg	=	123;									//Send
		usart_sending_fifo_add(123);
		if(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_DRE)
		{
			usart_sending_fifo_send();
		}
	}
}

void SERCOM4_Handler()
{
	if(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_RXC)			//If this is receive complete interrupt
	{
		SERCOM4->USART.INTFLAG.reg	=	SERCOM_USART_INTFLAG_RXC;			//Clear interrput flag
		char data = SERCOM4->USART.DATA.reg;								//Read received data
		usart_sending_fifo_add(73);
		if(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_DRE)
		{
			usart_sending_fifo_send();
		}
		for(char i = 0 ; i < 10 ; i++)
		{
			usart_sending_fifo_add(i);
		}
		usart_sending_fifo_add(data);
		//while(!(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_DRE));	//Wait for USART data buffer
		//SERCOM4->USART.DATA.reg	=	73;										//Send
		//while(!(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_DRE));	//Wait for USART data buffer
		//SERCOM4->USART.DATA.reg	=	data;									//Send
	}
	if(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_TXC)			//If this is transmit complete interrupt
	{
		SERCOM4->USART.INTFLAG.reg	=	SERCOM_USART_INTFLAG_TXC;
	}
	if(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_DRE)
	{
		usart_sending_fifo_send();
	}
}