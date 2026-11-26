#ifndef _TASK_CAN_H__
#define _TASK_CAN_H__

/**Include Header Files**/
#include "sysconfig.h"
#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"
/** Macro Definition **/
#define CANSEND_1 1
#define CANSEND_2 2
#define CANSEND_3 3

/**Struct Definition**/
typedef struct
{
    uint8_t            CANx;               //CAN���ͽṹ��
    uint32_t           stdid;              
		uint8_t            Data[8];
}CanSend_Type;

/**Function Declaration**/
void Task_CAN(void *parameters);
void CANTransmit(FDCAN_HandleTypeDef *hfdcan, uint32_t std_id, uint8_t aData[]);

/**Task Function Declaration**/
extern QueueHandle_t Queue_CANSend;
#endif
