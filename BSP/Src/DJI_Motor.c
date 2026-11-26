/**Include Header Files**/
#include "DJI_Motor.h"

/**Function Declaration**/
/**
 * @brief  Handle DJI Motor Parameter
 * @note
 * @param  mot, adata[]
 * @retval
 */
void DJIMotor_ParaHandle(motor_t *mot, uint8_t adata[])
{
    if (mot->type == MOTOR_3508 || mot->type == MOTOR_2006)
    {
        mot->FeedbackData.FrameCounter++;

        if (mot->First_Frame == 1)
            mot->FeedbackData.Mechanical_Angle[1] = mot->FeedbackData.Mechanical_Angle[0];

        mot->FeedbackData.Mechanical_Angle[0] = (uint16_t)((adata[0] << 8) | adata[1]);

        if (mot->First_Frame == 0)
        {
            mot->FeedbackData.Mechanical_Angle[1] = mot->FeedbackData.Mechanical_Angle[0];
            mot->First_Frame = 1;
        }

        mot->FeedbackData.RealSpeed = (int16_t)((adata[2] << 8) | adata[3]);
        mot->FeedbackData.Current = (int16_t)((adata[4] << 8) | adata[5]);
        mot->Realrotationrate = mot->FeedbackData.RealSpeed;

        DJIMotor_AngleHandle(mot);
    }
    else if (mot->type == MOTOR_6020)
    {
        mot->FeedbackData.FrameCounter++;

        if (mot->First_Frame == 1)
            mot->FeedbackData.Mechanical_Angle[1] = mot->FeedbackData.Mechanical_Angle[0];

        mot->FeedbackData.Mechanical_Angle[0] = (uint16_t)((adata[0] << 8) | adata[1]);

        if (mot->First_Frame == 0)
        {
            mot->FeedbackData.Mechanical_Angle[1] = mot->FeedbackData.Mechanical_Angle[0];
            mot->First_Frame = 1;
        }

        mot->FeedbackData.RealSpeed = (int16_t)((adata[2] << 8) | adata[3]);
        mot->FeedbackData.Current = (int16_t)((adata[4] << 8) | adata[5]);
        mot->Realrotationrate = mot->FeedbackData.RealSpeed;

        DJIMotor_AngleHandle(mot);
    }
    else
        return;
}

/**
 * @brief  ���µ���ĽǶ��Լ�Ȧ��
 * @note   ���ڼ�����Ĵ��ڣ����ת��Ȧ��Ӧ�����תһȦ���˺������ڼ��������ת���ĽǶȡ���������һȦ������Ϊ8192�ĵ����
 * @param  mot: ��Ӧ����ṹ��ָ��
 * @retval -1��������㣻1��������㣻0 ���ޱ仯or����
 */
int8_t DJIMotor_AngleHandle(motor_t *mot)
{
    if (mot->type == MOTOR_3508 || mot->type == MOTOR_2006)
    {
        // �û�е��+�ٶ��жϹ���ķ�ʽ�����ף����������߻ش���ͬ�����ᵼ�µ������������е�ǹ��㣬���½ǶȽ��������
        // �����ԣ�3508������ת��9000�����̴�������һ��ﲻ������ٶȣ�����ﵽ�˾��ٶ��޷���ʱ��1000Hz����ش������»�е��ÿ�α仯1250���ң������ŵ�1500�����жϻ�е������/�������

        /*���ݻ�е��Mechanical_Angle���㵱ǰת��ת��Ȧ��Oricirnum*/
        if (mot->FeedbackData.Mechanical_Angle[0] > 6700 && mot->FeedbackData.Mechanical_Angle[1] < 1500)
        {
            mot->Oricirnum -= 1;
            return -1;
        }
        else if (mot->FeedbackData.Mechanical_Angle[0] < 1500 && mot->FeedbackData.Mechanical_Angle[1] > 6700)
        {
            mot->Oricirnum += 1;
            return 1;
        }

        /*����Oricirnum����Realcirnum ���ݼ��ٱȵĶ��壬Oricirnum����Reductionratio�����ٱȣ�ʱ����Ӧ�����ת��һȦ*/
        while (mot->Oricirnum >= mot->Reductionratio)
        {
            mot->Oricirnum -= mot->Reductionratio;
            mot->Realcirnum += 1;
        }

        while (mot->Oricirnum < 0)
        {
            mot->Oricirnum += mot->Reductionratio;
            mot->Realcirnum -= 1;
        }

        /*���ݻ�е��Mechanical_Angle��ת��ת��Ȧ��Oricirnum����Ŀǰ����Ƕȣ���Χ��0~360֮��*/
        float tempAngle = ((float)(mot->Oricirnum) / (float)(mot->Reductionratio) * 360.0f) + MechanicalAngle2RealAngle(mot->FeedbackData.Mechanical_Angle[0]) / (float)(mot->Reductionratio);

        while (tempAngle >= 360)
        {
            tempAngle -= 360;
        }

        while (tempAngle < 0)
        {
            tempAngle += 360;
        }

        mot->RealAngle = tempAngle;
        return 0;
    }
    else if (mot->type == MOTOR_6020)
    {
        /*6020Ȧ�����㣬���ڵ�����ٱ�Ϊ1������ֱ�����û�е��Mechanical_Angle���ж������Ȧ��Realcirnum*/
        if (mot->FeedbackData.Mechanical_Angle[0] > 6700 && mot->FeedbackData.Mechanical_Angle[1] < 1500)
        {
            mot->Realcirnum -= 1;
            return -1;
        }
        else if (mot->FeedbackData.Mechanical_Angle[0] < 1500 && mot->FeedbackData.Mechanical_Angle[1] > 6700)
        {
            mot->Realcirnum += 1;
            return 1;
        }

        /*6020�ǶȽ��㣬��Χ��0~360*/
        float tempAngle = MechanicalAngle2RealAngle(mot->FeedbackData.Mechanical_Angle[0]) / (float)(mot->Reductionratio);

        while (tempAngle >= 360)
        {
            tempAngle -= 360;
        }

        while (tempAngle < 0)
        {
            tempAngle += 360;
        }

        mot->RealAngle = tempAngle;
        return 0;
    }
    else
    {
        return 0;
    }
}
