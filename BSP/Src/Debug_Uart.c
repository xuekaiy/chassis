/**Include Header Files**/
#include "Debug_Uart.h"

#if (DEBUG_UART_ENABLE != 0)

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/*
 * sysconfig.h 的包含链里没有 queue.h / task.h，QueueHandle_t、TaskHandle_t
 * 以及 xQueueCreate/xQueueSend/xQueueReceive/xTaskCreate 的声明需要显式引入
 * （工程里 PID.c、Task_Can.h 也是这么做的）。FreeRTOS.h 必须在前面。
 */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* USART10 的句柄在 Core/Src/usart.c 里定义 */
extern UART_HandleTypeDef huart10;

/**Private Struct**/
typedef struct
{
    char text[DEBUG_UART_MSG_LEN];
} DebugMsg_t;

/**Static Variables**/
static QueueHandle_t     s_dbgQueue   = NULL;
static TaskHandle_t      s_dbgTask    = NULL;
static volatile uint32_t s_dbgDropped = 0U;

/**
 * @brief  调试串口发送任务
 * @note   优先级最低。用阻塞发送：TX 方向没有 overrun 问题，
 *         即使被 500 Hz 控制任务打断，也只是这一行晚几十毫秒发完。
 * @param  argument: 未使用
 * @retval None
 */
static void Task_Debug(void *argument)
{
    DebugMsg_t msg;

    (void)argument;

    for (;;)
    {
        if (xQueueReceive(s_dbgQueue, &msg, portMAX_DELAY) != pdPASS)
            continue;

        (void)HAL_UART_Transmit(&huart10,
                                (uint8_t *)msg.text,
                                (uint16_t)strlen(msg.text),
                                50U);
    }
}

/**
 * @brief  绕过队列直接阻塞发一条（只用于初始化失败告警）
 * @note   正常数据不要用这个，会卡调用方
 * @param  text: 以 \0 结尾的字符串
 * @retval None
 */
static void Dbg_DirectWrite(const char *text)
{
    if (text == NULL)
        return;

    (void)HAL_UART_Transmit(&huart10, (uint8_t *)text,
                            (uint16_t)strlen(text), 100U);
}

/**
 * @brief  创建调试串口队列与发送任务
 * @note   必须在 MX_USART10_UART_Init() 之后、且建议在 osKernelStart() 之前调用
 * @retval None
 */
void BSP_Init_DebugUart(void)
{
    if (s_dbgQueue == NULL)
    {
        s_dbgQueue = xQueueCreate(DEBUG_UART_QUEUE_LEN, sizeof(DebugMsg_t));
    }

    if (s_dbgQueue == NULL)
    {
        /* 队列没建起来：后面所有 Debug_Printf 都会静默失败，这里先喊一声 */
        Dbg_DirectWrite("\r\n[dbg] ERROR: queue create failed (heap)\r\n");
        return;
    }

    if (s_dbgTask == NULL)
    {
        if (xTaskCreate(Task_Debug, "DebugUart",
                        (uint16_t)DEBUG_UART_TASK_STACK, NULL,
                        (UBaseType_t)DEBUG_UART_TASK_PRIO,
                        &s_dbgTask) != pdPASS)
        {
            s_dbgTask = NULL;
            /* 任务没建起来 = 队列永远没人取，输出会“看起来没有数据” */
            Dbg_DirectWrite("\r\n[dbg] ERROR: task create failed (heap)\r\n");
        }
    }
}

/**
 * @brief  printf 风格输出到调试串口
 * @note   非阻塞；单条超长会被截断；结尾自动补 \r\n
 * @param  fmt: 格式串（支持 %d %u %lu %x %s %f 等标准格式）
 * @retval 1：已入队；0：未入队（队列满或未初始化）
 */
uint8_t Debug_Printf(const char *fmt, ...)
{
    DebugMsg_t msg;
    va_list    ap;
    int        len;

    if ((s_dbgQueue == NULL) || (fmt == NULL))
        return 0U;

    va_start(ap, fmt);
    len = vsnprintf(msg.text, sizeof(msg.text), fmt, ap);
    va_end(ap);

    if (len < 0)
        return 0U;

    /* vsnprintf 已保证以 \0 结尾；给 CRLF 预留 2 字节空间 */
    if (len > (int)(sizeof(msg.text) - 3U))
        len = (int)(sizeof(msg.text) - 3U);

    if ((len == 0) || (msg.text[len - 1] != '\n'))
    {
        msg.text[len++] = '\r';
        msg.text[len++] = '\n';
        msg.text[len]   = '\0';
    }

    if (xQueueSend(s_dbgQueue, &msg, 0U) != pdPASS)
    {
        s_dbgDropped++;
        return 0U;
    }

    return 1U;
}

/**
 * @brief  返回因队列满而被丢弃的条数
 * @note   长期 > 0 说明打印太频繁或串口太慢，需要降低频率
 * @retval 丢弃计数
 */
uint32_t Debug_DroppedCount(void)
{
    return s_dbgDropped;
}

#endif /* DEBUG_UART_ENABLE */
