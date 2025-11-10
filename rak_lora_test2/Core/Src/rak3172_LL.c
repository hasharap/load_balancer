/*
 * rak3172_LL.c
 *
 *  Created on: Apr 10, 2025
 *      Author: hashara
 */




#include "rak3172_LL.h"


HAL_StatusTypeDef Set_BufferBaseAddress(SUBGHZ_HandleTypeDef *hsubghz ,uint8_t TxBaseAddr ,uint8_t RxBaseAddr)
{
	uint8_t buf[2];
	buf[0]=TxBaseAddr;
	buf[1]=RxBaseAddr;
	return HAL_SUBGHZ_ExecSetCmd(hsubghz,RADIO_SET_BUFFERBASEADDRESS,buf,2);
}


HAL_StatusTypeDef write_Payload_TxData(SUBGHZ_HandleTypeDef *hsubghz, uint8_t offset ,uint8_t *pBuffer , uint16_t size)
{
	return HAL_SUBGHZ_WriteBuffer(hsubghz, 0x00, pBuffer, size);

}


HAL_StatusTypeDef Set_PacketType_LoRa(SUBGHZ_HandleTypeDef *hsubghz)
{
	uint8_t buf[0]={0x01};
	return HAL_SUBGHZ_ExecSetCmd(hsubghz,RADIO_SET_PACKETTYPE, &buf,1);
}

HAL_StatusTypeDef  Set_PacketParams(SUBGHZ_HandleTypeDef *hsubghz)
{
	  uint8_t RadioParam[6];
	  RadioParam[0] = 0x00U; // PbLength MSB - 12-symbol-long preamble sequence
	  RadioParam[1] = 0x0CU; // PbLength LSB - 12-symbol-long preamble sequence
	  RadioParam[2] = 0x00U; // explicit header type
	  RadioParam[3] = 0x40U; // 64 bit packet length.
	  RadioParam[4] = 0x01U; // CRC enabled
	  RadioParam[5] = 0x00U; // standard IQ setup


	 return HAL_SUBGHZ_ExecSetCmd(hsubghz, RADIO_SET_PACKETPARAMS, RadioParam, 6);
}





