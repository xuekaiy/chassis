/**Include Header Files**/
#include "Motor.h"

/**Function Delaration**/
/**
 * @brief  电机类型注册函数
 * @note   初始化电机结构体
 * @param  mot: 电机结构体；type: 电机类型；pospid：位置环pid；speedpid：速度环pid；
 * @param  Reductionratio:减速比；Can_id：电机CAN通信id号
 * @retval 注册成功1 失败0
 */
 

uint8_t DM_ErrorCode_To_MotorErrorMask(uint8_t dm_err)
{
    uint8_t err_mask = 0;

    switch (dm_err)
    {
    case 0x00:
    case 0x01:
        break;

    case 0x03:
    case 0x04:
    case 0x05:
        err_mask |= MOTOR_ERR_SENSOR_CALIB_FAULT;
        break;

    case 0x08:
        err_mask |= MOTOR_ERR_OVER_VOLTAGE;
        break;

    case 0x09:
        err_mask |= MOTOR_ERR_UNDER_VOLTAGE;
        break;

    case 0x0A:
        err_mask |= MOTOR_ERR_OVER_CURRENT;
        break;

    case 0x0B:
        err_mask |= MOTOR_ERR_DRIVER_OVER_TEMP;
        break;

    case 0x0C:
        err_mask |= MOTOR_ERR_MOTOR_OVER_TEMP;
        break;

    case 0x0D:
        err_mask |= MOTOR_ERR_COMM_LOST;
        break;

    case 0x0E:
        err_mask |= MOTOR_ERR_OVERLOAD_STALL;
        break;

    default:
        err_mask |= MOTOR_ERR_SENSOR_CALIB_FAULT;
        break;
    }

    return err_mask;
}


uint8_t LK_ErrorState_To_MotorErrorMask(uint8_t lk_errorState)
{
    uint8_t err_mask = 0;

    if (lk_errorState & (1u << 0))
        err_mask |= MOTOR_ERR_UNDER_VOLTAGE;

    if (lk_errorState & (1u << 1))
        err_mask |= MOTOR_ERR_OVER_VOLTAGE;

    if (lk_errorState & (1u << 2))
        err_mask |= MOTOR_ERR_DRIVER_OVER_TEMP;

    if (lk_errorState & (1u << 3))
        err_mask |= MOTOR_ERR_MOTOR_OVER_TEMP;

    if (lk_errorState & (1u << 4))
        err_mask |= MOTOR_ERR_OVER_CURRENT;

    if (lk_errorState & (1u << 5))
        err_mask |= MOTOR_ERR_OVER_CURRENT;

    if (lk_errorState & (1u << 6))
        err_mask |= MOTOR_ERR_OVERLOAD_STALL;

    if (lk_errorState & (1u << 7))
        err_mask |= MOTOR_ERR_COMM_LOST;

    return err_mask;
}


uint8_t RS_FaultValue_To_MotorErrorMask(uint32_t rs_fault)
{
    uint8_t err_mask = 0;

    if (rs_fault & RS_FAULT_UNDER_VOLTAGE)
        err_mask |= MOTOR_ERR_UNDER_VOLTAGE;

    if (rs_fault & RS_FAULT_OVER_VOLTAGE)
        err_mask |= MOTOR_ERR_OVER_VOLTAGE;

    if (rs_fault & (RS_FAULT_PHASE_A_OVERCURRENT |
                    RS_FAULT_PHASE_B_OVERCURRENT |
                    RS_FAULT_PHASE_C_OVERCURRENT))
        err_mask |= MOTOR_ERR_OVER_CURRENT;

    if (rs_fault & RS_FAULT_DRIVER_CHIP)
        err_mask |= MOTOR_ERR_SENSOR_CALIB_FAULT;

    if (rs_fault & RS_FAULT_MOTOR_OVER_TEMP)
        err_mask |= MOTOR_ERR_MOTOR_OVER_TEMP;

    if (rs_fault & RS_FAULT_STALL_OVERLOAD)
        err_mask |= MOTOR_ERR_OVERLOAD_STALL;

    if (rs_fault & (RS_FAULT_ENCODER_UNCALIBRATED |
                    RS_FAULT_HARDWARE_IDENTIFY |
                    RS_FAULT_POSITION_INIT))
        err_mask |= MOTOR_ERR_SENSOR_CALIB_FAULT;

    return err_mask;
}


uint8_t Motor_IsError(motor_t *mot)
{
    if (mot == NULL)
    {
        return 0;
    }

    return (mot->Error_id != 0) ? 1 : 0;
}


uint8_t ArmMotor_ErrorStatePack_Get(void)
{
    uint8_t error_pack = 0;
    uint8_t motor_num = MNUM;

    if (motor_num > MNUM)
    {
        motor_num = MNUM;
    }

    for (uint8_t i = 0; i < motor_num; i++)
    {
        if (Arm_Motor[i].Error_id != 0)
        {
            error_pack |= (1u << i);
        }
    }

    return error_pack;
}


void ArmMotor_ErrorMaskArray_Get(uint8_t error_mask_array[7])
{
    if (error_mask_array == NULL)
    {
        return;
    }

    for (uint8_t i = 0; i < 7; i++)
    {
        error_mask_array[i] = Arm_Motor[i].Error_id;
    }
}


void MotortypeRegister(motor_t *mot,MotorDirection_t dir, motor_type type, pid_type *pospid, pid_type *speedpid, uint8_t Reductionratio, uint32_t Motor_id,uint8_t Can_id,float Kp,float Ki,float Kd,float MAX_angle,float MIN_angle)
{
    if (mot != NULL)
    {
        mot->id = Motor_id;
				mot->CAN_ID=Can_id;
        mot->type = type;
				mot->State=CLOSE;
				mot->Online=OFFLINE;
				mot->Dir=dir;
        mot->Reductionratio = Reductionratio;
        mot->PositionPID = pospid;
        mot->SpeedPID = speedpid;
	      mot->Kp=Kp;
				mot->Ki=Ki;
			  mot->Kd=Kd;
			  mot->MAX_angle=MAX_angle;
			  mot->MIN_angle=MIN_angle;
				mot->SumError=0;
    }
}

