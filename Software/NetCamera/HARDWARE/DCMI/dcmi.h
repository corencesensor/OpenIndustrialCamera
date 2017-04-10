#ifndef __DCMI_H
#define __DCMI_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F746������
//DCMI��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2015/12/16
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

extern void (*dcmi_rx_callback)(void);//DCMI DMA���ջص�����

extern DCMI_HandleTypeDef DCMI_Handler;        //DCMI���
extern DMA_HandleTypeDef  DMADMCI_Handler;     //DMA���

void DCMI_Init(void);
void DCMI_DMA_Init(u32 mem0addr,u32 mem1addr,u16 memsize,u32 memblen,u32 meminc);
void DCMI_Start(void);
void DCMI_Stop(void);
void DCMI_Set_Window(u16 sx,u16 sy,u16 width,u16 height);
void DCMI_CR_Set(u8 pclk,u8 hsync,u8 vsync);
#endif
