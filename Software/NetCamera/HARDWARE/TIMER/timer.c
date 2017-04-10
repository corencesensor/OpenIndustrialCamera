#include "timer.h"
#include "led.h"

TIM_HandleTypeDef TIM3_Handler;         //��ʱ��3PWM��� 
TIM_OC_InitTypeDef TIM3_CH4Handler;	    //��ʱ��3ͨ��4���

//TIM3 PWM���ֳ�ʼ�� 
 
void TIM3_PWM_Init(u16 arr,u16 psc)
{ 
    TIM3_Handler.Instance=TIM3;            //��ʱ��3
    TIM3_Handler.Init.Prescaler=psc;       //��ʱ����Ƶ
    TIM3_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;//���ϼ���ģʽ
    TIM3_Handler.Init.Period=arr;          //�Զ���װ��ֵ
    TIM3_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&TIM3_Handler);       //��ʼ��PWM
    
    TIM3_CH4Handler.OCMode=TIM_OCMODE_PWM1; //ģʽѡ��PWM1
    TIM3_CH4Handler.Pulse=arr/2;            //���ñȽ�ֵ,��ֵ����ȷ��ռ�ձȣ�Ĭ�ϱȽ�ֵΪ�Զ���װ��ֵ��һ��,��ռ�ձ�Ϊ50%
    TIM3_CH4Handler.OCPolarity=TIM_OCPOLARITY_HIGH; //����Ƚϼ���Ϊ��
    HAL_TIM_PWM_ConfigChannel(&TIM3_Handler,&TIM3_CH4Handler,TIM_CHANNEL_4);//����TIM3ͨ��4
	
    HAL_TIM_PWM_Start(&TIM3_Handler,TIM_CHANNEL_4);//����PWMͨ��4
}


//��ʱ���ײ�������ʱ��ʹ�ܣ���������
 
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_Initure;
	__HAL_RCC_TIM3_CLK_ENABLE();			//ʹ�ܶ�ʱ��3
    __HAL_RCC_GPIOB_CLK_ENABLE();			 
	
    GPIO_Initure.Pin=GPIO_PIN_1;           	 
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;  	 
    GPIO_Initure.Pull=GPIO_PULLUP;           
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;      
	GPIO_Initure.Alternate= GPIO_AF2_TIM3;	 
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);
}


//����TIMͨ��4��ռ�ձ�
 
void TIM_SetTIM3Compare4(u32 compare)
{
	TIM3->CCR4=compare; 
}
