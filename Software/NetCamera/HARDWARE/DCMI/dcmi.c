#include "sys.h"
#include "dcmi.h" 
#include "lcd.h" 
#include "ltdc.h"
#include "led.h" 
#include "mt9d111.h"
 

DCMI_HandleTypeDef  DCMI_Handler;           //DCMI���
DMA_HandleTypeDef   DMADMCI_Handler;        //DMA���

u8 ov_frame=0;  						//֡��
extern void jpeg_data_process(void);	//JPEG���ݴ�����

//DCMI��ʼ��
void DCMI_Init(void)
{
    DCMI_Handler.Instance=DCMI;
    DCMI_Handler.Init.SynchroMode=DCMI_SYNCHRO_HARDWARE;    //Ӳ��ͬ��HSYNC,VSYNC
    DCMI_Handler.Init.PCKPolarity=DCMI_PCKPOLARITY_RISING;  //PCLK ��������Ч
    DCMI_Handler.Init.VSPolarity=DCMI_VSPOLARITY_LOW;       //VSYNC �͵�ƽ��Ч
    DCMI_Handler.Init.HSPolarity=DCMI_HSPOLARITY_LOW;       //HSYNC �͵�ƽ��Ч
    DCMI_Handler.Init.CaptureRate=DCMI_CR_ALL_FRAME;        //ȫ֡����
    DCMI_Handler.Init.ExtendedDataMode=DCMI_EXTEND_DATA_8B; //8λ���ݸ�ʽ 
    HAL_DCMI_Init(&DCMI_Handler);                           //��ʼ��DCMI���˺����Ὺ��֡�ж�  
}

//DCMI�ײ��������������ã�ʱ��ʹ�ܣ��ж�����
//�˺����ᱻHAL_DCMI_Init()����
//hdcmi:DCMI���
void HAL_DCMI_MspInit(DCMI_HandleTypeDef* hdcmi)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    __HAL_RCC_DCMI_CLK_ENABLE();                //ʹ��DCMIʱ��

    __HAL_RCC_GPIOA_CLK_ENABLE();               //ʹ��GPIOAʱ��
    __HAL_RCC_GPIOB_CLK_ENABLE();               //ʹ��GPIOBʱ��
    __HAL_RCC_GPIOC_CLK_ENABLE();               //ʹ��GPIOCʱ��
    __HAL_RCC_GPIOD_CLK_ENABLE();               //ʹ��GPIODʱ��
    __HAL_RCC_GPIOH_CLK_ENABLE();               //ʹ��GPIOHʱ��
    
    //��ʼ��PA6
    GPIO_Initure.Pin=GPIO_PIN_6;  
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;          //���츴��
    GPIO_Initure.Pull=GPIO_PULLUP;              //����
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;         //����
    GPIO_Initure.Alternate=GPIO_AF13_DCMI;      //����ΪDCMI   
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);         //��ʼ��
    
    //PB7,8,9
    GPIO_Initure.Pin=GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;  
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);         //��ʼ��
    
    //PC6,7,8,9,11
    GPIO_Initure.Pin=GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|\
                     GPIO_PIN_9|GPIO_PIN_11;  
    HAL_GPIO_Init(GPIOC,&GPIO_Initure);         //��ʼ��
    
    //PD3
    GPIO_Initure.Pin=GPIO_PIN_3; 
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);         //��ʼ��
    
    //PH8
    GPIO_Initure.Pin=GPIO_PIN_8; 
    HAL_GPIO_Init(GPIOH,&GPIO_Initure);         //��ʼ��   
    
    HAL_NVIC_SetPriority(DCMI_IRQn,0,0);        //��ռ���ȼ�1�������ȼ�2
    HAL_NVIC_EnableIRQ(DCMI_IRQn);              //ʹ��DCMI�ж�
}

//DCMI DMA����
//mem0addr:�洢����ַ0  ��Ҫ�洢����ͷ���ݵ��ڴ��ַ(Ҳ�����������ַ)
//mem1addr:�洢����ַ1  ��ֻʹ��mem0addr��ʱ��,��ֵ����Ϊ0
//memblen:�洢��λ��,����Ϊ:DMA_MDATAALIGN_BYTE/DMA_MDATAALIGN_HALFWORD/DMA_MDATAALIGN_WORD
//meminc:�洢��������ʽ,����Ϊ:DMA_MINC_ENABLE/DMA_MINC_DISABLE
void DCMI_DMA_Init(u32 mem0addr,u32 mem1addr,u16 memsize,u32 memblen,u32 meminc)
{ 
    __HAL_RCC_DMA2_CLK_ENABLE();                                    //ʹ��DMA2ʱ��
    __HAL_LINKDMA(&DCMI_Handler,DMA_Handle,DMADMCI_Handler);        //��DMA��DCMI��ϵ����
    DMADMCI_Handler.Instance=DMA2_Stream1;                          //DMA2������1                     
    DMADMCI_Handler.Init.Channel=DMA_CHANNEL_1;                     //ͨ��1
    DMADMCI_Handler.Init.Direction=DMA_PERIPH_TO_MEMORY;            //���赽�洢��
    DMADMCI_Handler.Init.PeriphInc=DMA_PINC_DISABLE;                //���������ģʽ
    DMADMCI_Handler.Init.MemInc=meminc;                             //�洢������ģʽ
    DMADMCI_Handler.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;   //�������ݳ���:32λ
    DMADMCI_Handler.Init.MemDataAlignment=memblen;                  //�洢�����ݳ���:8/16/32λ
    DMADMCI_Handler.Init.Mode=DMA_CIRCULAR;                         //ʹ��ѭ��ģʽ 
    DMADMCI_Handler.Init.Priority=DMA_PRIORITY_HIGH;                //�����ȼ�
    DMADMCI_Handler.Init.FIFOMode=DMA_FIFOMODE_ENABLE;              //ʹ��FIFO
    DMADMCI_Handler.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_HALFFULL; //ʹ��1/2��FIFO 
    DMADMCI_Handler.Init.MemBurst=DMA_MBURST_SINGLE;                //�洢��ͻ������
    DMADMCI_Handler.Init.PeriphBurst=DMA_PBURST_SINGLE;             //����ͻ�����δ��� 
    HAL_DMA_DeInit(&DMADMCI_Handler);                               //�������ǰ������
    HAL_DMA_Init(&DMADMCI_Handler);	                                //��ʼ��DMA
    
 
    __HAL_UNLOCK(&DMADMCI_Handler);
    if(mem1addr==0)    //����DMA����ʹ��˫����
    {
        HAL_DMA_Start(&DMADMCI_Handler,(u32)&DCMI->DR,mem0addr,memsize);
    }
    else                //ʹ��˫����
    {
        HAL_DMAEx_MultiBufferStart(&DMADMCI_Handler,(u32)&DCMI->DR,mem0addr,mem1addr,memsize);//����˫����
        __HAL_DMA_ENABLE_IT(&DMADMCI_Handler,DMA_IT_TC);    //������������ж�
        HAL_NVIC_SetPriority(DMA2_Stream1_IRQn,0,0);        //DMA�ж����ȼ�
        HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
    }
}

//DCMI,��������
void DCMI_Start(void)
{  
   // LCD_SetCursor(0,0);  
	//LCD_WriteRAM_Prepare();		        //��ʼд��GRAM
    __HAL_DMA_ENABLE(&DMADMCI_Handler); //ʹ��DMA
    DCMI->CR|=DCMI_CR_CAPTURE;          //DCMI����ʹ��
}

//DCMI,�رմ���
void DCMI_Stop(void)
{ 
    DCMI->CR&=~(DCMI_CR_CAPTURE);       //�رղ���
    while(DCMI->CR&0X01);               //�ȴ��������
    __HAL_DMA_DISABLE(&DMADMCI_Handler);//�ر�DMA
} 

//DCMI�жϷ�����
void DCMI_IRQHandler(void)
{
    HAL_DCMI_IRQHandler(&DCMI_Handler);
}

//����һ֡ͼ������
//hdcmi:DCMI���
void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
	jpeg_data_process();//jpeg���ݴ���
	LED1=!LED1;
	ov_frame++; 
    //����ʹ��֡�ж�,��ΪHAL_DCMI_IRQHandler()������ر�֡�ж�
    __HAL_DCMI_ENABLE_IT(&DCMI_Handler,DCMI_IT_FRAME);
}

 

//DCMI������ʾ����
//sx,sy;LCD����ʼ����
//width,height:LCD��ʾ��Χ.
void DCMI_Set_Window(u16 sx,u16 sy,u16 width,u16 height)
{
	DCMI_Stop(); 
//	LCD_Clear(WHITE);
//	LCD_Set_Window(sx,sy,width,height);
//	OV2640_OutSize_Set(width,height);
 //   LCD_SetCursor(0,0);  
//	LCD_WriteRAM_Prepare();		        //��ʼд��GRAM  
    __HAL_DMA_ENABLE(&DMADMCI_Handler); //����DMA2,Stream1 
    DCMI->CR|=DCMI_CR_CAPTURE;          //DCMI����ʹ��	
}
   
//ͨ��usmart����,����������.
//pclk/hsync/vsync:�����źŵ����޵�ƽ����
void DCMI_CR_Set(u8 pclk,u8 hsync,u8 vsync)
{
    HAL_DCMI_DeInit(&DCMI_Handler);//���ԭ��������
    DCMI_Handler.Instance=DCMI;
    DCMI_Handler.Init.SynchroMode=DCMI_SYNCHRO_HARDWARE;//Ӳ��ͬ��HSYNC,VSYNC
    DCMI_Handler.Init.PCKPolarity=pclk<<5;              //PCLK ��������Ч
    DCMI_Handler.Init.VSPolarity=vsync<<7;              //VSYNC �͵�ƽ��Ч
    DCMI_Handler.Init.HSPolarity=hsync<<6;              //HSYNC �͵�ƽ��Ч
    DCMI_Handler.Init.CaptureRate=DCMI_CR_ALL_FRAME;    //ȫ֡����
    DCMI_Handler.Init.ExtendedDataMode=DCMI_EXTEND_DATA_8B;//8λ���ݸ�ʽ 
    HAL_DCMI_Init(&DCMI_Handler);                       //��ʼ��DCMI
    DCMI_Handler.Instance->CR|=DCMI_MODE_CONTINUOUS;    //����ģʽ
}


