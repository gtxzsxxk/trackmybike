/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define GUI_BIGWORD_LINE 4
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

IWDG_HandleTypeDef hiwdg;

TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* Definitions for screenTask */
osThreadId_t screenTaskHandle;
const osThreadAttr_t screenTask_attributes = {
  .name = "screenTask",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for dataTrans */
osThreadId_t dataTransHandle;
const osThreadAttr_t dataTrans_attributes = {
  .name = "dataTrans",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for posLedTask */
osThreadId_t posLedTaskHandle;
const osThreadAttr_t posLedTask_attributes = {
  .name = "posLedTask",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* USER CODE BEGIN PV */
const uint8_t BIKENAME[]="XXXX";
const uint8_t SERVER_LICENSE[]="XXX";
const uint8_t SERVER[]="XXXXXX";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_IWDG_Init(void);
static void MX_ADC_Init(void);
static void MX_TIM6_Init(void);
void screenFunc(void *argument);
void dataTransFunc(void *argument);
void posLedFunc(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t u1_rev_dat;
uint16_t u1_rev_cnt=0;
uint8_t u1_rev_on=0;
uint8_t u1_single_cmd[200];

uint8_t u2_rev_dat,u2_rev_last;
uint8_t u2_rev_on=0;
uint16_t u2_rev_cnt=0;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance==huart1.Instance){
		if(u1_rev_dat=='$'){
			u1_rev_on=1;
			u1_rev_cnt=0;
		}
		else if(u1_rev_dat=='\r'&&u1_rev_on){
			u1_rev_on=0;
			gps_msg_read(u1_single_cmd);
			u1_rev_cnt=0;
			memset(u1_single_cmd,0,sizeof(u1_single_cmd));
		}
		if(u1_rev_on){
			u1_single_cmd[u1_rev_cnt++]=u1_rev_dat;
		}
		HAL_UART_Receive_IT(huart, &u1_rev_dat, 1);
	}
	else if(huart->Instance==huart2.Instance){
		if(sim800l_data_summary.wait_until_ok_mode){
			if(u2_rev_last=='O'&&u2_rev_dat=='K'){
				sim800l_data_summary.wait_until_ok_mode=0;
				sim800l_data_summary.wait_until_ok_flag=1;
			}
		}
		if(sim800l_data_summary.rev_sync_mode){
			sim800l_data_summary.rev_sync_buffer[sim800l_data_summary.rev_sync_cnt++]=u2_rev_dat;
			if(sim800l_data_summary.rev_sync_cnt>=sizeof(sim800l_data_summary.rev_sync_buffer)){
				sim800l_data_summary.rev_sync_cnt=0;
				sim800l_data_summary.rev_sync_mode=0;
				memset(sim800l_data_summary.rev_sync_buffer,0,sizeof(sim800l_data_summary.rev_sync_buffer));
			}
		}
		if(u2_rev_dat=='&'){
			u2_rev_on=1;
		}
		else if(u2_rev_dat=='\r'&&u2_rev_on){
			u2_rev_on=0;
			sim800l_server_cmd_resolve();
			sim800l_data_summary.rev_cmd_cnt=0;
			memset(sim800l_data_summary.rev_cmd_buffer,0,sizeof(sim800l_data_summary.rev_cmd_buffer));
		}
		if(u2_rev_on){
			sim800l_data_summary.rev_cmd_buffer[sim800l_data_summary.rev_cmd_cnt++]=u2_rev_dat;
		}
		u2_rev_last=u2_rev_dat;
		HAL_UART_Receive_IT(huart, &u2_rev_dat, 1);
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
	if(READ_BIT(huart->Instance->ISR,UART_FLAG_NE)){
		__HAL_UART_CLEAR_NEFLAG(huart);
	}
	else if(READ_BIT(huart->Instance->ISR,UART_FLAG_ORE)){
		__HAL_UART_CLEAR_OREFLAG(huart);
	}
	else if(READ_BIT(huart->Instance->ISR,UART_FLAG_FE)){
		__HAL_UART_CLEAR_FEFLAG(huart);
	}
	else if(READ_BIT(huart->Instance->ISR,UART_FLAG_PE)){
		__HAL_UART_CLEAR_PEFLAG(huart);
	}
}

void GUI_BOOT_REGISTER_NET(){
	LCD_Clean();
	l_print("Powered By", 0, Middle);
	LCD_draw_freertos(3,1);
	LCD_draw_big(3,GUI_BIGWORD_LINE,6);
	LCD_draw_big(5,GUI_BIGWORD_LINE,7);
	LCD_draw_big(7,GUI_BIGWORD_LINE,8);
	LCD_draw_big(9,GUI_BIGWORD_LINE,9);
}

void GUI_BOOT_TCP_CONNECT(){
	LCD_Clean();
	l_print("Powered By", 0, Middle);
	LCD_draw_freertos(3,1);
	LCD_draw_big(2,GUI_BIGWORD_LINE,10);
	LCD_draw_big(4,GUI_BIGWORD_LINE,11);
	LCD_draw_big(6,GUI_BIGWORD_LINE,12);
	LCD_draw_big(8,GUI_BIGWORD_LINE,13);
	LCD_draw_big(10,GUI_BIGWORD_LINE,14);
}

void GUI_BOOT_TCP_FAIL(){
	LCD_Clean();
	l_print("Powered By", 0, Middle);
	LCD_draw_freertos(3,1);
	LCD_draw_big(0,GUI_BIGWORD_LINE,10);
	LCD_draw_big(2,GUI_BIGWORD_LINE,11);
	LCD_draw_big(4,GUI_BIGWORD_LINE,12);
	LCD_draw_big(6,GUI_BIGWORD_LINE,13);
	LCD_draw_big(8,GUI_BIGWORD_LINE,14);
	LCD_draw_big(10,GUI_BIGWORD_LINE,15);
	LCD_draw_big(12,GUI_BIGWORD_LINE,16);
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
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
//  MX_IWDG_Init();
  MX_ADC_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */

  HAL_UART_Receive_IT(&huart1, &u1_rev_dat, 1);
  HAL_UART_Receive_IT(&huart2, &u2_rev_dat, 1);
  HAL_GPIO_WritePin(LCD_BK_GPIO_Port, LCD_BK_Pin, 1);
  LCD_Init();
  HAL_Delay(3000);
  sim800l_init();
  MX_IWDG_Init();
  HAL_TIM_Base_Start_IT(&htim6);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of screenTask */
  screenTaskHandle = osThreadNew(screenFunc, NULL, &screenTask_attributes);

  /* creation of dataTrans */
  dataTransHandle = osThreadNew(dataTransFunc, NULL, &dataTrans_attributes);

  /* creation of posLedTask */
  posLedTaskHandle = osThreadNew(posLedFunc, NULL, &posLedTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_8;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.OversamplingMode = DISABLE;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.SamplingTime = ADC_SAMPLETIME_39CYCLES_5;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerFrequencyMode = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
  hiwdg.Init.Window = 4095;
  hiwdg.Init.Reload = 4000;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 4000-1;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 800-1;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT;
  huart1.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT;
  huart2.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, POS_LED_Pin|LCD_DIN_Pin|LCD_RST_Pin|LCD_DC_Pin
                          |LCD_CE_Pin|LCD_BK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_SCK_GPIO_Port, LCD_SCK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : POS_LED_Pin LCD_DIN_Pin LCD_RST_Pin LCD_DC_Pin
                           LCD_CE_Pin LCD_BK_Pin */
  GPIO_InitStruct.Pin = POS_LED_Pin|LCD_DIN_Pin|LCD_RST_Pin|LCD_DC_Pin
                          |LCD_CE_Pin|LCD_BK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : GPS_PPS_Pin */
  GPIO_InitStruct.Pin = GPS_PPS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPS_PPS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_SCK_Pin */
  GPIO_InitStruct.Pin = LCD_SCK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_SCK_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SIM_WKUP_Pin */
  GPIO_InitStruct.Pin = SIM_WKUP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SIM_WKUP_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_screenFunc */
const uint8_t week2str[][4]={"err","mon","tue","wed","thu","fri","sat","sun"};
const uint8_t hdg_direction[][14]={"NORTH    ","NORTHEAST","EAST     ",
		"SOUTHEAST","SOUTH    ","SOUTHWEST","WEST     ","NORTHWEST","ERROR    "};
uint8_t get_heading_direction(){
	uint8_t gap=20;
	if(gps_data_summary.heading_int<gap||gps_data_summary.heading_int>=360-gap){
		return 0;
	}
	else if(gps_data_summary.heading_int>=gap&&gps_data_summary.heading_int<90-gap){
		return 1;
	}
	else if(gps_data_summary.heading_int>=90-gap&&gps_data_summary.heading_int<90+gap){
		return 2;
	}
	else if(gps_data_summary.heading_int>=90+gap&&gps_data_summary.heading_int<180-gap){
		return 3;
	}
	else if(gps_data_summary.heading_int>=180-gap&&gps_data_summary.heading_int<180+gap){
		return 4;
	}
	else if(gps_data_summary.heading_int>=180+gap&&gps_data_summary.heading_int<270-gap){
		return 5;
	}
	else if( gps_data_summary.heading_int>=270-gap&&gps_data_summary.heading_int<270+gap){
		return 6;
	}
	else if( gps_data_summary.heading_int>=270+gap&&gps_data_summary.heading_int<360-gap){
		return 7;
	}
	else{
		return 8;
	}
}

void fillstr_bynum(uint8_t* buffer,int num){
	if(num>=10){
		buffer[0]=num/10+'0';
	}else{
		buffer[0]='0';
	}
	buffer[1]=num%10+'0';
}
/**
  * @brief  Function implementing the screenTask thread.
  * @param  argument: Not used
  * @retval None
  */
uint8_t battery_voltage_str[5]="0.0";
float battery_voltage=0;
float battery_sample_K=0.05;
#define SCREEN_BAT_MILE_DISP_INTERVAL 10
/* USER CODE END Header_screenFunc */
void screenFunc(void *argument)
{
  /* USER CODE BEGIN 5 */
	uint8_t current_mode=0,tm_loop_cnt=0,bat_disp_cnt=0;
	uint8_t headline[10]="thu 14:03";
	uint8_t last_stage=0;
	osDelay(100);
	HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED);
  /* Infinite loop */
  for(;;)
  {
	HAL_ADC_Start(&hadc);
	HAL_ADC_PollForConversion(&hadc, 50);
	if(HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc),HAL_ADC_STATE_EOC_REG)){
		float adc_value=((float)HAL_ADC_GetValue(&hadc))/4095.0f*3.3*3;
		if(battery_voltage==0){
			battery_voltage=adc_value;
		}else{
			battery_voltage=battery_sample_K*adc_value+(1-battery_sample_K)*battery_voltage;
		}
	}
	if(current_mode!=gps_data_summary.parked_mode){
	  LCD_Clean();
	}
	LCD_SetXY(0,0);
	g_puts(WORD_TOWER_SIM);
	g_puts(WORD_SIGNAL(sim800l_data_summary.signal_level));
	g_puts(WORD_TOWER_GPS);
	g_puts(WORD_SIGNAL(gps_data_summary.signal_level*gps_data_summary.connected));
	headline[0]=week2str[sim800l_data_summary.net_week][0];
	headline[1]=week2str[sim800l_data_summary.net_week][1];
	headline[2]=week2str[sim800l_data_summary.net_week][2];
	fillstr_bynum(headline+4,sim800l_data_summary.net_hour);
	fillstr_bynum(headline+7,sim800l_data_summary.net_min);
	if(tm_loop_cnt==2){
		if(headline[6]==':'){
			headline[6]=' ';
		}else{
			headline[6]=':';
		}
		tm_loop_cnt=0;
	}
	l_print((char*)headline, 0, Right);
	if(!gps_data_summary.parked_mode&&gps_data_summary.connected){
		if(last_stage!=1){
			l_print("              ", 2, Middle);
			l_print("              ", 3, Middle);
		}
		LCD_draw_big(0,2,0);
		LCD_draw_big(2,2,1);
		LCD_draw_big_num(6, 2, gps_data_summary.velocity_kmh_int/10);
		LCD_draw_big_num(8, 2, gps_data_summary.velocity_kmh_int%10);
		l_print("              ", 1, Middle);
		l_print("km/h", 3, Right);
		if(sim800l_data_summary.net_hour>=18||sim800l_data_summary.net_hour<=7){
			HAL_GPIO_WritePin(LCD_BK_GPIO_Port, LCD_BK_Pin, 1);
		}else{
			HAL_GPIO_WritePin(LCD_BK_GPIO_Port, LCD_BK_Pin, 0);
		}
		last_stage=1;
	}else{
		if(bat_disp_cnt<SCREEN_BAT_MILE_DISP_INTERVAL){
			if(last_stage!=2){
				l_print("              ", 2, Middle);
				l_print("              ", 3, Middle);
			}
			LCD_draw_big(0,2,2);
			LCD_draw_big(2,2,3);
			LCD_draw_big_num(5, 2, sim800l_data_summary.net_aux[1]);
			LCD_draw_big_num(7, 2, sim800l_data_summary.net_aux[2]);
			LCD_SetXY(9*6,3);
			g_puts('.');
			LCD_SetXY(12*6,3);
			g_puts('k');
			LCD_SetXY(13*6,3);
			g_puts('m');
//			LCD_draw_big_num(10, 2, mile_num_2);
			LCD_draw_big_num(10, 2, sim800l_data_summary.net_aux[3]);
			last_stage=2;
		}
		else if(bat_disp_cnt>=SCREEN_BAT_MILE_DISP_INTERVAL&&bat_disp_cnt<SCREEN_BAT_MILE_DISP_INTERVAL*2){
			if(last_stage!=3){
				l_print("              ", 2, Middle);
				l_print("              ", 3, Middle);
			}
			uint8_t bat_num_1=(uint8_t)battery_voltage;
			uint8_t bat_num_2=(battery_voltage-bat_num_1)*10;
			uint8_t bat_num_3=((uint8_t)((battery_voltage-bat_num_1)*100))%10;
			LCD_draw_big(0,2,4);
			LCD_draw_big(2,2,5);
			LCD_draw_big_num(5, 2, bat_num_1);
			LCD_SetXY(7*6,3);
			g_puts('.');
			LCD_SetXY(13*6,3);
			g_puts('V');
			LCD_draw_big_num(8, 2, bat_num_2);
			LCD_draw_big_num(10, 2, bat_num_3);
			battery_voltage_str[0]=bat_num_1+'0';
			battery_voltage_str[1]='.';
			battery_voltage_str[2]=bat_num_2+'0';
			battery_voltage_str[3]=bat_num_3+'0';
			battery_voltage_str[4]=0;
			last_stage=3;
		}else{
			bat_disp_cnt=0;
		}
		if(gps_data_summary.parked_mode){
			l_print("   [PARKED]   ", 1, Middle);
		}
		HAL_GPIO_WritePin(LCD_BK_GPIO_Port, LCD_BK_Pin, 0);
	}
	if(!gps_data_summary.connected){
		l_print("[GPS MALFUNC] ", 1, Middle);
	}
	if(sim800l_data_summary.connection_loss_flag){
		l_print("TRY RECONNECT.", 1, Middle);
	}
	uint8_t now_dir=get_heading_direction();
	l_print((char*)hdg_direction[now_dir], 5, Left);
	uint8_t hdg[4]="000";
	uint8_t hdg_cnt=0;
	uint16_t heading_tmp=gps_data_summary.heading_int;
	while(heading_tmp!=0){
		hdg[2-hdg_cnt]=heading_tmp%10+'0';
		heading_tmp/=10;
		hdg_cnt++;
	}
	l_print((char*)hdg, 5, Right);
	tm_loop_cnt++;
	bat_disp_cnt++;
	HAL_IWDG_Refresh(&hiwdg);
	current_mode=gps_data_summary.parked_mode;
	//每天自动重启
	if(sim800l_data_summary.net_hour==2&&sim800l_data_summary.net_min==1&&
			(sim800l_data_summary.net_sec>=0&&sim800l_data_summary.net_sec<=20)){
		HAL_NVIC_SystemReset();
	}
	osDelay(250);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_dataTransFunc */
/**
* @brief Function implementing the dataTrans thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_dataTransFunc */
void dataTransFunc(void *argument)
{
  /* USER CODE BEGIN dataTransFunc */
	uint8_t state_transfer_flag=0;//如果检测到停止，立刻发送数据包，之后再恢复正常间隔
	uint32_t interval=6000;
	uint32_t tick_1=0,tick_2=0;
	sim800l_data_summary.osReady=1;
	gps_data_summary.os_ready=1;
  /* Infinite loop */
  for(;;)
  {
	  tick_1=HAL_GetTick();
	  sim800l_noecho();
	  sim800l_read_rssi();
	  //It seems that i ignored the process safety.
	  if(battery_voltage_str[0]!='0'){
		  if(gps_data_summary.queue_tail>0){
			  sim800l_send_queue();
			  state_transfer_flag=1;
		  }else{
			  gps_generate_null();
			  sim800l_send(gps_pulse_message);
			  state_transfer_flag=0;
		  }
	  }else{
		  osDelay(1000);
		  continue;
	  }
	  HAL_GPIO_TogglePin(LCD_BK_GPIO_Port, LCD_BK_Pin);
	  osDelay(100);
	  HAL_GPIO_TogglePin(LCD_BK_GPIO_Port, LCD_BK_Pin);
	  if(!sim800l_check_connection(1000)){
		  sim800l_init_quick();
	  }
	  tick_2=HAL_GetTick();
	  if(state_transfer_flag){
		  interval=6000;
	  }else{
		  interval=5*6000;
	  }
	  if(tick_2-tick_1<interval){
		  uint32_t delta=interval-(tick_2-tick_1);
		  osDelay(delta);
	  }
  }
  /* USER CODE END dataTransFunc */
}

/* USER CODE BEGIN Header_posLedFunc */
/**
* @brief Function implementing the posLedTask thread.
* @param argument: Not used
* @retval None
*/
uint8_t self_timer_cnt=0;
/* USER CODE END Header_posLedFunc */
void posLedFunc(void *argument)
{
  /* USER CODE BEGIN posLedFunc */
  /* Infinite loop */
  for(;;)
  {
	  //0:strobe 1:steady 2:off
	  if(sim800l_data_summary.net_aux[0]==0){
		  HAL_GPIO_WritePin(POS_LED_GPIO_Port, POS_LED_Pin, 1);
		  osDelay(25);
		  HAL_GPIO_WritePin(POS_LED_GPIO_Port, POS_LED_Pin, 0);
		  osDelay(50);
		  HAL_GPIO_WritePin(POS_LED_GPIO_Port, POS_LED_Pin, 1);
		  osDelay(25);
		  HAL_GPIO_WritePin(POS_LED_GPIO_Port, POS_LED_Pin, 0);
		  osDelay(25);
	  }else if(sim800l_data_summary.net_aux[0]==1){
		  HAL_GPIO_WritePin(POS_LED_GPIO_Port, POS_LED_Pin, 1);
	  }
	  else if(sim800l_data_summary.net_aux[0]==2){
		  if(sim800l_data_summary.net_hour>=18||sim800l_data_summary.net_hour<=7){
			  HAL_GPIO_WritePin(POS_LED_GPIO_Port, POS_LED_Pin, 1);
			  osDelay(25);
			  HAL_GPIO_WritePin(POS_LED_GPIO_Port, POS_LED_Pin, 0);
			  osDelay(50);
			  HAL_GPIO_WritePin(POS_LED_GPIO_Port, POS_LED_Pin, 1);
			  osDelay(25);
			  HAL_GPIO_WritePin(POS_LED_GPIO_Port, POS_LED_Pin, 0);
			  osDelay(25);
		  }
		  else{
			  HAL_GPIO_WritePin(POS_LED_GPIO_Port, POS_LED_Pin, 0);
		  }
	  }
	  else{
		  HAL_GPIO_WritePin(POS_LED_GPIO_Port, POS_LED_Pin, 0);
	  }
	  osDelay(1000);
  }
  /* USER CODE END posLedFunc */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  else if(htim->Instance==htim6.Instance){
		self_timer_cnt++;
		if(self_timer_cnt==10){
		  self_timer_cnt=0;
		  sim800l_data_summary.net_sec++;
		  if(sim800l_data_summary.net_sec==60){
			sim800l_data_summary.net_sec=0;
			sim800l_data_summary.net_min++;
			if(sim800l_data_summary.net_min==60){
				sim800l_data_summary.net_min=0;
				sim800l_data_summary.net_hour++;
				if(sim800l_data_summary.net_hour==24){
					sim800l_data_summary.net_hour=0;
				}
			}
		  }
		}
		HAL_TIM_Base_Start_IT(&htim6);
	}
  /* USER CODE END Callback 1 */
}

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
