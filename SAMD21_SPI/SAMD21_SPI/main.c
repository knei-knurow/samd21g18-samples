/*
 * SAMD21_SPI.c
 *
 * Created: 16.01.2021 09:25:53
 * Author : USER
 */ 

/*
LED: PA17 (INV)

Generic Clocks:
GEN1: OSC8M (8MHz)
GEN2: OSC32K (32.768kHz)

SERCOM0:
SPI Master
MOSI: PAD2
MISO: PAD1
SCK: PAD3
*/

#include "sam.h"

#define SPI_SENDING_FIFO_SIZE 16

unsigned char spi_sending_fifo[SPI_SENDING_FIFO_SIZE];
unsigned char *spi_sending_fifo_w_ptr;
unsigned char *spi_sending_fifo_r_ptr;

void spi_sending_fifo_add(unsigned char data);
void spi_sending_fifo_send();
//char spi_sending_fifo_size();

void spi_sending_fifo_add(unsigned char data)
{
	spi_sending_fifo_w_ptr++;
	if((spi_sending_fifo_w_ptr - spi_sending_fifo) >= SPI_SENDING_FIFO_SIZE)
	{
		spi_sending_fifo_w_ptr = spi_sending_fifo;
	}
	*spi_sending_fifo_w_ptr = data;
	if(SERCOM0->SPI.INTFLAG.reg & SERCOM_SPI_INTFLAG_DRE)	//If data register is currently empty
	{
		spi_sending_fifo_send();	//Send data
	}
}

void spi_sending_fifo_send()
{
	if((SERCOM0->SPI.INTFLAG.reg & SERCOM_SPI_INTFLAG_DRE) && (spi_sending_fifo_w_ptr != spi_sending_fifo_r_ptr))	//If data register is currently empty
	{
		spi_sending_fifo_r_ptr++;
		if((spi_sending_fifo_r_ptr - spi_sending_fifo) >= SPI_SENDING_FIFO_SIZE)
		{
			spi_sending_fifo_r_ptr = spi_sending_fifo;
		}
		unsigned char data = *spi_sending_fifo_r_ptr;
		PORT->Group[1].OUTCLR.reg	=	(1	<<	9);	//SS low
		SERCOM0->SPI.DATA.reg = data;	//Send data
		
	}
}

//char spi_sending_fifo_size()
//{
	//signed long size = spi_sending_fifo_w_ptr - spi_sending_fifo_r_ptr;
	//if(size < 0) size += SPI_SENDING_FIFO_SIZE;
	//return (char)size;
//}


int main(void)
{
    SystemInit();
    
    NVIC_EnableIRQ(TC3_IRQn);		//Enable interrupts from TC3
    NVIC_EnableIRQ(SERCOM0_IRQn);	//Enable interrupts from SERCOM0
    
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
    
    //SERCOM0
	PM->APBCMASK.reg			|=	PM_APBCMASK_SERCOM0;
	GCLK->CLKCTRL.reg			=	GCLK_CLKCTRL_ID_SERCOM0_CORE	|	GCLK_CLKCTRL_GEN_GCLK1	|	GCLK_CLKCTRL_CLKEN;
    SERCOM0->SPI.CTRLA.reg		=	SERCOM_SPI_CTRLA_MODE_SPI_MASTER	//SPI Master
								|	(1	<<	SERCOM_SPI_CTRLA_DOPO_Pos)	//MOSI at PAD2, SCK at PAD3
								|	(1	<<	SERCOM_SPI_CTRLA_DIPO_Pos)	//MISO at PAD1
								|	(0	<<	SERCOM_SPI_CTRLA_FORM_Pos)	//Normal SPI frame
								|	(1	<<	SERCOM_SPI_CTRLA_CPHA_Pos)	//Change on falling, sample on rising
								|	(1	<<	SERCOM_SPI_CTRLA_CPOL_Pos)	//SCK is high when idle
								|	(0	<<	SERCOM_SPI_CTRLA_DORD_Pos);	//MSB first
	while(SERCOM0->SPI.SYNCBUSY.reg	&	SERCOM_SPI_SYNCBUSY_ENABLE);	//Wait to synchronization
	SERCOM0->SPI.CTRLB.reg		=	(0	<<	SERCOM_SPI_CTRLB_CHSIZE_Pos)//8bit
								|	(0	<<	SERCOM_SPI_CTRLB_MSSEN_Pos);//Do not use hardware SS
	while(SERCOM0->SPI.SYNCBUSY.reg	&	SERCOM_SPI_SYNCBUSY_CTRLB);		//Wait to synchronization
	//SERCOM0->SPI.BAUD.reg		=	39;									//100kHz
	SERCOM0->SPI.BAUD.reg		=	156;								//25kHz
	SERCOM0->SPI.INTENSET.reg	=	SERCOM_SPI_INTENSET_TXC;			//Data register empty interrupt
	SERCOM0->SPI.CTRLA.reg		|=	SERCOM_SPI_CTRLA_ENABLE;			//Enable SPI
	PORT->Group[0].DIRSET.reg	=	(1	<<	6)							//MOSI as OUTPUT
								|	(1	<<	7);							//SCK as OUTPUT
	PORT->Group[0].DIRCLR.reg	=	(1	<<	5);							//MISO as INPUT
	PORT->Group[1].DIRSET.reg	=	(1	<<	9);							//SS as OUTPUT
	PORT->Group[0].PMUX[2].reg	=	PORT_PMUX_PMUXO_D;					//MISO config
	PORT->Group[0].PMUX[3].reg	=	PORT_PMUX_PMUXE_D					//MOSI config
								|	PORT_PMUX_PMUXO_D;					//SCK config
	PORT->Group[0].PINCFG[5].reg=	PORT_PINCFG_PMUXEN					//Enable multiplexing on MISO
								|	PORT_PINCFG_INEN;					//MISO as INPUT
	PORT->Group[0].PINCFG[6].reg=	PORT_PINCFG_PMUXEN;					//Enable multiplexing on MOSI
	PORT->Group[0].PINCFG[7].reg=	PORT_PINCFG_PMUXEN;					//Enable multiplexing on SCK
	
	//SPI sending FIFO
	for(int i = 0 ; i < SPI_SENDING_FIFO_SIZE ; i++)
	{
		spi_sending_fifo[i] = 0;
	}
	spi_sending_fifo_w_ptr = spi_sending_fifo;
	spi_sending_fifo_r_ptr = spi_sending_fifo;
	
    while (1) 
    {
		for(long i = 0 ; i < 0xFFFF * 100 ; i++)
		{
			asm("nop;");
		}
		//SERCOM0->SPI.DATA.reg	=	0b00001010;
		spi_sending_fifo_add(0b00001010);
		spi_sending_fifo_add(0b00001111);
		spi_sending_fifo_add(0b11000010);
		spi_sending_fifo_add(0b00001010);
		spi_sending_fifo_add(0b00001111);
		spi_sending_fifo_add(0b11000010);
    }
}

void TC3_Handler()
{
	if(TC3->COUNT8.INTFLAG.reg	&	TC_INTFLAG_OVF)							//If this is overflow interrupt
	{
		TC3->COUNT8.INTFLAG.reg		=	TC_INTFLAG_OVF;						//Clear interrupt flag
		PORT->Group[0].OUTTGL.reg	=	(1	<<	17);						//Toggle LED
		//spi_sending_fifo_add(0b00001010);
		//spi_sending_fifo_add(0b00001111);
	}
}

void SERCOM0_Handler()
{
	if(SERCOM0->SPI.INTFLAG.reg	&	SERCOM_SPI_INTFLAG_TXC)			//Transmit complete interrupt
	{
		SERCOM0->SPI.INTFLAG.reg = SERCOM_SPI_INTFLAG_TXC;
		if(spi_sending_fifo_w_ptr == spi_sending_fifo_r_ptr)
		{
			PORT->Group[1].OUTSET.reg	=	(1	<<	9);	//SS high
		}
	}
	if(SERCOM0->SPI.INTFLAG.reg & SERCOM_SPI_INTFLAG_DRE)			//Data register empty interrupt
	{
		if(spi_sending_fifo_w_ptr != spi_sending_fifo_r_ptr)
		{
			spi_sending_fifo_send();
			//PORT->Group[0].OUTTGL.reg	=	(1	<<	17);						//Toggle LED
		}
	}
	//SERCOM0->SPI.INTFLAG.reg = 0xFF;	//Clear interrupt flags
}