/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
#include "usart.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart10;
DMA_HandleTypeDef hdma_uart5_rx;
DMA_HandleTypeDef hdma_uart5_tx;
DMA_HandleTypeDef hdma_usart10_rx;
DMA_HandleTypeDef hdma_usart10_tx;

/* UART5 init function */
void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 100000;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  /*
   * DBUS 标准帧格式：100000 bps，8 数据位 + 偶校验 + 1 停止位。
   *
   * 关于 fe（帧错误计数）：曾经观察到 fe 以 ~1000/秒 增长（约 78% 的字节），
   * 一度怀疑帧宽/校验位不匹配。但同一份数据里 len 恒为 18、bad 恒为 2、by 严格
   * 按 18×71 增长、通道值完全正确；而且把 Parity 改成 NONE 后 fe 一点没降。
   * 因此判定：fe 是 H7 上 FE 标志置位/清除时序导致的观测假象，不是真的帧错误。
   * 判断接收机是否正常请看 len / bad / by 三个量，不要看 fe。
   */
  huart5.Init.Parity = UART_PARITY_EVEN;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  huart5.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart5.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  /*
   * 反相：两种都关。
   *   - RXINV  : 本板 DBUS 座子上已有硬件反相，MCU 侧 PD2 看到的是正常 UART
   *              （空闲高）。开 RXINV 会双重反相 -> 每字节 FE、IDLE 不触发。
   *   - DATAINV: 只反数据位、不反起止位，对 DBUS/SBUS 都只会产生乱码。
   * 这两个值必须和 RemoteControl_Comm.c 里 RC_ApplyInvert() 的设置一致，
   * 否则运行第一拍会被它改回去，配置与实际行为对不上，排查时会误导。
   */
  huart5.AdvancedInit.AdvFeatureInit = 0;
  huart5.AdvancedInit.RxPinLevelInvert = UART_ADVFEATURE_RXINV_DISABLE;
  huart5.AdvancedInit.DataInvert = UART_ADVFEATURE_DATAINV_DISABLE;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart5, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart5, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}
/* USART10 init function */

void MX_USART10_UART_Init(void)
{

  /* USER CODE BEGIN USART10_Init 0 */

  /* USER CODE END USART10_Init 0 */

  /* USER CODE BEGIN USART10_Init 1 */

  /* USER CODE END USART10_Init 1 */
  huart10.Instance = USART10;
  huart10.Init.BaudRate = 115200;
  huart10.Init.WordLength = UART_WORDLENGTH_8B;
  huart10.Init.StopBits = UART_STOPBITS_1;
  huart10.Init.Parity = UART_PARITY_NONE;
  huart10.Init.Mode = UART_MODE_TX_RX;
  huart10.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart10.Init.OverSampling = UART_OVERSAMPLING_16;
  huart10.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart10.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart10.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT;
  huart10.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
  if (HAL_UART_Init(&huart10) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart10, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart10, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart10) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART10_Init 2 */

  /* USER CODE END USART10_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(uartHandle->Instance==UART5)
  {
  /* USER CODE BEGIN UART5_MspInit 0 */

  /* USER CODE END UART5_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_UART5;
    PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* UART5 clock enable */
    __HAL_RCC_UART5_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**UART5 GPIO Configuration
    PB13     ------> UART5_TX
    PD2     ------> UART5_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_UART5;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART5;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* UART5 DMA Init */
    /* UART5_RX Init */
    hdma_uart5_rx.Instance = DMA1_Stream1;
    hdma_uart5_rx.Init.Request = DMA_REQUEST_UART5_RX;
    hdma_uart5_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_uart5_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart5_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart5_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart5_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart5_rx.Init.Mode = DMA_CIRCULAR;
    hdma_uart5_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_uart5_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_uart5_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_uart5_rx);

    /* UART5_TX Init */
    hdma_uart5_tx.Instance = DMA1_Stream2;
    hdma_uart5_tx.Init.Request = DMA_REQUEST_UART5_TX;
    hdma_uart5_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_uart5_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart5_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart5_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart5_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart5_tx.Init.Mode = DMA_NORMAL;
    hdma_uart5_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_uart5_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_uart5_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_uart5_tx);

    /* UART5 interrupt Init */
    HAL_NVIC_SetPriority(UART5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(UART5_IRQn);
  /* USER CODE BEGIN UART5_MspInit 1 */

  /* USER CODE END UART5_MspInit 1 */
  }
  else if(uartHandle->Instance==USART10)
  {
  /* USER CODE BEGIN USART10_MspInit 0 */

  /* USER CODE END USART10_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART10;
    PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16910CLKSOURCE_D2PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* USART10 clock enable */
    __HAL_RCC_USART10_CLK_ENABLE();

    __HAL_RCC_GPIOE_CLK_ENABLE();
    /**USART10 GPIO Configuration
    PE2     ------> USART10_RX
    PE3     ------> USART10_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_USART10;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF11_USART10;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* USART10 DMA Init */
    /* USART10_RX Init */
    hdma_usart10_rx.Instance = DMA1_Stream4;
    hdma_usart10_rx.Init.Request = DMA_REQUEST_USART10_RX;
    hdma_usart10_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart10_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart10_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart10_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart10_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart10_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart10_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_usart10_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart10_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart10_rx);

    /* USART10_TX Init */
    hdma_usart10_tx.Instance = DMA1_Stream3;
    hdma_usart10_tx.Init.Request = DMA_REQUEST_USART10_TX;
    hdma_usart10_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart10_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart10_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart10_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart10_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart10_tx.Init.Mode = DMA_NORMAL;
    hdma_usart10_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_usart10_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart10_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart10_tx);

    /* USART10 interrupt Init */
    HAL_NVIC_SetPriority(USART10_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART10_IRQn);
  /* USER CODE BEGIN USART10_MspInit 1 */

  /* USER CODE END USART10_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==UART5)
  {
  /* USER CODE BEGIN UART5_MspDeInit 0 */

  /* USER CODE END UART5_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART5_CLK_DISABLE();

    /**UART5 GPIO Configuration
    PB13     ------> UART5_TX
    PD2     ------> UART5_RX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);

    /* UART5 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* UART5 interrupt Deinit */
    HAL_NVIC_DisableIRQ(UART5_IRQn);
  /* USER CODE BEGIN UART5_MspDeInit 1 */

  /* USER CODE END UART5_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART10)
  {
  /* USER CODE BEGIN USART10_MspDeInit 0 */

  /* USER CODE END USART10_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART10_CLK_DISABLE();

    /**USART10 GPIO Configuration
    PE2     ------> USART10_RX
    PE3     ------> USART10_TX
    */
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_2|GPIO_PIN_3);

    /* USART10 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART10 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART10_IRQn);
  /* USER CODE BEGIN USART10_MspDeInit 1 */

  /* USER CODE END USART10_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

