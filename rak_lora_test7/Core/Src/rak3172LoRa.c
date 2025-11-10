/*
 * rak3172LoRa.c
 *
 *  Created on: Apr 24, 2025
 *      Author: hashara
 */


#include "rak3172LoRa.h"
#include "radio.h"
#include "string.h"
#include "main.h"


uint8_t rxDone   = 0;
uint8_t rxTimout = 0;
uint8_t txTimout = 0;
uint8_t txDone   = 0;
uint8_t rxerror  = 0;

int16_t rssiVal;
int8_t snrVal;
uint8_t message[6];
char tx[]="hi hi";

void OnTxDone(void)
{
	HAL_Delay(100);
	Radio.Send((uint8_t*)tx,sizeof(tx));
	txDone=1;
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
	rssiVal=rssi;
	snrVal=snr;
    memcpy(message,payload,sizeof(message));
    rxDone=1;
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);

}

void OnTxTimeout(void)
{
	txTimout=1;

}


void OnRxTimeout(void)
{
	rxTimout=1;
}

void OnRxError(void)
{

	rxerror=1;
}



void RadioInit(void) {
    static RadioEvents_t RadioEvents;  // Make sure it's static or global

    // Assign the callback functions
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;

    // Initialize the radio with the events
    Radio.Init(&RadioEvents);

    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetTxConfig(MODEM_LORA,TX_OUTPUT_POWER,0,LORA_BANDWIDTH,LORA_SPREADING_FACTOR,LORA_CODINGRATE,
    		LORA_PREAMBLE_LENGTH,LORA_FIX_LENGTH_PAYLOAD_ON,true,0,0,LORA_IQ_INVERSION_ON,TX_TIMOUT_VALUE);

    Radio.SetRxConfig(MODEM_LORA,LORA_BANDWIDTH,LORA_SPREADING_FACTOR,LORA_CODINGRATE,0,
    		LORA_PREAMBLE_LENGTH,LORA_SYMBOL_TIMEOUT,LORA_FIX_LENGTH_PAYLOAD_ON,0,true,0,0,LORA_IQ_INVERSION_ON,true);

    Radio.SetMaxPayloadLength(MODEM_LORA,MAX_APP_BUFFEE_SIZE);

}


