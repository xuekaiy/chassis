/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Task_Can.h"     /* CANTransmit() / CanSend_Type */
#include "Motor.h"        /* Arm_Motor / motor_t / FeedbackData，内部已含 PID.h */
#include "PID.h"  
#include "RemoteControl_Comm.h"        /* Pid_cal / PID_clear / pid_type */
#include "usart.h"
#include "Debug_Uart.h"    /* Debug_Printf()：调试串口 USART10 / PE3，115200 */

extern FDCAN_HandleTypeDef hfdcan1;
extern uint8_t  sendmessage[8];   /* main.c 里的全局 0x200 报文缓冲 */
extern int16_t  out;              /* main.c 里的全局电流指令 */
extern pid_type pid; 
extern pid_type WheelPid[4];
extern const int8_t WheelSign[4];      
/* main.c 里的全局速度环 PID */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  /* 512B 对"控制环 + vsnprintf 调试输出"太紧：Debug_Printf 里有一个 96 字节
     的局部缓冲，再叠加 vsnprintf 自己的栈帧，容易踩栈。给到 2KB。 */
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Task_Init(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(Task_Init, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_Task_Init */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Task_Init */
__weak void Task_Init(void *argument)
{
  /* USER CODE BEGIN Task_Init */
  
  
  int16_t  cur[4]        = {0};
  uint32_t LastFrame[4]  = {0};
  uint16_t LostCnt[4]    = {0};
  uint8_t  enable        = 0;
  uint8_t  rcOk          = 0;                     /* 0 = 遥控帧超时，强制停车 */
  uint8_t  lostMask      = 0;                     /* 掉线位图，bit i = Arm_Motor[i] 掉线 */
  float    vx, vy, wz;
  uint32_t wakeTick      = osKernelGetTickCount();   /* P1-1：固定周期基准 */ 

  (void)argument;

  for (;;)
  {
    /* ================= 0. 遥控接收观测（调试用，可删） ================= */
    RC_DebugPoll();     /* 采集 PD2 电平/边沿、清极性变更 */
    /*
     * RC_AutoScan() 已关闭：接收机链路已经确认正常（cfg=0 即原始配置就够，
     * len=18/18、frm≈71/s、bad 恒 2）。保留它会在未锁定时每秒重配一次 UART，
     * 以后带电机调试时是不必要的风险。需要再做格式扫描时把下面一行放开即可。
     */
    // RC_AutoScan();

    /* ================= 1. 读遥控 ================= */

    /*
     * 遥控丢失保护（必须要有，否则车会"失控直冲"）：
     * DBUS 是 14ms 一帧。遥控器关机 / 失联 / 信号线松了以后，RC_IRQHandler()
     * 不再进来，Remote_controler.buffer 会保持最后一帧的值 —— Get_Switch_Value
     * 依然返回 sw_up、Get_Channel_Value 依然返回最后的摇杆量，于是就按最后那个
     * 指令一直跑下去。这里连续 50 拍（2ms × 50 = 100ms，约丢 7 帧）收不到新帧
     * 就判定遥控丢失，强制停车。
     */
    {
      static uint32_t rcLastFrm = 0U;
      static uint16_t rcLostCnt = 0U;
      uint32_t rcFrm = Get_RC_FrameCount();

      if (rcFrm == rcLastFrm)
      {
        if (rcLostCnt < 0xFFFFU) rcLostCnt++;
      }
      else
      {
        rcLastFrm = rcFrm;
        rcLostCnt = 0U;
      }

      rcOk = (rcLostCnt > 50U) ? 0U : 1U;
    }

    /* 左拨码当“安全开关”：打到 UP(1) 才允许输出；遥控丢失也同样停车 */
    enable = ((Get_Switch_Value(Switch_Left) == sw_up) && (rcOk != 0U)) ? 1U : 0U;
    //enable = 1U;   /* 先不管安全开关，直接输出 */

    /* 摇杆 → 底盘三轴速度（单位：转子 rpm）
       注意 Get_Channel_Value 的“正”是右/下，所以前后要取负 */
    vx = -Get_Channel_Value(CH_LeftVert)  * 3000.0f;   /* 前后 */
    vy =  Get_Channel_Value(CH_LeftHori)  * 3000.0f;   /* 横移 */
    wz =  Get_Channel_Value(CH_RightHori) * 3000.0f;   /* 旋转 */

    /* ================= 2. 麦轮解算 ================= */
    static float Target[4];
    Target[0] = -vy + vx - wz;      /* 左前 */
    Target[1] = vy + vx - wz;      /* 左后 */
    Target[2] = -vy + vx + wz;      /* 右后 */
    Target[3] = vy + vx + wz;      /* 右前 */

    /* P0-4：调试发送已移除。原来每 2ms 用 DMA 往 USART10 发 16 字节 float，
       115200 下约 1.4ms 一帧，几乎占满周期，且不检查 HAL_BUSY 返回值。 */
    /* 如果推杆方向反了，把上面 4 行的 +/- 号按实际情况调；

       若按输出轴转速给，要乘减速比：Target[i] *= 19.0f; */

    lostMask = 0U;                  /* 本拍重新统计，交给调试串口输出 */

    for (uint8_t i = 0; i < 4; i++)
    {
      float  real   = (float)Arm_Motor[i].FeedbackData.RealSpeed;
      float  pid_out;
      uint8_t lost = 0;

      /* 掉线保护：帧计数不变就累加，超过 50 拍(100ms) 判定掉线 */
      if (Arm_Motor[i].FeedbackData.FrameCounter == LastFrame[i])
      {
        if (LostCnt[i] < 0xFFFFU) LostCnt[i]++;
      }
      else
      {
        LastFrame[i] = Arm_Motor[i].FeedbackData.FrameCounter;
        LostCnt[i]   = 0;
      }
      lost = (LostCnt[i] > 50U) ? 1U : 0U;
      if (lost != 0U) lostMask |= (uint8_t)(1U << i);

      if ((enable == 0U) || (lost != 0U))
      {
        cur[i] = 0;
        PID_clear(&WheelPid[i]);
        continue;
      }

      if (WheelSign[i] < 0) real = -real;     /* 方向统一 */

      pid_out = Pid_cal(&WheelPid[i], Target[i], real);

      /* P1-2 抗积分饱和：只回退 sum_error。
         Iout 下一拍会被 Pid_cal() 用 ki*sum_error 重新算出来，改它没有作用。 */
      if ((fabsf(pid_out) >= (WheelPid[i].MaxOut - 0.5f)) &&
          ((WheelPid[i].error[0] * pid_out) > 0.0f))
      {
        WheelPid[i].sum_error -= WheelPid[i].error[0];
      }

      if (WheelSign[i] < 0) pid_out = -pid_out;   /* 与上面取反配对 */

      cur[i] = (pid_out >= 0.0f) ? (int16_t)(pid_out + 0.5f)
                                 : (int16_t)(pid_out - 0.5f);
    }

    /* ================= 4. 打包一帧 0x200 发出去 ================= */
    for (uint8_t i = 0; i < 4; i++)
    {
      sendmessage[2U * i]      = (uint8_t)((uint16_t)cur[i] >> 8);
      sendmessage[2U * i + 1U] = (uint8_t)((uint16_t)cur[i] & 0xFFU);
    }
    CANTransmit(&hfdcan1, 0x200U, sendmessage);

    /* ================= 调试串口输出（USART10 / PE3，115200 8N1） =================
       频率/开关在 BSP/Inc/Debug_Uart.h 里：
         DEBUG_UART_ENABLE     0 = 整个调试串口模块编译掉
         DEBUG_UART_CSV_MODE   1 = 纯逗号分隔，方便 Excel / 串口示波器画图
         DEBUG_UART_PRINT_DIV  每多少控制周期一行（50 → 2ms×50 = 100ms）
    */
#if (DEBUG_UART_ENABLE != 0)
    {
      static uint8_t  dbgBoot   = 0U;
      static uint8_t  dbgPrevEn = 0xFFU;
      static uint8_t  dbgPrevLd = 0xFFU;
      static uint16_t dbgDiv    = 0U;
      static uint32_t dbgSeq    = 0U;

      if (dbgBoot == 0U)
      {
        dbgBoot = 1U;
        Debug_Printf("=== debug uart ready (USART10/PE3, 115200 8N1) ===");
        Debug_Printf("!! 未接电调：ld=0F、S/I 全 0 属正常，只看 [rc] 行");
        Debug_Printf("[rc] frm=好帧(约71/s) bad=坏帧 bi=缓冲 ndtr=DMA最小剩余"
                     " fe/ne/ore=UART错误 inv=极性 pin=电平/边沿/高电平计数");
        Debug_Printf("[rc] sw=拨码 raw=原始通道(1024=中位,364~1684) pct=百分比 en=安全开关");
      }

      /* 安全开关动作（边沿触发，不刷屏） */
      if (enable != dbgPrevEn)
      {
        dbgPrevEn = enable;
        Debug_Printf("[evt] enable=%u switchL=%u",
                     (unsigned)enable,
                     (unsigned)Get_Switch_Value(Switch_Left));
      }

      /* 遥控链路状态变化（边沿触发）：LOST 时电机会被强制停 */
      {
        static uint8_t dbgPrevRcOk = 0xFFU;
        if (rcOk != dbgPrevRcOk)
        {
          dbgPrevRcOk = rcOk;
          Debug_Printf("[evt] RC link %s",
                       (rcOk != 0U) ? "OK" : "LOST -> motors forced stop");
        }
      }

      /* 掉线位图变化 */
      if (lostMask != dbgPrevLd)
      {
        dbgPrevLd = lostMask;
        Debug_Printf("[evt] lost=0x%02X", (unsigned)lostMask);
      }

      if (++dbgDiv >= DEBUG_UART_PRINT_DIV)
      {
        dbgDiv = 0U;
        dbgSeq++;

        /* ================= 接收机自检：每 100ms 两行 =================
           判读（只看 len / bad / by 三项就够）：
             frm  成功解出的帧数，约 71 帧/秒，一直在涨 = 链路通
             bad  帧长≠18 的次数；正常只有启动那 2 次
             bi   0/1 = 正常；2 = MEMORYRESET（帧长不匹配）
             len  最近/最大帧长 —— DBUS 恒为 18/18
             by   累计字节数，应严格按 18 × 帧率 增长
             inv  RXINV 极性；pin 电平为 1（空闲高）说明极性正确
           注意：fe（帧错误计数）在 H7 上是观测假象（改校验位后数值不变），
                 已从本行去掉避免误导；ne/ore 保留。
        */
        Debug_Printf("[rc] frm=%u bad=%u bi=%u len=%u/%u ne=%u ore=%u inv=%u"
                     " by=%u pin=%u/%u/%u",
                     (unsigned)Get_RC_FrameCount(),
                     (unsigned)Get_RC_BadFrameCount(),
                     (unsigned)Get_RC_BufferIndex(),
                     (unsigned)RC_Dbg_LastLen,
                     (unsigned)RC_Dbg_MaxLen,
                     (unsigned)RC_Dbg_NeCnt,
                     (unsigned)RC_Dbg_OreCnt,
                     (unsigned)RC_Dbg_InvertMode,
                     (unsigned)RC_RxByteTotal(),
                     (unsigned)RC_Dbg_PinLevel,
                     (unsigned)RC_Dbg_PinEdges,
                     (unsigned)RC_Dbg_PinHighCnt);


        /* raw = 原始通道值（1024 = 中位，364~1684 为量程）；pct = 百分比 */
        Debug_Printf("[rc] sw=%u,%u raw=%u,%u,%u,%u,%u pct=%d,%d,%d,%d,%d en=%u",
                     (unsigned)Get_Switch_Value(Switch_Left),
                     (unsigned)Get_Switch_Value(Switch_Right),
                     (unsigned)Get_Channel_Raw(CH_RightHori),
                     (unsigned)Get_Channel_Raw(CH_RightVert),
                     (unsigned)Get_Channel_Raw(CH_LeftHori),
                     (unsigned)Get_Channel_Raw(CH_LeftVert),
                     (unsigned)Get_Channel_Raw(CH_Roll),
                     (int)(Get_Channel_Value(CH_RightHori) * 100.0f),
                     (int)(Get_Channel_Value(CH_RightVert) * 100.0f),
                     (int)(Get_Channel_Value(CH_LeftHori)  * 100.0f),
                     (int)(Get_Channel_Value(CH_LeftVert)  * 100.0f),
                     (int)(Get_Channel_Value(CH_Roll)      * 100.0f),
                     (unsigned)enable);

        /* ================= 电机 / 解算：每 500ms 一行 =================
           未接电调时 ld 恒为 0F、S 与 I 全 0，属正常现象，不影响接收机自检。
        */
        if ((dbgSeq % 5U) == 0U)
        {
#if (DEBUG_UART_CSV_MODE != 0)
          Debug_Printf("%u,%u,%u,"          /* seq en ld   */
                       "%d,%d,%d,%d,"       /* T0..T3      */
                       "%d,%d,%d,%d,"       /* S0..S3      */
                       "%d,%d,%d,%d,"       /* I0..I3      */
                       "%u",                /* drop        */
                       (unsigned)dbgSeq, (unsigned)enable, (unsigned)lostMask,
                       (int)Target[0], (int)Target[1], (int)Target[2], (int)Target[3],
                       (int)Arm_Motor[0].FeedbackData.RealSpeed,
                       (int)Arm_Motor[1].FeedbackData.RealSpeed,
                       (int)Arm_Motor[2].FeedbackData.RealSpeed,
                       (int)Arm_Motor[3].FeedbackData.RealSpeed,
                       (int)cur[0], (int)cur[1], (int)cur[2], (int)cur[3],
                       (unsigned)Debug_DroppedCount());
#else
          Debug_Printf("[mtr] seq=%u en=%u ld=%u"
                       " T %d %d %d %d"
                       " S %d %d %d %d"
                       " I %d %d %d %d"
                       " drop=%u",
                       (unsigned)dbgSeq, (unsigned)enable, (unsigned)lostMask,
                       (int)Target[0], (int)Target[1], (int)Target[2], (int)Target[3],
                       (int)Arm_Motor[0].FeedbackData.RealSpeed,
                       (int)Arm_Motor[1].FeedbackData.RealSpeed,
                       (int)Arm_Motor[2].FeedbackData.RealSpeed,
                       (int)Arm_Motor[3].FeedbackData.RealSpeed,
                       (int)cur[0], (int)cur[1], (int)cur[2], (int)cur[3],
                       (unsigned)Debug_DroppedCount());
#endif
        }
      }
    }
#endif /* DEBUG_UART_ENABLE */

    /* P1-1：固定 2 tick（500 Hz）周期；若本拍超时就重新对齐，不做补拍 */
    wakeTick += 2U;
    if ((int32_t)(wakeTick - osKernelGetTickCount()) <= 0)
    {
      wakeTick = osKernelGetTickCount() + 2U;
    }
    osDelayUntil(wakeTick);
  
  }
  /* USER CODE END Task_Init */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

