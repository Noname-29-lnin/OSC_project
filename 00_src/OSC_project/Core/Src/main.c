/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "i2c-lcd.h"
#include "OSC_LIB_H.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// Dinh nghia ham read and write
#define __rw volatile
#define __r  volatile
#define __w  const volatile
#define DEBOUNCE_TIME 20 // thoi gian chong rung (ms)

OSC_FLAG_s   	FLAG  = {0};
OSC_VALUE_s 	VALUE = {
    .RANGE = 0,       // Default measurement range
    .HEXAC = 0,         // Default hexadecimal value
		.HEXDC = 0,         // Default hexadecimal value
    .VMAX = 0,        // Initial maximum peak value
    .VMIN = 0xFFFFFFFF, // Initial minimum peak value set to maximum possible
    .ZERO = 0,        // Initial zero crossing point
    .VRMS = 0.0,      // Default RMS voltage
    .VPP = 0.0,       // Default Peak-to-Peak voltage
    .FREQ = 0.0,      // Default frequency
    .VDC = 0.0        // Default DC voltage
};
OSC_TEMP_s TEMP  = {0};
volatile uint8_t status = STATUS_IDLE;
uint16_t OSC_Buf[ADC_BUFFER_SIZE];
volatile uint32_t last_setup_press_time = 0;
volatile uint32_t last_mode_press_time 	= 0;
volatile uint32_t last_start_press_time = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HandleError(void);

// =================== EXIT IQR ===================
/**
  * @brief  Callback to handle external interrupts triggered by GPIO pins.
  * @param  GPIO_Pin: The GPIO pin that triggered the interrupt.
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	
	if(GPIO_Pin == SETUP_Pin) {
		HandleButtonSetup();
	} else if(GPIO_Pin == MODE_Pin){
		HandleButtonMode();
	} else if(GPIO_Pin == START_Pin){
		HandleButtonStart();
	}
	
}

// =================== ADC IQR ===================
/**
  * @brief  ADC conversion complete callback.
  *         This function is triggered when an ADC conversion is complete.
  *         It updates signal metrics (max, min, zero-crossing count) based on the sampled ADC value.
  * @param  hadc: Pointer to the ADC handle that triggered the callback.
  */
//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
//    if (hadc->Instance == hadc1.Instance) {
//		
//		for (int i = 0; i < sizeof(OSC_Buf); i++) {
//        uint16_t adc_value = OSC_Buf[i];
//			if (adc_value > VALUE.VMAX) {
//				VALUE.VMAX = adc_value;
//      } else if (adc_value < VALUE.VMIN) {
//				VALUE.VMIN = adc_value;
//			}

//			if ((adc_value >= (adc_zero - threshold)) && (adc_value <= (adc_zero + threshold))) {
//				VALUE.ZERO++; // Tang bi?n d?m c?t qua không
//			}
//		}
//	}
//}
// =================== Timer IQR ===================
/**
  * @brief  Timer Period Elapsed Callback.
  *         Handles actions based on different timer instances.
  * @param  htim: Pointer to the TIM handle.
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) { 		// 1ms
			FLAG.TIMER2 = 1;
//			FLAG.TIMER3 = 0;
			if(status == STATUS_START && FLAG.START == 1 && FLAG.STOP == 0){
				if(FLAG.AC == 1 && FLAG.DC == 0){
					VALUE.HEXAC = OSC_Buf[0];
					CDC_Transmit_FS((uint8_t *)VALUE.HEXAC, sizeof(VALUE.HEXAC));
				} else if(FLAG.AC == 0 && FLAG.DC == 1){
					CDC_Transmit_FS((uint8_t *)VALUE.HEXDC, sizeof(VALUE.HEXDC));
				}
			}
    } 
		else if (htim->Instance == TIM3) { // 1s
			FLAG.TIMER3 = 1;
//			FLAG.TIMER2 = 0;
    }
}

void HandleStart(void){
	HAL_Delay(500);
	HD44780_Clear();
	
	if (FLAG.AC == 1 && FLAG.DC == 0) {
		FLAG.ERROR  = 0;
		HAL_GPIO_WritePin(MODE_CONT_GPIO_Port, MODE_CONT_Pin, GPIO_PIN_RESET);
		UpdateLCDStartAC();
			while (status == STATUS_START && FLAG.START == 1 && FLAG.STOP == 0){
				UpdateACValues();
				if(FLAG.TIMER2 == 1){ // ngat o 1ms
					FLAG.TIMER2 = 0;
					uint16_t adc_zero = 2048;
					uint16_t threshold = 102;
					for(int i = 0; i < ADC_BUFFER_SIZE; i++){
						uint16_t adc_value = OSC_Buf[i];
						if(adc_value > VALUE.VMAX){
							VALUE.VMAX = adc_value;
						} else if(adc_value < VALUE.VMIN){
							VALUE.VMIN = adc_value;
						}
						
						if((adc_value >= (adc_zero - threshold)) && (adc_value <= (adc_zero + threshold))){
							VALUE.ZERO ++;
						}
					}
					VALUE.VPP	 = ValueVpp(VALUE.VMAX, VALUE.VMIN, VALUE.RANGE);
					VALUE.VMAX = 0;          // Reset maximum ADC value
					VALUE.VMIN = 0xFFFFFFFF; // Reset minimum ADC value to the highest possible
				}
				else if(FLAG.TIMER3 == 1){
					FLAG.TIMER3 = 0;
					VALUE.FREQ = ValueFreq(VALUE.ZERO, 1);
					// Reset the accumulated values in VALUE structure
					VALUE.ZERO = 0;          // Reset zero-crossing count
				}
			}	
	} else if (FLAG.DC == 1 && FLAG.AC == 0) {
		FLAG.ERROR  = 0;
		HAL_GPIO_WritePin(MODE_CONT_GPIO_Port, MODE_CONT_Pin, GPIO_PIN_SET);
		UpdateLCDStartDC();
			while (status == STATUS_START && FLAG.START == 1 && FLAG.STOP == 0){
					UpdateDCValues();
						HAL_ADC_Start(&hadc2);
						HAL_ADC_PollForConversion(&hadc2, 100);
						VALUE.HEXDC = HAL_ADC_GetValue(&hadc2);
						HAL_ADC_Stop(&hadc2);
						VALUE.VDC  = ValueDc(VALUE.HEXDC, VALUE.RANGE);
					HAL_Delay(500);
				}
	} else if(FLAG.DC == 0 && FLAG.AC == 0) {
			FLAG.ERROR = 1;
			performUnkown();
			HAL_Delay(1000);
	}
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
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_I2C1_Init();
  MX_USB_DEVICE_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
	HD44780_Init(2);
	HAL_Delay(500);
	OSC_Init();
		// ------------- Start system ------------- 
		while(1){
			switch(status){
				case STATUS_IDLE:
						while(FLAG.SETUP != 1){}
//						status = STATUS_SETUP;
					break;
							
				case STATUS_SETUP:
					performSetup();			// Hien thi Mode SETUP
					HandleSetup();			// Cho co MODE bat len
//					status = STATUS_MODE;
					break;
					
				case STATUS_MODE:
					performMode();			// Hien thi Mode MODE
					HAL_TIM_Base_Stop_IT(&htim2); 
					HAL_TIM_Base_Stop_IT(&htim3);
					HAL_GPIO_WritePin(MODE_CONT_GPIO_Port, MODE_CONT_Pin, GPIO_PIN_RESET);
				
					if(FLAG.ERROR == 1) {
						HandleError();
						status = STATUS_IDLE;
						break;
					} else if(FLAG.UNUSED == 1){
						performUnkown();
						HAL_Delay(500);
					} else {
						HandleMode();				// Xu li ham o mode MODE, out khi co xuat hien cua trang thai START
					}
					break;
					
				case STATUS_START:
					HAL_ADC_Start_DMA(&hadc1, (uint32_t *)OSC_Buf, sizeof(OSC_Buf));
					
					if(FLAG.ERROR == 1) {
						HandleError();
						status = STATUS_IDLE;
					} else if(FLAG.UNUSED == 1){
						performUnkown();
						HAL_Delay(500);
					}else if(FLAG.START == 1 && FLAG.STOP == 0){
						HAL_TIM_Base_Start_IT(&htim2);
						HAL_TIM_Base_Start_IT(&htim3);
						performStart();
						HandleStart();
					} else if(FLAG.START == 0 && FLAG.STOP == 1){
						performStop();
						HandleStop();
					}
					break;
					
				default:
					FLAG.UNUSED = 1; // Khong xac dinh duoc stage hoat dong
					performUnkown();
					HAL_Delay(500);
					status = STATUS_IDLE;
					break;
			}
		}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_USB;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV4;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_41CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.ContinuousConvMode = ENABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_SlaveConfigTypeDef sSlaveConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 10;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 3272;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_DISABLE;
  sSlaveConfig.InputTrigger = TIM_TS_ITR0;
  if (HAL_TIM_SlaveConfigSynchro(&htim2, &sSlaveConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */
	
  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 548;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
	hdma_adc1.Init.Mode = DMA_CIRCULAR;
	
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MODE_CONT_GPIO_Port, MODE_CONT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : START_Pin MODE_Pin SETUP_Pin */
  GPIO_InitStruct.Pin = START_Pin|MODE_Pin|SETUP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : AC_RANGE_Pin DC_RANGE_Pin */
  GPIO_InitStruct.Pin = AC_RANGE_Pin|DC_RANGE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : MODE_CONT_Pin */
  GPIO_InitStruct.Pin = MODE_CONT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(MODE_CONT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MODE_PRO_Pin */
  GPIO_InitStruct.Pin = MODE_PRO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(MODE_PRO_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// Function definitions

void HandleError(void) {
    // Handle error condition
    HAL_ADC_Stop_DMA(&hadc1);
		HAL_TIM_Base_Stop_IT(&htim2);
		HAL_TIM_Base_Stop_IT(&htim3);

    // Display error message on LCD
    HD44780_Clear();
    HD44780_SetCursor(0, 0);
    HD44780_PrintStr("Error Detected!");
    HD44780_SetCursor(0, 1);
    HD44780_PrintStr("System Halted");
	
		HAL_Delay(1000);
		
		HD44780_Clear();
    HD44780_SetCursor(0, 0);
    HD44780_PrintStr("Please SETUP");
    HD44780_SetCursor(0, 1);
    HD44780_PrintStr("Wait ...");
}
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
