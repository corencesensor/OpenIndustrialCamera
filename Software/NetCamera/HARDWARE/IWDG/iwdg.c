#include "iwdg.h"


IWDG_HandleTypeDef IWDG_Handler; //独立看门狗句柄


void IWDG_Init(u8 prer,u16 rlr)
{
    IWDG_Handler.Instance=IWDG;
    IWDG_Handler.Init.Prescaler=prer;	//设置IWDG分频系数
    IWDG_Handler.Init.Reload=rlr;		//重装载值
    HAL_IWDG_Init(&IWDG_Handler);		//初始化IWDG  
	
    HAL_IWDG_Start(&IWDG_Handler);		//开启独立看门狗
}
    
//喂独立看门狗
void IWDG_Feed(void)
{   
    HAL_IWDG_Refresh(&IWDG_Handler); 	//喂狗
}
