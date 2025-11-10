/*
 *
 *
 *
 *
 * mcp3912Lib.c
 *
 * Library for interfacing with the MCP3912 3V Four-Channel Analog Front end
 * Author:      Vega Innovations (Pvt) Ltd
 * Date:        29/05/2024
 * Description: This library provides functions to communicate with and control the MCP3912 Four Channel AFE.
 * Notes:       The function dataExchangeMCP3912 has been tailored to accommodate the microcontroller utilized in
 *              the implementation.
 *
 *
 *
 */

#include <mcp3912Lib.h>
#include <math.h>
#include <loraSTM32.h>


/* Additional includes and definition tailored to the specific microcontroller used */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_spi.h"
#include <FLASH_PAGE_F1.h>

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim2;
LoRa myLoRa;

const float SQRT2 = 1.41421356237;
const float PI    =  3.1415;

float  volt[300];
float  voltLag[300];
float  voltLead[300];
uint8_t sampleRate=78;


float i1[300];
float i2[300];
float i3[300];
float send[4];




uint8_t reference=0;
uint8_t currentLag=0;
uint8_t currentLag_pre=0;

uint8_t reference_saved;
uint8_t currentLag_saved;


int8_t direction=0;
int8_t dirLag=0;
int8_t dirLead=0;

int8_t direction_saved;
int8_t dirLag_saved;
int8_t dirLead_saved;

uint8_t cnfirm1=0;
uint8_t cnfirm2=0;
uint8_t cnfirm3=0;
uint8_t cnfirm4=0;
uint8_t sendCount=0;

float Vrms_next=0;
float Vrms=0;
float VsqrdTotal=0;
float I1sqrdTotal=0;
float I2sqrdTotal=0;
float I3sqrdTotal=0;

int16_t n1=0;
float ratio=0;


float powerSum=0;
float powerSumLag=0;
float powerSumLead=0;






void mcp3912begin()
{


	reference_saved=Flash_Read_NUM(0x0801FC20);
	currentLag_saved=Flash_Read_NUM(0x0801FC40);
	direction_saved= Flash_Read_NUM(0x0801FC60);
	dirLag_saved =Flash_Read_NUM(0x0801FC80);
	dirLead_saved= Flash_Read_NUM(0x0801FCA0);

    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3, GPIO_PIN_SET); 	//pull up the reset button
 	TIM2->CCR4 = (uint32_t)8;
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
	HAL_Delay(100);

}

void transferInit(){
	myLoRa = newLoRa();

	myLoRa.hSPIx                 = &hspi2;
	myLoRa.CS_port               = GPIOD;    //NSS_GPIO_Port;
	myLoRa.CS_pin                = GPIO_PIN_2; //NSS_Pin;
	myLoRa.reset_port            = GPIOA;//RESET_GPIO_Port;
	myLoRa.reset_pin             = GPIO_PIN_10; //RESET_Pin;
	myLoRa.DIO0_port			 = GPIOB;//DIO0_GPIO_Port;
	myLoRa.DIO0_pin				 =  GPIO_PIN_5;//DIO0_Pin;

	myLoRa.frequency             = 433;				  // default = 433 MHz
	myLoRa.spredingFactor        = SF_7;							// default = SF_7
	myLoRa.bandWidth			 = BW_125KHz;				  // default = BW_125KHz
	myLoRa.crcRate				 = CR_4_5;						// default = CR_4_5
	myLoRa.power			     = POWER_20db;				// default = 20db
	myLoRa.overCurrentProtection = 120; 							// default = 100 mA
	myLoRa.preamble				 = 10;		  					// default = 8;

	LoRa_reset(&myLoRa);
	LoRa_init(&myLoRa);
	// START CONTINUOUS RECEIVING -----------------------------------
	LoRa_startReceiving(&myLoRa);
	//---------------------------------------------------------------
}



void phaseDetect(uint8_t *cal ,uint16_t *cnt)
{


	uint8_t chanlReadCmd =  0x41; //0b01000001;

	uint8_t dummyData2 = 0x00;
	uint8_t dummyData1 = 0x00;

	int8_t data1 = 0x00;
	int8_t data2 = 0x00;
	int8_t data3 = 0x00;
	int8_t data4 = 0x00;
	int8_t data5 = 0x00;
	int8_t data6 = 0x00;
	int8_t data7 = 0x00;
	int8_t data8 = 0x00;

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi1, &chanlReadCmd, &dummyData2, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data1, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data2, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data3, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data4, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data5, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data6, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data7, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data8, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

	float VoltageSample= ((data1<< 8) | (data2 & 0xff))* 0.014204;
	volt[(*cnt)]= VoltageSample;


	float CurrentSample1=((data3<<8) | (data4 & 0xff))*0.00140;
	if(CurrentSample1<0.1 && CurrentSample1>-0.1)
	{
		CurrentSample1=0;
	}
	i1[(*cnt)] = CurrentSample1;

	float CurrentSample2=((data5<<8) | (data6 & 0xff))*0.00140;
	if(CurrentSample2<0.1 && CurrentSample2>-0.1)
	{
		CurrentSample2=0;
	}
	i2[(*cnt)] = CurrentSample2;

	float CurrentSample3=((data7<<8) | (data8 & 0xff))* 0.00140;
	if(CurrentSample3<0.1 && CurrentSample3>-0.1)
	{
		CurrentSample3=0;
	}
	i3[(*cnt)] = CurrentSample3;


    (*cnt)++;

    if((*cal)==0 && (*cnt)==300)
    {

    	__disable_irq();
    	(*cnt)=0;

    	uint8_t reference_pre=0;
    	for(uint16_t i=120 ;i<300;i++)
    	{

    		uint16_t indx=0;

    		if(volt[i]<0 && volt[i+1]>0)
    		{
    			indx=i;

    			for(uint16_t j=0;j<20;j++)
    			{
    				if((indx+j+1)>300 || (indx-j-1)<0)
		            {
    					(*cal)=0;
    					 break;
		            }

    				else if((i1[indx+j]<=0 && i1[indx+j+1]>=0) || (i1[indx+j]>=0 && i1[indx+j+1]<=0) || (i1[indx-j]<=0 && i1[indx-j-1]>=0) || (i1[indx-j]>=0 && i1[indx-j-1]<=0))
    				{

    					reference_pre=1;
    					break;
    				}
    				else if((i2[indx+j]<=0 && i2[indx+j+1]>=0) || (i2[indx+j]>=0 && i2[indx+j+1]<=0) || (i2[indx-j]<=0 && i2[indx-j-1]>=0) || (i2[indx-j]>=0 && i2[indx-j-1]<=0))
    				{

    					reference_pre=2;
    					break;
    				}
    				else if((i3[indx+j]<=0 && i3[indx+j+1]>=0) || (i3[indx+j]>=0 && i3[indx+j+1]<=0) || (i3[indx-j]<=0 && i3[indx-j-1]>=0) || (i3[indx-j]>=0 && i3[indx-j-1]<=0))
    				{

    				    reference_pre=3;
    				    break;
    				}


    			}
    			if(reference_pre!=0)
    			{
    				break;

    			}

    		}

    	}

    	//confirm the phase
    	if(reference_pre!=0 && reference==reference_pre){
    		cnfirm1++;
    	}
    	else
    	{
    		cnfirm1=0;
    	}
    	if(cnfirm1>=5)
    	{
    		cnfirm1=0;
    		reference_saved=reference;
    		Flash_Write_NUM(0x0801FC20,reference_saved);
    		(*cal)=1;


    	}
    	reference=reference_pre;

    	__enable_irq();
    }

}

void currentLagDetect(uint8_t *cal,uint16_t *cnt)
{
	    uint8_t chanlReadCmd =  0x41; //0b01000001;

		uint8_t dummyData2 = 0x00;
		uint8_t dummyData1 = 0x00;

		int8_t data1 = 0x00;
		int8_t data2 = 0x00;
		int8_t data3 = 0x00;
		int8_t data4 = 0x00;
		int8_t data5 = 0x00;
		int8_t data6 = 0x00;
		int8_t data7 = 0x00;
		int8_t data8 = 0x00;

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
		HAL_SPI_TransmitReceive(&hspi1, &chanlReadCmd, &dummyData2, 1, HAL_MAX_DELAY);
		HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data1, 1, HAL_MAX_DELAY);
		HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data2, 1, HAL_MAX_DELAY);
		HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

		HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data3, 1, HAL_MAX_DELAY);
		HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data4, 1, HAL_MAX_DELAY);
		HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

		HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data5, 1, HAL_MAX_DELAY);
		HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data6, 1, HAL_MAX_DELAY);
		HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

		HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data7, 1, HAL_MAX_DELAY);
		HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data8, 1, HAL_MAX_DELAY);
		HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

		float VoltageSample= ((data1<< 8) | (data2 & 0xff))* 0.014204;
		volt[(*cnt)]= VoltageSample;

		float CurrentSample1=((data3<<8) | (data4 & 0xff))* 0.00140;
		if(CurrentSample1<0.1 && CurrentSample1>-0.1)
		{
			CurrentSample1=0;
		}
		i1[(*cnt)] = CurrentSample1;

		float CurrentSample2=((data5<<8) | (data6 & 0xff))*0.00140;
		if(CurrentSample2<0.1 && CurrentSample2>-0.1)
		{
			CurrentSample2=0;
		}
		i2[(*cnt)] = CurrentSample2;

		float CurrentSample3=((data7<<8) | (data8 & 0xff))* 0.00140;
		if(CurrentSample3<0.1 && CurrentSample3>-0.1)
		{
			CurrentSample3=0;
		}
		i3[(*cnt)] = CurrentSample3;


		(*cnt)++;

		if((*cnt)==2)
		    {
		    	if(volt[0]<=0 && volt[1]>=0 && (volt[0]!=volt[1]))
		    	{
		    		//do nothing
		    	}
		    	else
		    	{
		    		(*cnt)=(*cnt)-2;
		    	}

		    }



if((*cal)==2 && (*cnt)==300)
	    {
   (*cnt)=0;
	__disable_irq();

	switch(reference)
	{

	case 1:
		currentLag_pre=0;
			for(uint16_t i=120 ;i<300;i++)
			{

				uint16_t indx=0;

				if(i1[i]<=0 && i1[i+1]>=0  && (i1[i]!=i1[i+1]))
				{
					indx=i;

					for(uint16_t j=0;j<20;j++)
					{
						if((indx+j+1)>300 || (indx-j-1)<0  )
						{
							(*cal)=2;
							 break;
						}

						else if((i2[indx+j]<=0 && i2[indx+j+1]>=0) || (i2[indx+j]>=0 && i2[indx+j+1]<=0))
						{
							currentLag_pre=2;
							break;
						}
						else if((i3[indx+j]<=0 && i3[indx+j+1]>=0) || (i3[indx+j]>=0 && i3[indx+j+1]<=0))
						{

							currentLag_pre=3;
							break;
						}

					}
					if(currentLag_pre!=0)
					{
						break;

					}

				}

			}

			//confirm the current
			if(currentLag_pre!=0 && currentLag==currentLag_pre)
			{
				cnfirm3++;
			}
			else
			{
				cnfirm3=0;
			}
			if(cnfirm3>=5)
			{
				cnfirm3=0;
				currentLag_saved=currentLag;
				Flash_Write_NUM(0x0801FC40, currentLag_saved);
				(*cal)=3;


			}
			currentLag=currentLag_pre;

			break;



	case 2:
		currentLag_pre=0;
					for(uint16_t i=120 ;i<300;i++)
					{

						uint16_t indx=0;

						if(i2[i]<=0 && i2[i+1]>=0  && (i2[i]!=i2[i+1]))
						{
							indx=i;

							for(uint16_t j=0;j<20;j++)
							{
								if((indx+j+1)>300 || (indx-j-1)<0)
								{
									(*cal)=2;
									 break;
								}

								else if((i1[indx+j]<=0 && i1[indx+j+1]>=0) || (i1[indx+j]>=0 && i1[indx+j+1]<=0))
								{

									currentLag_pre=1;
									break;
								}
								else if((i3[indx+j]<=0 && i3[indx+j+1]>=0) || (i3[indx+j]>=0 && i3[indx+j+1]<=0))
								{

									currentLag_pre=3;
									break;
								}

							}
							if(currentLag_pre!=0)
							{
								break;

							}

						}

					}

					//confirm the current
					if(currentLag_pre!=0 && currentLag==currentLag_pre)
					{
						cnfirm3++;
					}
					else
					{
						cnfirm3=0;
					}
					if(cnfirm3>=5)
					{
						cnfirm3=0;

						currentLag_saved=currentLag;
					    Flash_Write_NUM(0x0801FC40, currentLag_saved);
						(*cal)=3;
						break;

					}
					currentLag=currentLag_pre;

					break;


	case 3:
		uint8_t currentLag_pre=0;
					for(uint16_t i=120 ;i<300;i++)
					{

						uint16_t indx=0;

						if(i3[i]<=0 && i3[i+1]>=0  && (i3[i]!=i3[i+1]))
						{
							indx=i;

							for(uint16_t j=0;j<20;j++)
							{
								if((indx+j+1)>300 || (indx-j-1)<0)
								{
									(*cal)=2;
									 break;
								}

								else if((i2[indx+j]<=0 && i2[indx+j+1]>=0) || (i2[indx+j]>=0 && i2[indx+j+1]<=0))
								{

									currentLag_pre=2;
									break;
								}
								else if((i1[indx+j]<=0 && i1[indx+j+1]>=0) || (i1[indx+j]>=0 && i1[indx+j+1]<=0))
								{

									currentLag_pre=1;
									break;
								}

							}
							if(currentLag_pre!=0)
							{
								break;

							}

						}

					}

					//confirm the current
					if(currentLag_pre!=0 && currentLag==currentLag_pre)
					{
						cnfirm3++;
					}
					else
					{
						cnfirm3=0;
					}
					if(cnfirm3>=5)
					{
						cnfirm3=0;
						currentLag_saved=currentLag;
						Flash_Write_NUM(0x0801FC40, currentLag_saved);
						(*cal)=3;
						break;


					}
					currentLag=currentLag_pre;

					break;

	default:
		//do nothing
		break;


	}


	__enable_irq();
	    }


}


void PowerDirSave(uint8_t *cal,uint16_t *cnt)
{


  voltLag[(*cnt)]=(Vrms*SQRT2)*sin((2*PI*((*cnt))/sampleRate)+(2*PI/3));
  voltLead[(*cnt)]=(Vrms*SQRT2)*sin((2*PI*((*cnt))/sampleRate)-(2*PI/3));

  switch(reference)
  {
  case 1:
	  powerSum+=volt[(*cnt)]*i1[(*cnt)];
	  if(currentLag==2)
	  {
		  powerSumLag+=voltLag[(*cnt)]*i2[(*cnt)];
		  powerSumLead+=voltLead[(*cnt)]*i3[(*cnt)];

	  }
	  else
	  {
		  powerSumLag+=voltLag[(*cnt)]*i3[(*cnt)];
		  powerSumLead+=voltLead[(*cnt)]*i2[(*cnt)];

	  }

	  break;
  case 2:
	  powerSum+=volt[(*cnt)]*i2[(*cnt)];
	  if(currentLag==1)
	  {
		  powerSumLag+=voltLag[(*cnt)]*i1[(*cnt)];
		  powerSumLead+=voltLead[(*cnt)]*i3[(*cnt)];

	  }
	  else
	  {
		  powerSumLag+=voltLag[(*cnt)]*i3[(*cnt)];
		  powerSumLead+=voltLead[(*cnt)]*i1[(*cnt)];

	  }
  case 3:
	  powerSum+=volt[(*cnt)]*i3[(*cnt)];
	  if(currentLag==1)
	  {
		  powerSumLag+=voltLag[(*cnt)]*i1[(*cnt)];
		  powerSumLead+=voltLead[(*cnt)]*i2[(*cnt)];

	  }
	  else
	  {
		  powerSumLag+=voltLag[(*cnt)]*i2[(*cnt)];
		  powerSumLead+=voltLead[(*cnt)]*i1[(*cnt)];

	  }

	  break;

  default:
	  //do nothing
	  break;



  }


 (*cnt)++;

 if((*cnt)==300 && (*cal)==3)
 {
	 (*cnt)=0;
	 __disable_irq();
	 if(powerSum<0)
	 {
		 direction=-1;
		 direction_saved=-1;
		 Flash_Write_NUM(0x0801FC60, direction_saved);

	 }
	 else
	 {
		 direction=1;
		 direction_saved=1;
		 Flash_Write_NUM(0x0801FC60, direction_saved);
	 }

	 if(powerSumLag<0)
	 {
		 dirLag=-1;
		 dirLag_saved=-1;
		 Flash_Write_NUM(0x0801FC80, dirLag_saved);
	 }
	 else
	 {
		 dirLag =1;
		 dirLag_saved=1;
		 Flash_Write_NUM(0x0801FC80, dirLag);
	 }
	 if(powerSumLead<0)
	 {
		 dirLead=-1;
		 dirLead_saved=-1;
		 Flash_Write_NUM(0x0801FCA0, dirLead_saved);
	 }
	 else
	 {
		 dirLead=1;
		 dirLead_saved=1;
		 Flash_Write_NUM(0x0801FCA0, dirLead_saved);

	 }

	 powerSum=0;
	 powerSumLag=0;
	 powerSumLead=0;

	 Flash_Write_NUM(0x0801FC00,4);  //save the state here
	 HAL_GPIO_WritePin(GPIOB, YEL_LED_Pin, RESET);
	 (*cal)=4;

	__enable_irq();

 }


}


void readChannels(uint8_t *cal,uint16_t *cnt)
{
	uint8_t chanlReadCmd =  0x41; //0b01000001;

	uint8_t dummyData2 = 0x00;
	uint8_t dummyData1 = 0x00;

	int8_t data1 = 0x00;;
	int8_t data2 = 0x00;
	int8_t data3 = 0x00;
	int8_t data4 = 0x00;
	int8_t data5 = 0x00;
	int8_t data6 = 0x00;
	int8_t data7 = 0x00;
	int8_t data8 = 0x00;

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi1, &chanlReadCmd, &dummyData2, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data1, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data2, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data3, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data4, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data5, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data6, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data7, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data8, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);


	float VoltageSample= ((data1<< 8) | (data2 & 0xff))* 0.014204;
	VsqrdTotal += (VoltageSample*VoltageSample);


	float CurrentSample1=((data3<<8) | (data4 & 0xff))* 0.00140;
	I1sqrdTotal += (CurrentSample1*CurrentSample1);


	float CurrentSample2=((data5<<8) | (data6 & 0xff))* 0.00140;
	I2sqrdTotal += (CurrentSample2*CurrentSample2);


	float CurrentSample3=((data7<<8) | (data8 & 0xff))* 0.00140;
	I3sqrdTotal += (CurrentSample3*CurrentSample3);


	(*cnt)++;

}




void channelReadingsSend(uint8_t *cal,uint16_t *numOfSample)
{
	__disable_irq();


    Vrms = sqrt((VsqrdTotal)/(*numOfSample));
    float I1rms = sqrt((I1sqrdTotal)/(*numOfSample));
    float I2rms = sqrt((I2sqrdTotal)/(*numOfSample));
    float I3rms = sqrt((I3sqrdTotal)/(*numOfSample));



     switch(reference_saved)
     {
     case 1:
    	 if(direction==-direction_saved)
    	 {
    		 I1rms=-I1rms;
    	 }


    	 switch(currentLag_saved)
		 {
    	 case 2:
    		 if(dirLag_saved==-dirLag)
    		 {
    			 I2rms=-I2rms;
    		 }

    		 if(dirLead==-dirLead_saved)
    		 {
    			 I3rms=-I3rms;
    		 }
    		 break;
    	 case 3:
    		 if(dirLag_saved==-dirLag)
			 {
				 I3rms=-I3rms;
			 }

			 if(dirLead==-dirLead_saved)
			 {
				 I2rms=-I2rms;
			 }
			 break;
    	 default:
    		 break;


    	 }
    	 break;
		 case 2:
			 if(direction==-direction_saved)
			 {
				 I2rms=I2rms;
			 }

			 switch(currentLag_saved)
			 {
			 case 1:
				 if(dirLag_saved==-dirLag)
				 {
					 I1rms=-I1rms;
				 }

				 if(dirLead==-dirLead_saved)
				 {
					 I3rms=-I3rms;
				 }
				 break;
			 case 3:
				 if(dirLag_saved==-dirLag)
				 {
					 I3rms=-I3rms;
				 }

				 if(dirLead==-dirLead_saved)
				 {
					 I1rms=-I1rms;
				 }
				 break;
			 default:
				 break;


			 }


			 break;
		 case 3:
			 if(direction==-direction_saved)
			    	 {
			    		 I3rms=-I3rms;
			    	 }


			    	 switch(currentLag_saved)
					 {
			    	 case 2:
			    		 if(dirLag_saved==-dirLag)
			    		 {
			    			 I2rms=-I2rms;
			    		 }
			    		 if(dirLead==-dirLead_saved)
						 {
							 I1rms=-I1rms;
						 }

			    		 break;
			    	 case 1:
			    		 if(dirLag_saved==-dirLag)
						 {
							 I1rms=-I1rms;
						 }

						 if(dirLead==-dirLead_saved)
						 {
							 I2rms=-I2rms;
						 }
						 break;
			    	 default:
			    		 break;


			    	 }

			 break;
		 default:
			 break;

     }

    send[0]=Vrms;
    send[1]=I1rms;
    send[2]=I2rms;
    send[3]=I3rms;



    if((*cal)==1 && Vrms_next>200 && ((Vrms-Vrms_next)<1 || (Vrms-Vrms_next)>-1 ))
    {
    	cnfirm2++;
    	if(cnfirm2>5)
    	{

    		cnfirm2=0;
    		(*cal)=2;
    	}

    }
    else
    {
    	cnfirm2=0;
    }

    Vrms_next=Vrms;

    //send data
     if((*cal)==4)
     {
    	 LoRa_transmit(&myLoRa, send, 16, 500);
    	 HAL_GPIO_TogglePin(GPIOB, GRN_LED_Pin);
    	 sendCount++;

    	 if(sendCount>2){
    		 sendCount=0;
    		 (*cal)=5;

    	 }


     }

	 VsqrdTotal=0;
	 I1sqrdTotal=0;
	 I2sqrdTotal=0;
	 I3sqrdTotal=0;
	(*numOfSample)=0;
	__enable_irq();



}


void buffersFill(uint8_t *cal,uint16_t *cnt)
{
	uint8_t chanlReadCmd =  0x41; //0b01000001;

	uint8_t dummyData2 = 0x00;
	uint8_t dummyData1 = 0x00;

	int8_t data1 = 0x00;
	int8_t data2 = 0x00;
	int8_t data3 = 0x00;
	int8_t data4 = 0x00;
	int8_t data5 = 0x00;
	int8_t data6 = 0x00;
	int8_t data7 = 0x00;
	int8_t data8 = 0x00;

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi1, &chanlReadCmd, &dummyData2, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data1, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data2, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data3, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data4, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data5, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data6, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data7, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &data8, 1, HAL_MAX_DELAY);
	HAL_SPI_TransmitReceive(&hspi1, &dummyData1, &dummyData2, 1, HAL_MAX_DELAY);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

	float VoltageSample= ((data1<< 8) | (data2 & 0xff))* 0.014204;
	volt[(*cnt)]= VoltageSample;


	float CurrentSample1=((data3<<8) | (data4 & 0xff))* 0.00140;
	if(CurrentSample1<0.1 && CurrentSample1>-0.1)
	{
		CurrentSample1=0;
	}
	i1[(*cnt)] = CurrentSample1;

	float CurrentSample2=((data5<<8) | (data6 & 0xff))*0.00140;
	if(CurrentSample2<0.1 && CurrentSample2>-0.1)
	{
		CurrentSample2=0;
	}
	i2[(*cnt)] = CurrentSample2;

	float CurrentSample3=((data7<<8) | (data8 & 0xff))*0.00140;
	if(CurrentSample3<0.1 && CurrentSample3>-0.1)
	{
		CurrentSample3=0;
	}
	i3[(*cnt)] = CurrentSample3;


	(*cnt)++;

	if((*cnt)==2)
		{
			if(volt[0]<=0 && volt[1]>=0 && (volt[0]!=volt[1]))
			{
				//do nothing
			}
			else
			{
				(*cnt)=(*cnt)-2;
			}

		}

	if((*cnt)==300 && (*cal)==5)
	{
		(*cnt)=0;
		(*cal)=6;
	}

}

void powerDirectionDetect(uint8_t *cal,uint16_t *cnt)
{
	voltLag[(*cnt)]=(Vrms*SQRT2)*sin((2*PI*((*cnt))/sampleRate)+(2*PI/3));
	voltLead[(*cnt)]=(Vrms*SQRT2)*sin((2*PI*((*cnt))/sampleRate)-(2*PI/3));





	switch(reference_saved)
	  {
	  case 1:
		  powerSum+=volt[(*cnt)]*i1[(*cnt)];
		  if(currentLag_saved==2)
		  {
			  powerSumLag+=voltLag[(*cnt)]*i2[(*cnt)];
			  powerSumLead+=voltLead[(*cnt)]*i3[(*cnt)];

		  }
		  else
		  {
			  powerSumLag+=voltLag[(*cnt)]*i3[(*cnt)];
			  powerSumLead+=voltLead[(*cnt)]*i2[(*cnt)];

		  }

		  break;
	  case 2:
		  powerSum+=volt[(*cnt)]*i2[(*cnt)];
		  if(currentLag_saved==1)
		  {
			  powerSumLag+=voltLag[(*cnt)]*i1[(*cnt)];
			  powerSumLead+=voltLead[(*cnt)]*i3[(*cnt)];

		  }
		  else
		  {
			  powerSumLag+=voltLag[(*cnt)]*i3[(*cnt)];
			  powerSumLead+=voltLead[(*cnt)]*i1[(*cnt)];

		  }
	  case 3:
		  powerSum+=volt[(*cnt)]*i3[(*cnt)];
		  if(currentLag_saved==1)
		  {
			  powerSumLag+=voltLag[(*cnt)]*i1[(*cnt)];
			  powerSumLead+=voltLead[(*cnt)]*i2[(*cnt)];

		  }
		  else
		  {
			  powerSumLag+=voltLag[(*cnt)]*i2[(*cnt)];
			  powerSumLead+=voltLead[(*cnt)]*i1[(*cnt)];

		  }

		  break;

	  default:
		  //do nothing
		  break;
	 }




	  (*cnt)++;

	 if((*cnt)==300 && (*cal)==6)
	  {
		 (*cnt)=0;
		 __disable_irq();
		 if(powerSum<0)
		 {
			 direction=-1;
		 }
		 else
		 {
			 direction=1;
		 }

		 if(powerSumLag<0)
		 {
			 dirLag=-1;
		 }
		 else
		 {
			 dirLag=1;
		 }
		 if(powerSumLead<0)
		 {
			 dirLead=-1;
		 }
		 else
		 {
			 dirLead=1;
		 }

		 powerSum=0;
		 powerSumLag=0;
		 powerSumLead=0;

		(*cal)=4;

		__enable_irq();

	  }



}


uint8_t dataExchangeMCP3912(uint8_t txData) {
	uint8_t rxdata = 0;

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi1, &txData, &rxdata, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
	return rxdata;
}
