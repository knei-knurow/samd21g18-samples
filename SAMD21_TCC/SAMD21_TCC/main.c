/*
 * SAMD21_TCC.c
 *
 * Created: 20.02.2021 09:37:17
 * Author : USER
 */ 

/*
LED: PA17 (INV)

Generic Clocks:
GEN1: OSC8M (8MHz)
GEN2: OSC32K (32.768kHz)
*/

#include "sam.h"

int counter1 = 0;
int counter2 = 0;
signed char cc0 = 0;
signed char cc1 = 0;


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
	
	//TCC0
	PM->APBCMASK.reg		|=	PM_APBCMASK_TCC0;			//Distribute clock to TCC0
	GCLK->CLKCTRL.reg		=	GCLK_CLKCTRL_ID_TCC0_TCC1	//Clock for TCC0 and TCC1
							|	GCLK_CLKCTRL_GEN_GCLK1		//Use GCLK Generator 1 (OSC8M 8MHz)
							|	GCLK_CLKCTRL_CLKEN;			//Enable this clock
	TCC0->CTRLA.reg			=	TCC_CTRLA_PRESCALER_DIV1;	//Prescaler 64
	TCC0->WAVE.reg			=	TCC_WAVE_WAVEGEN_NPWM;		//Normal PWM mode
	TCC0->CTRLA.reg			|=	TCC_CTRLA_ENABLE;			//Enable
	PORT->Group[0].DIRSET.reg=	(1	<<	8)					//Output
							|	(1	<<	9);					//Output
	PORT->Group[0].PMUX[4].reg=	PORT_PMUX_PMUXE_E			//TCC
							|	PORT_PMUX_PMUXO_E;			//TCC
	PORT->Group[0].PINCFG[8].reg=PORT_PINCFG_PMUXEN			//Enable multiplexing
							|	PORT_PINCFG_DRVSTR;
	PORT->Group[0].PINCFG[9].reg=PORT_PINCFG_PMUXEN			//Enable multiplexing
							|	PORT_PINCFG_DRVSTR;
	TCC0->PER.reg			=	(100	<<	TCC_PER_PER_Pos);//Period 100
	TCC0->CC[0].reg			=	(10	<<	TCC_CC_CC_Pos);		//Duty cycle
	TCC0->CC[1].reg			=	(60	<<	TCC_CC_CC_Pos);		//Duty cycle
	
    while (1) 
    {
		counter1++;
		counter2++;
		if(counter1 > 250)
		{
			counter1=0;
			cc0++;
			if(cc0 > 100) cc0 = 0;
		}
		if(counter2 > 500)
		{
			counter2=0;
			cc1--;
			if(cc1 < 0) cc1 = 100;
		}
		TCC0->CCB[0].reg			=	(((unsigned int)cc0)	<<	TCC_CC_CC_Pos);		//Duty cycle
		TCC0->CCB[1].reg			=	(((unsigned int)cc1)	<<	TCC_CC_CC_Pos);		//Duty cycle
		for(char i = 0 ; i < 255 ; i++)
		{
			asm("nop;");
		}
    }
}
