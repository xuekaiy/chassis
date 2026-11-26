/** Include Headder Files **/
#include "DM_driver.h"
#include "Task_Can.h"
/**Variable Definition**/

/**Function Declaration**/
/**
 * @description: DMSend Tasks
 * @param Motor_ID, CAN
 * @return: void
 * @note: Start the DM Motor
 */
uint8_t sb_count = 0;
static uint8_t DM_IsPiPositionType(motor_type type)
{
    return (type == MOTOR_DM8009);
}

static float DM_TorqueMinGet(motor_type type)
{
    if (type == MOTOR_DM10010)
        return DM10010_T_MIN;
    if (type == MOTOR_DM8009)
        return -54.0f;
    return T_MIN;
}

static float DM_TorqueMaxGet(motor_type type)
{
    if (type == MOTOR_DM10010)
        return DM10010_T_MAX;
    if (type == MOTOR_DM8009)
        return 54.0f;
    return T_MAX;
}

void DM_motorOpen(motor_t* mot)
{
    static CanSend_Type CANSend;

    CANSend.CANx = mot->CAN_ID;

    CANSend.stdid = mot->id;
		
		mot->State=OPEN;

    CANSend.Data[0] = 0xFF;
    CANSend.Data[1] = 0xFF;
    CANSend.Data[2] = 0xFF;
    CANSend.Data[3] = 0xFF;
    CANSend.Data[4] = 0xFF;
    CANSend.Data[5] = 0xFF;
    CANSend.Data[6] = 0xFF;
    CANSend.Data[7] = 0xFC;


    xQueueSend(Queue_CANSend, &CANSend, 3);
}

/**
 * @description: DMSend Tasks
 * @param Motor_ID, CAN
 * @return: void
 * @note: Close the DM Motor
 */
void DM_motorClose(motor_t* mot)
{
    static CanSend_Type CANSend;

    CANSend.CANx = mot->CAN_ID;
    CANSend.stdid = mot->id;
    /* 关闭命令一旦入队，软件状态必须同步为 CLOSE。 */
    mot->State = CLOSE;

    CANSend.Data[0] = 0xFF;
    CANSend.Data[1] = 0xFF;
    CANSend.Data[2] = 0xFF;
    CANSend.Data[3] = 0xFF;
    CANSend.Data[4] = 0xFF;
    CANSend.Data[5] = 0xFF;
    CANSend.Data[6] = 0xFF;
    CANSend.Data[7] = 0xFD;
    xQueueSend(Queue_CANSend, &CANSend, 3);
}

// �����ѹ�����ȴ����־
void DM_motorErrClear(motor_t mot)
{
    static CanSend_Type CANSend;

    CANSend.CANx = mot.CAN_ID;
		//此处stdid的设置需要与控制模式一致。
    CANSend.stdid = mot.id;

    CANSend.Data[0] = 0xFF;
    CANSend.Data[1] = 0xFF;
    CANSend.Data[2] = 0xFF;
    CANSend.Data[3] = 0xFF;
    CANSend.Data[4] = 0xFF;
    CANSend.Data[5] = 0xFF;
    CANSend.Data[6] = 0xFF;
    CANSend.Data[7] = 0xFB;

    xQueueSend(Queue_CANSend, &CANSend, 3);
}

// �������
void DM_motorZeroSet(motor_t mot)
{
    static CanSend_Type CANSend;

    CANSend.CANx = mot.CAN_ID;

    CANSend.stdid = mot.id;

    CANSend.Data[0] = 0xFF;
    CANSend.Data[1] = 0xFF;
    CANSend.Data[2] = 0xFF;
    CANSend.Data[3] = 0xFF;
    CANSend.Data[4] = 0xFF;
    CANSend.Data[5] = 0xFF;
    CANSend.Data[6] = 0xFF;
    CANSend.Data[7] = 0xFE;

    xQueueSend(Queue_CANSend, &CANSend, 3);
}

/**
 * @description: DMSend Tasks
 * @param Motor_ID, CAN
 * @return: void
 * @note: Control the Motor with position and speed
 */
void DM_speedpositionControl(motor_t mot, float _vel)
{
    uint8_t *pbuf, *vbuf;
	float _pos=mot.Targetcirnum*360.0f+mot.TargetAngle;
    _pos = DM_DEG_TO_RAD(_pos);
    pbuf = (uint8_t *)&_pos;
    vbuf = (uint8_t *)&_vel;

    static CanSend_Type CANSend;

    CANSend.CANx = mot.CAN_ID;

    CANSend.stdid = DMJ_STDID + mot.id;

    CANSend.Data[0] = *pbuf;
    CANSend.Data[1] = *(pbuf + 1);
    CANSend.Data[2] = *(pbuf + 2);
    CANSend.Data[3] = *(pbuf + 3);
    CANSend.Data[4] = *vbuf;
    CANSend.Data[5] = *(vbuf + 1);
    CANSend.Data[6] = *(vbuf + 2);
    CANSend.Data[7] = *(vbuf + 3);

    xQueueSend(Queue_CANSend, &CANSend, 3);
}

void DM_mitControl(motor_t *mot, float f_vel, float f_tff)
{
    
    float f_pos = mot->Targetcirnum * 360.0f + mot->TargetAngle;
    float tor = mot->Ki * mot->SumError;
    
    // 建议加上 CAN_ID 校验防止多总线同 ID 误判
    if(mot->id == Arm_Motor[MGripper].id && mot->CAN_ID == Arm_Motor[MGripper].CAN_ID)
    {
        tor = f_tff;
    }

    uint16_t i_pos;
    // DM8009 使用 [-pi, +pi] 位置量程；其他 DM 维持原工程换算。
    if(DM_IsPiPositionType(mot->type) != 0U)
    {
        // 角度转弧度，[-3.14, 3.14] 对应 [-180°, 180°]。
        f_pos = DM_DEG_TO_RAD(f_pos);
        // 修复：最小值 -3.14，最大值 3.14
        i_pos = float_to_uint(f_pos, -DM_PI, DM_PI, 16);
    }
    else
    {
        // 普通电机
        f_pos = DM_DEG_TO_RAD(f_pos);
        i_pos = float_to_uint(f_pos, P_MIN, P_MAX, 16);
    }

    uint16_t i_vel = float_to_uint(f_vel, V_MIN, V_MAX, 12);
    uint16_t i_tff = float_to_uint(tor,
                                   DM_TorqueMinGet(mot->type),
                                   DM_TorqueMaxGet(mot->type),
                                   12);

    // 获取 Kp Kd
    /*
     * 新参数按物理增益填写。兼容旧工程：Kd 大于物理上限 5 时，
     * Kp/Kd 均视为旧 12 位原始码，并在驱动内部换算后重新编码。
    */
    float kp_physical = mot->Kp;
    float kd_physical = mot->Kd;
    if (mot->Kd > KD_MAX)
    {
        kp_physical = KP_MIN +
                      ((mot->Kp + 0.25f) * (KP_MAX - KP_MIN) / 4095.0f);
        kd_physical = KD_MIN +
                      ((mot->Kd + 0.25f) * (KD_MAX - KD_MIN) / 4095.0f);
    }
    uint16_t Kp = float_to_uint(kp_physical, KP_MIN, KP_MAX, 12);
    uint16_t Kd = float_to_uint(kd_physical, KD_MIN, KD_MAX, 12);

    // 发送打包
    static CanSend_Type CANSend;
    CANSend.CANx = mot->CAN_ID;
    CANSend.stdid = mot->id;
    CANSend.Data[0] = (uint8_t)((i_pos >> 8) & 0xFF);
    CANSend.Data[1] = (uint8_t)(i_pos & 0xFF);
    CANSend.Data[2] = (uint8_t)((i_vel >> 4) & 0xFF);
    CANSend.Data[3] = (uint8_t)((i_vel & 0x0F) << 4) | ((Kp >> 8) & 0x0F);
    CANSend.Data[4] = (uint8_t)(Kp & 0xFF);
    CANSend.Data[5] = (uint8_t)((Kd >> 4) & 0xFF);
    CANSend.Data[6] = (uint8_t)((Kd & 0x0F) << 4) | ((i_tff >> 8) & 0x0F);
    CANSend.Data[7] = (uint8_t)(i_tff & 0xFF);
    
    xQueueSend(Queue_CANSend, &CANSend, 3);
}
    

/**
 * @description: DMReceive Tasks
 * @param Motor, Data
 * @return: void
 * @note: Receive the Data of the Motor
 */
uint32_t error_code;

void DM_infoHandle(motor_t *mot, uint8_t *Data)
{
    Motor_Inf mtr;
    float temp_angle = 0;
    int16_t temp_cirnum = 0;

    if (mot == NULL || Data == NULL) return;

    // 解析 ID 和状态
    mtr.id = Data[0] & 0x0F;
    mtr.state = Data[0] >> 4;

    if ((uint32_t)mtr.id != mot->id) return;

    mot->Error_id = DM_ErrorCode_To_MotorErrorMask(mtr.state);

    mtr.p_int = (Data[1] << 8) | Data[2];
    mtr.v_int = (Data[3] << 4) | (Data[4] >> 4);
    mtr.t_int = ((Data[4] & 0x0F) << 8) | Data[5];
    
    // DM8009 使用 [-pi, +pi] 位置量程；其他 DM 维持原工程换算。
    if(DM_IsPiPositionType(mot->type) != 0U)
    {
        mtr.pos = uint_to_float(mtr.p_int, -DM_PI, DM_PI, 16);
    }
    else
    {
        mtr.pos = uint_to_float(mtr.p_int, P_MIN, P_MAX, 16);
    }
    
    mtr.vel = uint_to_float(mtr.v_int, V_MIN, V_MAX, 12);
    mtr.toq = uint_to_float(mtr.t_int,
                            DM_TorqueMinGet(mot->type),
                            DM_TorqueMaxGet(mot->type),
                            12);

    mot->FeedbackData.test = mtr.toq;
    mtr.Tmos = (float)Data[6];
    mtr.Tcoil = (float)Data[7];
    mot->FeedbackData.Temperature = (int16_t)mtr.Tcoil;
    mot->FeedbackData.FrameCounter++;
    mot->Realrotationrate = mtr.vel;

    // DM8009 反馈按弧度还原；其他 DM 维持原工程还原公式。
    temp_angle = DM_RAD_TO_DEG(mtr.pos);

    // 圈数计算逻辑保持不变
    while (temp_angle >= 360.0f)
    {
        temp_angle -= 360.0f;
        temp_cirnum += 1;
    }

    while (temp_angle < 0.0f)
    {
        temp_angle += 360.0f;
        temp_cirnum -= 1;
    }

    mot->Realcirnum = temp_cirnum;
    mot->RealAngle = temp_angle;
}
