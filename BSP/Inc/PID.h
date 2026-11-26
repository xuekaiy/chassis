#ifndef __PID_H__
#define __PID_H__
/**Include Header Files**/
#include <stdint.h>
#include <math.h>  
/**Enum Definition**/
typedef enum
{
    PID_POSITION = 0,
    PID_DELTA
} PID_MODE;


/**Struct Definition**/
typedef struct
{
    /* data */	
    PID_MODE pidmode;          // 工作模式：位置式/增量式

    float error[3];            // error[0]=本次误差  error[1]=上一次误差  error[2]=上上次误差
    float sum_error;           // 误差累加和（积分累加器）
    float Dbuf[3];             // 微分缓存：0=本次 1=上一次 2=上上次

    float kp;
    float ki;
    float kd;                  // PID 系数

    float Pout;
    float Iout;
    float Dout;                // 比例、积分、微分分项输出
    float Out;                 // 总输出量

    float dst;                 // 目标值
    float real;                // 实际值

    float MaxOut;              // PID 输出限幅（0 为不限制）
    float MaxIout;             // 积分输出限幅（0 为不限制）

    float MaxBlockIount;       // 抗积分饱和（积分分离）时积分项的额外限制

    uint16_t DeadZone;         // 死区大小

} pid_type;




/**Function Delaration**/
uint8_t PID_Parameter_Init(pid_type *PID, PID_MODE mode, const float Pidpara[], uint32_t MaxOut, uint32_t MaxIout, uint16_t DeadZone);
uint8_t PID_Parameter_Update(pid_type * pid,const float Pidpara[]);
uint8_t PID_clear(pid_type *pid);
float Pid_cal(pid_type *PID, float dst, float real);

#endif
