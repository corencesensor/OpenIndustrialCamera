#ifndef __SCCB_H
#define __SCCB_H
#include "sys.h"

//IO方向设置
#define SCCB_SDA_IN()  {GPIOD->MODER&=~(3<<(4*2));GPIOD->MODER|=0<<4*2;}	 
#define SCCB_SDA_OUT() {GPIOD->MODER&=~(3<<(4*2));GPIOD->MODER|=1<<4*2;}     
//IO操作
#define SCCB_SCL        PDout(5)    //SCL
#define SCCB_SDA        PDout(4)    //SDA

#define SCCB_READ_SDA   PDin(4)     //输入SDA 

#define SCCB_MT9V111_W_ID  0X90    
#define SCCB_MT9V111_R_ID  0X91   
///////////////////////////////////////////
void SCCB_Init(void);
void SCCB_Start(void);
void SCCB_Stop(void);
void SCCB_No_Ack(void);
u8 SCCB_WR_Byte(u8 dat);
u8 SCCB_RD_Byte(void);
u8 SCCB_WR_Reg(u8 reg,u8 data);
u8 SCCB_RD_Reg(u8 reg);
u16 SCCB_16bit_WR_Reg(u8 reg,u16 data);
u16 SCCB_16bit_RD_Reg(u8 reg);
u16 Page_16bit_WR_Reg(u8 page,u8 reg,u16 data);
u16 Page_16bit_RD_Reg(u8 page,u8 reg);

#endif

