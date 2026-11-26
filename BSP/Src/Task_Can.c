#include "Task_Can.h"
#include "User_CAN.h"

extern FDCAN_HandleTypeDef hfdcan1;

/* 
 * CAN 发送任务 (纯硬件发送，无监控代码)
 */
CanSend_Type CAN_Tx_Msg;

void Task_CAN(void *parameters)
{
  (void)parameters;
  
  // 🌟 初始化 CAN。如果你硬件没有 CAN2/CAN3，请把下面两行注释掉！
  CAN_Init(&hfdcan1);
  // CAN_Init(&hfdcan2); 
  // CAN_Init(&hfdcan3);

  while (1)
  {
    // 死等队列数据，没数据就休眠
    if (xQueueReceive(Queue_CANSend, &CAN_Tx_Msg, portMAX_DELAY) != pdPASS)
      continue;

    // 根据报文里的 CANx 选择对应的硬件发送
    switch (CAN_Tx_Msg.CANx)
    {
    case CANSEND_1:
      CANTransmit(&hfdcan1, CAN_Tx_Msg.stdid, CAN_Tx_Msg.Data);
      break;
    default:
      break;
    }
  }
}

void CANTransmit(FDCAN_HandleTypeDef *hfdcan, uint32_t std_id, uint8_t aData[])
{
  FDCAN_TxHeaderTypeDef TxHeader;

  // 配置报文头
  TxHeader.Identifier = std_id;
  TxHeader.IdType = FDCAN_STANDARD_ID;
  TxHeader.TxFrameType = FDCAN_DATA_FRAME;
  TxHeader.DataLength = FDCAN_DLC_BYTES_8;
  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
  TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  TxHeader.MessageMarker = 0x00;

  // 丢给底层硬件发送
  if (HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &TxHeader, aData) != HAL_OK)
  {
    // 发送失败，重启一次 CAN 硬件（简易的总线恢复机制）
    HAL_FDCAN_Stop(hfdcan);
    HAL_FDCAN_Start(hfdcan);
  }
}