/*
 * rak3172LoRa.h
 *
 *  Created on: Apr 24, 2025
 *      Author: hashara
 */

#ifndef INC_RAK3172LORA_H_
#define INC_RAK3172LORA_H_

#include "stdbool.h"
#include "main.h"

#define LB_RX_SIZE 32

typedef enum {
    STATE_IDLE,
    STATE_TX_DONE,
	STATE_TX_TIMEOUT,
    STATE_RX_DONE,
    STATE_RX_TIMEOUT,
    STATE_RX_ERROR
} LoRaState_t;

extern volatile LoRaState_t LoRaState;

extern volatile uint8_t LoRaLB_Rx[LB_RX_SIZE];


#define RF_FREQUENCY                                915000000 /* Hz */
#define TX_OUTPUT_POWER                             22
#define LORA_BANDWIDTH                              2         /* [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved] */
#define LORA_SPREADING_FACTOR                       10         /* [SF7..SF12] */
#define LORA_CODINGRATE                             1         /* [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8] */
#define LORA_PREAMBLE_LENGTH                        8         /* Same for Tx and Rx */
#define LORA_SYMBOL_TIMEOUT                         5         /* Symbols */
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false
#define PAYLOAD_LEN                                 64

#define TX_TIMOUT_VALUE                             10000
#define RX_TIMOUT_VALUE                             30000

#define MAX_APP_BUFFEE_SIZE                         255

void RadioInit(void);

#endif /* INC_RAK3172LORA_H_ */



