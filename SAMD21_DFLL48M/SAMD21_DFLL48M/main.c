/*
 * SAMD21_DFLL48M.c
 *
 * Created: 06.02.2021 09:36:03
 * Author : USER
 */ 


/*
LED: PA17 (INV)

Generic Clocks:
GEN0: DFLL48M (48MHz)
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
    //GCLK Generators
	//GCLK->GENCTRL.reg		=	(0	<<	GCLK_GENCTRL_ID_Pos)	//Generator 0
							//|	GCLK_GENCTRL_SRC_OSC8M			//OSC8M is source
							//|	GCLK_GENCTRL_GENEN;				//Enable generator
    GCLK->GENCTRL.reg		=	(1	<<	GCLK_GENCTRL_ID_Pos)	//Generator 1
							|	GCLK_GENCTRL_SRC_OSC8M			//OSC8M is source
							|	GCLK_GENCTRL_GENEN;				//Enable generator
	while(GCLK->STATUS.bit.SYNCBUSY);
    GCLK->GENCTRL.reg		=	(2	<<	GCLK_GENCTRL_ID_Pos)	//Generator 2
							|	GCLK_GENCTRL_SRC_OSC32K			//OSC32K is source
							|	GCLK_GENCTRL_GENEN;				//Enable generator
	while(GCLK->STATUS.bit.SYNCBUSY);
    
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
	
	PORT->Group[1].DIRSET.reg		=	(1	<<	9);			//Output
	PORT->Group[1].OUTSET.reg		=	(1	<<	9);			//High
	
	while(!(SYSCTRL->PCLKSR.reg	&	SYSCTRL_PCLKSR_OSC32KRDY));
	
	//DFLL48M
	GCLK->CLKCTRL.reg		=	GCLK_CLKCTRL_ID_DFLL48							//DFLL48M reference
							|	GCLK_CLKCTRL_GEN_GCLK2							//Use GCLK Generator 2 (OSC32K)
							|	GCLK_CLKCTRL_CLKEN;								//Enable this clock
	SYSCTRL->DFLLCTRL.reg	&=~	SYSCTRL_DFLLCTRL_ONDEMAND;						//Write 0 to ONDEMAND (issue described in errata)
	while(!(SYSCTRL->PCLKSR.reg	&	SYSCTRL_PCLKSR_DFLLRDY));					//Wait for ready
	SYSCTRL->DFLLMUL.reg	=	(1464	<<	SYSCTRL_DFLLMUL_MUL_Pos)			//48MHz
	//SYSCTRL->DFLLMUL.reg	=	(1000	<<	SYSCTRL_DFLLMUL_MUL_Pos)			//12MHz
							|	(4	<<	SYSCTRL_DFLLMUL_FSTEP_Pos)				//Fine step 4
							|	(32	<<	SYSCTRL_DFLLMUL_CSTEP_Pos);				//Coarse step 32
	while(!(SYSCTRL->PCLKSR.reg	&	SYSCTRL_PCLKSR_DFLLRDY));					//Wait for ready
	SYSCTRL->DFLLCTRL.reg	=	SYSCTRL_DFLLCTRL_ENABLE							//Enable DFLL48M
							|	SYSCTRL_DFLLCTRL_MODE							//Closed-loop mode
							|	SYSCTRL_DFLLCTRL_WAITLOCK;						//Output clock only when locked
	while(!(SYSCTRL->PCLKSR.reg	&	SYSCTRL_PCLKSR_DFLLRDY));					//Wait for ready
	while(!(SYSCTRL->PCLKSR.reg	&	SYSCTRL_PCLKSR_DFLLLCKC));					//Wait for coarse lock
	while(!(SYSCTRL->PCLKSR.reg	&	SYSCTRL_PCLKSR_DFLLLCKF));					//Wait for fine lock
	
	NVMCTRL->CTRLB.bit.RWS = 1;
	
	GCLK->GENCTRL.reg		=	(0	<<	GCLK_GENCTRL_ID_Pos)	//Generator 0
							|	GCLK_GENCTRL_SRC_DFLL48M		//DFLL48M is source
							|	GCLK_GENCTRL_IDC				//Improve duty cycle
							|	GCLK_GENCTRL_GENEN;				//Enable generator
	while(GCLK->STATUS.bit.SYNCBUSY);
	
    //LED
    PORT->Group[0].DIRSET.reg		=	(1	<<	17);		//Output
    PORT->Group[0].OUTCLR.reg		=	(1	<<	17);		//Low
    PORT->Group[0].PINCFG[17].reg	=	PORT_PINCFG_DRVSTR;	//Stronger output
	
	
    while (1) 
    {
		for(int i = 0 ; i < 0xFFFF ; i++)
		{
			for(int j = 0 ; j < 10 ; j++)
			{
				asm("nop;");
			}
		}
		PORT->Group[0].OUTTGL.reg	=	(1	<<	17);	//Toggle LED
		PORT->Group[1].OUTTGL.reg	=	(1	<<	9);		//Toggle
    }
}
