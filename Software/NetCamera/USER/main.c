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


//����������ʼ��
extern SavePara_TypeDef DefaultList;


//KEY����
#define KEY_TASK_PRIO 		8
//�����ջ��С
#define KEY_STK_SIZE		128		
//�����ջ
OS_STK KEY_TASK_STK[KEY_STK_SIZE];
//������
void key_task(void *pdata);  

//��LCD����ʾ��ַ��Ϣ����
//�������ȼ�
#define DISPLAY_TASK_PRIO	9
//�����ջ��С
#define DISPLAY_STK_SIZE	128
//�����ջ
OS_STK	DISPLAY_TASK_STK[DISPLAY_STK_SIZE];
//������
void display_task(void *pdata);


//LED����
//�������ȼ�
#define LED_TASK_PRIO		10
//�����ջ��С
#define LED_STK_SIZE		64
//�����ջ
OS_STK	LED_TASK_STK[LED_STK_SIZE];
//������
void led_task(void *pdata);  


//START����
//�������ȼ�
#define START_TASK_PRIO		11
//�����ջ��С
#define START_STK_SIZE		128
//�����ջ
OS_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *pdata); 


extern u16 TCP_SERVER_PORT;
extern IWDG_HandleTypeDef IWDG_Handler;  
extern u16 TCP_SERVER_PORT;
extern u8 keyFlag;
extern u32 timeUse;
u8 keyPres=0;//1�̰�  2����; 2 �а�   2-5����;  3����  5��-15��  ;����15�뵱���ް�������  0�ް���  

//���ڴ�ӡ��Ϣ
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
	/*�����ϵͳ��ʼ��*/
    Stm32_Clock_Init(360,25,2,8);   
    HAL_Init();                    
    delay_init(180);                
    uart_init(115200);             
    usmart_dev.init(90); 		    
    LED_Init();                     
	EXTI_Init();
	TIM3_PWM_Init(500-1,90-1);      //90M/90=1M�ļ���Ƶ�ʣ��Զ���װ��Ϊ500����ôPWMƵ��Ϊ1M/500=2kHZ
    SDRAM_Init();                   
    my_mem_init(SRAMIN);		    //��ʼ���ڲ��ڴ��
	my_mem_init(SRAMEX);		    //��ʼ���ⲿ�ڴ��
	my_mem_init(SRAMCCM);		    //��ʼ��CCM�ڴ��

    
	OSInit(); 					    //UCOS��ʼ��
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
	OSStart(); //����UCOS
}
 
//start����
void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr;
	pdata = pdata ;
	
	OSStatInit();  			 
	OS_ENTER_CRITICAL();  	 
#if LWIP_DHCP
	lwip_comm_dhcp_creat(); //����DHCP����
#endif
	
	OSTaskCreate(led_task,(void*)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);//����LED����
    OSTaskCreate(key_task,(void*)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);//��������
	OSTaskCreate(display_task,(void*)0,(OS_STK*)&DISPLAY_TASK_STK[DISPLAY_STK_SIZE-1],DISPLAY_TASK_PRIO); //��ʾ����
	OSTaskSuspend(OS_PRIO_SELF); //����start_task����
	OS_EXIT_CRITICAL();  		//���ж�
}

 
void display_task(void *pdata)
{
	while(1)
	{ 
#if LWIP_DHCP									//������DHCP��ʱ��
		if(lwipdev.dhcpstatus != 0) 			 
		{
			show_address(lwipdev.dhcpstatus );	//��ʾ��ַ��Ϣ
			OSTaskSuspend(OS_PRIO_SELF); 		 
		}
#else
		show_address(0); 						//��ʾ��̬��ַ
		OSTaskSuspend(OS_PRIO_SELF); 		 	 
#endif //LWIP_DHCP
		OSTimeDlyHMSM(0,0,0,500);
	}
}

//key����
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
			
			printf("����ʱ��=%d,�����ȼ�=%d \r\n",timeUse,keyPres);
			 
			keyFlag=0;	
			if(keyPres == 3)
			{
				SaveParaList=DefaultList;
				STMFLASH_Write(FLASH_SAVE_ADDR,(u32 *)&SaveParaList,sizeof(SavePara_TypeDef)/4+((sizeof(SavePara_TypeDef)%4)?1:0));
				printf("��ʼ��Ĭ�ϲ�����ϣ�����������\r\n");
				HAL_NVIC_SystemReset();
			}
		}
		if(INPUT == 0)		//input�������˸ߵ�ƽ
		{			
			printf("INPUT������Ϊ�ߵ�ƽ\r\n");
		}
		OSTimeDlyHMSM(0,0,0,500);  //��ʱ500MS
	}
}

//led����
void led_task(void *pdata)
{
	IWDG_Init(IWDG_PRESCALER_256,4000); 
	while(1)
	{
		HAL_IWDG_Refresh(&IWDG_Handler); 
		OSTimeDlyHMSM(0,0,1,0);  
 	}
}
