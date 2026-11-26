#ifndef __BSP_INIT_H__
#define __BSP_INIT_H__

/**Include Header Files**/
#include "sysconfig.h"
#include "Motor.h"
#include "Task_Can.h"

/**Macro Definition**/
/* CAN 发送队列长度：一条报文 CanSend_Type 只有 12 字节，32 条足够吸收突发发送 */
#define BSP_CAN_QUEUE_LEN       (32U)

/* CAN 发送任务栈(word)与优先级(原生 FreeRTOS 优先级，数值越大越高) */
#define BSP_CAN_TASK_STACK      (256U)
#define BSP_CAN_TASK_PRIO       (2U)

/**Variable Declaration**/
/*
 * 这两个对象原来只有 extern 声明、没有实体，所以链接时报 L6218E:
 *   Queue_CANSend  <- Task_Can.c / DM_driver.c 在用
 *   Arm_Motor      <- Motor.c / User_CAN.c 在用
 * 实体定义在 BSP_Init.c 里，全工程只此一份。
 */
extern motor_t       Arm_Motor[MNUM];
extern QueueHandle_t Queue_CANSend;

/**Function Declaration**/
/*
 * 只创建 CAN 发送队列（如果你自己创建 Task_CAN 任务，就只调这个就够）。
 * 返回 1：成功；0：失败。
 */
uint8_t BSP_Queue_Init(void);

/*
 * 创建 CAN 发送队列 + 启动 Task_CAN 任务。
 * 建议在 main.c 的 USER CODE BEGIN 2 段里、osKernelStart() 之前调用一次。
 * 重复调用不会重复创建任务。返回 1：成功；0：失败。
 */
uint8_t BSP_Init(void);

#endif /* __BSP_INIT_H__ */
