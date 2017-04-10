#ifndef __DCMI_H
#define __DCMI_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F746开发板
//DCMI驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/12/16
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

extern void (*dcmi_rx_callback)(void);//DCMI DMA接收回调函数

extern DCMI_HandleTypeDef DCMI_Handler;        //DCMI句柄
extern DMA_HandleTypeDef  DMADMCI_Handler;     //DMA句柄

void DCMI_Init(void);
void DCMI_DMA_Init(u32 mem0addr,u32 mem1addr,u16 memsize,u32 memblen,u32 meminc);
void DCMI_Start(void);
void DCMI_Stop(void);
void DCMI_Set_Window(u16 sx,u16 sy,u16 width,u16 height);
void DCMI_CR_Set(u8 pclk,u8 hsync,u8 vsync);
#endif
