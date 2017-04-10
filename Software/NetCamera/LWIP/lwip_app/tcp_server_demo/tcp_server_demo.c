#include "tcp_server_demo.h"
#include "lwip/opt.h"
#include "lwip_comm.h"
#include "led.h"
#include "lwip/lwip_sys.h"
#include "lwip/api.h"
#include "delay.h"
#include "bb.h"
#include "sdram.h"
#include "dcmi.h"
#include "mt9d111.h"
#include "protocol.h"
#include "stmflash.h"
#include "timer.h"



u16 TCP_SERVER_PORT;//�������˿�
//����������ʼ��
extern SavePara_TypeDef SaveParaList;
extern u8 ParaLen;//����������
extern u8 ValidCmd[100];//��Ӧ֡�������� 


//����������ʼ��
SavePara_TypeDef DefaultList={

	99,//�����ɼ�
	0,//���� ����
	192,168,1,150,//ip
	8088,//�˿ں�
	640,480,//�ֱ���
	30,//֡��
	50,//����
	50,//�Աȶ�
	60,//�ع�ʱ��
	1//��������	
	
};
/*����ͷ���*/

extern char g_header[1024]; 

u8 ovx_mode=0;							 
u16 curline=0;							 

#define jpeg_buf_size   63*1024		 
  
u32 jpeg_data_buf[jpeg_buf_size] __attribute__((at(0XC0000000))); 
u32 jpeg_data_buf2[jpeg_buf_size] __attribute__((at(0XC0040000)));

u8 buf_flag=0;//���ж���Ϊ1������д��BUF1     0������д��BUF2
							//����ѭ����1��Ϊ����buf1       0����BUF2

volatile u32 jpeg_data_len=0; 			//buf�е�JPEG��Ч���ݳ��� 
volatile u8 jpeg_data_ok=0;				//JPEG���ݲɼ���ɱ�־ 
										//0,����û�вɼ���;
										//1,���ݲɼ�����,���ǻ�û����;
										//2,�����Ѿ����������,���Կ�ʼ��һ֡����
										
 
static  char send_logo[16]="CORENCE";  
static char send_end[2]={0xff,0xd9};



#define width 1024
#define height 768										

 
//����JPEG����
  
void jpeg_data_process(void)
{
 
    curline=0;

		if(jpeg_data_ok!=1)	//jpeg���ݻ�δ�ɼ���?
		{
			__HAL_DMA_DISABLE(&DMADMCI_Handler); 
			jpeg_data_len=jpeg_buf_size-__HAL_DMA_GET_COUNTER(&DMADMCI_Handler);//�õ�ʣ�����ݳ���	   
	
			jpeg_data_ok=1; 				 
		}
		if(jpeg_data_ok==2)	 
		{
			jpeg_data_ok=0;					    
		}
		if(buf_flag==0) 
		{
			DCMI_DMA_Init((u32)&jpeg_data_buf2,0,jpeg_buf_size,DMA_MDATAALIGN_WORD,DMA_MINC_ENABLE); 
 
		}
		else if(buf_flag==1) 
		{
			DCMI_DMA_Init((u32)&jpeg_data_buf,0,jpeg_buf_size,DMA_MDATAALIGN_WORD,DMA_MINC_ENABLE); 
 
		}
		__HAL_DMA_ENABLE(&DMADMCI_Handler);  
		
		buf_flag=buf_flag+1;
		if(buf_flag==2) 
		{buf_flag=0;}
 
}

//JPEG����
 
void jpeg_test(void)
{

	
	MT9D111_Jpeg_Config();	//JPEGģʽ
	DCMI_Init();			//DCMI����
	DCMI_DMA_Init((u32)&jpeg_data_buf,0,jpeg_buf_size,DMA_MDATAALIGN_WORD,DMA_MINC_ENABLE);  //������
	
	
	DCMI_Start(); 		//��������
  
}
 




/*�������*/


u8 tcp_server_recvbuf[TCP_SERVER_RX_BUFSIZE];	//TCP�ͻ��˽������ݻ�����
u8 tcp_server_sendbuf[1000]="Apollo STM32F4/F7 NETCONN TCP Server send data\r\n";	
u8 tcp_server_flag;								//TCP���������ݷ��ͱ�־λ

//TCP�ͻ�������
#define TCPSERVER_PRIO		6
//�����ջ��С
#define TCPSERVER_STK_SIZE	1024
//�����ջ
OS_STK TCPSERVER_TASK_STK[TCPSERVER_STK_SIZE];

//tcp����������
static void tcp_server_thread(void *arg)
{
	OS_CPU_SR cpu_sr;
	u32 data_len = 0;
	struct pbuf *q;
	err_t err,recv_err;
	u8 remot_addr[4];
	struct netconn *conn, *newconn;
	static ip_addr_t ipaddr;
	static u16_t 			port;
	u32 i;
	u8 *p;
	u32 len_m;
	u32 len_n;
	u32 len;
	
	u16 header_len;
	int ledLight;
	STMFLASH_Read(FLASH_SAVE_ADDR,(u32*)&SaveParaList,sizeof(SavePara_TypeDef)/4+((sizeof(SavePara_TypeDef)%4)?1:0));
		
	printf("last para is:\n");
	printf("mode=%d\n",SaveParaList.mode);
	printf("cap=%d\n",SaveParaList.cap);
	printf("ip=%d.%d.%d.%d\n",SaveParaList.ip[0],SaveParaList.ip[1],SaveParaList.ip[2],SaveParaList.ip[3]);
	printf("port=%d\n",SaveParaList.port);
	printf("size=%d,%d\n",SaveParaList.size[0],SaveParaList.size[1]);
	printf("rate=%d\n",SaveParaList.rate);
	printf("bright=%d\n",SaveParaList.bright);
	printf("contrast=%d\n",SaveParaList.contrast);
	printf("exp=%d\n",SaveParaList.exp);
	printf("pol=%d\n",SaveParaList.pol);
	
 
	ledLight = SaveParaList.bright*5;
	if(ledLight == 500) ledLight = 499;
	if(ledLight<6) ledLight = 0;
	TIM_SetTIM3Compare4(ledLight);	 
	
	
	
	if(SaveParaList.mode==0xff&&SaveParaList.cap==0xff) 
	{
 
		SaveParaList=DefaultList;
		STMFLASH_Write(FLASH_SAVE_ADDR,(u32 *)&SaveParaList,sizeof(SavePara_TypeDef)/4+((sizeof(SavePara_TypeDef)%4)?1:0));
	}
		
	
	TCP_SERVER_PORT = SaveParaList.port;
	LWIP_UNUSED_ARG(arg);

	conn = netconn_new(NETCONN_TCP);   
	netconn_bind(conn,IP_ADDR_ANY,TCP_SERVER_PORT);  //�󶨶˿� 8�Ŷ˿�
	netconn_listen(conn);  		//�������ģʽ
	conn->recv_timeout = 10;  	//��ֹ�����߳� �ȴ�10ms
	
		//����ͷ��س�ʼ��
	while(MT9D111_Init()) 
	{
	}
	jpeg_test();
	

	
	while (1) 
	{
		err = netconn_accept(conn,&newconn);  
		if(err==ERR_OK)newconn->recv_timeout = 10;

		if (err == ERR_OK)    
		{ 
			struct netbuf *recvbuf;

			netconn_getaddr(newconn,&ipaddr,&port,0); //��ȡԶ��IP��ַ�Ͷ˿ں�
			
			remot_addr[3] = (uint8_t)(ipaddr.addr >> 24); 
			remot_addr[2] = (uint8_t)(ipaddr.addr>> 16);
			remot_addr[1] = (uint8_t)(ipaddr.addr >> 8);
			remot_addr[0] = (uint8_t)(ipaddr.addr);
			printf("����%d.%d.%d.%d�����Ϸ�����,�����˿ں�Ϊ:%d\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3],port);
			
			while(1)
			{
				/*********����ͼƬ***********/
				if(jpeg_data_ok==1&& buf_flag==1)	 
				{  
 
						len = jpeg_data_len*4;
						len_m = len/1024;
						len_n = len%1024;

						 
						send_logo[8] = len&0x000000ff;
						send_logo[9]=(len&0x0000ff00)>>8;
						send_logo[10]=(len&0x00ff0000)>>16;
						send_logo[11]=(len&0xff000000)>>24;
						send_logo[12]=SaveParaList.size[0]&0x00ff;//width
						send_logo[13]=(SaveParaList.size[0]&0xff00)>>8;
						send_logo[14]=SaveParaList.size[1]&0x00ff;//height
						send_logo[15]=(SaveParaList.size[1]&0xff00)>>8;
						
					 
						
						err = netconn_write(newconn ,(char *)send_logo,16,NETCONN_COPY); 
				
						 if((err==ERR_CLSD)||(err==ERR_RST)) 
						 {
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
 
						 header_len=MT9D111_Greate_Header();
						err = netconn_write(newconn ,(char *)g_header,header_len,NETCONN_COPY); //����tcp_server_sendbuf�е�����
				
						 if((err==ERR_CLSD)||(err==ERR_RST)) 
						 {
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
						
			 
						p=(u8*)jpeg_data_buf;
						for(i = 0;i < len_m;i++)
						{
							err = netconn_write(newconn ,(char *)p+i*1024,1024,NETCONN_COPY); 
							if(err != ERR_OK)
							{
								printf("����ʧ��\r\n");break;
							}
							delay_us(10);
							
						}
						if((err==ERR_CLSD)||(err==ERR_RST)) 
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
						
						err = netconn_write(newconn ,(char *)p+len_m*1024,len_n,NETCONN_COPY); 
						 if((err==ERR_CLSD)||(err==ERR_RST))  
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
 
						err = netconn_write(newconn ,(char *)send_end,2,NETCONN_COPY);  
						 if((err==ERR_CLSD)||(err==ERR_RST))  
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
						 
						 
						
						 if((recv_err = netconn_recv(newconn,&recvbuf)) == ERR_OK)  	//���յ�����
						{		
							OS_ENTER_CRITICAL();  
							memset(tcp_server_recvbuf,0,TCP_SERVER_RX_BUFSIZE);   
							for(q=recvbuf->p;q!=NULL;q=q->next)  
							{
 
								if(q->len > (TCP_SERVER_RX_BUFSIZE-data_len)) memcpy(tcp_server_recvbuf+data_len,q->payload,(TCP_SERVER_RX_BUFSIZE-data_len));//��������
								else memcpy(tcp_server_recvbuf+data_len,q->payload,q->len);
								data_len += q->len;  	
								if(data_len > TCP_SERVER_RX_BUFSIZE) break; //����TCP�ͻ��˽�������,����	
							}
							OS_EXIT_CRITICAL();   
							data_len=0;   	
							printf("%s\r\n",tcp_server_recvbuf);  
							
							PollMessage(tcp_server_recvbuf);//���յ����ݽ��봦��
							err = netconn_write(newconn ,(char *)ValidCmd,ParaLen+3,NETCONN_COPY); //���� ��Ӧ֡ 
							if((err==ERR_CLSD)||(err==ERR_RST)) 
							{
								netconn_close(newconn);
								netconn_delete(newconn);
								printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
								remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
								break;
							 }
							 memset(ValidCmd,0,100); 
							
							
							netbuf_delete(recvbuf);
						}else if(recv_err == ERR_CLSD)   
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("2����:%d.%d.%d.%d�Ͽ��������������\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						}
						 
						
						
					jpeg_data_ok=2;	 
				}	
				
				if(jpeg_data_ok==1&& buf_flag==0)	 
				{  
 
						len = jpeg_data_len*4;
						len_m = len/1024;
						len_n = len%1024;

					 
						send_logo[8] = len&0x000000ff;
						send_logo[9]=(len&0x0000ff00)>>8;
						send_logo[10]=(len&0x00ff0000)>>16;
						send_logo[11]=(len&0xff000000)>>24;
						send_logo[12]=SaveParaList.size[0]&0x00ff;//width
						send_logo[13]=(SaveParaList.size[0]&0xff00)>>8;
						send_logo[14]=SaveParaList.size[1]&0x00ff;//height
						send_logo[15]=(SaveParaList.size[1]&0xff00)>>8;
						
						 
						
						err = netconn_write(newconn ,(char *)send_logo,16,NETCONN_COPY);  
				
						 if((err==ERR_CLSD)||(err==ERR_RST)) 
						 {
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
 
						 header_len=MT9D111_Greate_Header();
						err = netconn_write(newconn ,(char *)g_header,header_len,NETCONN_COPY);  
				
						 if((err==ERR_CLSD)||(err==ERR_RST))//�ر�����,������������ 
						 {
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
 
						p=(u8*)jpeg_data_buf2;
						for(i = 0;i < len_m;i++)
						{
							err = netconn_write(newconn ,(char *)p+i*1024,1024,NETCONN_COPY); 
							if(err != ERR_OK)
							{
								printf("����ʧ��\r\n");break;
							}
							delay_us(10);
							
						}
						if((err==ERR_CLSD)||(err==ERR_RST))  
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
						
						err = netconn_write(newconn ,(char *)p+len_m*1024,len_n,NETCONN_COPY);  
						 if((err==ERR_CLSD)||(err==ERR_RST)) 
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
  
						err = netconn_write(newconn ,(char *)send_end,2,NETCONN_COPY);  
						 if((err==ERR_CLSD)||(err==ERR_RST))  
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
						 
						 
						
						 if((recv_err = netconn_recv(newconn,&recvbuf)) == ERR_OK)  	//���յ�����
						{		
							OS_ENTER_CRITICAL();  
							memset(tcp_server_recvbuf,0,TCP_SERVER_RX_BUFSIZE);  
							for(q=recvbuf->p;q!=NULL;q=q->next)   
							{
								 
								if(q->len > (TCP_SERVER_RX_BUFSIZE-data_len)) memcpy(tcp_server_recvbuf+data_len,q->payload,(TCP_SERVER_RX_BUFSIZE-data_len));//��������
								else memcpy(tcp_server_recvbuf+data_len,q->payload,q->len);
								data_len += q->len;  	
								if(data_len > TCP_SERVER_RX_BUFSIZE) break; //����TCP�ͻ��˽�������,����	
							}
							OS_EXIT_CRITICAL();   
							data_len=0;   	
							printf("%s\r\n",tcp_server_recvbuf);   
							
							
							PollMessage(tcp_server_recvbuf); 
							err = netconn_write(newconn ,(char *)ValidCmd,ParaLen+3,NETCONN_COPY); //���� ��Ӧ֡ 
							if((err==ERR_CLSD)||(err==ERR_RST))  
							{
								netconn_close(newconn);
								netconn_delete(newconn);
								printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
								remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
								break;
							 }
							 memset(ValidCmd,0,100); 
							 
							
							netbuf_delete(recvbuf);
						}else if(recv_err == ERR_CLSD)   
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("2����:%d.%d.%d.%d�Ͽ��������������\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						}
						 
						
						
					jpeg_data_ok=2;	 
				}
				
				
				else
				{
					if((err==ERR_CLSD)||(err==ERR_RST)) 
					{
						netconn_close(newconn);
						netconn_delete(newconn);
						printf("1����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
						remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
						break;
					 }
					if((recv_err = netconn_recv(newconn,&recvbuf)) == ERR_OK)  	//���յ�����
						{		
							OS_ENTER_CRITICAL(); 
							memset(tcp_server_recvbuf,0,TCP_SERVER_RX_BUFSIZE);  
							for(q=recvbuf->p;q!=NULL;q=q->next)   
							{
								 
								if(q->len > (TCP_SERVER_RX_BUFSIZE-data_len)) memcpy(tcp_server_recvbuf+data_len,q->payload,(TCP_SERVER_RX_BUFSIZE-data_len));//��������
								else memcpy(tcp_server_recvbuf+data_len,q->payload,q->len);
								data_len += q->len;  	
								if(data_len > TCP_SERVER_RX_BUFSIZE) break; //����TCP�ͻ��˽�������,����	
							}
							OS_EXIT_CRITICAL();   
							data_len=0;  	
							printf("%s\r\n",tcp_server_recvbuf);  
							
							PollMessage(tcp_server_recvbuf); 
							err = netconn_write(newconn ,(char *)ValidCmd,ParaLen+3,NETCONN_COPY); //���� ��Ӧ֡ 
							if((err==ERR_CLSD)||(err==ERR_RST))//�ر�����,������������ 
							{
								netconn_close(newconn);
								netconn_delete(newconn);
								printf("����:%d.%d.%d.%d�Ͽ�����Ƶ������������\r\n",\
								remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
								break;
							 }
							 memset(ValidCmd,0,100);//���������
							
							
							netbuf_delete(recvbuf);
						}else if(recv_err == ERR_CLSD)  //�ر�����
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("2����:%d.%d.%d.%d�Ͽ��������������\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						}
				}
			
			}
		}
	}
}


//����TCP�������߳�
//����ֵ:0 TCP�����������ɹ�
//		���� TCP����������ʧ��
INT8U tcp_server_init(void)
{
	INT8U res;
	OS_CPU_SR cpu_sr;
	
	OS_ENTER_CRITICAL();	//���ж�
	res = OSTaskCreate(tcp_server_thread,(void*)0,(OS_STK*)&TCPSERVER_TASK_STK[TCPSERVER_STK_SIZE-1],TCPSERVER_PRIO); //����TCP�������߳�
	OS_EXIT_CRITICAL();		//���ж�
	
	return res;
}


