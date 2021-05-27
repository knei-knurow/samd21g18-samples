/*
 * SAMD21_I2C.c
 *
 * Created: 16.01.2021 10:40:29
 * Author : USER
 */ 

/*
LED: PA17 (INV)

Generic Clocks:
GEN1: OSC8M (8MHz)
GEN2: OSC32K (32.768kHz)

SERCOM2:
I2C Master
SDA: PAD0
SCL: PAD1

MPU6050
gyroscope
accelerometer
address
*/

#include "sam.h"

#define EXPANDER_ADDRESS 0b01110000
#define PINS_STATES 0b00101000

char counter = 0;
unsigned char pins_states = PINS_STATES;

//uint32_t calculate_baud(uint32_t fgclk,uint32_t fscl)
//{
	//float f_temp, f_baud;
	//f_temp = ((float)fgclk/(float)fscl)-(((float)fgclk/(float)1000000)*0.3);
	//f_baud=(f_temp/2)-5;
	//return((uint32_t)f_baud);
//}

unsigned char roll_right(unsigned char val)
{
	char r_bit = (val & 1) ? 1 : 0;
	return (val >> 1) | (r_bit << 7);
}


int main(void)
{
    SystemInit();
    
    NVIC_EnableIRQ(TC3_IRQn);		//Enable interrupts from TC3
    NVIC_EnableIRQ(SERCOM2_IRQn);	//Enable interrupts from SERCOM4
    
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
	
	//SERCOM2
	PM->APBCMASK.reg		|=	PM_APBCMASK_SERCOM2;					//Deliver clock to SERCOM
	GCLK->CLKCTRL.reg		=	GCLK_CLKCTRL_ID_SERCOM2_CORE			//Clock for SERCOM2
							|	GCLK_CLKCTRL_GEN_GCLK1					//Use GCLK Generator 1
							|	GCLK_CLKCTRL_CLKEN;						//Enable this clock
	GCLK->CLKCTRL.reg		=	GCLK_CLKCTRL_ID_SERCOMX_SLOW			//Clock for SERCOM SLOW
							|	GCLK_CLKCTRL_GEN_GCLK1					//Use GCLK Generator 1
							|	GCLK_CLKCTRL_CLKEN;						//Enable this clock
	SERCOM2->I2CM.CTRLA.reg	=	SERCOM_I2CM_CTRLA_MODE_I2C_MASTER		//I2C Master
							|	(2	<<	SERCOM_I2CM_CTRLA_SDAHOLD_Pos)	//SDA hold 300-600ns
							|	(0	<<	SERCOM_I2CM_CTRLA_SPEED_Pos)	//Standard speed
							|	(3	<<	SERCOM_I2CM_CTRLA_INACTOUT_Pos);//Inactivity time-out
	SERCOM2->I2CM.CTRLB.reg	=	SERCOM_I2CM_CTRLB_SMEN;					//Smart Mode
	while(SERCOM2->I2CM.SYNCBUSY.reg	&	SERCOM_I2CM_SYNCBUSY_SYSOP);//Wait for synchronization
	SERCOM2->I2CM.BAUD.reg	=	(11	<<	SERCOM_I2CM_BAUD_BAUD_Pos)		//Baud rate
							|	(22	<<	SERCOM_I2CM_BAUD_BAUDLOW_Pos);
	while(SERCOM2->I2CM.SYNCBUSY.reg	&	SERCOM_I2CM_SYNCBUSY_SYSOP);//Wait for synchronization
	SERCOM2->I2CM.CTRLA.reg	|=	SERCOM_I2CM_CTRLA_ENABLE;				//Enable SERCOM
	while(SERCOM2->I2CM.SYNCBUSY.reg	&	SERCOM_I2CM_SYNCBUSY_ENABLE);//Wait for synchronization
	
	while(SERCOM2->I2CM.SYNCBUSY.reg	&	SERCOM_I2CM_SYNCBUSY_ENABLE);//Wait for synchronization
	
	SERCOM2->I2CM.STATUS.reg=	(1	<<	SERCOM_I2CM_STATUS_BUSSTATE_Pos)	&	(SERCOM2->I2CM.STATUS.reg	&	(~SERCOM_I2CM_STATUS_BUSSTATE_Msk));	//Force idle state
	while(SERCOM2->I2CM.SYNCBUSY.reg	&	SERCOM_I2CM_SYNCBUSY_SYSOP);//Wait for synchronization
	SERCOM2->I2CM.INTENSET.reg=	SERCOM_I2CM_INTENSET_MB					//Master bus interrupt
							|	SERCOM_I2CM_INTENSET_SB;				//Slave bus interrupt
	
	PORT->Group[0].DIRSET.reg=	(1	<<	8)								//SDA as output
							|	(1	<<	9);								//SCL as output
	PORT->Group[0].PMUX[4].reg=	PORT_PMUX_PMUXE_D						//SDA as I2C
							|	PORT_PMUX_PMUXO_D;						//SCL as I2C
	PORT->Group[0].PINCFG[8].reg=PORT_PINCFG_PMUXEN						//Turn on multiplexing on SDA
							|	PORT_PINCFG_INEN;						//Turn on input on SDA
							//|	PORT_PINCFG_PULLEN;						//Turn on pull-up on SDA
	PORT->Group[0].PINCFG[9].reg=PORT_PINCFG_PMUXEN						//Turn on multiplexing on SCL
							|	PORT_PINCFG_INEN;						//Turn on input on SCL
							//|	PORT_PINCFG_PULLEN;						//Turn on pull-up on SCL
	//PORT->Group[0].OUTSET.reg	=	(1	<<	8)	|	(1	<<	9);			//Pull-ups on SDA and SCL
	
    while (1) 
    {
    }
}

void TC3_Handler()
{
	if(TC3->COUNT8.INTFLAG.reg	&	TC_INTFLAG_OVF)			//If this is overflow interrupt
	{
		TC3->COUNT8.INTFLAG.reg		=	TC_INTFLAG_OVF;		//Clear interrupt flag
		PORT->Group[0].OUTTGL.reg	=	(1	<<	17);		//Toggle LED
		//PORT->Group[0].OUTTGL.reg	=	(1	<<	8)	|	(1	<<	9);			//Low level on SDA and SCL
		
		//SERCOM2->I2CM.CTRLB.reg	&=~	SERCOM_I2CM_CTRLB_ACKACT;
		//while(SERCOM2->I2CM.SYNCBUSY.reg	&	SERCOM_I2CM_SYNCBUSY_SYSOP);
		if((SERCOM2->I2CM.STATUS.reg	&	SERCOM_I2CM_STATUS_BUSSTATE_Msk)	>>	SERCOM_I2CM_STATUS_BUSSTATE_Pos == 0)	//If unknown state
		{
			SERCOM2->I2CM.STATUS.reg=	(1	<<	SERCOM_I2CM_STATUS_BUSSTATE_Pos)	&	(SERCOM2->I2CM.STATUS.reg	&	(~SERCOM_I2CM_STATUS_BUSSTATE_Msk));	//Force idle state
			while(SERCOM2->I2CM.SYNCBUSY.reg	&	SERCOM_I2CM_SYNCBUSY_SYSOP);//Wait for synchronization
		}
		if((SERCOM2->I2CM.STATUS.reg	&	SERCOM_I2CM_STATUS_BUSSTATE_Msk)	>>	SERCOM_I2CM_STATUS_BUSSTATE_Pos == 1)	//If idle state
		{
			SERCOM2->I2CM.ADDR.reg	=	(EXPANDER_ADDRESS	<<	SERCOM_I2CM_ADDR_ADDR_Pos);	//Slave address (start transmission)
			counter = 0;
			while(SERCOM2->I2CM.SYNCBUSY.reg	&	SERCOM_I2CM_SYNCBUSY_SYSOP);//Wait for synchronization
		}
		//while(SERCOM2->I2CM.SYNCBUSY.reg	&	SERCOM_I2CM_SYNCBUSY_SYSOP);
	}
}

void SERCOM2_Handler()
{
	if(SERCOM2->I2CM.INTFLAG.reg	&	SERCOM_I2CM_INTFLAG_MB)		//Master bus interrupt
	{
		if(counter == 0)
		{
			SERCOM2->I2CM.DATA.reg	=	pins_states;				//Send data
			while(SERCOM2->I2CM.SYNCBUSY.reg	&	SERCOM_I2CM_SYNCBUSY_SYSOP);
			counter = 1;
			pins_states = roll_right(pins_states);
		}
		else
		{
			SERCOM2->I2CM.CTRLB.reg	|=	(3	<<	SERCOM_I2CM_CTRLB_CMD_Pos);		//Stop
			while(SERCOM2->I2CM.SYNCBUSY.reg	&	SERCOM_I2CM_SYNCBUSY_SYSOP);
			counter = 0;
		}
	}
	if(SERCOM2->I2CM.INTFLAG.reg	&	SERCOM_I2CM_INTFLAG_SB)		//Slave bus interrupt
	{
		SERCOM2->I2CM.CTRLB.reg	|=	(3	<<	SERCOM_I2CM_CTRLB_CMD_Pos);			//Stop
		while(SERCOM2->I2CM.SYNCBUSY.reg	&	SERCOM_I2CM_SYNCBUSY_SYSOP);
		counter = 0;
	}
	SERCOM2->I2CM.INTFLAG.reg	=	SERCOM_I2CM_INTFLAG_MB	|	SERCOM_I2CM_INTFLAG_SB;	//Clear interrupt flags
}