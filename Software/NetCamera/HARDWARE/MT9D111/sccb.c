#include "sccb.h"
#include "delay.h"


//初始化SCCB接口
void SCCB_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_GPIOD_CLK_ENABLE();            
    
    //PB3.4初始化设置
    GPIO_Initure.Pin=GPIO_PIN_4|GPIO_PIN_5;
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;   
    GPIO_Initure.Pull=GPIO_PULLUP;           
    GPIO_Initure.Speed=GPIO_SPEED_FAST;      
    HAL_GPIO_Init(GPIOD,&GPIO_Initure); 
}

//SCCB起始信号
 
void SCCB_Start(void)
{
    SCCB_SDA=1;         
    SCCB_SCL=1;	    
    delay_us(50);  
    SCCB_SDA=0;
    delay_us(50);	 
    SCCB_SCL=0;	     	  
}

//SCCB停止信号
 
void SCCB_Stop(void)
{
    SCCB_SDA=0;
    delay_us(50);	 
    SCCB_SCL=1;	
    delay_us(50); 
    SCCB_SDA=1;	
    delay_us(50);
}  
//产生NA信号
void SCCB_No_Ack(void)
{
	delay_us(50);
	SCCB_SDA=1;	
	SCCB_SCL=1;	
	delay_us(50);
	SCCB_SCL=0;	
	delay_us(50);
	SCCB_SDA=0;	
	delay_us(50);
}
//SCCB,写入一个字节
 
u8 SCCB_WR_Byte(u8 dat)
{
	u8 j,res;	 
	for(j=0;j<8;j++)  
	{
		if(dat&0x80)SCCB_SDA=1;	
		else SCCB_SDA=0;
		dat<<=1;
		delay_us(50);
		SCCB_SCL=1;	
		delay_us(50);
		SCCB_SCL=0;		   
	}			 
	SCCB_SDA_IN();		 
	delay_us(50);
	SCCB_SCL=1;			 
	delay_us(50);
	if(SCCB_READ_SDA)res=1;   
	else res=0;          
	SCCB_SCL=0;		 
	SCCB_SDA_OUT();		   
	return res;  
}	 
//SCCB 读取一个字节
 

u8 SCCB_RD_Byte(void)
{
	u8 temp=0,j;    
	SCCB_SDA_IN();		   
	for(j=8;j>0;j--) 	 
	{		     	  
		delay_us(50);
		SCCB_SCL=1;
		temp=temp<<1;
		if(SCCB_READ_SDA)temp++;   
		delay_us(50);
		SCCB_SCL=0;
	}	
	SCCB_SDA_OUT();		     
	return temp;
} 							    


//写寄存器，写入16bit数据
u16 SCCB_16bit_WR_Reg(u8 reg,u16 data)
{
	u8 res;
	u8 tmp;
	SCCB_Start(); 					 
	if(SCCB_WR_Byte(SCCB_MT9V111_W_ID))res=1;	   
	delay_us(100);
	if(SCCB_WR_Byte(reg))res=1;		 	  
	delay_us(100);
	tmp = data>>8;
	if(SCCB_WR_Byte(tmp))res=1; 	 
	delay_us(100);	
	tmp = data&0x00ff;
	if(SCCB_WR_Byte(tmp))res=1; 	 
	SCCB_Stop();	  
  return	res;
}


//读寄存器，读取16bit数据
//返回值:读到的寄存器值
u16 SCCB_16bit_RD_Reg(u8 reg)
{
	u16 val=0;
	SCCB_Start(); 				 
	SCCB_WR_Byte(SCCB_MT9V111_W_ID);		   
	delay_us(100);	 
  SCCB_WR_Byte(reg);			   
	delay_us(100);	  
	SCCB_Stop();   
	delay_us(100);	   
 
	SCCB_Start();
	SCCB_WR_Byte(SCCB_MT9V111_R_ID);	 
	delay_us(100);
  val|=SCCB_RD_Byte()<<8;		 	 
 
  delay_us(50);
	SCCB_SDA=0;	
	SCCB_SCL=1;	
	delay_us(50);
	SCCB_SCL=0;	
	delay_us(50);
	SCCB_SDA=0;	
	delay_us(50);
	
	
	
  val|=SCCB_RD_Byte();		 	 
	

  SCCB_No_Ack();
  SCCB_Stop();
  return val;
}

u16 Page_16bit_WR_Reg(u8 page,u8 reg,u16 data)
{
	SCCB_16bit_WR_Reg(0xf0,page);
	SCCB_16bit_WR_Reg(reg,data);
	return 0;
}
u16 Page_16bit_RD_Reg(u8 page,u8 reg)
{
	u16 ret;
	SCCB_16bit_WR_Reg(0xf0,page);
	ret=SCCB_16bit_RD_Reg(reg);
	return ret;
}
