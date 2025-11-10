/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "rtc.h"
#include "app_subghz_phy.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "radio.h"
#include "stdbool.h"
//#include "radio_board_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define RF_FREQUENCY                                868000000 /* Hz */
//#ifndef TX_OUTPUT_POWER   /* please, to change this value, redefine it in USER CODE SECTION */
#define TX_OUTPUT_POWER                             22
#define LORA_BANDWIDTH                              0         /* [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved] */
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

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
char message[] = "Hello, World!\n";
uint8_t BufferTx[2]={10,23};
char txm[] = "Tx Done\n";
char rxm[] = "Rx Done\n";

char txtmo[]= "Tx Timeout\n";
char rxtmo[]="Rx Timeout\n";
char errradio[]="Radio error\n";


uint8_t rxDone=0;
uint8_t rxTimout=0;
uint8_t txTimout=0;
uint8_t txDone=0;
uint8_t rxerror=0;

radio_status_t test;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void OnTxDone(void) {
  //  HAL_UART_Transmit(&huart2, (uint8_t *)txm, sizeof(txm), HAL_MAX_DELAY);
	txDone=1;
	Radio.Send(BufferTx,64);
    //Radio.Rx(3000); // Restart receiving after transmission
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
//    payload[size] = '\0';  // Null-terminate received data
  //  HAL_UART_Transmit(&huart2, (uint8_t *)rxm, sizeof(rxm), HAL_MAX_DELAY);
   // Radio.Rx(3000);  // Restart listening

    rxDone=1;
}

void OnTxTimeout(void) {
   // HAL_UART_Transmit(&huart2, (uint8_t *)txtmo, sizeof(txtmo), HAL_MAX_DELAY);
	txTimout=1;
	Radio.Send(BufferTx,sizeof(BufferTx));
}


void OnRxTimeout(void) {
   // HAL_UART_Transmit(&huart2, (uint8_t *)rxtmo, sizeof(rxtmo), HAL_MAX_DELAY);
//    Radio.Rx(0);  // Restart listening
	Radio.Rx(RX_TIMOUT_VALUE);
	rxTimout=1;
}

void OnRxError(void){
	//HAL_UART_Transmit(&huart2, (uint8_t *)errradio, sizeof(errradio), HAL_MAX_DELAY);
//	Radio.Rx(0);  // Restart listening
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
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SubGHz_Phy_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */


  RadioInit();
  HAL_Delay(1000);


//Radio.Send((uint8_t *)BufferTx,sizeof(BufferTx));
  Radio.Rx(RX_TIMOUT_VALUE);
//  Radio.RxBoosted(RX_TIMOUT_VALUE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	//  HAL_Delay(1000);
//	  rxDone=0;
//	  rxTimout=0;
//	  txTimout=0;
//	  txDone=0;
//	  rxerror=0;



    /* USER CODE END WHILE */
    MX_SubGHz_Phy_Process();

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK3|RCC_CLOCKTYPE_HCLK
                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK3Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
