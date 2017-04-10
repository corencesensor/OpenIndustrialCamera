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

/***************����֡***************/
typedef struct
{
	Pro_Com_TypeDef  Pro_HeadPart; 
	u8 Head[2]; 
	char Cmd[7]; 
	char Para[20]; 
	u8 End[2]; 
  
}Pro_CMD_TypeDef;

/***************��Ӧ֡***************/
typedef struct
{
	Pro_Com_TypeDef  Pro_HeadPart; 
	u8 Head[2]; 
	char Cmd[7]; 
	char Value[100]; 
	u8 End[2]; 
  
}Pro_ACK_TypeDef;


u8 PollMessage(u8 *recbuf);//��ѯ����������
u8 ProcessMessage(void);
u8 GetCmd(char *str);//�ж��������Ƿ����
u8 GetParaLen(u8 *para);//�õ��������ĳ���

//setָ��  ��λ��������'\0' �ַ�Ϊ������־   �ɹ��������ò����ĳ��ȣ�ʧ�ܷ���״̬��
u8 Pro_setmod(char *para,int *pArray);//���ù���ģʽ��0ֹͣ�ɼ���1������δ�����2IO������99�����ɼ�    
u8 Pro_setcap(char *para,int *pArray);//���ÿ�ʼ���񣺵��������ģʽ�������ɼ�һ��ͼƬ��Ȼ��ȴ���һ�������IO����ģʽ���ȴ�IO�ĵ�ƽ�仯�������ɼ�һ��ͼƬ��Ȼ��ȴ�һ��IO��ƽ�仯���������ɼ�ģʽ��һֱ�ɼ�   
u8 Pro_setipa(char *para,int *pArray);// [192,168,0,100]����IP,��������  �����ַ�
u8 Pro_setpot(char *para,int *pArray);//8088 ���ö˿ڣ����� �ַ��������Ȳ���
u8 Pro_setsiz(char *para,int *pArray);//[480,320] ���÷ֱ��ʣ��������飬480,720,1024��,ʵ�ʷֱ�����֧�ֵ���ӽ���ȥ���� 
u8 Pro_setrat(char *para,int *pArray);//����֡��:30,15,5��
u8 Pro_setbrt(char *para,int *pArray);//�������ȣ�0-100����
u8 Pro_setcon(char *para,int *pArray);//���öԱȶ�:0-100����
u8 Pro_setexp(char *para,int *pArray);//�����ع�ʱ�䣺us
u8 Pro_setpol(char *para,int *pArray);//����IO��������:1�����أ�0�½���

u8 Pro_getver(char *para);//��ð汾��,���������
u8 Pro_getipa(char *para);//���IP, ���������
u8 Pro_getpot(char *para);//��ö˿ڣ����������
u8 Pro_getsiz(char *para);//��÷ֱ��ʣ����������
u8 Pro_getrat(char *para);//���֡��: ���������
u8 Pro_getbrt(char *para);//������ȣ����������
u8 Pro_getcon(char *para);//����öԱȶ�: ���������
u8 Pro_getexp(char *para);//����ع�ʱ�䣺���������
void Pro_gethep(char *para);//�������֧��������������


#endif
