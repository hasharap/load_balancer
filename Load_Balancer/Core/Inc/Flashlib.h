/*
 * Flashlib.h
 *
 *  Created on: Jul 3, 2025
 *      Author: hashara
 */

#ifndef INC_FLASHLIB_H_
#define INC_FLASHLIB_H_

#include "stm32wlxx_hal.h"
#include <stdint.h>



HAL_StatusTypeDef Flash_Write_data(uint32_t address, uint64_t* data, uint32_t length);
HAL_StatusTypeDef Flash_Read_data(uint32_t address, uint64_t* buffer, uint32_t length);
HAL_StatusTypeDef Flash_Erase_Page(uint32_t pageAddress);
HAL_StatusTypeDef Flash_Read_data_uint8(uint32_t address, uint8_t *buffer, uint32_t length);


#endif /* INC_FLASHLIB_H_ */
