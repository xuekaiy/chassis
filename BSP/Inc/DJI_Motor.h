#ifndef __DJI_MOTOR_H__
#define __DJI_MOTOR_H__

/**Include Header Files**/
#include "sysconfig.h"
#include "Motor.h"

#define MAXBLOCKTIME 500

/*3508��2006��غ궨��*/
#define MOTOR3508_Reductionratio 				19				//3508��װ���������Ƽ��ٱ�
#define MOTOR3508Lift_Reductionratio    27
#define M3508KIMAX											10000			//3508���Ki�������ֵ
#define M3508SPEEDMAX 									10000		  	//M3508�ٶ�����
#define M3508OUTMAX 										15999 			//C620���������ֵ

#define MOTOR2006_Reductionratio 				36				//2006��װ���������Ƽ��ٱ�
#define M2006KIMAX											5000			//2006���Ki�������ֵ
#define M2006OUTMAX 										9999 		  //C610���������ֵ

/*6020��غ궨��*/
#define MOTOR6020_Reductionratio				1
#define M6020OutMax											29999

#define MechanicalAngle2RealAngle(Mechanical_Angle) (Mechanical_Angle / 8191.0f * 360.0f) //��е�ǣ�0~8191��->��0~360�㣩

/*������غ궨��*/
#define MOTORMS_Reductionratio				1


/*���е��id�ź궨��*/
#define Non_id                  				0x000       //���ֵ����CAN�����ɴӰ���ɣ�����ֻ�������ֵ���㣬�����Щ�������Ҫid
#define M3508_M2006_RX_OFFSETID					0x200 			//M3508	�������can��ƫ��id 
#define M3508_M2006_CH1_TX_OFFSETID	    0x200 			//M3508 M2006ͨ��һ����can��ƫ��id
#define M3508_M2006_CH2_TX_OFFSETID     0x1FF       //M3508 M2006ͨ��������can��ƫ��id

#define M6020_RX_OFFSETID								0x204				//M6020	�������can��ƫ��id 
#define M6020_CH1_TX_OFFSETID						0x1FF				//M6020	ͨ��һ����can��ƫ��id
#define M6020_CH2_TX_OFFSETID						0x2FF				//M6020	ͨ��������can��ƫ��id


#define M6020_CH1_TX_TORID						0x1FE				//M6020	ͨ��һ����can��ƫ��id �°�������������������id�͵�ѹģʽ��һ��
#define M6020_CH2_TX_TORID						0x2FE				//M6020	ͨ��������can��ƫ��id �°�������������������id�͵�ѹģʽ��һ��


/**Function Declaration**/
void DJIMotor_ParaHandle(motor_t* mot,uint8_t adata[]);
int8_t DJIMotor_AngleHandle(motor_t * mot);

#endif
