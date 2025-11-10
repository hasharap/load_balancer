/*
 * Flashlib.c
 *
 *  Created on: Jul 3, 2025
 *      Author: hashara
 */


#include "Flashlib.h"


HAL_StatusTypeDef Flash_Write_data(uint32_t address, uint64_t* data, uint32_t length)
{
    HAL_StatusTypeDef status = HAL_OK;

    HAL_FLASH_Unlock();

    for (uint32_t i = 0; i < length; i++)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data[i]);
        if (status != HAL_OK)
            break;

        address += 8;  // Double word size
    }

    HAL_FLASH_Lock();
    return status;
}

HAL_StatusTypeDef Flash_Read_data(uint32_t address, uint64_t* buffer, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        buffer[i] = *(uint64_t*)(address);
        address += 8;
    }

    return HAL_OK;
}

HAL_StatusTypeDef Flash_Read_data_uint8(uint32_t address, uint8_t *buffer, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        buffer[i] = *(volatile uint8_t*)(address + i);
    }
    return HAL_OK;
}


HAL_StatusTypeDef Flash_Erase_Page(uint32_t pageAddress)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError;

    HAL_FLASH_Unlock();

    eraseInit.TypeErase   = FLASH_TYPEERASE_PAGES;
    eraseInit.Page        = (pageAddress - FLASH_BASE) / FLASH_PAGE_SIZE;
    eraseInit.NbPages     = 1;


    status = HAL_FLASHEx_Erase(&eraseInit, &pageError);

    HAL_FLASH_Lock();
    return status;
}
