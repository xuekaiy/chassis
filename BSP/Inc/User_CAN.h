#ifndef __USER_CAN_H__
#define __USER_CAN_H__

#include "sysconfig.h"

#include "main.h"
#include "Motor.h"
/*
 * BSP CAN boundary.
 *
 * Receive direction:
 *   HAL FDCAN callback -> CanXReceived_infoHandle() -> motor driver parser
 *   -> Arm_Motor[] feedback fields.
 *
 * Send direction:
 *   Motion/control wrappers enqueue frames; Task_CAN calls CANTransmit().
 */

void CAN_Init(FDCAN_HandleTypeDef *hfdcan);
void CAN_Fliter_Init(FDCAN_HandleTypeDef *hfdcan);

void Can1Received_infoHandle(uint32_t stdid, uint8_t adata[]);
void Can2Received_infoHandle(uint32_t stdid, uint8_t adata[]);
void Can3Received_infoHandle(uint32_t stdid, uint8_t adata[]);

#endif
