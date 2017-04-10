#ifndef __LAN8720_H
#define __LAN8720_H
#include "sys.h"
#include "stm32f4xx_hal_conf.h"
 
extern ETH_HandleTypeDef ETH_Handler;               //以太网句柄
extern ETH_DMADescTypeDef *DMARxDscrTab;			//以太网DMA接收描述符数据结构体指针
extern ETH_DMADescTypeDef *DMATxDscrTab;			//以太网DMA发送描述符数据结构体指针 
extern uint8_t *Rx_Buff; 							//以太网底层驱动接收buffers指针 
extern uint8_t *Tx_Buff; 							//以太网底层驱动发送buffers指针
extern ETH_DMADescTypeDef  *DMATxDescToSet;			//DMA发送描述符追踪指针
extern ETH_DMADescTypeDef  *DMARxDescToGet; 		//DMA接收描述符追踪指针 
 

u8 LAN8720_Init(void);
u32 LAN8720_ReadPHY(u16 reg);
void LAN8720_WritePHY(u16 reg,u16 value);
u8 LAN8720_Get_Speed(void);
u8 ETH_MACDMA_Config(void);
u8 ETH_Mem_Malloc(void);
void ETH_Mem_Free(void);
u32  ETH_GetRxPktSize(ETH_DMADescTypeDef *DMARxDesc);
#endif
