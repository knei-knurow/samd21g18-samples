/*
 * SAMD21_WDT.c
 *
 * Created: 06.03.2021 08:11:55
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


char rcause = 0;


int main(void)
{
	rcause = PM->RCAUSE.reg;
	
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
    
    //LEDs
    PORT->Group[0].DIRSET.reg		=	(1	<<	17);		//Output
    PORT->Group[0].OUTSET.reg		=	(1	<<	17);		//High
    PORT->Group[0].PINCFG[17].reg	=	PORT_PINCFG_DRVSTR;	//Stronger output
	
    PORT->Group[0].DIRSET.reg		=	(1	<<	8);			//Output
    PORT->Group[0].OUTCLR.reg		=	(1	<<	8);			//Low
    PORT->Group[0].PINCFG[8].reg	=	PORT_PINCFG_DRVSTR;	//Stronger output
	
    PORT->Group[0].DIRSET.reg		=	(1	<<	9);			//Output
    PORT->Group[0].OUTCLR.reg		=	(1	<<	9);			//Low
    PORT->Group[0].PINCFG[9].reg	=	PORT_PINCFG_DRVSTR;	//Stronger output
	
	if(rcause	&	PM_RCAUSE_POR)
		PORT->Group[0].OUTSET.reg		=	(1	<<	8);		//High
	if(rcause	&	PM_RCAUSE_WDT)
		PORT->Group[0].OUTSET.reg		=	(1	<<	9);		//High
	
	for(int i = 0 ; i < 0xFFFF ; i++)
	{
		for(int j = 0 ; j < 50 ; j++)
		{
			asm("nop;");
		}
	}
	
	//WDT
	GCLK->CLKCTRL.reg		=	GCLK_CLKCTRL_ID_WDT
							|	GCLK_CLKCTRL_GEN_GCLK2
							|	GCLK_CLKCTRL_CLKEN;
	WDT->CONFIG.reg			=	WDT_CONFIG_PER_16K;
	WDT->CTRL.reg			=	WDT_CTRL_ENABLE;			//Enable WDT
	
	PORT->Group[0].OUTCLR.reg	=	(1	<<	17);			//Turn on LED
	
	while(1);
	
    while (1) 
    {
		for(int i = 0 ; i < 0xFFFF ; i++)
		{
			for(int j = 0 ; j < 6 ; j++)
			{
				asm("nop;");
			}
		}
	    PORT->Group[0].OUTTGL.reg	=	(1	<<	17);		//Toggle LED
		WDT->CLEAR.reg				=	0xA5;				//Reset WDT
    }
}
