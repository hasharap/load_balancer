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








void phaseDetect(uint8_t *cal ,uint16_t *cnt);
void setupMCP3912();
void mcp3912begin();
void readChannels(uint8_t *cal,uint16_t *cnt);
void channelReadingsSend(uint8_t *cal,uint16_t *numOfSample);
void transferInit();
void currentLagDetect(uint8_t *cal,uint16_t *cnt);
void buffersFill(uint8_t *cal,uint16_t *cnt);
void powerDirectionDetect(uint8_t *cal,uint16_t *cnt);
void PowerDirSave(uint8_t *cal,uint16_t *cnt);
void PowerDirDetect(uint8_t *cal,uint16_t *cnt);
void TransmitReadings();
uint8_t dataExchangeMCP3912(uint8_t txData);

#endif /* __MCP3912LIB_H_ */
