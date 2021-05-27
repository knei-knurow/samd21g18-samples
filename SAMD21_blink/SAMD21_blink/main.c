/*
 * SAMD21_blink.c
 *
 * Created: 19.12.2020 08:53:43
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


int main(void)
{
	SystemInit();
	
	NVIC_EnableIRQ(TC3_IRQn);		//Enable interrupts from TC3
	//NVIC_EnableIRQ(SERCOM4_IRQn);
	
	//GCLK Generators
	GCLK->GENCTRL.reg		=	(1	<<	GCLK_GENCTRL_ID_Pos)	//GCLK Generator 1
							|	GCLK_GENCTRL_SRC_OSC8M			//Source is OSC8M (8MHz)
							|	GCLK_GENCTRL_GENEN;				//Enable generator
	GCLK->GENCTRL.reg		=	(2	<<	GCLK_GENCTRL_ID_Pos)	//GCLK Generator 2
							|	GCLK_GENCTRL_SRC_OSC32K			//Source is OSC32K (32kHz)
							|	GCLK_GENCTRL_GENEN;				//Enable generator
	
	//OSC8M
	SYSCTRL->OSC8M.reg		=	SYSCTRL_OSC8M_ENABLE		//Enable OSC8M
							|	SYSCTRL_OSC8M_ONDEMAND		//OSC8M will run on demand
							|	SYSCTRL_OSC8M_PRESC_0		//Without prescaler
							|	(SYSCTRL->OSC8M.reg	&	SYSCTRL_OSC8M_CALIB_Msk)	//Keep default calibration
							|	SYSCTRL_OSC8M_FRANGE_2;		//Frequency range 8-11MHz
	
	//OSC32K
	long long calib			= *((long long *)0x806020);
	long long osc32k_calib	= (calib >> 38) & 0b1111111;	//Load factory calibration from Flash
	SYSCTRL->OSC32K.reg		=	SYSCTRL_OSC32K_ENABLE		//Enable OSC32K (32kHz)
							|	SYSCTRL_OSC32K_EN32K		//Output frequency 32kHz
							|	(osc32k_calib	<<	SYSCTRL_OSC32K_CALIB_Pos);	//Use factory calibration
	
	//LED
    PORT->Group[0].DIRSET.reg		=	(1	<<	17);		//Output
	PORT->Group[0].OUTSET.reg		=	(1	<<	17);		//High
	PORT->Group[0].PINCFG[17].reg	=	PORT_PINCFG_DRVSTR;	//Stronger output
	
	//TC3
	PM->APBCMASK.reg		|=	PM_APBCMASK_TC3;			//Deliver clock to timer
	GCLK->CLKCTRL.reg		=	GCLK_CLKCTRL_ID_TCC2_TC3	//Clock for TCC2 and TC3
							|	GCLK_CLKCTRL_GEN_GCLK2		//GCLK Generator 2
							|	GCLK_CLKCTRL_CLKEN;			//Enable clock
	TC3->COUNT8.CTRLA.reg	=	TC_CTRLA_MODE_COUNT8		//8bit mode
							|	TC_CTRLA_PRESCALER_DIV256;	//Prescaler 256
	TC3->COUNT8.INTENSET.reg=	TC_INTENSET_OVF;			//Turn on overflow interrupt
	TC3->COUNT8.PER.reg		=	128;						//Peroid 128
	//TC3->COUNT8.PER.reg	=	2;
	TC3->COUNT8.CTRLA.reg	|=	TC_CTRLA_ENABLE;			//Enable timer
	
	////SERCOM4
	//PM->APBCMASK.reg			|=	PM_APBCMASK_SERCOM4;
	//GCLK->CLKCTRL.reg			=	GCLK_CLKCTRL_ID_SERCOM4_CORE	|	GCLK_CLKCTRL_GEN_GCLK1	|	GCLK_CLKCTRL_CLKEN;
	//SERCOM4->USART.CTRLA.reg	=	SERCOM_USART_CTRLA_MODE_USART_INT_CLK	|	(2	<<	SERCOM_USART_CTRLA_SAMPR_Pos)	|	(0	<<	SERCOM_USART_CTRLA_TXPO_Pos)	|	(1	<<	SERCOM_USART_CTRLA_RXPO_Pos)	|	(0	<<	SERCOM_USART_CTRLA_SAMPA_Pos)	|	(0	<<	SERCOM_USART_CTRLA_CMODE_Pos)	|	(1	<<	SERCOM_USART_CTRLA_DORD_Pos);
	//SERCOM4->USART.CTRLB.reg	=	SERCOM_USART_CTRLB_TXEN	|	SERCOM_USART_CTRLB_RXEN;
	//SERCOM4->USART.BAUD.reg		=	64907;
	//SERCOM4->USART.INTENSET.reg	=	SERCOM_USART_INTENSET_RXC;
	//SERCOM4->USART.CTRLA.reg	|=	SERCOM_USART_CTRLA_ENABLE;
	//PORT->Group[1].DIRSET.reg	=	(1	<<	8);
	//PORT->Group[1].OUTSET.reg	=	(1	<<	9);
	//PORT->Group[1].PMUX[4].reg	|=	PORT_PMUX_PMUXO_D	|	PORT_PMUX_PMUXE_D;
	//PORT->Group[1].PINCFG[8].reg=	PORT_PINCFG_PMUXEN;
	//PORT->Group[1].PINCFG[9].reg=	PORT_PINCFG_PMUXEN	|	PORT_PINCFG_INEN	|	PORT_PINCFG_PULLEN;
	
    while (1) 
    {
    }
}

void TC3_Handler()
{
	if(TC3->COUNT8.INTFLAG.reg	&	TC_INTFLAG_OVF)		//If this is overflow interrupt
	{
		TC3->COUNT8.INTFLAG.reg		=	TC_INTFLAG_OVF;	//Clear interrupt flag
		PORT->Group[0].OUTTGL.reg	=	(1	<<	17);	//Toggle LED
		//while(!(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_DRE));
		//SERCOM4->USART.DATA.reg	=	123;
	}
}

//void SERCOM4_Handler()
//{
	//if(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_RXC)
	//{
		//SERCOM4->USART.INTFLAG.reg	=	SERCOM_USART_INTFLAG_RXC;
		//char data = SERCOM4->USART.DATA.reg;
		//while(!(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_DRE));
		//SERCOM4->USART.DATA.reg	=	73;
		//while(!(SERCOM4->USART.INTFLAG.reg	&	SERCOM_USART_INTFLAG_DRE));
		//SERCOM4->USART.DATA.reg	=	data;
	//}
//}