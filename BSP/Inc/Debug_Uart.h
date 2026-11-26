#ifndef __DEBUG_UART_H__
#define __DEBUG_UART_H__

/**Include Header Files**/
#include "sysconfig.h"

/*
 * 调试串口（printf 风格）
 * ---------------------------------------------------------------------------
 * 硬件：USART10，PE3 = TX，PE2 = RX，115200 8N1
 *       （UART5/PD2 是 DBUS 遥控接收，不要混用）
 *
 * 用法：
 *   1) main() 里 MX_USART10_UART_Init() 之后调用一次 BSP_Init_DebugUart();
 *   2) 任何【任务】里直接：
 *          Debug_Printf("vx=%d rpm=%d", (int)vx, (int)rpm);
 *      自动补 CRLF；不需要手动加 \n。
 *
 * 特性：
 *   - 非阻塞：内部是 FreeRTOS 队列 + 最低优先级发送任务，
 *     调用方只做一次 vsnprintf + 一次 xQueueSend(0)，不会卡住 500 Hz 控制环。
 *   - 队列满时丢弃并计数，可用 Debug_DroppedCount() 查看丢了多少条。
 *   - 只能在任务上下文调用（不要在中断里调，xQueueSend 不是 ISR 版本）。
 *
 * 关掉它：把 DEBUG_UART_ENABLE 改成 0，整个模块连队列带任务一起被编译掉，
 *         不影响任何调用点（宏会变成空操作）。
 */

/**Macro Definition**/
/* 0 = 完全禁用；1 = 启用 */
#define DEBUG_UART_ENABLE       1

/* 0 = 人类可读的 key=value；1 = 纯逗号分隔（给 Excel / 串口示波器画图） */
#define DEBUG_UART_CSV_MODE     0

/* 队列深度与单条最大长度。
   注意：[rc] 那一行长约 100 字符，96 字节会把尾部截断（曾经把 pin 的第三个
   数截掉）。这里给到 160，留足余量。8×160 = 1280B 堆，可以接受。 */
#define DEBUG_UART_QUEUE_LEN    8U
#define DEBUG_UART_MSG_LEN      160U

/* 发送任务：栈(word)与优先级(数值越小越低) */
#define DEBUG_UART_TASK_STACK   256U
#define DEBUG_UART_TASK_PRIO    1U

/* 每多少个控制周期打印一行（50 × 2ms = 100ms → 10 行/秒，约占 8% 带宽） */
#define DEBUG_UART_PRINT_DIV    50U

/**Function Declaration**/
#if (DEBUG_UART_ENABLE != 0)

void     BSP_Init_DebugUart(void);
uint8_t  Debug_Printf(const char *fmt, ...);
uint32_t Debug_DroppedCount(void);

#else /* DEBUG_UART_ENABLE == 0 : 全部退化成空操作 */

#define BSP_Init_DebugUart()    ((void)0)
#define Debug_Printf(...)       ((uint8_t)0)
#define Debug_DroppedCount()    ((uint32_t)0)

#endif /* DEBUG_UART_ENABLE */

#endif /* __DEBUG_UART_H__ */
