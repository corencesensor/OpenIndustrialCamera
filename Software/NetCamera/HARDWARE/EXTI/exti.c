#include "exti.h"
#include "delay.h"
#include "key.h"
#include <ucos_ii.h>

u32 timeUse=0;
u8 keyFlag=0;//1按键释放，可以使用timeUse的值，使用完后将该标志清0。0无按键释放

//外部中断初始化
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


//中断服务函数


void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6); 
}

//中断服务程序中需要做的事情
//在HAL库中所有的外部中断服务函数都会调用此函数
//GPIO_Pin:中断引脚号
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	OSIntEnter();  
    delay_ms(10);     
	if(PGin(6)==0)  
	{
		printf("按下了APPKEY\r\n");
		timeUse = OSTimeGet();
	}
	else if(PGin(6) ==1)
	{
		printf("释放了APPKEY\r\n");
		timeUse = OSTimeGet() -timeUse;
		printf("时间差 = %d  ms\r\n",timeUse);
		keyFlag = 1;
		
	}
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);
	OSIntExit();//退出临界段
}
