/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

	#include "mh-z14_sm.h"
	#include "lcd1602_fc113_sm.h"
	#include "ringbuffer_dma_sm.h"
	#include "average_calc_3_from_5.h"
	#include "one-pin-debug-sm.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

	#define CIRCLE_QNT 5
	#define ADR_I2C_FC113 0x27
	#define SOFT_VERSION 	120

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

	extern		OnePin_Debug_struct		hOnePin;
	uint32_t co2_u32[CIRCLE_QNT];
	char uart_buff_char[100];

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

	OnePin_Init(  ONE_DEBUG_GPIO_Port, ONE_DEBUG_Pin, 1) ;

	sprintf( uart_buff_char, "\r\n\r\n\t Start:\r\n"  ) ;
	One_pin_debug_print ( (uint8_t *)uart_buff_char, strlen(uart_buff_char) ) ;

	int soft_version_arr_int[3];
	soft_version_arr_int[0] = ((SOFT_VERSION) / 100) %10 ;
	soft_version_arr_int[1] = ((SOFT_VERSION) /  10) %10 ;
	soft_version_arr_int[2] = ((SOFT_VERSION)      ) %10 ;

	sprintf( uart_buff_char, "\t Ver: v%d.%d.%d\r\n", soft_version_arr_int[0], soft_version_arr_int[1], soft_version_arr_int[2] ) ;
	One_pin_debug_print ( (uint8_t *)uart_buff_char, strlen(uart_buff_char) ) ;

	#define 	DATE_as_int_str 	(__DATE__)
	#define 	TIME_as_int_str 	(__TIME__)
	sprintf(uart_buff_char,"\t build: %s,  time: %s. \r\n" , DATE_as_int_str , TIME_as_int_str ) ;
	One_pin_debug_print ( (uint8_t *)uart_buff_char, strlen(uart_buff_char) ) ;

	HAL_TIM_Base_Start_IT(&htim3);
	MH_Z14A_Init( );

	lcd1602_fc113_struct h1_lcd1602_fc113 =
		{
			.i2c = &hi2c1,
			.device_i2c_address = ADR_I2C_FC113
		};

	LCD1602_Init(&h1_lcd1602_fc113);
	LCD1602_Scan_I2C_bus( &h1_lcd1602_fc113 ) ;
	//LCD1602_Scan_I2C_to_UART( &h1_lcd1602_fc113, &huart1 ) ;
	LCD1602_Clear(&h1_lcd1602_fc113);

	sprintf(uart_buff_char,"LCD1602 Started\r\n");
	LCD1602_Print_Line(&h1_lcd1602_fc113, uart_buff_char, strlen(uart_buff_char));
	HAL_Delay(1000);

	LCD1602_Clear(&h1_lcd1602_fc113);
	sprintf(uart_buff_char,"connect WiFi...\r\n");
	LCD1602_Print_Line(&h1_lcd1602_fc113, uart_buff_char, strlen(uart_buff_char));

	RingBuffer_DMA_Connect( );	// todo wait why answer is: "busy p..."

	LCD1602_Clear(&h1_lcd1602_fc113);
	sprintf(uart_buff_char,"WiFi Started\r\n");
	LCD1602_Print_Line(&h1_lcd1602_fc113, uart_buff_char, strlen(uart_buff_char));

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
while (1)
{
	static uint32_t sec_counter_u32 ;
	static uint8_t	time_to_work_u8 ;
	static uint8_t 	circle_u8 = 0 		;

	if (GetTimeFlag() == 1) {
		HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
		sec_counter_u32++;

		sprintf(uart_buff_char,"  %d) wait %02d \r", (int)(CIRCLE_QNT-circle_u8), (int)(60 - sec_counter_u32) ) ;
		One_pin_debug_print ( (uint8_t *)uart_buff_char, strlen(uart_buff_char) ) ;
		LCD1602_Clear(&h1_lcd1602_fc113);
		LCD1602_Print_Line(&h1_lcd1602_fc113, uart_buff_char, strlen(uart_buff_char));

		if (sec_counter_u32 > 59 )
		{
			time_to_work_u8 = 1 ;
			sec_counter_u32 = 0;
		}
		SetTimeFlag(0);
	}

	if (time_to_work_u8 == 1 ) {
		time_to_work_u8 = 0 ;
		if (circle_u8 < CIRCLE_QNT) {
			co2_u32[circle_u8] = MH_Z14A_Main( );
			sprintf(uart_buff_char,"CO2: %d ppm\r\n", (int)co2_u32[circle_u8]);
			LCD1602_Print_Line(&h1_lcd1602_fc113, uart_buff_char, strlen(uart_buff_char));
			One_pin_debug_print( (uint8_t *)uart_buff_char, strlen(uart_buff_char) ) ;
			circle_u8++;
		}

		if (circle_u8 == CIRCLE_QNT) {
			uint32_t total_co2_u32 = Calc_Average( co2_u32, CIRCLE_QNT);

			sprintf(uart_buff_char,"total CO2: %d\r\n", (int)total_co2_u32);
			LCD1602_Clear(&h1_lcd1602_fc113);
			LCD1602_Print_Line(&h1_lcd1602_fc113, uart_buff_char, strlen(uart_buff_char));

			char http_req[200];
			sprintf(http_req, "&field7=%d\r\n\r\n", (int)total_co2_u32 );
			RingBuffer_DMA_Main( http_req ) ;	//	todo If answer is: "SEND FAIL"
			circle_u8 = 0;
		}
	}

    /* USER CODE END WHILE */

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

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
  while(1)
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
