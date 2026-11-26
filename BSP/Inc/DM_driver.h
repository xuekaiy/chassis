#ifndef __DM_DRIVER_H__
#define __DM_DRIVER_H__

/**Include Header Files**/
#include "sysconfig.h"
#include "Motor.h"



/**Macro Definition**/
#define DMJ_STDID							0x100

#define MOTORDM4310_Reductionratio 			10
#define MOTORDM4310_Reductionratio_E    2
#define MOTORDM4340_Reductionratio 			40
#define MOTORDM6248_Reductionratio 			48
#define MOTORDM10010_Reductionratio      10
#define MOTORDM8009_Reductionratio       9


#define P_MIN -12.5f
#define P_MAX 12.5f
#define V_MIN -30.0f
#define V_MAX 30.0f
#define KP_MIN 0.0f
#define KP_MAX 500.0f
#define KD_MIN 0.0f
#define KD_MAX 5.0f
#define T_MIN -10.0f
#define T_MAX 10.0f
#define DM10010_T_MIN -150.0f
#define DM10010_T_MAX 150.0f

/* DM 协议中的位置单位为弧度。 */
#define DM_PI 3.14159265358979323846f
#define DM_DEG_TO_RAD(degree) ((degree) * (DM_PI / 180.0f))
#define DM_RAD_TO_DEG(radian) ((radian) * (180.0f / DM_PI))

/**Struct Definition**/
typedef struct
{
		int id;
		int state;
		int p_int;
		int v_int;
		int t_int;
		int kp_int;
		int kd_int;
		float pos;
		float vel;
		float toq;
		float Kp;
		float Kd;
		float Tmos;
		float Tcoil;
}Motor_Inf;



/**Function Delaration**/
//MCU to DM
void DM_motorOpen(motor_t* mot);
void DM_motorClose(motor_t* mot);
void DM_speedpositionControl(motor_t mot, float _vel);
void DM_motorErrClear(motor_t mot);
void DM_motorZeroSet(motor_t mot);
void DM_mitControl(motor_t *mot,float _vel,float _tff);

//DM to MCU
void DM_infoHandle(motor_t* mot, uint8_t *Data);


#endif
