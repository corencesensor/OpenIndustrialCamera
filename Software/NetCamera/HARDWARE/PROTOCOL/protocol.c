#include "protocol.h"
#include "string.h"
#include "stdlib.h"
#include "stmflash.h"
#include "sccb.h"
#include "timer.h"

#include "lwip/opt.h"
#include "lwip_comm.h"
#include "lwip/lwip_sys.h"
#include "lwip/api.h"


SavePara_TypeDef SaveParaList={

	99,//连续采集
	0,//捕获 暂留
	192,168,1,150,//ip
	8088,//端口号
	640,480,//分辨率
	30,//帧率
	50,//亮度
	50,//对比度
	60,//曝光时间
	1//触发极性	
	
};//参数保存表

u8 ParaLen;//参数区长度
u8 ValidCmd[100];//响应帧发送数组 

u8 iCount;//命令帧的字符计数
char Value[100];//gethelp参数返回值



err_t sendGetErr;
//--------PROTOCOL----------------------------------------------------------//
u8 PollMessage(u8 *recbuf)//轮询发来的命令
{
	int i;
	char *ptr;
	
	char Start[8];
	u8 Lenth[4];
	u8 Width[2];
	u8 Height[2];
	u8 Head[2];
	char Cmd[7];
	char Para[20];
	u8 End[2];
	

	
	u8 CmdVal; 
 
	int Str2num; 
	
	u8 AckLen;
	
	int RtnValue[20]; 
	u8 RtnLen; 
	
	Pro_ACK_TypeDef AckFrame;
	
	iCount = 0; 
	
	memcpy(Start,recbuf,8);
	if(strcmp(Start,"CORENCE")==0) 
	{
		printf("获得Start[8]=%s\n",Start);
		sprintf(AckFrame.Pro_HeadPart.Start,"CORENCE"); 
		iCount += 8; 
		
		
		memcpy(Lenth,recbuf+iCount,4); 
		printf("获得Lenth[4]=%x %x %x %x\n",Lenth[0],Lenth[1],Lenth[2],Lenth[3]);
		iCount += 4;
		AckFrame.Pro_HeadPart.Lenth[0]=0;
		AckFrame.Pro_HeadPart.Lenth[1]=0;
		AckFrame.Pro_HeadPart.Lenth[2]=0;
		AckFrame.Pro_HeadPart.Lenth[3]=0; 
		
		
		memcpy(Width,recbuf+iCount,2); 
		printf("获得Width[2]=%x %x\n",Width[0],Width[1]);
		iCount += 2;
		AckFrame.Pro_HeadPart.Width[0]=0;
		AckFrame.Pro_HeadPart.Width[1]=0; 
		
		
		memcpy(Height,recbuf+iCount,2); 
		printf("获得Height[2]=%x %x\n",Height[0],Height[1]);
		iCount += 2;
		AckFrame.Pro_HeadPart.Height[0]=0;
		AckFrame.Pro_HeadPart.Height[1]=0;
		
		if((Width[0]+Width[1]+Height[0]+Height[1])==0) 
		{
			memcpy(Head,recbuf+iCount,2);
			printf("获得Head[2]=%x %x\n",Head[0],Head[1]);
			if((Head[0]==0xff)&&(Head[1]==0x7b)) 
			{
				
				iCount += 2;
				AckFrame.Head[0]=0xff;
				AckFrame.Head[1]=0x7b; 
				
				memcpy(Cmd,recbuf+iCount,7); 
				printf("获得Cmd[7]=%c%c%c%c%c%c%c\n",Cmd[0],Cmd[1],Cmd[2],Cmd[3],Cmd[4],Cmd[5],Cmd[6]);
				iCount += 7;
				CmdVal=GetCmd(Cmd);
				printf("解析命令码是否正确\n");
				AckFrame.Cmd[0]=Cmd[0];AckFrame.Cmd[1]=Cmd[1];AckFrame.Cmd[2]=Cmd[2];AckFrame.Cmd[3]=Cmd[3];AckFrame.Cmd[4]=Cmd[4];AckFrame.Cmd[5]=Cmd[5];AckFrame.Cmd[6]=Cmd[6];
				
				
				if(CmdVal!=0xff) 
				{
					printf("有效命令:%c%c%c%c%c%c%c\n",Cmd[0],Cmd[1],Cmd[2],Cmd[3],Cmd[4],Cmd[5],Cmd[6]);
					
					ParaLen=GetParaLen(recbuf+iCount);
					if(ParaLen ==0xff) printf("参数区超范围\n");
					else if(ParaLen == 0x00) printf("参数区为空\n");
					
					else 
					{
						memcpy(Para,recbuf+iCount,ParaLen); 
						iCount = iCount+ParaLen+1; 
						Str2num=atoi(Para);
						memcpy(End,recbuf+iCount,2);
						if((End[0]==0x7d)&& (End[1]== 0x00))
						{
							switch(CmdVal)
							{
								case 0x01:printf("setmod \n");RtnLen=Pro_setmod(Para,RtnValue);SaveParaList.mode=RtnValue[0];break;
								case 0x02:printf("setcap \n");RtnLen=Pro_setcap(Para,RtnValue);SaveParaList.cap=RtnValue[0];break;
								case 0x03:printf("setipa \n");RtnLen=Pro_setipa(Para,RtnValue);SaveParaList.ip[0]=RtnValue[0];SaveParaList.ip[1]=RtnValue[1];SaveParaList.ip[2]=RtnValue[2];SaveParaList.ip[3]=RtnValue[3];break;
								case 0x04:printf("setpot \n");RtnLen=Pro_setpot(Para,RtnValue);SaveParaList.port=RtnValue[0];break;
								case 0x05:printf("setsiz \n");RtnLen=Pro_setsiz(Para,RtnValue);SaveParaList.size[0]=RtnValue[0];SaveParaList.size[1]=RtnValue[1];break;
								case 0x06:printf("setrat \n");RtnLen=Pro_setrat(Para,RtnValue);SaveParaList.rate=RtnValue[0];break;
								case 0x07:printf("setbrt \n");RtnLen=Pro_setbrt(Para,RtnValue);SaveParaList.bright=RtnValue[0];break;
								case 0x08:printf("setcon \n");RtnLen=Pro_setcon(Para,RtnValue);SaveParaList.contrast=RtnValue[0];break;
								case 0x09:printf("setexp \n");RtnLen=Pro_setexp(Para,RtnValue);SaveParaList.exp=RtnValue[0];break;
								case 0x0A:printf("setpol \n");RtnLen=Pro_setpol(Para,RtnValue);SaveParaList.pol=RtnValue[0];break;
								
								case 0x10:printf("getver \n");Pro_getver(Para);break;
								case 0x11:printf("getipa \n");Pro_getipa(Para);sprintf((char *)ValidCmd+25,"[%d,%d,%d,%d]",SaveParaList.ip[0],SaveParaList.ip[1],SaveParaList.ip[2],SaveParaList.ip[3]);break;
								case 0x12:printf("getpot \n");Pro_getpot(Para);sprintf((char *)ValidCmd+25,"%d",SaveParaList.port);break;
								case 0x13:printf("getsiz \n");Pro_getsiz(Para);sprintf((char *)ValidCmd+25,"[%d,%d]",SaveParaList.size[0],SaveParaList.size[1]);break;
								case 0x14:printf("getrat \n");Pro_getrat(Para);sprintf((char *)ValidCmd+25,"%d",SaveParaList.rate);break;
								case 0x15:printf("getbrt \n");Pro_getbrt(Para);sprintf((char *)ValidCmd+25,"%d",SaveParaList.bright);break;
								case 0x16:printf("getcon \n");Pro_getcon(Para);sprintf((char *)ValidCmd+25,"%d",SaveParaList.contrast);break;
								case 0x17:printf("getexp \n");Pro_getexp(Para);sprintf((char *)ValidCmd+25,"%d",SaveParaList.exp);break;
								case 0x18:printf("gethep \n");Pro_gethep(Para);break;
								
								default:printf("无效命令,使用gethep获取支持命令\n");break;
								
							}
							sprintf((char *)ValidCmd,"CORENCE");
							
							ValidCmd[12]= 0x00;ValidCmd[13]=0x00;ValidCmd[14]=0x00;ValidCmd[15]=0x00;
							
							ValidCmd[16]=AckFrame.Head[0];ValidCmd[17]=AckFrame.Head[1];
							ValidCmd[18]=AckFrame.Cmd[0];ValidCmd[19]=AckFrame.Cmd[1];ValidCmd[20]=AckFrame.Cmd[2];ValidCmd[21]=AckFrame.Cmd[3];ValidCmd[22]=AckFrame.Cmd[4];ValidCmd[23]=AckFrame.Cmd[5];ValidCmd[24]=AckFrame.Cmd[6];
							
							if(CmdVal >0 && CmdVal<0x0B)
							{
							
								AckLen = 15+ RtnLen+1; 
									
								ValidCmd[25]=0;ValidCmd[26]='\0';
								ParaLen = 26;
								ValidCmd[27]=End[0];ValidCmd[28]=End[1];

								STMFLASH_Write(FLASH_SAVE_ADDR,(u32 *)&SaveParaList,sizeof(SavePara_TypeDef)/4+((sizeof(SavePara_TypeDef)%4)?1:0));
							}
							
							else if(CmdVal >0x0f && CmdVal < 0x18)
							{
								ptr = strchr((char *)ValidCmd+25,'\0');
								ParaLen = ptr - (char *)&ValidCmd[0];
								ValidCmd[ParaLen+1]=End[0];ValidCmd[ParaLen+2]=End[1];
								
								AckLen = ParaLen -9;
								
							}
							else
							{
								sprintf((char *)ValidCmd+25,"getver,get ip,getpot,getsiz,getrat,getbrt,getcon,getexp");
								ptr = strchr((char *)ValidCmd+25,'\0');
								ParaLen = ptr - (char *)&ValidCmd[0];
								ValidCmd[ParaLen+1]=End[0];ValidCmd[ParaLen+2]=End[1];
								
								AckLen = ParaLen -9;
								
							}
							
							AckFrame.Pro_HeadPart.Lenth[0] = AckLen&0x000000ff;
							AckFrame.Pro_HeadPart.Lenth[1]=(AckLen&0x0000ff00)>>8;
							AckFrame.Pro_HeadPart.Lenth[2]=(AckLen&0x00ff0000)>>16;
							AckFrame.Pro_HeadPart.Lenth[3]=(AckLen&0xff000000)>>24;
							
							ValidCmd[8]=AckFrame.Pro_HeadPart.Lenth[3];ValidCmd[9]=AckFrame.Pro_HeadPart.Lenth[2];ValidCmd[10]=AckFrame.Pro_HeadPart.Lenth[1];ValidCmd[11]=AckFrame.Pro_HeadPart.Lenth[0];
							
							
							for(i=0;i<ParaLen+3;i++)
							{
								printf("%02x ",ValidCmd[i]);
							}
							
							if(CmdVal==0x03 || CmdVal==0x05)
							{
								HAL_NVIC_SystemReset();
							}
							
						}
						else
						{
							printf("缺少帧尾\n");
						}
						
						
					}
					
					
				}
				else
				{
					printf("无效命令,使用gethep获取支持命令\n");

				}

			}
			else
			{
				printf("不合法命令帧\n");

			}
			
		}	
		
	}
	else
	{
		printf("无效帧头\n");

	}
	return 0;
}



u8 GetCmd(char *str)
{
	str[7] = '\0';
    if(!strcmp(str,"setmod:")) return 0x01;
    if(!strcmp(str,"setcap:")) return 0x02;
    if(!strcmp(str,"setipa:")) return 0x03;
    if(!strcmp(str,"setpot:")) return 0x04;
    if(!strcmp(str,"setsiz:")) return 0x05;
	if(!strcmp(str,"setrat:")) return 0x06;
	if(!strcmp(str,"setbrt:")) return 0x07;
	if(!strcmp(str,"setcon:")) return 0x08;
	if(!strcmp(str,"setexp:")) return 0x09;
	if(!strcmp(str,"setpol:")) return 0x0A;
	
	
	if(!strcmp(str,"getver:")) return 0x10;
	if(!strcmp(str,"getipa:")) return 0x11;
	if(!strcmp(str,"getpot:")) return 0x12;
	if(!strcmp(str,"getsiz:")) return 0x13;
	if(!strcmp(str,"getrat:")) return 0x14;
	if(!strcmp(str,"getbrt:")) return 0x15;
	if(!strcmp(str,"getcon:")) return 0x16;
	if(!strcmp(str,"getexp:")) return 0x17;
	if(!strcmp(str,"gethep:")) return 0x18;
	
   return 0xff;
}

u8 GetParaLen(u8 *para)
{
	int i;
	i=0;

	while(para[i]!='\0')
	{
		i++;
		if(i>20)
		break;
	}
	if(i>20)
	{return 0xff;}
	return i;
	
}
//扫描字符串，读取整形数值或数组[192,168,0,1] 或 182
//para 输入字符串
//pArray 返回数组
//iMaxArrayLen 最长数组长度
//返回 数组长度，0异常
u8 Scan_Params(char *para,int *pArray,int iMaxArrayLen) 
{
	char * pStart=para;
	char * pEnd=NULL;
	int	iRtnCount=0;
	int i;
	pStart = strchr(pStart,'[');
	
	if(pStart==NULL)
	{
		pStart=para;
		pEnd = strchr(pStart,',');
		if(pEnd!=NULL)
			*pEnd='\0';
		if(strlen(pStart)<10)
		{
			pArray[0]=atoi(pStart);
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		for(i=0;i<iMaxArrayLen;i++)
		{
			pStart++;
			pEnd = strchr(pStart,',');
			if(pEnd!=NULL)
			{
				*pEnd='\0';
				if(strlen(pStart)<10)
				{
					pArray[iRtnCount]=atoi(pStart);
					iRtnCount++;
					if(iRtnCount>=iMaxArrayLen)
						return iRtnCount;
				}
				pStart=pEnd;
			}
			else
			{
				pEnd = strchr(pStart,']');
				*pEnd ='\0'; 
				if(strlen(pStart)<10)
				{
					
					pArray[iRtnCount]=atoi(pStart);
					iRtnCount++;
					if(iRtnCount>=iMaxArrayLen)
						return iRtnCount;
				}
				return iRtnCount;
			}
		}
	}
	return 0;
}


u8 Pro_setmod(char *para,int *pArray) 
{
	int iRtnData[8];
	int iRtnCount;
	iRtnCount=Scan_Params(para,iRtnData,8); 
	*pArray = iRtnData[0];
	return iRtnCount;
}

u8 Pro_setcap(char *para,int *pArray) 
{

	int iRtnData[8];
	int iRtnCount;
	iRtnCount=Scan_Params(para,iRtnData,8); 
	*pArray = iRtnData[0];
	return iRtnCount;
}

u8 Pro_setipa(char *para,int *pArray)
{
	int i;
	int iRtnData[8];
	int iRtnCount;
	iRtnCount=Scan_Params(para,iRtnData,8); 
	for(i=0;i<iRtnCount;i++)
	{
		*pArray = iRtnData[i];
		pArray++;
	}	
	return iRtnCount;

}

u8 Pro_setpot(char *para,int *pArray)
{
	int iRtnData[8];
	int iRtnCount;
	iRtnCount=Scan_Params(para,iRtnData,8); 
	*pArray = iRtnData[0];
	return iRtnCount;
}

u8 Pro_setsiz(char *para,int *pArray) 
{
	int i;
	int iRtnData[8];
	int iRtnCount;
	iRtnCount=Scan_Params(para,iRtnData,8); 

	if(iRtnData[0]>1024)
	{
		iRtnData[0]=1024;
	}
	if(iRtnData[1]>768)
	{
		iRtnData[1]=768;
	}
	if(iRtnData[1]>=iRtnData[0]*0.75)
	{
		iRtnData[1] = iRtnData[0]*0.75;
	}
	else
	{
		iRtnData[0] = 4.0/3.0*iRtnData[1];
	}
	
	for(i=0;i<iRtnCount;i++)
	{
		*pArray = iRtnData[i];
		pArray++;
	}	
	
	
	return iRtnCount;
}

u8 Pro_setrat(char *para,int *pArray)
{
	int iRtnData[8];
	int iRtnCount;
	iRtnCount=Scan_Params(para,iRtnData,8); 
	*pArray = iRtnData[0];
	return iRtnCount;
}

u8 Pro_setbrt(char *para,int *pArray)
{
	int iRtnData[8];
	int iRtnCount;
	int ledLight;
	
	iRtnCount=Scan_Params(para,iRtnData,8); 
	*pArray = iRtnData[0];
	
	ledLight = iRtnData[0]*5;
	if(ledLight == 500) ledLight = 499;
	if(ledLight<6) ledLight = 0;
	TIM_SetTIM3Compare4(ledLight);
	return iRtnCount;
}

u8 Pro_setcon(char *para,int *pArray)
{
	int iRtnData[8];
	int iRtnCount;
	iRtnCount=Scan_Params(para,iRtnData,8); 
	printf("获得长度为%d，数据为%d\n",iRtnCount,iRtnData[0]);
	*pArray = iRtnData[0];
	
	
	Page_16bit_WR_Reg(0x01,0xC6,0XA743);
	Page_16bit_WR_Reg(0x01,0xC8,((iRtnData[0]/25)<<4)+3);
	
	Page_16bit_WR_Reg(0x01,0xC6,0XA744);
	Page_16bit_WR_Reg(0x01,0xC8,((iRtnData[0]/25)<<4)+3);
	printf("iRtnData[0]=%d",iRtnData[0]);
	return iRtnCount;
}

u8 Pro_setexp(char *para,int *pArray)
{
	int iRtnData[8];
	int iRtnCount;
	iRtnCount=Scan_Params(para,iRtnData,8); 
	*pArray = iRtnData[0];

	if(iRtnData[0]==99)
	{
	
		Page_16bit_WR_Reg(0x01,0xC6,0xa102);
		Page_16bit_WR_Reg(0x01,0xC8,0x0f);
	}
	else if(iRtnData[0]==1)
	{
		Page_16bit_WR_Reg(0x01,0xC6,0xa102);
		Page_16bit_WR_Reg(0x01,0xC8,0x00);
	}
	else
	{

	Page_16bit_WR_Reg(0x00,0x09,iRtnData[0]*13);
	}

	
	return iRtnCount;
}

u8 Pro_setpol(char *para,int *pArray)
{
	int iRtnData[8];
	int iRtnCount;
	iRtnCount=Scan_Params(para,iRtnData,8); 
	
	*pArray = iRtnData[0];
	return iRtnCount;
}


u8 Pro_getver(char *para) //获得版本号,参数不检查
{
	printf("ver = 1\n");
	return 0;
}

u8 Pro_getipa(char *para) //获得IP, 参数不检查
{
	printf("ip = %d.%d.%d.%d\n",SaveParaList.ip[0],SaveParaList.ip[1],SaveParaList.ip[2],SaveParaList.ip[3]);

	return 0;
}

u8 Pro_getpot(char *para) //获得端口，参数不检查
{
	printf("port = %d\n",SaveParaList.port);
	
	
	return 0;
}

u8 Pro_getsiz(char *para) //获得分辨率：参数不检查
{
	printf("size = %d,%d\n",SaveParaList.size[0],SaveParaList.size[1]);
	return 0;
}

u8 Pro_getrat(char *para) //获得帧率: 参数不检查
{
	printf("rate = %d\n",SaveParaList.rate);
	return 0;
}

u8 Pro_getbrt(char *para) //获得亮度：参数不检查
{
	printf("bright = %d\n",SaveParaList.bright);
	return 0;
}

u8 Pro_getcon(char *para) //获得置对比度: 参数不检查
{
	printf("contrast = %d\n",SaveParaList.contrast);
	return 0;
}

u8 Pro_getexp(char *para) //获得曝光时间：参数不检查
{
	printf("exp = %d\n",SaveParaList.exp);
	return 0;
}

void Pro_gethep(char *para) //获得所有支持命令：参数不检查
{
	sprintf(Value,"getver,get ip,getpot,getsiz,getrat,getbrt,getcon,getexp");
}

