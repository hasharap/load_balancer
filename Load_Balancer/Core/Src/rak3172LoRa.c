/*
 * rak3172LoRa.c
 *
 *  Created on: Apr 24, 2025
 *      Author: hashara
 */


#include "rak3172LoRa.h"
#include "radio.h"
#include "mcp3912Lib.h"
#include "string.h"


int16_t rssiVal;
int8_t snrVal;



volatile uint8_t LoRaLB_Rx[LB_RX_SIZE];

volatile LoRaState_t LoRaState = STATE_IDLE;




void OnTxDone(void)
{
	Radio.Sleep();
	LoRaState = STATE_TX_DONE;
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
	Radio.Sleep();
	rssiVal=rssi;
	snrVal=snr;


    memcpy(LoRaLB_Rx,payload,sizeof(LoRaLB_Rx));
    LoRaState = STATE_RX_DONE;


}

void OnTxTimeout(void)
{
	Radio.Sleep();
	LoRaState = STATE_TX_TIMEOUT;
}


void OnRxTimeout(void)
{
	Radio.Sleep();
	LoRaState = STATE_RX_TIMEOUT;

}

void OnRxError(void)
{
	Radio.Sleep();
	LoRaState = STATE_RX_ERROR;
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


