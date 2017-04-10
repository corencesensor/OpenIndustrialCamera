#include "sys.h"
#include "dcmi.h" 
#include "lcd.h" 
#include "ltdc.h"
#include "led.h" 
#include "mt9d111.h"
 

DCMI_HandleTypeDef  DCMI_Handler;           //DCMI句柄
DMA_HandleTypeDef   DMADMCI_Handler;        //DMA句柄

u8 ov_frame=0;  						//帧率
extern void jpeg_data_process(void);	//JPEG数据处理函数

//DCMI初始化
void DCMI_Init(void)
{
    DCMI_Handler.Instance=DCMI;
    DCMI_Handler.Init.SynchroMode=DCMI_SYNCHRO_HARDWARE;    //硬件同步HSYNC,VSYNC
    DCMI_Handler.Init.PCKPolarity=DCMI_PCKPOLARITY_RISING;  //PCLK 上升沿有效
    DCMI_Handler.Init.VSPolarity=DCMI_VSPOLARITY_LOW;       //VSYNC 低电平有效
    DCMI_Handler.Init.HSPolarity=DCMI_HSPOLARITY_LOW;       //HSYNC 低电平有效
    DCMI_Handler.Init.CaptureRate=DCMI_CR_ALL_FRAME;        //全帧捕获
    DCMI_Handler.Init.ExtendedDataMode=DCMI_EXTEND_DATA_8B; //8位数据格式 
    HAL_DCMI_Init(&DCMI_Handler);                           //初始化DCMI，此函数会开启帧中断  
}

//DCMI底层驱动，引脚配置，时钟使能，中断配置
//此函数会被HAL_DCMI_Init()调用
//hdcmi:DCMI句柄
void HAL_DCMI_MspInit(DCMI_HandleTypeDef* hdcmi)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    __HAL_RCC_DCMI_CLK_ENABLE();                //使能DCMI时钟

    __HAL_RCC_GPIOA_CLK_ENABLE();               //使能GPIOA时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();               //使能GPIOB时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();               //使能GPIOC时钟
    __HAL_RCC_GPIOD_CLK_ENABLE();               //使能GPIOD时钟
    __HAL_RCC_GPIOH_CLK_ENABLE();               //使能GPIOH时钟
    
    //初始化PA6
    GPIO_Initure.Pin=GPIO_PIN_6;  
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;          //推挽复用
    GPIO_Initure.Pull=GPIO_PULLUP;              //上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;         //高速
    GPIO_Initure.Alternate=GPIO_AF13_DCMI;      //复用为DCMI   
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);         //初始化
    
    //PB7,8,9
    GPIO_Initure.Pin=GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;  
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);         //初始化
    
    //PC6,7,8,9,11
    GPIO_Initure.Pin=GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|\
                     GPIO_PIN_9|GPIO_PIN_11;  
    HAL_GPIO_Init(GPIOC,&GPIO_Initure);         //初始化
    
    //PD3
    GPIO_Initure.Pin=GPIO_PIN_3; 
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);         //初始化
    
    //PH8
    GPIO_Initure.Pin=GPIO_PIN_8; 
    HAL_GPIO_Init(GPIOH,&GPIO_Initure);         //初始化   
    
    HAL_NVIC_SetPriority(DCMI_IRQn,0,0);        //抢占优先级1，子优先级2
    HAL_NVIC_EnableIRQ(DCMI_IRQn);              //使能DCMI中断
}

//DCMI DMA配置
//mem0addr:存储器地址0  将要存储摄像头数据的内存地址(也可以是外设地址)
//mem1addr:存储器地址1  当只使用mem0addr的时候,该值必须为0
//memblen:存储器位宽,可以为:DMA_MDATAALIGN_BYTE/DMA_MDATAALIGN_HALFWORD/DMA_MDATAALIGN_WORD
//meminc:存储器增长方式,可以为:DMA_MINC_ENABLE/DMA_MINC_DISABLE
void DCMI_DMA_Init(u32 mem0addr,u32 mem1addr,u16 memsize,u32 memblen,u32 meminc)
{ 
    __HAL_RCC_DMA2_CLK_ENABLE();                                    //使能DMA2时钟
    __HAL_LINKDMA(&DCMI_Handler,DMA_Handle,DMADMCI_Handler);        //将DMA与DCMI联系起来
    DMADMCI_Handler.Instance=DMA2_Stream1;                          //DMA2数据流1                     
    DMADMCI_Handler.Init.Channel=DMA_CHANNEL_1;                     //通道1
    DMADMCI_Handler.Init.Direction=DMA_PERIPH_TO_MEMORY;            //外设到存储器
    DMADMCI_Handler.Init.PeriphInc=DMA_PINC_DISABLE;                //外设非增量模式
    DMADMCI_Handler.Init.MemInc=meminc;                             //存储器增量模式
    DMADMCI_Handler.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;   //外设数据长度:32位
    DMADMCI_Handler.Init.MemDataAlignment=memblen;                  //存储器数据长度:8/16/32位
    DMADMCI_Handler.Init.Mode=DMA_CIRCULAR;                         //使用循环模式 
    DMADMCI_Handler.Init.Priority=DMA_PRIORITY_HIGH;                //高优先级
    DMADMCI_Handler.Init.FIFOMode=DMA_FIFOMODE_ENABLE;              //使能FIFO
    DMADMCI_Handler.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_HALFFULL; //使用1/2的FIFO 
    DMADMCI_Handler.Init.MemBurst=DMA_MBURST_SINGLE;                //存储器突发传输
    DMADMCI_Handler.Init.PeriphBurst=DMA_PBURST_SINGLE;             //外设突发单次传输 
    HAL_DMA_DeInit(&DMADMCI_Handler);                               //先清除以前的设置
    HAL_DMA_Init(&DMADMCI_Handler);	                                //初始化DMA
    
 
    __HAL_UNLOCK(&DMADMCI_Handler);
    if(mem1addr==0)    //开启DMA，不使用双缓冲
    {
        HAL_DMA_Start(&DMADMCI_Handler,(u32)&DCMI->DR,mem0addr,memsize);
    }
    else                //使用双缓冲
    {
        HAL_DMAEx_MultiBufferStart(&DMADMCI_Handler,(u32)&DCMI->DR,mem0addr,mem1addr,memsize);//开启双缓冲
        __HAL_DMA_ENABLE_IT(&DMADMCI_Handler,DMA_IT_TC);    //开启传输完成中断
        HAL_NVIC_SetPriority(DMA2_Stream1_IRQn,0,0);        //DMA中断优先级
        HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
    }
}

//DCMI,启动传输
void DCMI_Start(void)
{  
   // LCD_SetCursor(0,0);  
	//LCD_WriteRAM_Prepare();		        //开始写入GRAM
    __HAL_DMA_ENABLE(&DMADMCI_Handler); //使能DMA
    DCMI->CR|=DCMI_CR_CAPTURE;          //DCMI捕获使能
}

//DCMI,关闭传输
void DCMI_Stop(void)
{ 
    DCMI->CR&=~(DCMI_CR_CAPTURE);       //关闭捕获
    while(DCMI->CR&0X01);               //等待传输完成
    __HAL_DMA_DISABLE(&DMADMCI_Handler);//关闭DMA
} 

//DCMI中断服务函数
void DCMI_IRQHandler(void)
{
    HAL_DCMI_IRQHandler(&DCMI_Handler);
}

//捕获到一帧图像处理函数
//hdcmi:DCMI句柄
void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
	jpeg_data_process();//jpeg数据处理
	LED1=!LED1;
	ov_frame++; 
    //重新使能帧中断,因为HAL_DCMI_IRQHandler()函数会关闭帧中断
    __HAL_DCMI_ENABLE_IT(&DCMI_Handler,DCMI_IT_FRAME);
}

 

//DCMI设置显示窗口
//sx,sy;LCD的起始坐标
//width,height:LCD显示范围.
void DCMI_Set_Window(u16 sx,u16 sy,u16 width,u16 height)
{
	DCMI_Stop(); 
//	LCD_Clear(WHITE);
//	LCD_Set_Window(sx,sy,width,height);
//	OV2640_OutSize_Set(width,height);
 //   LCD_SetCursor(0,0);  
//	LCD_WriteRAM_Prepare();		        //开始写入GRAM  
    __HAL_DMA_ENABLE(&DMADMCI_Handler); //开启DMA2,Stream1 
    DCMI->CR|=DCMI_CR_CAPTURE;          //DCMI捕获使能	
}
   
//通过usmart调试,辅助测试用.
//pclk/hsync/vsync:三个信号的有限电平设置
void DCMI_CR_Set(u8 pclk,u8 hsync,u8 vsync)
{
    HAL_DCMI_DeInit(&DCMI_Handler);//清除原来的设置
    DCMI_Handler.Instance=DCMI;
    DCMI_Handler.Init.SynchroMode=DCMI_SYNCHRO_HARDWARE;//硬件同步HSYNC,VSYNC
    DCMI_Handler.Init.PCKPolarity=pclk<<5;              //PCLK 上升沿有效
    DCMI_Handler.Init.VSPolarity=vsync<<7;              //VSYNC 低电平有效
    DCMI_Handler.Init.HSPolarity=hsync<<6;              //HSYNC 低电平有效
    DCMI_Handler.Init.CaptureRate=DCMI_CR_ALL_FRAME;    //全帧捕获
    DCMI_Handler.Init.ExtendedDataMode=DCMI_EXTEND_DATA_8B;//8位数据格式 
    HAL_DCMI_Init(&DCMI_Handler);                       //初始化DCMI
    DCMI_Handler.Instance->CR|=DCMI_MODE_CONTINUOUS;    //持续模式
}


