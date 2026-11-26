/* -------------------------------- Includes -------------------------------- */
#include "RemoteControl_Comm.h"
/* ----------------------------- Private Defines ---------------------------- */
#define RC_INTERVAL 14
/*------------------------ Private macro Function ----------------------------*/

/* ---------------------- Static Functions Reference ------------------------ */
static uint8_t GetRcIndex(void);

static uint8_t RcDataUpdate(void);

static void Updata_Press_time(void);

static void Updata_Trigger(void);
/* ---------------------------- Global Variables ---------------------------- */
uint8_t RCBuffer[2][RC_FRAME_LEN+RC_FRAME_LEN_BACK];
extern UART_HandleTypeDef huart5;
extern DMA_HandleTypeDef hdma_uart5_rx;

/* ---------------------------- Static Variables ---------------------------- */
typedef struct __packed
{
    uint8_t buffer_index; //当前使用的缓存区
    uint8_t offline_check;//离线计数，用于判断遥控是否离线

    int32_t framecounter; //用于计数包数

    uint8_t swtrigger[2]; //拨码开关切换状态
    uint8_t mousetrigger[2];//鼠标切换状态
    uint16_t mousePresstime[2];//鼠标按下时间

    uint16_t keyPresstime[16]; //键盘按键按下时间
    uint16_t keytrigger;   //键盘按键切换状态
    __packed struct
    {
        __packed struct
        {
            /* data */
            switch_state sw[2];
            uint16_t ch[5];
        } rc;

        __packed struct
        {
            /* data */
            int16_t x;
            int16_t y;
            int16_t z;
            uint8_t press_l;
            uint8_t press_r;
        } mouse;

        __packed struct
        {
            /* data */
            uint16_t key_bit;
        } keyboard;
    } buffer[2];

} RCDecoding_Type;

RCDecoding_Type Remote_controler;

/* 接收机自检计数：成功解出的帧 / 被丢弃的帧（只在 RcDataUpdate 里累加） */
static uint32_t s_rcGoodFrames = 0U;
static uint32_t s_rcBadFrames  = 0U;

/* ---------------------- Static Functions Declarations   --------------------- */
uint8_t GetRcIndex(void)
{
    return (Remote_controler.buffer_index == MEMORYRESET) ? MEMORY0
                                                          : Remote_controler.buffer_index;
}

/**
 * @brief  遥控数据更新
 * @note   RC_IRQHandler中调用
 * @retval 更新成功返回1 失败返回0
 */
uint8_t RcDataUpdate(void)
{
    /* P0-2：原来是 GetRcIndex()，它永远不会返回 MEMORYRESET，这个分支是死代码，
       帧长不对时不会清零，脏数据会一直保留。这里必须判断原始字段。 */
    if (Remote_controler.buffer_index == MEMORYRESET)
    {
        Remote_controler.buffer[0].rc.sw[0] = sw_offset;
        Remote_controler.buffer[0].rc.sw[1] = sw_offset;

        Remote_controler.buffer[0].rc.ch[0] = RC_CH_VALUE_OFFSET;
        Remote_controler.buffer[0].rc.ch[1] = RC_CH_VALUE_OFFSET;
        Remote_controler.buffer[0].rc.ch[2] = RC_CH_VALUE_OFFSET;
        Remote_controler.buffer[0].rc.ch[3] = RC_CH_VALUE_OFFSET;
        Remote_controler.buffer[0].rc.ch[4] = RC_CH_VALUE_OFFSET;

        Remote_controler.buffer[0].mouse.x = MOUSE_SPEED_OFFSET;
        Remote_controler.buffer[0].mouse.y = MOUSE_SPEED_OFFSET;
        Remote_controler.buffer[0].mouse.z = MOUSE_SPEED_OFFSET;
        Remote_controler.buffer[0].mouse.press_l = MOUSE_PRESSED_OFFSET;
        Remote_controler.buffer[0].mouse.press_r = MOUSE_PRESSED_OFFSET;
        Remote_controler.buffer[0].keyboard.key_bit = KEY_OFFSET;

        Remote_controler.buffer[1] = Remote_controler.buffer[0];

        Remote_controler.mousePresstime[0] = Remote_controler.mousePresstime[1] = 0;
        Remote_controler.keyPresstime[0] = Remote_controler.keyPresstime[1] = Remote_controler.keyPresstime[2] = \
                                                                                                                 Remote_controler.keyPresstime[3] = Remote_controler.keyPresstime[4] = Remote_controler.keyPresstime[5] = \
                                                                                                                     Remote_controler.keyPresstime[6] = Remote_controler.keyPresstime[7] = Remote_controler.keyPresstime[8] = \
                                                                                                                         Remote_controler.keyPresstime[9] = Remote_controler.keyPresstime[10] = Remote_controler.keyPresstime[11] = \
                                                                                                                             Remote_controler.keyPresstime[12] = Remote_controler.keyPresstime[13] = Remote_controler.keyPresstime[14] = \
                                                                                                                                 Remote_controler.keyPresstime[15] = 0;
        Remote_controler.offline_check = 1;
        s_rcBadFrames++;            /* 帧长不对 / 没有有效帧 */
        return 0;
    }
    else
    {
        const uint8_t *pRaw = RCBuffer[GetRcIndex()];   /* 取本次 DMA 真正写入的那块缓冲 */

        Remote_controler.buffer[GetRcIndex()].rc.sw[0] = (switch_state)(((pRaw[5] >> 4) & 0x000C) >> 2); //!< Switch left
        Remote_controler.buffer[GetRcIndex()].rc.sw[1] = (switch_state)((pRaw[5] >> 4) & 0x0003);     //!< Switch right

        Remote_controler.buffer[GetRcIndex()].rc.ch[0] = ((pRaw[0] | (pRaw[1] << 8)) & 0x07ff);        //!< Channel 0
        Remote_controler.buffer[GetRcIndex()].rc.ch[1] = (((pRaw[1] >> 3) | (pRaw[2] << 5)) & 0x07ff); //!< Channel 1
        Remote_controler.buffer[GetRcIndex()].rc.ch[2] = (((pRaw[2] >> 6) | (pRaw[3] << 2) | (pRaw[4] << 10)) & 0x07ff);
        Remote_controler.buffer[GetRcIndex()].rc.ch[3] = (((pRaw[4] >> 1) | (pRaw[5] << 7)) & 0x07ff); //!< Channel 3
        Remote_controler.buffer[GetRcIndex()].rc.ch[4] = ((pRaw[16] | (pRaw[17] << 8)) & 0x07ff); //!< Channel 4

        Remote_controler.buffer[GetRcIndex()].mouse.x = pRaw[6] | (pRaw[7] << 8);   //!< Mouse X axis
        Remote_controler.buffer[GetRcIndex()].mouse.y = pRaw[8] | (pRaw[9] << 8);   //!< Mouse Y axis
        Remote_controler.buffer[GetRcIndex()].mouse.z = pRaw[10] | (pRaw[11] << 8); //!< Mouse Z axis
        Remote_controler.buffer[GetRcIndex()].mouse.press_l = pRaw[12];                    //!< Mouse Left Is Press ?
        Remote_controler.buffer[GetRcIndex()].mouse.press_r = pRaw[13];                    //!< Mouse Right Is Press ?
        Remote_controler.buffer[GetRcIndex()].keyboard.key_bit = pRaw[14] | (pRaw[15] << 8);//!< KeyBoard value

    }

    Updata_Press_time();
    Updata_Trigger();
    s_rcGoodFrames++;               /* 一帧成功解出 */
    return 1;
}

/* ---------------------------- global functions ---------------------------- */

/**
 * @brief  中断服务函数
 * @note   在串口中断服务函数中调用，判断接收状态并重置dma
 * @retval None
 */
/* ---- 字节级接收（不再用 DMA 双缓冲）---------------------------------------
   原方案靠 DMA 双缓冲 + 手动翻 CT + IDLE 算长度，本板实测 DMA 一个字节都没搬
   进去（len 恒 0），而 FE 又说明线上确实有数据。这条链路变量太多（DBM/CIRC/
   NDTR/CT/DMAMUX），改成最简单的字节中断：每字节进一次中断，IDLE 切帧，
   帧长就是实际收到的字节数 —— 一眼能看出到底是 18、25 还是收到的根本不是帧。
   1286 B/s 的字节中断量对 480MHz 内核可以忽略。
   ---------------------------------------------------------------------- */
#define RC_RX_BUF_MAX   RC_FRAME_LEN     /* 只缓存需要的 18 字节 */

static uint8_t  s_rxBuf[RC_RX_BUF_MAX];
static uint8_t  s_rxIdx   = 0U;          /* 本帧已收字节数（可以超过 18） */
static uint32_t s_rxTotal = 0U;          /* 累计收到的字节数 */

uint32_t RC_RxByteTotal(void)
{
    return s_rxTotal;
}

/**
 * @brief  UART5 中断服务：逐字节接收 + IDLE 切帧
 * @note   在 stm32h7xx_it.c 的 UART5_IRQHandler 里调用
 * @param  huart: 未使用（固定操作 huart5）
 * @retval None
 */
void RC_IRQHandler(UART_HandleTypeDef *huart)
{
    uint32_t isr;
    uint8_t  b;

    (void)huart;

    /*
     * 先取 ISR 快照：在 H7 上「读 RDR」会顺带清掉 PE/FE/NE，所以错误标志
     * 必须在读 RDR 之前抓，否则永远统计不到。
     */
    isr = huart5.Instance->ISR;

    /* 1. 把现成的字节全部取走（H7 的 RXNE 标志叫 RXNE_RXFNE，不是 RXNE） */
    while ((huart5.Instance->ISR & USART_ISR_RXNE_RXFNE) != 0U)
    {
        b = (uint8_t)(huart5.Instance->RDR & 0xFFU);

        s_rxTotal++;
        if (s_rxIdx < RC_RX_BUF_MAX)
            s_rxBuf[s_rxIdx] = b;
        s_rxIdx++;                      /* 超过 18 也继续计数，用来看真实帧长 */
    }

    /* 2. 错误计数（用快照），然后统一清标志 */
    if ((isr & USART_ISR_FE) != 0U)
        RC_Dbg_FeCnt++;
    if ((isr & USART_ISR_NE) != 0U)
        RC_Dbg_NeCnt++;
    if ((isr & USART_ISR_ORE) != 0U)
        RC_Dbg_OreCnt++;

    huart5.Instance->ICR = USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF;

    /* 3. IDLE = 一帧结束，按实际字节数判断 */
    if ((isr & USART_ISR_IDLE) != 0U)
    {
        huart5.Instance->ICR = USART_ICR_IDLECF;

        RC_Dbg_IdleCnt++;               /* 帧计数（等于收到的帧数） */
        RC_Dbg_LastLen = s_rxIdx;
        if (RC_Dbg_LastLen > RC_Dbg_MaxLen)
            RC_Dbg_MaxLen = RC_Dbg_LastLen;

        if (s_rxIdx == RC_FRAME_LEN)
        {
            /* 正好 18 字节：拷到 RCBuffer[0] 交给原解析函数 */
            memcpy(RCBuffer[0], s_rxBuf, RC_FRAME_LEN);
            Remote_controler.buffer_index = MEMORY0;
            s_rxIdx = 0U;
            RcDataUpdate();             /* 成功分支会累加 s_rcGoodFrames */
        }
        else
        {
            s_rxIdx = 0U;
            Remote_controler.buffer_index = MEMORYRESET;
            RcDataUpdate();             /* 失败分支会累加 s_rcBadFrames */
        }
    }
}

/**
 * @brief  获取遥控拨杆的值 百分比值
 * @note   CH_RightHori:右水平 CH_RightVert:右竖直 CH_LeftHori:左水平 CH_LeftVert:左竖直 CH_Roll:左侧拨轮
 * @param  channel: enum channel_num( CH_RightHori,CH_RightVert,CH_LeftHori,CH_LeftVert)
* @retval  对应通道数值百分比，-100% ~ 100%(右上正)
 */
float Get_Channel_Value(channel_num channel)
{
    switch (channel)
    {
        case CH_RightHori:
            if (abs(Remote_controler.buffer[GetRcIndex()].rc.ch[0] - RC_CH_VALUE_OFFSET) < 10)
                return 0;
            else
                return ((Remote_controler.buffer[GetRcIndex()].rc.ch[0] - RC_CH_VALUE_OFFSET)/660.0f);

        case CH_RightVert:
            if (abs((Remote_controler.buffer[GetRcIndex()].rc.ch[1] - RC_CH_VALUE_OFFSET)) < 10)
                return 0;
            else
                return ((Remote_controler.buffer[GetRcIndex()].rc.ch[1] - RC_CH_VALUE_OFFSET)/660.0f);

        case CH_LeftHori:
            if (abs((Remote_controler.buffer[GetRcIndex()].rc.ch[2] - RC_CH_VALUE_OFFSET)) < 10)
                return 0;
            else
                return ((Remote_controler.buffer[GetRcIndex()].rc.ch[2] - RC_CH_VALUE_OFFSET)/660.0f);

        case CH_LeftVert:
            if (abs((Remote_controler.buffer[GetRcIndex()].rc.ch[3] - RC_CH_VALUE_OFFSET)) < 10)
                return 0;
            else
                return ((Remote_controler.buffer[GetRcIndex()].rc.ch[3] - RC_CH_VALUE_OFFSET)/660.0f);

        case CH_Roll:
            if (abs((Remote_controler.buffer[GetRcIndex()].rc.ch[4] - RC_CH_VALUE_OFFSET)) < 10)
                return 0;
            else
                return ((Remote_controler.buffer[GetRcIndex()].rc.ch[4] - RC_CH_VALUE_OFFSET)/660.0f);

        default:
            return 0.0f;
    }
}

/**
 * @brief  取某通道的原始值
 * @note   接收机自检用：0~2047，1024 为中位。比百分比更能看出中位偏移/量程异常
 * @param  channel: enum channel_num
 * @retval 原始通道值；通道号非法返回 0
 */
uint16_t Get_Channel_Raw(channel_num channel)
{
    if ((uint8_t)channel > (uint8_t)CH_Roll)
        return 0U;

    return Remote_controler.buffer[GetRcIndex()].rc.ch[(uint8_t)channel];
}

/**
 * @brief  成功解出的帧数
 * @note   接收机每 14ms 一帧，正常应约 71 帧/秒；完全不涨说明一帧都没收到
 * @retval 帧计数
 */
uint32_t Get_RC_FrameCount(void)
{
    return s_rcGoodFrames;
}

/**
 * @brief  被丢弃的帧数
 * @note   持续增长说明 波特率 / 极性 / 帧长 不对
 * @retval 坏帧计数
 */
uint32_t Get_RC_BadFrameCount(void)
{
    return s_rcBadFrames;
}

/**
 * @brief  当前双缓冲下标
 * @note   0/1 = 正常；2(MEMORYRESET) = 帧长不匹配，处于复位态
 * @retval 缓冲下标
 */
uint8_t Get_RC_BufferIndex(void)
{
    return Remote_controler.buffer_index;
}

/**
 * @brief  遥控拨码开关值
 * @note   Switch_Left 左 Switch_Right右
 * @param  sw: enum rc_switch(Switch_Left,Switch_Right)
 * @retval switch_state(sw_up:1,sw_down:2,sw_mid:3)
 */
switch_state Get_Switch_Value(rc_switch sw)
{
    switch (sw)
    {
        case Switch_Left:
            /* P0-2：不能用 buffer_index 直接索引，它可能是 MEMORYRESET(2) 越界 */
            return Remote_controler.buffer[GetRcIndex()].rc.sw[0];

        case Switch_Right:
            return Remote_controler.buffer[GetRcIndex()].rc.sw[1];

        default:
            return sw_mid;
    }
}

/**
 * @brief  获取键盘按键状态
 * @note   返回值有待检验！！！
 * @param  key: 需判断的按键
 * @retval pressed:1  UnPressed:0
 */
uint8_t Get_KeyBoard_Value(uint16_t key)
{
    switch (key)
    {
        case KEY_W:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_W)&&1);

        case KEY_S:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_S)&&1);

        case KEY_A:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_A)&&1);

        case KEY_D:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_D)&&1);

        case KEY_SHIFT:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_SHIFT)&&1);

        case KEY_CTRL:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_CTRL)&&1);

        case KEY_Q:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_Q)&&1);

        case KEY_E:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_E)&&1);

        case KEY_R:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_R)&&1);

        case KEY_F:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_F)&&1);

        case KEY_G:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_G)&&1);

        case KEY_Z:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_Z)&&1);

        case KEY_X:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_X)&&1);

        case KEY_C:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_C)&&1);

        case KEY_V:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_V)&&1);

        case KEY_B:
            return ((Remote_controler.buffer[GetRcIndex()].keyboard.key_bit & KEY_PRESSED_OFFSET_B)&&1);

        default:
            return KEY_OFFSET;
    }
}

/**
 * @brief  获取键盘按住时间
 * @note   单位:ms  以及遥控器发送是14ms一帧
 * @param  key:
 * @retval 0-255 按键对应按下时间 单位:ms
 */
uint16_t Get_KeyBoard_Presstime(uint16_t key)
{
    //	if(Get_KeyBoard_Value(key)) Remote_controler.kPressTime++;
    //	uint8_t kt=Remote_controler.kPressTime;
    //	Remote_controler.kPressTime = 0;
    //	return kt;
    return 0;
}

/**
 * @brief  获取鼠标按下状态
 * @note
 * @param  key: 对应鼠标按键
 * @retval
 */
uint8_t Get_Mouse_State(mouse_key key)
{
    switch (key)
    {
        case mouse_left:
            return Remote_controler.buffer[GetRcIndex()].mouse.press_l;

        case mouse_right:
            return Remote_controler.buffer[GetRcIndex()].mouse.press_r;

        default:
            return MOUSE_PRESSED_OFFSET;
    }
}

/**
 * @brief  获取鼠标移动速度
 * @note
 * @param  axis: 鼠标三轴速度
 * @retval 鼠标对应方向移动速度(单位忘了。。。)
 */
int16_t Get_Mouse_Speed(mouse_axis axis)
{
    switch (axis)
    {
        case mouse_axis_x:
            return Remote_controler.buffer[GetRcIndex()].mouse.x;

        case mouse_axis_y:
            return Remote_controler.buffer[GetRcIndex()].mouse.y;

        case mouse_axis_z:
            return Remote_controler.buffer[GetRcIndex()].mouse.z;

        default:
            break;
    }

    return 0;
}

/**
 * @brief
 * @note
 * @param  key:
 * @retval 鼠标对应按键按压时间
 */
uint16_t Get_Mouse_Presstime(mouse_key key)
{
    return 0;
}

/**
 * @brief  获取所有键盘状态
 * @note
 * @retval 键盘状态位向量
 */
uint16_t Get_Key_All(void)
{
    return 0;
}

uint8_t GetSwTrigger(rc_switch sw)
{
    if (sw >= swcounter)
    {
        return 0;
    }

    return Remote_controler.swtrigger[sw];
}

uint8_t CleanSwTrigger(rc_switch sw)
{
    if (sw >= swcounter)
    {
        return 0;
    }

    Remote_controler.swtrigger[sw] = 0;
    return 1;
}

uint8_t GetMouseTrigger(mouse_key mouse)
{
    if (mouse >= mouse_counter)
    {
        return 0;
    }

    return Remote_controler.mousetrigger[mouse];
}

uint8_t CleanMouseTrigger(mouse_key mouse)
{
    if (mouse >= mouse_counter)
    {
        return 0;
    }

    Remote_controler.mousetrigger[mouse] = 0;
    return 1;
}

uint8_t GetKeyTrigger(uint8_t key)
{
    if (key >= 16)
    {
        return 0;
    }

    uint8_t trigger = (Remote_controler.keytrigger >> key) & 0x01;
    return trigger;
}

uint8_t CleanKeyTrigger(uint8_t key)
{
    if (key >= 16)
    {
        return 0;
    }

    Remote_controler.keytrigger = Remote_controler.keytrigger & (~((uint16_t)0x01 << key));
    return 1;
}

/**
 * @brief 初始化RC
 * @note  创建任务前调用
 * @retval
 */
void BSP_Init_RemoteControl(void)
{
    Remote_controler.framecounter = 0;                 /*帧率计数*/
    Remote_controler.offline_check = 0;                /*离线计数*/
    Remote_controler.buffer_index = MEMORYRESET;       /*串口缓存区默认值*/

    /* 改为字节中断接收：不用 DMA 了（见 RC_IRQHandler 上方说明） */
    s_rxIdx = 0U;
    __HAL_UART_ENABLE_IT(&RC_HUART, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&RC_HUART, UART_IT_IDLE);
}

/* ---------------------- 接收机帧格式自动扫描（调试用） ---------------------- */
/*
 * 目的：只接接收机、不接电调时，让固件自己找出"该用哪种帧格式"，免得反复烧写。
 * 每 RC_AUTOSCAN_TICKS 个控制周期换一种组合；一旦能解出帧（好帧数增长）就锁定。
 *
 * 组合编码 cfg：
 *   bit0 = 1 -> 9 位字长（= 8 数据位 + 偶校验，DBUS 的正确配置）
 *   bit1 = 1 -> 2 停止位（SBUS 用）
 *   bit2 = 1 -> 打开 RXINV 反相
 */
#define RC_AUTOSCAN_TICKS   500U        /* 2ms × 500 = 1s 换一种 */
#define RC_AUTOSCAN_COMBO   8U

static uint8_t  s_scanCfg     = 0U;     /* 当前组合编码 */
static uint8_t  s_scanLocked  = 0U;     /* 1 = 已锁定（找到能解帧的格式） */
static uint32_t s_scanBaseFrm = 0U;     /* 切换时刻的好帧数基准 */

uint8_t RC_ScanLocked(void)
{
    return s_scanLocked;
}

uint8_t RC_ScanCfg(void)
{
    return s_scanCfg;
}

/**
 * @brief  按组合编码重新配置 UART5 并重启接收
 * @note   只能在任务上下文调用（内部会 DeInit/Init UART）
 * @param  cfg: bit0=9位字长, bit1=2停止位, bit2=反相
 * @retval None
 */
void RC_ApplyCfg(uint8_t cfg)
{
    __HAL_UART_DISABLE_IT(&huart5, UART_IT_IDLE);
    __HAL_UART_DISABLE_IT(&huart5, UART_IT_RXNE);

    huart5.Init.WordLength = ((cfg & 0x01U) != 0U) ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
    huart5.Init.StopBits   = ((cfg & 0x02U) != 0U) ? UART_STOPBITS_2   : UART_STOPBITS_1;

    (void)HAL_UART_DeInit(&huart5);         /* 走 MspDeInit：关时钟/引脚/DMA/IRQ */
    if (HAL_UART_Init(&huart5) != HAL_OK)   /* 走 MspInit：重新配置 */
        return;

    /* 反相交给 RC_ApplyInvert() 统一处理，这里只同步期望值 */
    RC_Dbg_InvertMode = ((cfg & 0x04U) != 0U) ? 1U : 0U;

    huart5.Instance->ICR = USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF |
                           USART_ICR_ORECF | USART_ICR_IDLECF;

    RC_Dbg_FeCnt   = 0U;
    RC_Dbg_NdtrMin = 0xFFFFU;
    RC_Dbg_LastLen = 0U;
    RC_Dbg_MaxLen  = 0U;

    BSP_Init_RemoteControl();               /* 复位缓冲标志 + 重挂 DMA 双缓冲 + IDLE */
}

/**
 * @brief  自动扫描帧格式
 * @note   控制环里每 2ms 调用一次；锁定后自动停止
 * @retval None
 */
void RC_AutoScan(void)
{
    static uint16_t tick = 0U;

    if (s_scanLocked != 0U)
        return;

    /* 好帧数增长 -> 当前组合可用，锁定 */
    if (s_rcGoodFrames != s_scanBaseFrm)
    {
        s_scanLocked = 1U;
        return;
    }

    if (++tick < RC_AUTOSCAN_TICKS)
        return;

    tick = 0U;
    s_scanCfg = (uint8_t)((s_scanCfg + 1U) % RC_AUTOSCAN_COMBO);
    RC_ApplyCfg(s_scanCfg);
    s_scanBaseFrm = s_rcGoodFrames;
}

/* ----------------------- Static Function Definitions ---------------------- */
/**
 * @brief  更新键盘按压时间
 * @note   每次更新数据后执行
 * @retval none
 */
void Updata_Press_time(void)
{
    //鼠标按压时间更新
    if (Get_Mouse_State(mouse_left) == 1)
    {
        if (Remote_controler.mousePresstime[mouse_left] < 50000)
            Remote_controler.mousePresstime[mouse_left] += RC_INTERVAL;
    }
    else
    {
        Remote_controler.mousePresstime[mouse_left] = 0;
    }

    if (Get_Mouse_State(mouse_right) == 1)
    {
        if (Remote_controler.mousePresstime[mouse_right] < 50000)
            Remote_controler.mousePresstime[mouse_right] += RC_INTERVAL;
    }
    else
    {
        Remote_controler.mousePresstime[mouse_right] = 0;
    }

    //键盘按压时间更新
    for (int key = 0; key < 16; key++)
    {
        if (Get_KeyBoard_Value(key) == 1)
        {
            if (Remote_controler.keyPresstime[key] < 50000)
                Remote_controler.keyPresstime[key] += RC_INTERVAL;
        }
        else
        {
            Remote_controler.keyPresstime[key] = 0;
        }
    }

}


/**
 * @brief  更新切换状态
 * @note   如果拨杆或者按键状态发生切换，则标志位制1
 * @retval None
 */
void Updata_Trigger(void)
{
    if (Remote_controler.buffer[0].rc.sw[0] != Remote_controler.buffer[1].rc.sw[0])
    {
        Remote_controler.swtrigger[0] = 1;
    }

    if (Remote_controler.buffer[0].rc.sw[1] != Remote_controler.buffer[1].rc.sw[1])
    {
        Remote_controler.swtrigger[1] = 1;
    }


    if (Remote_controler.buffer[0].mouse.press_l != Remote_controler.buffer[1].mouse.press_l)
    {
        Remote_controler.mousetrigger[0] = 1;
    }

    if (Remote_controler.buffer[0].mouse.press_r != Remote_controler.buffer[1].mouse.press_r)
    {
        Remote_controler.mousetrigger[1] = 1;
    }

    Remote_controler.keytrigger = (Remote_controler.buffer[0].keyboard.key_bit) ^ (Remote_controler.buffer[1].keyboard.key_bit);
}

/* ---------------------- 调试观测桩（正式发布可删） ---------------------- */
volatile uint32_t RC_Dbg_FeCnt   = 0;      /* 帧错误(FE)次数 */volatile uint32_t RC_Dbg_NeCnt   = 0;      /* 噪声(NE)次数 */
volatile uint32_t RC_Dbg_OreCnt  = 0;      /* 溢出(ORE)次数 */
volatile uint32_t RC_Dbg_IdleCnt = 0;      /* IDLE 中断次数 = 收到的帧数 */
volatile uint32_t RC_Dbg_NdtrMin = 0xFFFF; /* NDTR 出现过的最小值：<20 说明字节真的进过 DMA */

/* IDLE 时实测到的帧长：正常 DBUS 应恒为 18。注意 ndtr 每 2ms 采样会和 14ms
   帧率混叠（14/2=7 整数倍），不能用来判长度，必须看这两个值。 */
volatile uint32_t RC_Dbg_LastLen = 0;      /* 最近一帧长度 */
volatile uint32_t RC_Dbg_MaxLen  = 0;      /* 见过的最大帧长 */

/*
 * 运行时极性开关：串口里直接看 [rc] 行的 inv= 字段，改这个值（0/1）后不用重烧。
 *   1 = 打开 RXINV（DBUS 反相信号，PD2 空闲为低时用这个）
 *   0 = 不反相（板上有硬件反相器、PD2 空闲为高时用这个）
 *
 * 注意：默认值必须和 usart.c 里 HAL_UART_Init 的配置一致！
 * usart.c 实测结论（DM-MC-Board02）：座子上已有硬件反相，两个反相都要关，
 * 否则双重反相 -> 每字节 FE、IDLE 不触发、一帧都收不到。
 * 原来这里默认是 1，RC_DebugPoll() 第一拍就通过 RC_ApplyInvert() 把 RXINV
 * 打开，等于把 usart.c 里关掉的又打开了，所以收不到帧。
 */
volatile uint32_t RC_Dbg_InvertMode = 0;

/*
 * 引脚探针：直接读 PD2 的原始电平，完全绕开 UART 的极性和波特率。
 * AF 模式下 GPIOD->IDR 仍然反映引脚实际电平，所以不需要改引脚配置。
 * 判读：
 *   RC_Dbg_PinHighCnt 一直是 0  -> PD2 从来没出现过高电平（线上没信号 / 接到 GND）
 *   RC_Dbg_PinHighCnt > 0 且 RC_Dbg_NdtrMin 恒 20 -> 线上有信号但 UART 没解出来
 */
volatile uint32_t RC_Dbg_PinEdges   = 0;   /* 采样到的电平跳变次数 */
volatile uint32_t RC_Dbg_PinHighCnt = 0;   /* 采样到高电平的次数 */
volatile uint32_t RC_Dbg_PinLevel   = 0;   /* 最近一次采样到的电平（0/1） */

/* 使能位快照：用来确认 DMA/UART 中途有没有被关掉 */
volatile uint32_t RC_Dbg_DmaCr   = 0;      /* DMA1_Stream1->CR：bit0=EN bit8=CIRC bit18=DBM */
volatile uint32_t RC_Dbg_UartCr1 = 0;      /* UART5->CR1：bit0=UE bit2=RE bit4=IDLEIE bit5=RXNEIE */
volatile uint32_t RC_Dbg_UartCr2 = 0;      /* UART5->CR2：bit16=RXINV bit18=DATAINV */

/*
 * 高速突发采样（每秒一次，每次约 20ms 连续读 PD2，绕过 2ms 轮询的漏采）
 *   RC_Dbg_BurstEdges : 这 20ms 内 PD2 的跳变次数
 *   RC_Dbg_BurstHigh  : 采样到高电平的次数
 * 判读：
 *   DBUS 正常发帧   -> BurstEdges 几百（14ms 一帧，每帧 18 字节 x 约 10 个跳变）
 *   PWM 接收机输出  -> BurstEdges 约 2（20ms 一个 1~2ms 脉冲）
 *   线上无任何信号  -> BurstEdges 0
 */
volatile uint32_t RC_Dbg_BurstEdges = 0;
volatile uint32_t RC_Dbg_BurstHigh  = 0;
volatile uint32_t RC_Dbg_BurstDone  = 0;

/*
 * 回环自测开关（验证 UART5+DMA+IDLE 这条链路本身）：
 *   0 = 正常使用接收机
 *   1 = 自测模式：强制不反相、每 100ms 发 1 字节；此时需用杜邦线短接 PD2 与 PB13（本板无 PB13 则保持 0）
 *       预期：RC_Dbg_NdtrMin 从 20 变成 19、RC_Dbg_IdleCnt 开始增长、RC_Dbg_FeCnt 保持 0
 */
#define RC_SELFTEST_LOOPBACK   0

/**
 * @brief  对 PD2 做一次约 20ms 的高速电平采样，统计边沿数
 * @note   纯诊断用：占用约 20ms CPU（每秒一次），正式版可删。
 */
static void RC_PinBurst(void)
{
    uint32_t last  = ((GPIOD->IDR >> 2) & 0x1U);
    uint32_t edges = 0U;
    uint32_t high  = 0U;
    uint32_t n;

    for (n = 0U; n < 2000000U; n++)
    {
        uint32_t cur = ((GPIOD->IDR >> 2) & 0x1U);   /* volatile 读，不会被优化掉 */
        if (cur != last)
        {
            edges++;
            last = cur;
        }
        high += cur;
    }

    RC_Dbg_BurstEdges = edges;
    RC_Dbg_BurstHigh  = high;
    RC_Dbg_BurstDone++;
}

/**
 * @brief  按 RC_Dbg_InvertMode 重新配置 RX 极性
 * @note   关键点：USART 的 CR2 在 UE=1 时是写不进去的（RM 要求先禁用 USART），
 *         所以必须先 UE=0 -> 改 CR2 -> UE=1，再清标志、重装 DMA、重新使能 IDLE 中断。
 */
static void RC_ApplyInvert(void)
{
    /* 1. 禁用 USART（只关 RE 是不够的，CR2 会被硬件忽略） */
    CLEAR_BIT(huart5.Instance->CR1, USART_CR1_UE);

    /* 2. 改极性 */
    if (RC_Dbg_InvertMode != 0U)
    {
        SET_BIT(huart5.Instance->CR2, USART_CR2_RXINV);
    }
    else
    {
        CLEAR_BIT(huart5.Instance->CR2, USART_CR2_RXINV);
    }
    CLEAR_BIT(huart5.Instance->CR2, USART_CR2_DATAINV);

    /* 3. 重新使能 USART */
    SET_BIT(huart5.Instance->CR1, USART_CR1_UE);

    /* 4. 清错误标志 */
    huart5.Instance->ICR = USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF |
                           USART_ICR_ORECF | USART_ICR_IDLECF;

    Remote_controler.buffer_index = MEMORYRESET;
    s_rxIdx = 0U;

    /* 5. 重新使能接收 + 字节/IDLE 中断（UE=0/1 过程会把这些位复位） */
    SET_BIT(huart5.Instance->CR1, USART_CR1_RE);
    __HAL_UART_ENABLE_IT(&huart5, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart5, UART_IT_IDLE);
}

/**
 * @brief  遥控接收调试观测
 * @note   放在 2ms 控制循环里周期调用，开销很小。
 *         作用：把锁存在 ISR 里的错误标志清掉并累计计数，同时记录 NDTR 的最小值。
 *         判读：
 *           RC_Dbg_NdtrMin < 20           -> 字节进过 DMA（数据在进来）
 *           RC_Dbg_FeCnt 持续增长          -> 有波形但帧格式错（极性/波特率不对）
 *           RC_Dbg_NdtrMin 恒 20 且 Fe=0  -> 线上没有任何数据
 */
void RC_DebugPoll(void)
{
    static uint32_t last_level  = 0;
    static uint32_t applied_mode = 0xFFFFFFFFU;
    /*
     * 注意：FE/NE/ORE/IDLE 这些标志现在由 RC_IRQHandler() 独占计数并清除。
     * 这里不再重复统计，否则同一个 FE 标志会被数两遍（曾经出现过 ~700/秒
     * 的虚高 fe 值，实际收帧完全正常）。
     */
    /* H7 的 Instance 是 void*，必须强转后才能读 NDTR */
    uint32_t ndtr = ((DMA_Stream_TypeDef *)hdma_uart5_rx.Instance)->NDTR;
    uint32_t lv   = ((GPIOD->IDR & GPIO_PIN_2) != 0U) ? 1U : 0U;   /* PD2 原始电平 */

    /* ---- 极性开关：调试器里改 RC_Dbg_InvertMode 后自动生效 ---- */
#if (RC_SELFTEST_LOOPBACK == 1)
    RC_Dbg_InvertMode = 0;      /* 回环自测必须不反相（TX 输出的是正常电平） */
#endif
    if (applied_mode != RC_Dbg_InvertMode)
    {
        applied_mode = RC_Dbg_InvertMode;
        RC_ApplyInvert();
    }

    /* ---- 使能位快照 ---- */
    RC_Dbg_DmaCr   = ((DMA_Stream_TypeDef *)hdma_uart5_rx.Instance)->CR;
    RC_Dbg_UartCr1 = huart5.Instance->CR1;
    RC_Dbg_UartCr2 = huart5.Instance->CR2;

    /* ---- 回环自测：每 100ms 自发 1 字节 ---- */
#if (RC_SELFTEST_LOOPBACK == 1)
    {
        static uint32_t st_tick = 0;
        if (++st_tick >= 50U)
        {
            st_tick = 0U;
            (void)HAL_UART_Transmit(&huart5, (uint8_t *)"U", 1U, 5U);
        }
    }
#endif

    /*
     * P0-4：原来这里每秒触发一次 RC_PinBurst()（200 万次 volatile 读空转），
     * 在 2ms 控制环里会独占 CPU 十几毫秒，导致每秒丢掉 10+ 拍控制，
     * 而且 CAN1 RX FIFO0 只有 8 深度，必然溢出丢反馈。
     * 需要再看边沿数时，把下面这段打开并在调试器里手动调用一次 RC_PinBurst()。
     */
#if 0
    {
        static uint32_t burst_tick = 0;
        if (++burst_tick >= 500U)      /* 2ms x 500 = 1s */
        {
            burst_tick = 0U;
            RC_PinBurst();
        }
    }
#endif

    /* ---- 引脚探针：不依赖 UART ---- */
    if (lv != last_level)
    {
        RC_Dbg_PinEdges++;
        last_level    = lv;
        RC_Dbg_PinLevel = lv;
    }
    if (lv != 0U)
    {
        RC_Dbg_PinHighCnt++;
    }

    if (ndtr < RC_Dbg_NdtrMin)
    {
        RC_Dbg_NdtrMin = ndtr;
    }

    /* 错误标志的计数与清除全部在 RC_IRQHandler() 里完成，这里不再处理 */
}

/**
 * @brief  把上面几个观测变量清零（调试器里手动调一次即可重新统计）
 */
void RC_DebugReset(void)
{
    RC_Dbg_FeCnt   = 0;
    RC_Dbg_NeCnt   = 0;
    RC_Dbg_OreCnt  = 0;
    RC_Dbg_IdleCnt = 0;
    RC_Dbg_NdtrMin = 0xFFFF;
}
