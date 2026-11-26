/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "dma.h"
#include "fdcan.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "DJI_Motor.h"
#include "DM_driver.h"
#include "Motor.h"
#include "PID.h"
#include "Task_Can.h"
#include "User_CAN.h"
#include "RemoteControl_Comm.h"
#include "BSP_Init.h"
#include "Debug_Uart.h"


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
/*
pid_type pid = {
  .kp = 20.0f,
  .ki = 0.05f,
  .kd = 0.0f,

  .error[0] = 0.0f,
  .error[1] = 0.0f, 
  .error[2] = 0.0f,
  .sum_error = 0.0f,
  .Dbuf[0] = 0.0f,
  .Dbuf[1] = 0.0f,
  .Dbuf[2] = 0.0f,  

  .Pout = 0.0f,
  .Iout = 0.0f,
  .Dout = 0.0f,
  .Out = 0.0f,

  .dst = 300.0f,
  .real = 0.0f,

  .MaxIout = 1000.0f,
  .MaxOut = 3000.0f,

  .MaxBlockIount = 1000.0f,
  .DeadZone = 5

};
*/
pid_type WheelPid[4];

static const float WheelPidPara[4][3] = {
  {20.0f, 0.05f, 0.0f},   /* 左前 */
  {20.0f, 0.05f, 0.0f},   /* 左后 */
  {20.0f, 0.05f, 0.0f},   /* 右后 */
  {20.0f, 0.05f, 0.0f},   /* 右前 */
};

/* 机械安装方向：左右镜像安装时，左侧两路取 -1（也可以用 MotortypeRegister 的 REVERSE） */
const int8_t WheelSign[4] = { -1, -1, +1, +1 };
float PID1_PARA[3] = {0.1f, 0.01f, 0.0f};

uint8_t sendmessage[8] = {0};
uint8_t receivemessage[8] = {0};
int16_t out = 100;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

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
  MX_FDCAN1_Init();
  MX_FDCAN2_Init();
  MX_FDCAN3_Init();
  MX_USART10_UART_Init();
  MX_TIM12_Init();
  MX_TIM2_Init();
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */
  BSP_Init();

  /* 调试串口：USART10 / PE3，115200 8N1，非阻塞（队列 + 最低优先级任务） */
  BSP_Init_DebugUart();

#if (DEBUG_UART_ENABLE != 0)
  /*
   * 最小验证：完全绕过队列和任务，直接在 main 里阻塞发一行。
   *   这一行能出来   => 接线 / 波特率 / USART10 初始化都正常，后面的问题在任务侧
   *   这一行也出不来 => 硬件、串口工具或烧写的问题，不用再查代码
   */
  {
    static const char bootMsg[] = "\r\n[boot] USART10 direct write OK\r\n";
    (void)HAL_UART_Transmit(&huart10, (uint8_t *)bootMsg,
                            (uint16_t)(sizeof(bootMsg) - 1U), 100U);
  }
#endif

  CAN_Init(&hfdcan1);
 
  for (uint8_t i = 0; i < 4; i++)
{
  /* 下标 i 必须等于 CAN ID-1：0x200 第 i 个通道 ↔ Arm_Motor[i] ↔ 电调 ID i+1 */
  MotortypeRegister(&Arm_Motor[i],
                    (WheelSign[i] > 0) ? FORWARD : REVERSE,
                    MOTOR_3508,
                    NULL,
                    NULL,
                    MOTOR3508_Reductionratio,      /* 19 */
                    (uint32_t)(i + 1U),            /* 电机 ID 1~4 */
                    1U,                            /* CAN1 */
                    0.0f, 0.0f, 0.0f,              /* Kp/Ki/Kd：3508 不用 */
                    360.0f, 0.0f);

  /* 速度环：输出限幅 16000（C620 满量程 16384），积分限幅 4000，死区 8 rpm */
  PID_Parameter_Init(&WheelPid[i], PID_POSITION, WheelPidPara[i], 16000.0f, 4000.0f, 8);
}

  /* 遥控接收：USART10 + DMA 双缓冲 + IDLE 空闲中断，必须在 MX_USART10_UART_Init() 之后 */
  BSP_Init_RemoteControl();
 
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 6;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

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
#ifdef USE_FULL_ASSERT
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
