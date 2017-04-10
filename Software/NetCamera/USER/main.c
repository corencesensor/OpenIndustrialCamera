#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "sdram.h"
#include "lan8720.h"
#include "timer.h"
#include "pcf8574.h"
#include "usmart.h"
#include "malloc.h"
#include "lwip/netif.h"
#include "lwip_comm.h"
#include "lwipopts.h"
#include "tcp_server_demo.h"
#include "iwdg.h"
#include "exti.h"
#include "timer.h"
#include "protocol.h"
#include "stmflash.h"
extern SavePara_TypeDef SaveParaList;


//参数保存表初始化
extern SavePara_TypeDef DefaultList;


//KEY任务
#define KEY_TASK_PRIO 		8
//任务堆栈大小
#define KEY_STK_SIZE		128		
//任务堆栈
OS_STK KEY_TASK_STK[KEY_STK_SIZE];
//任务函数
void key_task(void *pdata);  

//在LCD上显示地址信息任务
//任务优先级
#define DISPLAY_TASK_PRIO	9
//任务堆栈大小
#define DISPLAY_STK_SIZE	128
//任务堆栈
OS_STK	DISPLAY_TASK_STK[DISPLAY_STK_SIZE];
//任务函数
void display_task(void *pdata);


//LED任务
//任务优先级
#define LED_TASK_PRIO		10
//任务堆栈大小
#define LED_STK_SIZE		64
//任务堆栈
OS_STK	LED_TASK_STK[LED_STK_SIZE];
//任务函数
void led_task(void *pdata);  


//START任务
//任务优先级
#define START_TASK_PRIO		11
//任务堆栈大小
#define START_STK_SIZE		128
//任务堆栈
OS_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *pdata); 


extern u16 TCP_SERVER_PORT;
extern IWDG_HandleTypeDef IWDG_Handler;  
extern u16 TCP_SERVER_PORT;
extern u8 keyFlag;
extern u32 timeUse;
u8 keyPres=0;//1短按  2秒内; 2 中按   2-5秒内;  3长按  5秒-15秒  ;大于15秒当做无按键处理  0无按键  

//串口打印信息
void show_address(u8 mode)
{
    
    
	u8 buf[30];
	if(mode==2)
	{
		sprintf((char*)buf,"DHCP IP :%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						 
		printf("%s",buf);
		sprintf((char*)buf,"DHCP GW :%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	 
		printf("%s",buf);
		sprintf((char*)buf,"NET MASK:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	 
		printf("%s",buf);
		printf("port %d\n",TCP_SERVER_PORT);
	}
	else 
	{
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						 
		printf("%s",buf);
		sprintf((char*)buf,"Static GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	 
		printf("%s",buf);
		sprintf((char*)buf,"NET MASK:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	 
		printf("%s",buf);
		printf("port %d\n",TCP_SERVER_PORT);
	}	
}

int main(void)
{   
	/*外设和系统初始化*/
    Stm32_Clock_Init(360,25,2,8);   
    HAL_Init();                    
    delay_init(180);                
    uart_init(115200);             
    usmart_dev.init(90); 		    
    LED_Init();                     
	EXTI_Init();
	TIM3_PWM_Init(500-1,90-1);      //90M/90=1M的计数频率，自动重装载为500，那么PWM频率为1M/500=2kHZ
    SDRAM_Init();                   
    my_mem_init(SRAMIN);		    //初始化内部内存池
	my_mem_init(SRAMEX);		    //初始化外部内存池
	my_mem_init(SRAMCCM);		    //初始化CCM内存池

    
	OSInit(); 					    //UCOS初始化
	while(lwip_comm_init()) 	    
	{
		printf("lwip failed\n");
	}
    while(tcp_server_init()) 
	{
		printf("tcp server failed\n");
	}
	printf("tcp server success");
	OSTaskCreate(start_task,(void*)0,(OS_STK*)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO);
	OSStart(); //开启UCOS
}
 
//start任务
void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr;
	pdata = pdata ;
	
	OSStatInit();  			 
	OS_ENTER_CRITICAL();  	 
#if LWIP_DHCP
	lwip_comm_dhcp_creat(); //创建DHCP任务
#endif
	
	OSTaskCreate(led_task,(void*)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);//创建LED任务
    OSTaskCreate(key_task,(void*)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);//按键任务
	OSTaskCreate(display_task,(void*)0,(OS_STK*)&DISPLAY_TASK_STK[DISPLAY_STK_SIZE-1],DISPLAY_TASK_PRIO); //显示任务
	OSTaskSuspend(OS_PRIO_SELF); //挂起start_task任务
	OS_EXIT_CRITICAL();  		//开中断
}

 
void display_task(void *pdata)
{
	while(1)
	{ 
#if LWIP_DHCP									//当开启DHCP的时候
		if(lwipdev.dhcpstatus != 0) 			 
		{
			show_address(lwipdev.dhcpstatus );	//显示地址信息
			OSTaskSuspend(OS_PRIO_SELF); 		 
		}
#else
		show_address(0); 						//显示静态地址
		OSTaskSuspend(OS_PRIO_SELF); 		 	 
#endif //LWIP_DHCP
		OSTimeDlyHMSM(0,0,0,500);
	}
}

//key任务
void key_task(void *pdata)
{
	while(1)
	{
		if(keyFlag==1)
		{
			if(timeUse>0&&timeUse<=2000){
				keyPres = 1;
			}else if(timeUse>2000&&timeUse<=5000){
				keyPres = 2;
			}else if(timeUse>5000&&timeUse<15000){
				keyPres = 3;
			}else{
				keyPres =0;
			}
			
			printf("调用时间=%d,长按等级=%d \r\n",timeUse,keyPres);
			 
			keyFlag=0;	
			if(keyPres == 3)
			{
				SaveParaList=DefaultList;
				STMFLASH_Write(FLASH_SAVE_ADDR,(u32 *)&SaveParaList,sizeof(SavePara_TypeDef)/4+((sizeof(SavePara_TypeDef)%4)?1:0));
				printf("初始化默认参数完毕，请重新连接\r\n");
				HAL_NVIC_SystemReset();
			}
		}
		if(INPUT == 0)		//input端输入了高电平
		{			
			printf("INPUT端输入为高电平\r\n");
		}
		OSTimeDlyHMSM(0,0,0,500);  //延时500MS
	}
}

//led任务
void led_task(void *pdata)
{
	IWDG_Init(IWDG_PRESCALER_256,4000); 
	while(1)
	{
		HAL_IWDG_Refresh(&IWDG_Handler); 
		OSTimeDlyHMSM(0,0,1,0);  
 	}
}
