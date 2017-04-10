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



u16 TCP_SERVER_PORT;//服务器端口
//参数保存表初始化
extern SavePara_TypeDef SaveParaList;
extern u8 ParaLen;//参数区长度
extern u8 ValidCmd[100];//响应帧发送数组 


//参数保存表初始化
SavePara_TypeDef DefaultList={

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
	
};
/*摄像头相关*/

extern char g_header[1024]; 

u8 ovx_mode=0;							 
u16 curline=0;							 

#define jpeg_buf_size   63*1024		 
  
u32 jpeg_data_buf[jpeg_buf_size] __attribute__((at(0XC0000000))); 
u32 jpeg_data_buf2[jpeg_buf_size] __attribute__((at(0XC0040000)));

u8 buf_flag=0;//在中断中为1则配置写入BUF1     0则配置写入BUF2
							//在主循环中1作为发送buf1       0则发送BUF2

volatile u32 jpeg_data_len=0; 			//buf中的JPEG有效数据长度 
volatile u8 jpeg_data_ok=0;				//JPEG数据采集完成标志 
										//0,数据没有采集完;
										//1,数据采集完了,但是还没处理;
										//2,数据已经处理完成了,可以开始下一帧接收
										
 
static  char send_logo[16]="CORENCE";  
static char send_end[2]={0xff,0xd9};



#define width 1024
#define height 768										

 
//处理JPEG数据
  
void jpeg_data_process(void)
{
 
    curline=0;

		if(jpeg_data_ok!=1)	//jpeg数据还未采集完?
		{
			__HAL_DMA_DISABLE(&DMADMCI_Handler); 
			jpeg_data_len=jpeg_buf_size-__HAL_DMA_GET_COUNTER(&DMADMCI_Handler);//得到剩余数据长度	   
	
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

//JPEG测试
 
void jpeg_test(void)
{

	
	MT9D111_Jpeg_Config();	//JPEG模式
	DCMI_Init();			//DCMI配置
	DCMI_DMA_Init((u32)&jpeg_data_buf,0,jpeg_buf_size,DMA_MDATAALIGN_WORD,DMA_MINC_ENABLE);  //单缓冲
	
	
	DCMI_Start(); 		//启动传输
  
}
 




/*网络相关*/


u8 tcp_server_recvbuf[TCP_SERVER_RX_BUFSIZE];	//TCP客户端接收数据缓冲区
u8 tcp_server_sendbuf[1000]="Apollo STM32F4/F7 NETCONN TCP Server send data\r\n";	
u8 tcp_server_flag;								//TCP服务器数据发送标志位

//TCP客户端任务
#define TCPSERVER_PRIO		6
//任务堆栈大小
#define TCPSERVER_STK_SIZE	1024
//任务堆栈
OS_STK TCPSERVER_TASK_STK[TCPSERVER_STK_SIZE];

//tcp服务器任务
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
	netconn_bind(conn,IP_ADDR_ANY,TCP_SERVER_PORT);  //绑定端口 8号端口
	netconn_listen(conn);  		//进入监听模式
	conn->recv_timeout = 10;  	//禁止阻塞线程 等待10ms
	
		//摄像头相关初始化
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

			netconn_getaddr(newconn,&ipaddr,&port,0); //获取远端IP地址和端口号
			
			remot_addr[3] = (uint8_t)(ipaddr.addr >> 24); 
			remot_addr[2] = (uint8_t)(ipaddr.addr>> 16);
			remot_addr[1] = (uint8_t)(ipaddr.addr >> 8);
			remot_addr[0] = (uint8_t)(ipaddr.addr);
			printf("主机%d.%d.%d.%d连接上服务器,主机端口号为:%d\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3],port);
			
			while(1)
			{
				/*********发送图片***********/
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
							printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
 
						 header_len=MT9D111_Greate_Header();
						err = netconn_write(newconn ,(char *)g_header,header_len,NETCONN_COPY); //发送tcp_server_sendbuf中的数据
				
						 if((err==ERR_CLSD)||(err==ERR_RST)) 
						 {
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
						
			 
						p=(u8*)jpeg_data_buf;
						for(i = 0;i < len_m;i++)
						{
							err = netconn_write(newconn ,(char *)p+i*1024,1024,NETCONN_COPY); 
							if(err != ERR_OK)
							{
								printf("发送失败\r\n");break;
							}
							delay_us(10);
							
						}
						if((err==ERR_CLSD)||(err==ERR_RST)) 
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
						
						err = netconn_write(newconn ,(char *)p+len_m*1024,len_n,NETCONN_COPY); 
						 if((err==ERR_CLSD)||(err==ERR_RST))  
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
 
						err = netconn_write(newconn ,(char *)send_end,2,NETCONN_COPY);  
						 if((err==ERR_CLSD)||(err==ERR_RST))  
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
						 
						 
						
						 if((recv_err = netconn_recv(newconn,&recvbuf)) == ERR_OK)  	//接收到数据
						{		
							OS_ENTER_CRITICAL();  
							memset(tcp_server_recvbuf,0,TCP_SERVER_RX_BUFSIZE);   
							for(q=recvbuf->p;q!=NULL;q=q->next)  
							{
 
								if(q->len > (TCP_SERVER_RX_BUFSIZE-data_len)) memcpy(tcp_server_recvbuf+data_len,q->payload,(TCP_SERVER_RX_BUFSIZE-data_len));//拷贝数据
								else memcpy(tcp_server_recvbuf+data_len,q->payload,q->len);
								data_len += q->len;  	
								if(data_len > TCP_SERVER_RX_BUFSIZE) break; //超出TCP客户端接收数组,跳出	
							}
							OS_EXIT_CRITICAL();   
							data_len=0;   	
							printf("%s\r\n",tcp_server_recvbuf);  
							
							PollMessage(tcp_server_recvbuf);//接收到数据进入处理
							err = netconn_write(newconn ,(char *)ValidCmd,ParaLen+3,NETCONN_COPY); //发送 响应帧 
							if((err==ERR_CLSD)||(err==ERR_RST)) 
							{
								netconn_close(newconn);
								netconn_delete(newconn);
								printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
								remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
								break;
							 }
							 memset(ValidCmd,0,100); 
							
							
							netbuf_delete(recvbuf);
						}else if(recv_err == ERR_CLSD)   
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("2主机:%d.%d.%d.%d断开与服务器的连接\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
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
							printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
 
						 header_len=MT9D111_Greate_Header();
						err = netconn_write(newconn ,(char *)g_header,header_len,NETCONN_COPY);  
				
						 if((err==ERR_CLSD)||(err==ERR_RST))//关闭连接,或者重启网络 
						 {
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
 
						p=(u8*)jpeg_data_buf2;
						for(i = 0;i < len_m;i++)
						{
							err = netconn_write(newconn ,(char *)p+i*1024,1024,NETCONN_COPY); 
							if(err != ERR_OK)
							{
								printf("发送失败\r\n");break;
							}
							delay_us(10);
							
						}
						if((err==ERR_CLSD)||(err==ERR_RST))  
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
						
						err = netconn_write(newconn ,(char *)p+len_m*1024,len_n,NETCONN_COPY);  
						 if((err==ERR_CLSD)||(err==ERR_RST)) 
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
  
						err = netconn_write(newconn ,(char *)send_end,2,NETCONN_COPY);  
						 if((err==ERR_CLSD)||(err==ERR_RST))  
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
							remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						 }
						 
						 
						
						 if((recv_err = netconn_recv(newconn,&recvbuf)) == ERR_OK)  	//接收到数据
						{		
							OS_ENTER_CRITICAL();  
							memset(tcp_server_recvbuf,0,TCP_SERVER_RX_BUFSIZE);  
							for(q=recvbuf->p;q!=NULL;q=q->next)   
							{
								 
								if(q->len > (TCP_SERVER_RX_BUFSIZE-data_len)) memcpy(tcp_server_recvbuf+data_len,q->payload,(TCP_SERVER_RX_BUFSIZE-data_len));//拷贝数据
								else memcpy(tcp_server_recvbuf+data_len,q->payload,q->len);
								data_len += q->len;  	
								if(data_len > TCP_SERVER_RX_BUFSIZE) break; //超出TCP客户端接收数组,跳出	
							}
							OS_EXIT_CRITICAL();   
							data_len=0;   	
							printf("%s\r\n",tcp_server_recvbuf);   
							
							
							PollMessage(tcp_server_recvbuf); 
							err = netconn_write(newconn ,(char *)ValidCmd,ParaLen+3,NETCONN_COPY); //发送 响应帧 
							if((err==ERR_CLSD)||(err==ERR_RST))  
							{
								netconn_close(newconn);
								netconn_delete(newconn);
								printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
								remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
								break;
							 }
							 memset(ValidCmd,0,100); 
							 
							
							netbuf_delete(recvbuf);
						}else if(recv_err == ERR_CLSD)   
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("2主机:%d.%d.%d.%d断开与服务器的连接\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
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
						printf("1主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
						remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
						break;
					 }
					if((recv_err = netconn_recv(newconn,&recvbuf)) == ERR_OK)  	//接收到数据
						{		
							OS_ENTER_CRITICAL(); 
							memset(tcp_server_recvbuf,0,TCP_SERVER_RX_BUFSIZE);  
							for(q=recvbuf->p;q!=NULL;q=q->next)   
							{
								 
								if(q->len > (TCP_SERVER_RX_BUFSIZE-data_len)) memcpy(tcp_server_recvbuf+data_len,q->payload,(TCP_SERVER_RX_BUFSIZE-data_len));//拷贝数据
								else memcpy(tcp_server_recvbuf+data_len,q->payload,q->len);
								data_len += q->len;  	
								if(data_len > TCP_SERVER_RX_BUFSIZE) break; //超出TCP客户端接收数组,跳出	
							}
							OS_EXIT_CRITICAL();   
							data_len=0;  	
							printf("%s\r\n",tcp_server_recvbuf);  
							
							PollMessage(tcp_server_recvbuf); 
							err = netconn_write(newconn ,(char *)ValidCmd,ParaLen+3,NETCONN_COPY); //发送 响应帧 
							if((err==ERR_CLSD)||(err==ERR_RST))//关闭连接,或者重启网络 
							{
								netconn_close(newconn);
								netconn_delete(newconn);
								printf("主机:%d.%d.%d.%d断开与视频服务器的连接\r\n",\
								remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
								break;
							 }
							 memset(ValidCmd,0,100);//发送完清空
							
							
							netbuf_delete(recvbuf);
						}else if(recv_err == ERR_CLSD)  //关闭连接
						{
							netconn_close(newconn);
							netconn_delete(newconn);
							printf("2主机:%d.%d.%d.%d断开与服务器的连接\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
							break;
						}
				}
			
			}
		}
	}
}


//创建TCP服务器线程
//返回值:0 TCP服务器创建成功
//		其他 TCP服务器创建失败
INT8U tcp_server_init(void)
{
	INT8U res;
	OS_CPU_SR cpu_sr;
	
	OS_ENTER_CRITICAL();	//关中断
	res = OSTaskCreate(tcp_server_thread,(void*)0,(OS_STK*)&TCPSERVER_TASK_STK[TCPSERVER_STK_SIZE-1],TCPSERVER_PRIO); //创建TCP服务器线程
	OS_EXIT_CRITICAL();		//开中断
	
	return res;
}


