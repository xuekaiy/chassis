/**Include Header Files**/
#include "sysconfig.h"
#include "stm32h7xx_hal_tim.h"

/*
 * TIM2 PWM compatibility init.
 *
 * 当前 CMake/Keil 工程没有把 Core/Src/tim.c 链接进来，但 main.c 和
 * Task_MotorControl.c 仍然需要 MX_TIM2_Init() / htim2。这里放在已收录的
 * BSP/Src/sysconfig.c 中兜底，避免修改 Keil 工程文件。
 *
 * 注意：
 *   - 这是弱定义；以后如果 Core/Src/tim.c 被重新加入工程，CubeMX 生成的
 *     强定义会自动覆盖这里。
 *   - 参数完全保持 CubeMX 原配置：PA0 -> TIM2_CH1，20 ms PWM 周期，
 *     初始 Pulse = 5000。
 */
__weak TIM_HandleTypeDef htim2;

__weak void MX_TIM2_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 239;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 19999;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
        Error_Handler();

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
        Error_Handler();

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 5000;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
        Error_Handler();

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * @description: Convert float to uint
 * @param
 * @return: int
 * @note:
 */
int float_to_uint(float x, float x_min, float x_max, int bits)
{
    /// Converts a float to an unsigned int, given range and number of bits ///
    float span = x_max - x_min;
    float offset = x_min;
    if (x > x_max)
        x = x_max;
    else if (x < x_min)
        x = x_min;
    return (int)((x - offset) * ((float)((1 << bits) - 1)) / span);
}

/**
 * @description: Convert uint to float
 * @param
 * @return: float
 * @note:
 */
float uint_to_float(int x_int, float x_min, float x_max, int bits)
{
    /// converts unsigned int to float, given range and number of bits ///
    float span = x_max - x_min;
    float offset = x_min;
    return ((float)x_int) * span / ((float)((1 << bits) - 1)) + offset;
}

