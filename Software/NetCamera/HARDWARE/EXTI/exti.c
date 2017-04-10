#include "exti.h"
#include "delay.h"
#include "key.h"
#include <ucos_ii.h>

u32 timeUse=0;
u8 keyFlag=0;//1�����ͷţ�����ʹ��timeUse��ֵ��ʹ����󽫸ñ�־��0��0�ް����ͷ�

//�ⲿ�жϳ�ʼ��
void EXTI_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_GPIOG_CLK_ENABLE();   
	GPIO_Initure.Pin=GPIO_PIN_6;               
    GPIO_Initure.Mode=GPIO_MODE_IT_RISING_FALLING;   
    GPIO_Initure.Pull=GPIO_PULLUP;
    HAL_GPIO_Init(GPIOG,&GPIO_Initure);
	HAL_NVIC_SetPriority(EXTI9_5_IRQn,2,4);  
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);        
	
}


//�жϷ�����


void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6); 
}

//�жϷ����������Ҫ��������
//��HAL�������е��ⲿ�жϷ�����������ô˺���
//GPIO_Pin:�ж����ź�
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	OSIntEnter();  
    delay_ms(10);     
	if(PGin(6)==0)  
	{
		printf("������APPKEY\r\n");
		timeUse = OSTimeGet();
	}
	else if(PGin(6) ==1)
	{
		printf("�ͷ���APPKEY\r\n");
		timeUse = OSTimeGet() -timeUse;
		printf("ʱ��� = %d  ms\r\n",timeUse);
		keyFlag = 1;
		
	}
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);
	OSIntExit();//�˳��ٽ��
}
