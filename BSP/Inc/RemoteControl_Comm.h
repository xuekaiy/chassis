#ifndef _REMOTE_CONTROL_H_
#define _REMOTE_CONTROL_H_

/* -------------------------------- Includes -------------------------------- */
#include "sysconfig.h"
/* ---------------------------- Defined constants --------------------------- */

#define RC_HUART huart5

#define RC_CH_MAX_RELATIVE 660.0f				//遥控器通道相对最大值

#define RC_FRAME_LEN        18U         //遥控器数据帧长
#define RC_FRAME_LEN_BACK   2U          //增加两个字节保持稳定

/* ----------------------- RC Channel Definition---------------------------- */
#define RC_CH_VALUE_MIN ((uint16_t)364 )
#define RC_CH_VALUE_OFFSET ((uint16_t)1024)
#define RC_CH_VALUE_MAX ((uint16_t)1684)
#define RC_CH0    ((uint8_t)0)
#define RC_CH1    ((uint8_t)1)
#define RC_CH2    ((uint8_t)2)
#define RC_CH3    ((uint8_t)3)
#define RC_CH4    ((uint8_t)4)
/* ----------------------- RC Switch Definition----------------------------- */
#define RC_SW_UP    ((uint16_t)1)
#define RC_SW_MID   ((uint16_t)3)
#define RC_SW_DOWN  ((uint16_t)2)
#define RC_SW_Right ((uint8_t)0)
#define RC_SW_Left  ((uint8_t)1)
/* ----------------------- PC Key Definition-------------------------------- */
#define KEY_PRESSED_OFFSET_W ((uint16_t)0x01<<0)//改成了uint16_t,原来是uint8_t
#define KEY_PRESSED_OFFSET_S ((uint16_t)0x01<<1)
#define KEY_PRESSED_OFFSET_A ((uint16_t)0x01<<2)
#define KEY_PRESSED_OFFSET_D ((uint16_t)0x01<<3)
#define KEY_PRESSED_OFFSET_SHIFT ((uint16_t)0x01<<4)
#define KEY_PRESSED_OFFSET_CTRL ((uint16_t)0x01<<5)
#define KEY_PRESSED_OFFSET_Q ((uint16_t)0x01<<6)
#define KEY_PRESSED_OFFSET_E ((uint16_t)0x01<<7)
#define KEY_PRESSED_OFFSET_R ((uint16_t)0x01<<8)
#define KEY_PRESSED_OFFSET_F ((uint16_t)0x01<<9)
#define KEY_PRESSED_OFFSET_G ((uint16_t)0x01<<10)
#define KEY_PRESSED_OFFSET_Z ((uint16_t)0x01<<11)
#define KEY_PRESSED_OFFSET_X ((uint16_t)0x01<<12)
#define KEY_PRESSED_OFFSET_C ((uint16_t)0x01<<13)
#define KEY_PRESSED_OFFSET_V ((uint16_t)0x01<<14)
#define KEY_PRESSED_OFFSET_B ((uint16_t)0x01<<15)


#define KEY_W         ((uint8_t)0) 
#define KEY_S         ((uint8_t)1)
#define KEY_A         ((uint8_t)2)
#define KEY_D         ((uint8_t)3)
#define KEY_SHIFT     ((uint8_t)4)
#define KEY_CTRL      ((uint8_t)5)
#define KEY_Q         ((uint8_t)6)
#define KEY_E         ((uint8_t)7)
#define KEY_R         ((uint8_t)8)
#define KEY_F         ((uint8_t)9)
#define KEY_G         ((uint8_t)10)
#define KEY_Z         ((uint8_t)11)
#define KEY_X         ((uint8_t)12)
#define KEY_C         ((uint8_t)13)
#define KEY_V         ((uint8_t)14)
#define KEY_B         ((uint8_t)15)
#define KEY_OFFSET    ((uint8_t)0)
/* ----------------------- PC Mouse Definition-------------------------------- */
#define MOUSE_X                 ((uint8_t)0)
#define MOUSE_Y                 ((uint8_t)1)
#define MOUSE_Z                 ((uint8_t)2)
#define MOUSE_SPEED_OFFSET      ((uint16_t)0)
#define MOUSE_LEFT              ((uint8_t)3)
#define MOUSE_RIGHT             ((uint8_t)4)
#define MOUSE_PRESSED_OFFSET    ((uint8_t)0)
/* ----------------------------- Macro functions ---------------------------- */

/* ------------------------------- Enum types ------------------------------- */
typedef enum 
{
    CH_RightHori,   //right horizon;	channel0
    CH_RightVert,   //right vertical;	channel1
    CH_LeftHori,   	//left  horizon;	channel2
    CH_LeftVert,   	//left  vertical;	channel3
		CH_Roll					//left wheel;			channel4
}channel_num;

typedef enum 
{
    Switch_Left,
    Switch_Right,
		swcounter
}rc_switch;

typedef enum
{
    sw_offset, //未赋值
    sw_up = 1,
    sw_down,
    sw_mid
}switch_state;

//鼠标键状态位序
typedef enum
{
  mouse_left = 0,
  mouse_right,
  mouse_counter
}mouse_key;
//鼠标轴位序
typedef enum
{
  mouse_axis_x = 0,
  mouse_axis_y,
  mouse_axis_z
}mouse_axis;

/* --------------------------------- Structs -------------------------------- */

/* ------------------------- Global variables extern ------------------------ */

/* ---------------------------- Global Functions ---------------------------- */

/**
 * @brief 初始化RC
 * @note  创建任务前调用
 * @retval
 */
void BSP_Init_RemoteControl(void);

 /**
  * @brief  中断服务函数
  * @note   在串口中断服务函数中调用，判断接收状态并重置dma
  * @retval None
  */
void RC_IRQHandler(UART_HandleTypeDef *huart);

 /**
  * @brief  获取遥控拨杆的值 百分比值
  * @note   CH_RightHori:右水平 CH_RightVert:右竖直 CH_LeftHori:左水平 CH_LeftVert:左竖直
  * @param  channel: enum channel_num( CH_RightHori,CH_RightVert,CH_LeftHori,CH_LeftVert)
  * @retval 对应通道数值百分比，-100% ~ 100%(右下正)
  */
float Get_Channel_Value(channel_num channel);

 /**
  * @brief  遥控拨码开关值
  * @note   Switch_Left 左 Switch_Right右
  * @param  sw: enum rc_switch(Switch_Left,Switch_Right)
  * @retval switch_state(sw_up:1,sw_down:2,sw_mid:3) 
  */
switch_state Get_Switch_Value(rc_switch sw);

 /**
  * @brief  获取键盘按键状态
  * @note   
  * @param  key: 需判断的按键状态
  * @retval pressed:0  UnPressed:1 
  */
uint8_t Get_KeyBoard_Value(uint16_t key);

 /**
  * @brief  获取键盘按住时间
  * @note   单位:ms  以及遥控器发送是14ms一帧
  * @param  key: 
  * @retval 0-255 按键对应按下时间 单位:ms
  */
uint16_t Get_KeyBoard_Presstime(uint16_t key);

 /**
  * @brief  获取鼠标按下状态
  * @note   
  * @param  key: 对应鼠标按键
  * @retval 
  */
uint8_t Get_Mouse_State(mouse_key key);
 /**
  * @brief  获取鼠标移动速度
  * @note   
  * @param  axis: 鼠标三轴速度
  * @retval 鼠标对应方向移动速度(单位忘了。。。)
  */
int16_t Get_Mouse_Speed(mouse_axis axis);

 /**
  * @brief  
  * @note   
  * @param  key: 
  * @retval 鼠标对应按键按压时间
  */
uint16_t Get_Mouse_Presstime(mouse_key key);

/**
 * @brief  获取所有键盘状态
 * @note   
 * @retval 键盘状态位向量
 */
uint16_t Get_Key_All(void);

uint8_t GetSwTrigger(rc_switch sw);
uint8_t CleanSwTrigger(rc_switch sw);

uint8_t GetMouseTrigger(mouse_key mouse);
uint8_t CleanMouseTrigger(mouse_key mouse);

uint8_t GetKeyTrigger(uint8_t key);
uint8_t CleanKeyTrigger(uint8_t key);

/* ---------------------- 接收机自检接口（调试用） ---------------------- */
/* 这几个接口专门给"只接接收机、不接电调"的链路自检用 */
uint16_t Get_Channel_Raw(channel_num channel);   /* 原始值 0~2047，1024 = 中位 */
uint32_t Get_RC_FrameCount(void);                /* 成功解出的帧数（正常约 71 帧/秒） */
uint32_t Get_RC_BadFrameCount(void);             /* 帧长不对被丢弃的次数 */
uint8_t  Get_RC_BufferIndex(void);               /* 0/1 = 正常，2(MEMORYRESET) = 复位态 */

/* ---------------- 接收机帧格式自动扫描（调试用） ---------------- */
/* IDLE 时实测的帧长：正常 DBUS 应恒为 18。注意 ndtr 是每 2ms 采样的，
   会和 14ms 帧率混叠（14/2=7 整数倍），不能用它判长度，要看这两个值。 */
extern volatile uint32_t RC_Dbg_LastLen;   /* 最近一帧长度 */
extern volatile uint32_t RC_Dbg_MaxLen;    /* 见过的最大帧长 */

uint32_t RC_RxByteTotal(void);             /* 累计收到的字节数：一直不涨 = 线上没数据 */

void    RC_AutoScan(void);   /* 控制环每 2ms 调一次：自动轮换 8 种帧格式，解出帧即锁定 */
uint8_t RC_ScanLocked(void); /* 1 = 已找到能解帧的格式 */
uint8_t RC_ScanCfg(void);    /* 锁定/当前的格式：bit0=9位字长 bit1=2停止位 bit2=反相 */

/* ---------------------- 调试观测（正式发布可删） ---------------------- */
/*
 * 用法：在 2ms 控制循环里调用 RC_DebugPoll()，然后在调试器监视：
 *   RC_Dbg_NdtrMin : NDTR 最小值，< 20 说明字节进过 DMA（数据在进来）
 *   RC_Dbg_IdleCnt : IDLE 中断次数，约等于收到的帧数
 *   RC_Dbg_FeCnt / RC_Dbg_NeCnt / RC_Dbg_OreCnt : 帧错误 / 噪声 / 溢出 次数
 */
extern volatile uint32_t RC_Dbg_FeCnt;
extern volatile uint32_t RC_Dbg_NeCnt;
extern volatile uint32_t RC_Dbg_OreCnt;
extern volatile uint32_t RC_Dbg_IdleCnt;
extern volatile uint32_t RC_Dbg_NdtrMin;

/* 引脚电平探针（直接读 PD2，绕过 UART，用来区分"没信号"和"解不出来"） */
extern volatile uint32_t RC_Dbg_PinEdges;
extern volatile uint32_t RC_Dbg_PinHighCnt;
extern volatile uint32_t RC_Dbg_PinLevel;

/* 使能位快照：确认 DMA / UART 中途有没有被关掉 */
extern volatile uint32_t RC_Dbg_DmaCr;     /* bit0=EN bit8=CIRC bit18=DBM */
extern volatile uint32_t RC_Dbg_UartCr1;   /* bit0=UE bit2=RE bit4=IDLEIE */
extern volatile uint32_t RC_Dbg_UartCr2;   /* bit16=RXINV bit18=DATAINV */

/* 高速突发采样（每秒一次，20ms 窗口）：区分 DBUS 数据流 / PWM 脉冲 / 无信号 */
extern volatile uint32_t RC_Dbg_BurstEdges;   /* 几百=DBUS 数据流；约 2=PWM；0=无信号 */
extern volatile uint32_t RC_Dbg_BurstHigh;
extern volatile uint32_t RC_Dbg_BurstDone;

/*
 * 运行时极性开关：调试器里直接改这个值即可，2ms 内生效，不用重烧。
 *   1 = 打开 RXINV（DBUS 反相信号，PD2 空闲为低）
 *   0 = 不反相（板上有硬件反相器，PD2 空闲为高）
 */
extern volatile uint32_t RC_Dbg_InvertMode;

void RC_DebugPoll(void);
void RC_DebugReset(void);

#endif
