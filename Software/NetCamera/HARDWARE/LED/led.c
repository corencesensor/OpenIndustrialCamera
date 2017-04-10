#include "led.h"


//初始化PA8为输出.并使能时钟	    
//LED IO初始化
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_GPIOA_CLK_ENABLE();           //开启GPIOA时钟
	__HAL_RCC_GPIOA_CLK_ENABLE();           //开启GPIOE时钟
    
	GPIO_Initure.Pin=GPIO_PIN_8; //PA8  
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);
	

	
    GPIO_Initure.Pin=GPIO_PIN_3; //PE3   IO输出0，MOS管输出VDD   1对应输出0
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
    GPIO_Initure.Pull=GPIO_PULLDOWN;          //下拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);
	
	GPIO_Initure.Pin=GPIO_PIN_2; //PE2 
    GPIO_Initure.Mode=GPIO_MODE_INPUT;  //输入    外部输入1，光耦输出0到INPUT
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);
	
	
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_SET);	//PA8置1  
	HAL_GPIO_WritePin(GPIOE,GPIO_PIN_3,GPIO_PIN_RESET);	//OUTPUT输出1
	
}
