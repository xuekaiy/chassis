/**Include Header Files**/
#include "BSP_Init.h"
#include "User_CAN.h"

/**Variable Definition**/
/*
 * 电机对象数组的实体。
 *   Motor.h    : extern motor_t Arm_Motor[7];   (MNUM == 7)
 *   User_CAN.c : extern motor_t Arm_Motor[];
 *   Motor.c    : ArmMotor_ErrorStatePack_Get() 等函数在用
 * 原来只有声明没有定义，链接报 L6218E: Undefined symbol Arm_Motor。
 */
motor_t Arm_Motor[MNUM];

/*
 * CAN 发送队列句柄的实体。
 *   Task_Can.h : extern QueueHandle_t Queue_CANSend;
 *   Task_Can.c : xQueueReceive(Queue_CANSend, ...) 取报文
 *   DM_driver.c: xQueueSend(Queue_CANSend, ...)    放报文
 * 原来只有声明没有定义，链接报 L6218E: Undefined symbol Queue_CANSend。
 */
QueueHandle_t Queue_CANSend = NULL;

static uint8_t BSP_TaskStarted = 0U;

/**Function Declaration**/
static void BSP_CanTaskEntry(void const *argument);

/*
 * 用原生 FreeRTOS 建任务。
 * CMSIS-RTOS v2 的 cmsis_os.h 里 osThreadDef 只有 4 个参数、且把 name 当入口函数，
 * 再包一层反而编不过；Task_CAN 的签名本来就是 void Task_CAN(void *)，直接 xTaskCreate 最干净。
 */
static TaskHandle_t BSP_CanTaskHandle = NULL;

/**
 * @brief  只创建 CAN 发送队列
 * @note   队列必须比 Task_CAN 先创建好，否则 Task_CAN 一进去就 xQueueReceive(NULL)
 * @retval 1：成功；0：失败
 */
uint8_t BSP_Queue_Init(void)
{
    if (Queue_CANSend == NULL)
    {
        Queue_CANSend = xQueueCreate(BSP_CAN_QUEUE_LEN, sizeof(CanSend_Type));
    }

    return (Queue_CANSend != NULL) ? 1U : 0U;
}

/**
 * @brief  创建队列 + 启动 CAN 发送任务
 * @retval 1：成功；0：失败
 */
uint8_t BSP_Init(void)
{
    if (BSP_Queue_Init() == 0U)
    {
        return 0U;
    }

    if (BSP_TaskStarted == 0U)
    {
        if (xTaskCreate(Task_CAN, "Task_CAN",
                        (uint16_t)BSP_CAN_TASK_STACK, NULL,
                        (UBaseType_t)BSP_CAN_TASK_PRIO,
                        &BSP_CanTaskHandle) != pdPASS)
        {
            return 0U;
        }
        BSP_TaskStarted = 1U;
    }

    return 1U;
}
