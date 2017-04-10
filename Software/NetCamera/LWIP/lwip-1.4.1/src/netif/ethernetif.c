#include "netif/ethernetif.h" 
#include "lan8720.h"  
#include "lwip_comm.h" 
#include "netif/etharp.h"  
#include "string.h"  

//��ethernetif_init()�������ڳ�ʼ��Ӳ  ��
//netif:�����ṹ��ָ�� 
//����ֵ:ERR_OK,����
//       ����,ʧ��
static err_t low_level_init(struct netif *netif)
{
	netif->hwaddr_len = ETHARP_HWADDR_LEN; //����MAC��ַ����,Ϊ6���ֽ�
	//��ʼ��MAC��ַ,����ʲô��ַ���û��Լ�����,���ǲ����������������豸MAC��ַ�ظ�
	netif->hwaddr[0]=lwipdev.mac[0]; 
	netif->hwaddr[1]=lwipdev.mac[1]; 
	netif->hwaddr[2]=lwipdev.mac[2];
	netif->hwaddr[3]=lwipdev.mac[3];   
	netif->hwaddr[4]=lwipdev.mac[4];
	netif->hwaddr[5]=lwipdev.mac[5];
	netif->mtu=1500; //��������䵥Ԫ,����������㲥��ARP����

	netif->flags = NETIF_FLAG_BROADCAST|NETIF_FLAG_ETHARP|NETIF_FLAG_LINK_UP;
	
    HAL_ETH_DMATxDescListInit(&ETH_Handler,DMATxDscrTab,Tx_Buff,ETH_TXBUFNB);//��ʼ������������
    HAL_ETH_DMARxDescListInit(&ETH_Handler,DMARxDscrTab,Rx_Buff,ETH_RXBUFNB);//��ʼ������������
	HAL_ETH_Start(&ETH_Handler); //����MAC��DMA				
	return ERR_OK;
} 
//���ڷ������ݰ�����ײ㺯��(lwipͨ��netif->linkoutputָ��ú���)
//netif:�����ṹ��ָ��
//p:pbuf���ݽṹ��ָ��
//����ֵ:ERR_OK,��������
//       ERR_MEM,����ʧ��
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    err_t errval;
    struct pbuf *q;
    uint8_t *buffer=(uint8_t *)(ETH_Handler.TxDesc->Buffer1Addr);
    __IO ETH_DMADescTypeDef *DmaTxDesc;
    uint32_t framelength = 0;
    uint32_t bufferoffset = 0;
    uint32_t byteslefttocopy = 0;
    uint32_t payloadoffset = 0;

    DmaTxDesc = ETH_Handler.TxDesc;
    bufferoffset = 0;

    INTX_DISABLE();
    //��pbuf�п���Ҫ���͵�����
    for(q=p;q!=NULL;q=q->next)
    {
        //�жϴ˷����������Ƿ���Ч�����жϴ˷����������Ƿ����̫��DMA����
        if((DmaTxDesc->Status&ETH_DMATXDESC_OWN)!=(uint32_t)RESET)
        {
            errval=ERR_USE;
            goto error;             //������������Ч��������
        }
        byteslefttocopy=q->len;     //Ҫ���͵����ݳ���
        payloadoffset=0;
   
        //��pbuf��Ҫ���͵�����д�뵽��̫�������������У���ʱ������Ҫ���͵����ݿ��ܴ���һ����̫��
        //��������Tx Buffer�����������Ҫ�ֶ�ν����ݿ��������������������
        while((byteslefttocopy+bufferoffset)>ETH_TX_BUF_SIZE )
        {
            //�����ݿ�������̫��������������Tx Buffer��
            memcpy((uint8_t*)((uint8_t*)buffer+bufferoffset),(uint8_t*)((uint8_t*)q->payload+payloadoffset),(ETH_TX_BUF_SIZE-bufferoffset));
            //DmaTxDscָ����һ������������
            DmaTxDesc=(ETH_DMADescTypeDef *)(DmaTxDesc->Buffer2NextDescAddr);
            //����µķ����������Ƿ���Ч
            if((DmaTxDesc->Status&ETH_DMATXDESC_OWN)!=(uint32_t)RESET)
            {
                errval = ERR_USE;
                goto error;     //������������Ч��������
            }
            buffer=(uint8_t *)(DmaTxDesc->Buffer1Addr);   //����buffer��ַ��ָ���µķ�����������Tx Buffer
            byteslefttocopy=byteslefttocopy-(ETH_TX_BUF_SIZE-bufferoffset);
            payloadoffset=payloadoffset+(ETH_TX_BUF_SIZE-bufferoffset);
            framelength=framelength+(ETH_TX_BUF_SIZE-bufferoffset);
            bufferoffset=0;
        }
        //����ʣ�������
        memcpy( (uint8_t*)((uint8_t*)buffer+bufferoffset),(uint8_t*)((uint8_t*)q->payload+payloadoffset),byteslefttocopy );
        bufferoffset=bufferoffset+byteslefttocopy;
        framelength=framelength+byteslefttocopy;
    }
    //������Ҫ���͵����ݶ��Ž�������������Tx Buffer�Ժ�Ϳɷ��ʹ�֡��
    HAL_ETH_TransmitFrame(&ETH_Handler,framelength);
    errval = ERR_OK;
error:    
    //���ͻ������������磬һ�����ͻ�������������TxDMA��������״̬
    if((ETH_Handler.Instance->DMASR&ETH_DMASR_TUS)!=(uint32_t)RESET)
    {
        //��������־
        ETH_Handler.Instance->DMASR = ETH_DMASR_TUS;
        //������֡�г�����������ʱ��TxDMA�������ʱ����Ҫ��DMATPDR�Ĵ���
        //���д��һ��ֵ�����份�ѣ��˴�����д0
        ETH_Handler.Instance->DMATPDR=0;
    }
    INTX_ENABLE();
    return errval;
}  
///���ڽ������ݰ�����ײ㺯��
//neitif:�����ṹ��ָ��
//����ֵ:pbuf���ݽṹ��ָ��
static struct pbuf * low_level_input(struct netif *netif)
{  
	struct pbuf *p = NULL;
    struct pbuf *q;
    uint16_t len;
    uint8_t *buffer;
    __IO ETH_DMADescTypeDef *dmarxdesc;
    uint32_t bufferoffset=0;
    uint32_t payloadoffset=0;
    uint32_t byteslefttocopy=0;
    uint32_t i=0;
  
    if(HAL_ETH_GetReceivedFrame(&ETH_Handler)!=HAL_OK)  //�ж��Ƿ���յ�����
    return NULL;
    
    INTX_DISABLE();
    len=ETH_Handler.RxFrameInfos.length;                //��ȡ���յ�����̫��֡����
    buffer=(uint8_t *)ETH_Handler.RxFrameInfos.buffer;  //��ȡ���յ�����̫��֡������buffer
  
    if(len>0) p=pbuf_alloc(PBUF_RAW,len,PBUF_POOL);     //����pbuf
    if(p!=NULL)                                        //pbuf����ɹ�
    {
        dmarxdesc=ETH_Handler.RxFrameInfos.FSRxDesc;    //��ȡ���������������еĵ�һ��������
        bufferoffset = 0;
        for(q=p;q!=NULL;q=q->next)                      
        {
            byteslefttocopy=q->len;                  
            payloadoffset=0;
            //��������������Rx Buffer�����ݿ�����pbuf��
            while((byteslefttocopy+bufferoffset)>ETH_RX_BUF_SIZE )
            {
                //�����ݿ�����pbuf��
                memcpy((uint8_t*)((uint8_t*)q->payload+payloadoffset),(uint8_t*)((uint8_t*)buffer+bufferoffset),(ETH_RX_BUF_SIZE-bufferoffset));
                 //dmarxdesc����һ������������
                dmarxdesc=(ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
                //����buffer��ַ��ָ���µĽ�����������Rx Buffer
                buffer=(uint8_t *)(dmarxdesc->Buffer1Addr);
 
                byteslefttocopy=byteslefttocopy-(ETH_RX_BUF_SIZE-bufferoffset);
                payloadoffset=payloadoffset+(ETH_RX_BUF_SIZE-bufferoffset);
                bufferoffset=0;
            }
            //����ʣ�������
            memcpy((uint8_t*)((uint8_t*)q->payload+payloadoffset),(uint8_t*)((uint8_t*)buffer+bufferoffset),byteslefttocopy);
            bufferoffset=bufferoffset+byteslefttocopy;
        }
    }    
    //�ͷ�DMA������
    dmarxdesc=ETH_Handler.RxFrameInfos.FSRxDesc;
    for(i=0;i<ETH_Handler.RxFrameInfos.SegCount; i++)
    {  
        dmarxdesc->Status|=ETH_DMARXDESC_OWN;       //�����������DMA����
        dmarxdesc=(ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
    }
    ETH_Handler.RxFrameInfos.SegCount =0;           //����μ�����
    if((ETH_Handler.Instance->DMASR&ETH_DMASR_RBUS)!=(uint32_t)RESET)  //���ջ�����������
    {
        //������ջ����������ñ�־
        ETH_Handler.Instance->DMASR = ETH_DMASR_RBUS;
        //�����ջ����������õ�ʱ��RxDMA���ȥ����״̬��ͨ����DMARPDRд������һ��ֵ������Rx DMA
        ETH_Handler.Instance->DMARPDR=0;
    }
    INTX_ENABLE();
    return p;
}
//������������(lwipֱ�ӵ���) 
//netif:�����ṹ��ָ��
//����ֵ:ERR_OK,��������
//       ERR_MEM,����ʧ��
err_t ethernetif_input(struct netif *netif)
{
	err_t err;
	struct pbuf *p;
	p=low_level_input(netif);   //����low_level_input������������
	if(p==NULL) return ERR_MEM;
	err=netif->input(p, netif); //����netif�ṹ���е�input�ֶ�(һ������)���������ݰ�
	if(err!=ERR_OK)
	{
		LWIP_DEBUGF(NETIF_DEBUG,("ethernetif_input: IP input error\n"));
		pbuf_free(p);
		p = NULL;
	} 
	return err;
} 
//ʹ��low_level_init()��������ʼ������
//netif:�����ṹ��ָ��
//����ֵ:ERR_OK,����
//       ����,ʧ��
err_t ethernetif_init(struct netif *netif)
{
	LWIP_ASSERT("netif!=NULL",(netif!=NULL));
#if LWIP_NETIF_HOSTNAME			//LWIP_NETIF_HOSTNAME 
	netif->hostname="lwip";  	//��ʼ������
#endif 
	netif->name[0]=IFNAME0; 	//��ʼ������netif��name�ֶ�
	netif->name[1]=IFNAME1; 	//���ļ��ⶨ�����ﲻ�ù��ľ���ֵ
	netif->output=etharp_output;//IP�㷢�����ݰ�����
	netif->linkoutput=low_level_output;//ARPģ�鷢�����ݰ�����
	low_level_init(netif); 		//�ײ�Ӳ����ʼ������
	return ERR_OK;
}














