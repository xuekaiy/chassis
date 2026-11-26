/**Include Header Files**/
#include "User_CAN.h"
#include "DJI_Motor.h"
#include "DM_driver.h"  
// 🌟 告诉编译器，hfdcan1 在其他文件（main.c/fdcan.c）里已经定义过了
extern FDCAN_HandleTypeDef hfdcan1;

// 🌟 声明外部电机数组。请确保你在 main.c 或全局定义了这个名字！
extern motor_t Arm_Motor[]; 

void CAN_Init(FDCAN_HandleTypeDef *hfdcan)
{
  // 如果传进来的不是 CAN1，直接退出
  if (hfdcan != &hfdcan1) {
    return;
  }

  CAN_Fliter_Init(hfdcan);
  HAL_FDCAN_Start(hfdcan);
  HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
}

void CAN_Fliter_Init(FDCAN_HandleTypeDef *hfdcan)
{
  FDCAN_FilterTypeDef sFilterConfig;

  if (hfdcan != &hfdcan1) {
    return;
  }

  sFilterConfig.IdType = FDCAN_STANDARD_ID;
  sFilterConfig.FilterIndex = 0;
  sFilterConfig.FilterType = FDCAN_FILTER_MASK;
  sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  sFilterConfig.FilterID1 = 0x00; 
  sFilterConfig.FilterID2 = 0x00; 
  
  HAL_FDCAN_ConfigFilter(hfdcan, &sFilterConfig);
  HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE);
  HAL_FDCAN_ConfigFifoWatermark(hfdcan, FDCAN_CFG_RX_FIFO0, 1);
}

/**
 * @brief  FIFO0的CAN接收回调 (只处理 CAN1)
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  FDCAN_RxHeaderTypeDef RxHeader;
  uint8_t aData[8];

  (void)RxFifo0ITs;
  
  if (hfdcan != &hfdcan1) return;

  if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, aData) == HAL_OK)
  {
    Can1Received_infoHandle(RxHeader.Identifier, aData);
  }
}

/**
 * @brief  处理 CAN1 接收到的数据 (专注大疆 3508)
 */
void Can1Received_infoHandle(uint32_t stdid, uint8_t adata[])
{
  uint32_t index;

  if (adata == NULL) return;

  /*
   * 只接受 0x201 ~ 0x201+MNUM-1（C620/C610 电调反馈），其余 ID 一律丢弃。
   * 原来直接用 Arm_Motor[stdid - 0x201]，而滤波器是全通，任何 ID（0x0xx/0x1xx/干扰帧）
   * 都会算出负数或巨大下标，越界写内存。
   */
  if ((stdid < 0x201UL) || (stdid >= (0x201UL + (uint32_t)MNUM)))
  {
    /* 以后接达妙电机就在这里按 ID 分发：DM_infoHandle(&Arm_Motor[x], adata); */
    return;
  }

  index = stdid - 0x201UL;

  /* 该槽位没注册电机就不解析，避免写进未初始化的结构体 */
  if (Arm_Motor[index].type == MOTOR_NONE)
    return;

  // 达妙电机的反馈 ID 通常是：0x00 + 电机 ID（MIT模式） 或 0x10 + 电机 ID 等
  // 这里根据你代码里的逻辑，达妙回包的报文第一个字节(adata[0])的低 4 位就是电机 ID
  /*uint8_t dm_payload_id = adata[0] & 0x0F;
  
  // 假设你的达妙电机都在 Arm_Motor 数组里
  for (int i = 0; i < MNUM; i++) {
    // 确保只处理 DM 类型的电机
    if (Arm_Motor[i].type >= MOTOR_DM8009 && // 假设 MOTOR_DM8009 是 DM 类型的起始枚举
        Arm_Motor[i].id == dm_payload_id && 
        Arm_Motor[i].CAN_ID == 1) 
    {
        DM_infoHandle(&Arm_Motor[i], adata);*/
  DJIMotor_ParaHandle(&Arm_Motor[index], adata);   
  
}

// 为了满足头文件里的声明，留个空函数在这里，防止链接报错
void Can2Received_infoHandle(uint32_t stdid, uint8_t adata[])
{
  (void)stdid; 
  (void)adata;
}

void Can3Received_infoHandle(uint32_t stdid, uint8_t adata[])
{
  (void)stdid;
  (void)adata;
}