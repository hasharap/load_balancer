/*
 *
 *
 *
 *
 * mcp3912Lib.h
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



#ifndef __MCP3912LIB_H_
#define __MCP3912LIB_H_

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "main.h"
#include "stdbool.h"


#define WINDOW_SIZE 500
#define SAMPLE_SIZE 3000
#define SAMPLE_RATE 98
#define SPI_TX_SIZE 13
#define SPI_RX_SIZE 13

extern volatile uint16_t bufferIndex;

extern float VoltB[WINDOW_SIZE];
extern float VoltLagB[WINDOW_SIZE];
extern float VoltLeadB[WINDOW_SIZE];

extern float I1B[WINDOW_SIZE];
extern float I2B[WINDOW_SIZE];
extern float I3B[WINDOW_SIZE];


struct PhaseDataStruct
{
	float Vrms;
	float I1rms;
	float I2rms;
	float I3rms;

};


struct Signs
{
	int i1s;
	int i2s;
	int i3s;
};

extern volatile struct PhaseDataStruct PhaseData;
extern volatile struct Signs  mark;


typedef enum {
    PHASE_NONE = 0,
    PHASE_MATCH = 1,
    PHASE_LAG = 2,
    PHASE_LEAD = 3
} PhaseType;

typedef enum {
    POWER_UNKNOWN = 0,
    POWER_FORWARD = 1,
    POWER_REVERSE = 2
} PowerDirection;

typedef struct {
    PhaseType currentRead_1;
    PhaseType currentRead_2;
    PhaseType currentRead_3;
} PhaseRelation;

typedef struct {
	PowerDirection ofcurrentRead_1;
	PowerDirection ofcurrentRead_2;
	PowerDirection ofcurrentRead_3;
}Directions;


typedef enum {
	NotApper,
	Sp1,
	Sp2,
	Sp3,
	Tp
} LoadBalancer_t;

extern LoadBalancer_t LB_type;


void mcp3912begin();

PhaseType findMatchingPhase(float *v, float *i1, float *i2, float *i3, uint16_t size);

void SwapToProcess(void);

void readChannels();

LoadBalancer_t DetectLBType(struct PhaseDataStruct data , float threshold);

PowerDirection calculateDirection(float *v, float *i, uint16_t size);


float* getVoltageBuffer(PhaseType phase);

uint8_t getbufferNumber(PhaseType phase);

bool directionsConfirm(Directions d1, Directions d2);

bool PhasesConfirm(PhaseRelation p1, PhaseRelation p2);

void updateMark(uint8_t readingNumber, PowerDirection currentDir, PowerDirection defaultDir) ;


#endif /* __MCP3912LIB_H_ */
