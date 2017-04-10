#ifndef _PROTOCOL_H
#define _PROTOCOL_H
#include "sys.h"






#define FLASH_SAVE_ADDR  0X08080000 

typedef struct
{
	u8 mode;
	u8 cap;
	u8 ip[4];
	u16 port;
	u16 size[2];
	u8 rate;
	u8 bright;
	u8 contrast;
	u8 exp;
	u8 pol;
}SavePara_TypeDef;



 

typedef struct
{
	char Start[8];
	u8 Lenth[4];
	u8 Width[2];
	u8 Height[2];
  
}Pro_Com_TypeDef;

/***************命令帧***************/
typedef struct
{
	Pro_Com_TypeDef  Pro_HeadPart; 
	u8 Head[2]; 
	char Cmd[7]; 
	char Para[20]; 
	u8 End[2]; 
  
}Pro_CMD_TypeDef;

/***************响应帧***************/
typedef struct
{
	Pro_Com_TypeDef  Pro_HeadPart; 
	u8 Head[2]; 
	char Cmd[7]; 
	char Value[100]; 
	u8 End[2]; 
  
}Pro_ACK_TypeDef;


u8 PollMessage(u8 *recbuf);//轮询发来的命令
u8 ProcessMessage(void);
u8 GetCmd(char *str);//判断命令码是否符合
u8 GetParaLen(u8 *para);//得到参数区的长度

//set指令  上位机发来以'\0' 字符为结束标志   成功返回配置参数的长度，失败返回状态码
u8 Pro_setmod(char *para,int *pArray);//设置工作模式：0停止采集；1软件单次触发；2IO触发；99连续采集    
u8 Pro_setcap(char *para,int *pArray);//设置开始捕获：当软件触发模式，立即采集一幅图片，然后等待下一个命令；当IO触发模式，等待IO的电平变化，立即采集一幅图片，然后等待一下IO电平变化；当连续采集模式，一直采集   
u8 Pro_setipa(char *para,int *pArray);// [192,168,0,100]设置IP,整型数组  都是字符
u8 Pro_setpot(char *para,int *pArray);//8088 设置端口，整数 字符串，长度不定
u8 Pro_setsiz(char *para,int *pArray);//[480,320] 设置分辨率：整形数组，480,720,1024等,实际分辨率用支持的最接近的去代替 
u8 Pro_setrat(char *para,int *pArray);//设置帧率:30,15,5等
u8 Pro_setbrt(char *para,int *pArray);//设置亮度：0-100整数
u8 Pro_setcon(char *para,int *pArray);//设置对比度:0-100整数
u8 Pro_setexp(char *para,int *pArray);//设置曝光时间：us
u8 Pro_setpol(char *para,int *pArray);//设置IO触发极性:1上升沿；0下降沿

u8 Pro_getver(char *para);//获得版本号,参数不检查
u8 Pro_getipa(char *para);//获得IP, 参数不检查
u8 Pro_getpot(char *para);//获得端口，参数不检查
u8 Pro_getsiz(char *para);//获得分辨率：参数不检查
u8 Pro_getrat(char *para);//获得帧率: 参数不检查
u8 Pro_getbrt(char *para);//获得亮度：参数不检查
u8 Pro_getcon(char *para);//获得置对比度: 参数不检查
u8 Pro_getexp(char *para);//获得曝光时间：参数不检查
void Pro_gethep(char *para);//获得所有支持命令：参数不检查


#endif
