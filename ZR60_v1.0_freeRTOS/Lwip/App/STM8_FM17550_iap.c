/**
  ******************************************************************************
  * @file    STM8_FM17550_iap.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    31-July-2013 
  * @brief   tcp echoclient application using LwIP RAW API
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "STM8_FM17550_iap.h"

#if LWIP_TCP
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static struct ip_addr LocalIPaddr;
static struct ip_addr DestIPaddr;
//static char   Recev_buf[1460];
//static stm8_fm17550_TxTempBufStruct   TxBuf;
static stm8_fm17550_IapFlagStruct      IapFlag;
static char   Se_u_Txbuf[STM8_FM17550_TXBUF] = {0};

struct tcp_pcb *stm8_fm17550_pcb = NULL;

static u8_t   Sehttp_client_u_cnntSt = STM8_FM17550_IDLE;//tcp连接状态:0--空闲
static u8_t   Se_u_BusyFlg = 0U;//忙标志：0--空闲，1--忙
static u16_t  Se_w_TxTimer = 0U;//
static u16_t  Se_w_DownloadTimer = 0U;//
static u16_t  Se_w_ConntTimer = 0U;//建立连接计时器
static u8_t   Se_u_TimeoutCnt = 0U;//下载连接超时计数
static u32_t  Se_dw_byteCnt = 0U;//下载字节数计数

/* ECHO protocol states */
enum stm8_fm17550_states
{
  ES_NOT_CONNECTED = 0,
  ES_CONNECTED,
  ES_RECEIVED,
  ES_CLOSING,
};

/* structure to be passed as argument to the tcp callbacks */
struct stm8_fm17550
{
  enum stm8_fm17550_states state; /* connection status */
  struct tcp_pcb *pcb;          /* pointer on the current tcp_pcb */
  struct pbuf *p_tx;            /* pointer on pbuf to be transmitted */
};

struct stm8_fm17550 * stm8_fm17550_es = NULL;

static stm8_fm17550_UpdataInfoStruct UpdataInfo;

static uint8 Se_u_EnterBL[7U] = {0x20,0x00,0x30,0x01,0x00,0xCE,0x03};

static uint8 Se_u_FirmwareRefreshSt = 0U;//固件刷新状态：0--空闲；1--刷新中；2--刷新完成
static stm8_fm17550_Frame_Tx Frame_Tx;
/* Private function prototypes -----------------------------------------------*/
static err_t http_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void http_client_connection_close(struct tcp_pcb *tpcb, struct stm8_fm17550 * es);
static err_t http_client_poll(void *arg, struct tcp_pcb *tpcb);
static err_t http_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static void http_client_PostRequest(char* postAddr,char* Le_u_in,u16_t * Le_u_len,char* Le_u_out,u8_t Le_u_ConnectionValue);
static void http_client_MacRecombine(char* Le_u_mac);
static void http_ConnectErr(void *arg,err_t err);

static char stm8_fm17550_iap_connect(void);
static void stm8_fm17550_iap_disconnect(void);
static void stm8_fm17550_iap_sendMsg(char* data,u16_t len);

static void http_EraseFlash(uint32_t FLASH_Sector);
static void http_wrFlash(uint32_t FLASH_addr,uint8_t* Le_u_Dt,uint16_t Le_w_lng);
static void http_rdFlash(uint32_t FLASH_addr,uint8_t* Le_u_Dt,uint16_t Le_w_lng);
static uint8 Gethttpclient_dw_XOR(uint8* Le_u_Dt,uint16 Le_w_Lng);
/* Private functions ---------------------------------------------------------*/


/******************************************************
*函数名：stm8_fm17550_iap_parameter

*形  参：

*返回值：

*描  述：初始化

*备  注：
******************************************************/
void stm8_fm17550_iap_parameter(void)
{
	uint8 Le_u_i;
	IapFlag.TskFlag = 0U;//任务标志
	IapFlag.downloadFlag = STM8_FM17550_DOWNLOAD_IDLE;//
	Frame_Tx.Frame.Header = 0x20;//帧头
	Frame_Tx.Frame.Seqnr = 0x00;//数据帧包序号
	Frame_Tx.Frame.Cmd = 0x00;//数据长度,0－数据，1－终止
	Frame_Tx.Frame.Lng = 64U;//数据长度
	for(Le_u_i = 0U;Le_u_i < 64U;Le_u_i++)
	{
		Frame_Tx.Frame.Data[Le_u_i] = 0x00;//数据
	}
	Frame_Tx.Frame.CC = 0U;//校验和
	Frame_Tx.Frame.ETX = 0x03;//结束字符
	//http_EraseFlash(FLASH_Sector_3);
}

/******************************************************
*函数名：stm8_fm17550_iap_MainFunction

*形  参：

*返回值：

*描  述：消息记录主任务,5ms周期调用

*备  注：
******************************************************/
void stm8_fm17550_iap_MainFunction(void)
{
	uint8 Le_u_i;
	uint16 Le_w_lng;
	uint32 Le_dw_CheckSum;
	if(0 == GET_PHY_LINK_STATUS())/* Get Ethernet link status*/
	{
		IapFlag.TskFlag = 0U;//任务标志
		IapFlag.downloadFlag = STM8_FM17550_DOWNLOAD_IDLE;//
		Se_u_BusyFlg = 0U;
		Se_w_ConntTimer = 0U;
		Se_w_DownloadTimer = 0U;
		stm8_fm17550_iap_disconnect();
		Sehttp_client_u_cnntSt = STM8_FM17550_IDLE;
		return;
	}
	
	if(1U == GetZR60Ctrl_u_CardReaderIapSt())//升级固件标志有效
	{
		ClrZR60Ctrl_CardReaderIapSt();
		IapFlag.TskFlag = 1U;
		IapFlag.downloadFlag = STM8_FM17550_DOWNLOAD_IDLE;
		http_EraseFlash(FLASH_Sector_3);//擦除固件存储区
		//UpdataInfo.IapFlag = 1U;
		//http_wrFlash(HTTP_CLIENT_UPLOAD_INFO_SEC,(uint8_t*)(&UpdataInfo.Xor),sizeof(UpdataInfo));
	}
	
	/**********建立连接**********/
	if((DHCP_ADDRESS_ASSIGNED == STM8_FM17550_DHCP_STATE) && \
			(STM8_FM17550_IDLE == Sehttp_client_u_cnntSt))
	{	
		if(IapFlag.TskFlag == 1U)
		{
			if(1U != GetdnsAnalysis_u_ipValidity(http_download))
			{
				return;
			}
			GetdnsAnalysis_ipAddr(http_download,&DestIPaddr);
			if(ERR_OK == stm8_fm17550_iap_connect())
			{
				USART_PRINTF_S("建立下载连接...");
				IapFlag.TskFlag = 0U;
				Se_u_BusyFlg = 1U;
				Se_w_ConntTimer = 0U;
				Sehttp_client_u_cnntSt = STM8_FM17550_WAIT;
			}
		}
	}
	if(STM8_FM17550_WAIT == Sehttp_client_u_cnntSt)
	{
		Se_w_ConntTimer++;
		if(Se_w_ConntTimer <= (600U/(STM8_IAP_SCHEDULING_CYCLE/5U)))
		{
			if((stm8_fm17550_es->state == ES_CONNECTED) &&(ESTABLISHED == stm8_fm17550_pcb->state))
			{//连接建立成功
				USART_PRINTF_S("下载连接建立成功!\n");
				Se_w_ConntTimer = 0U;
				Sehttp_client_u_cnntSt = STM8_FM17550_LOADPCKT;//
			}
		}
		else
		{//连接建立失败
			USART_PRINTF_S("建立下载连接超时");
			Se_u_TimeoutCnt++;
			if(Se_u_TimeoutCnt >= 3U)
			{
				Se_u_TimeoutCnt = 0U;
				SetdnsAnalysis_ipUpdate(http_download);//更新ip
			}
			Sehttp_client_u_cnntSt = STM8_FM17550_DISCNNT;
		}	
	}
	
	/**********下载数据**********/
	if(STM8_FM17550_LOADPCKT == Sehttp_client_u_cnntSt)
	{
		if(IapFlag.downloadFlag == STM8_FM17550_DOWNLOAD_IDLE)
		{
			IapFlag.downloadFlag = STM8_FM17550_DOWNLOAD_UNDERWAY;
			USART_PRINTF_S("发送下载请求");
			memset(Se_u_Txbuf,0,sizeof(Se_u_Txbuf));//清0
			Le_w_lng = 0U;
			Se_dw_byteCnt = 0U;
			Se_w_DownloadTimer = 0U;//清下载超时计时器
			http_client_PostRequest("GET /CR_stm8_fm17550.bin HTTP/1.1\n",NULL,&Le_w_lng,Se_u_Txbuf,1U);
			stm8_fm17550_iap_sendMsg(Se_u_Txbuf,Le_w_lng);//数据发送	
		}
		
		Se_w_DownloadTimer++;
		if(Se_w_DownloadTimer >= STM8_FM17550_DOWNLOAD_TIMEOUT)//下载超时
		{
			USART_PRINTF_S("下载超时");	
			Se_w_DownloadTimer = 0U;
			Sehttp_client_u_cnntSt = STM8_FM17550_DISCNNT;//断开连接
		}
		else
		{
			if(IapFlag.downloadFlag == STM8_FM17550_DOWNLOAD_FINISH)//下载完成
			{
				Se_w_DownloadTimer = 0U;
				Sehttp_client_u_cnntSt = STM8_FM17550_DISCNNT;//断开连接
			}
		}
	}
	
	/**********断开连接**********/
	if(STM8_FM17550_DISCNNT == Sehttp_client_u_cnntSt)
	{
		stm8_fm17550_iap_disconnect();
		USART_PRINTF_S("断开连接...");
		Se_u_BusyFlg = 0U;
		Se_w_DownloadTimer = 0U;
		Sehttp_client_u_cnntSt = STM8_FM17550_IDLE;
	}
	
	/***********下载完成，刷新读卡器模块固件*********/
	if((IapFlag.downloadFlag == STM8_FM17550_DOWNLOAD_FINISH) && \
			stm8_fm17550_PerformCondition)
	{
		switch(Se_u_FirmwareRefreshSt)
		{
			case 0:
			{
				Le_dw_CheckSum = ((*((__IO uint8*)UpdataInfo.DtFirstAddr + UpdataInfo.Lng -4)) << 24) & 0xff000000;/*下载区数据校验和,下载的二进制文件的最后4字节为校验和*/
				Le_dw_CheckSum = Le_dw_CheckSum | ((*((__IO uint8*)UpdataInfo.DtFirstAddr + UpdataInfo.Lng -3)) << 16);/*下载区数据校验和*/
				Le_dw_CheckSum = Le_dw_CheckSum | ((*((__IO uint8*)UpdataInfo.DtFirstAddr + UpdataInfo.Lng -2)) << 8);/*下载区数据校验和*/
				Le_dw_CheckSum |= *((__IO uint8*)UpdataInfo.DtFirstAddr + UpdataInfo.Lng -1);/*下载区数据校验和*/
				//Le_dw_CheckSum = crc32(0xFFFFFFFF,(__IO uint8*)UpdataInfo.DtFirstAddr,(UpdataInfo.Lng -4));//根据下载区数据计算得到的校验和		
				if(Le_dw_CheckSum == crc32(0xFFFFFFFF,(__IO uint8*)UpdataInfo.DtFirstAddr,(UpdataInfo.Lng -4)))//crc32校验通过
				{
					USART_PRINTF_S("crc32校验通过，设置读卡器进入bootloader模式");
					SetUartCmn_TxMsg(Se_u_EnterBL,7);//设置读卡器进入bootloader模式
					Se_w_ConntTimer = 0U;
					IapFlag.EchoFlag = 0U;
					IapFlag.TxBusyFlag = 0U;
					IapFlag.SlaveRxSt = 0U;
					UpdataInfo.Cnt = 0U;
					UpdataInfo.CheckSum = 0U;
					Frame_Tx.Frame.Cmd = 0x00;
					UpdataInfo.Lng = UpdataInfo.Lng -4;//去掉4字节校验位后有效数据长度
					//SetUSART_BaudRate(UART3_USART,9600);//设置波特率9600
					Se_u_FirmwareRefreshSt = 1U;
				}
				else
				{
					USART_PRINTF_S("crc32校验未通过");
					IapFlag.downloadFlag = STM8_FM17550_DOWNLOAD_IDLE;
				}
			}
			break;
			case 1:
			{
				if(Se_w_ConntTimer >= (200U/(STM8_IAP_SCHEDULING_CYCLE/5U)))
				{
					//IapFlag.EchoFlag = 1U;
					if(IapFlag.TxBusyFlag == 0U)
					{//通信线空闲
						if(IapFlag.SlaveRxSt == 0U)
						{//从机正常接收数据
							if(Frame_Tx.Frame.Cmd == 0x32)
							{//表明在线刷新完成，退出在线刷新		
								USART_PRINTF_S("在线刷新完成，退出在线刷新");
								Se_u_FirmwareRefreshSt = 0U;
								//SetUSART_BaudRate(UART3_USART,115200);//设置波特率115200
								IapFlag.downloadFlag = STM8_FM17550_DOWNLOAD_IDLE;
								return;
							}
							
							memset(Frame_Tx.Frame.Data,0,64);//清0
							if(UpdataInfo.Cnt < UpdataInfo.Lng)
							{/* 拷贝升级文件数据发送给从机 */
								for(Le_u_i = 0U;Le_u_i < 64U;Le_u_i++)
								{
									Frame_Tx.Frame.Data[Le_u_i] = *(__IO uint8*)UpdataInfo.DtFirstAddr;
									UpdataInfo.CheckSum += Frame_Tx.Frame.Data[Le_u_i];
									UpdataInfo.DtFirstAddr++;
									UpdataInfo.Cnt++;
									if(UpdataInfo.Cnt == UpdataInfo.Lng)
									{
										break;
									}
								}
								Frame_Tx.Frame.Seqnr = 0xFF;
								Frame_Tx.Frame.Cmd = 0x31;
							}
							else
							{//发送校验和等信息
								USART_PRINTF_S("发送校验和等信息");
								Frame_Tx.Frame.Seqnr = 0xFF;
								Frame_Tx.Frame.Cmd = 0x32;
								Frame_Tx.Frame.Data[0] = (char)(UpdataInfo.Lng >> 24);//此处注意stm8s高字节在前
								Frame_Tx.Frame.Data[1] = (char)(UpdataInfo.Lng >> 16);
								Frame_Tx.Frame.Data[2] = (char)(UpdataInfo.Lng >> 8);
								Frame_Tx.Frame.Data[3] = (char)UpdataInfo.Lng;
								Frame_Tx.Frame.Data[4] = (char)(UpdataInfo.CheckSum >> 24);
								Frame_Tx.Frame.Data[5] = (char)(UpdataInfo.CheckSum >> 16);
								Frame_Tx.Frame.Data[6] = (char)(UpdataInfo.CheckSum >> 8);
								Frame_Tx.Frame.Data[7] = (char)UpdataInfo.CheckSum;
							}
							Frame_Tx.Frame.CC = Gethttpclient_dw_XOR(&Frame_Tx.Frame.Seqnr,67);
							SetUartCmn_TxMsg(Frame_Tx.FrmTx,70U);//发送升级文件数据
							Se_w_TxTimer = 0U;
							IapFlag.TxBusyFlag = 1U;
						}
						else
						{//数据传输过程中出现误码，重新发送升级文件
							USART_PRINTF_S("数据传输过程中出现误码，重新发送升级文件");
							Se_w_ConntTimer = 0U;
							IapFlag.EchoFlag = 0U;
							IapFlag.TxBusyFlag = 0U;
							IapFlag.SlaveRxSt = 0U;
							UpdataInfo.Cnt = 0U;
							UpdataInfo.CheckSum = 0U;
							Frame_Tx.Frame.Cmd = 0x00;
							UpdataInfo.DtFirstAddr = STM8_FM17550_CODE_SEC;/*数据段首地址*/
						}
					}
					else
					{
						Se_w_TxTimer++;
						if(Se_w_TxTimer >= (400U/(STM8_IAP_SCHEDULING_CYCLE/5U)))
						{//发送超时，退出在线刷新
							USART_PRINTF_S("发送超时，退出在线刷新");
							Se_u_FirmwareRefreshSt = 0U;
							IapFlag.downloadFlag = STM8_FM17550_DOWNLOAD_IDLE;
						}
					}				
				}
				else
				{
					Se_w_ConntTimer++;
				}
			}
			break;
			default:
			break;
		}
	}
}

/* 在线升级状态 */
uint8_t GetStm8_fm17550_iapIdle(void)
{
	return ((Se_u_BusyFlg == 0) && (IapFlag.downloadFlag == STM8_FM17550_DOWNLOAD_IDLE));
}

/**
* @brief  Connects to the TCP echo server
* @param  None
* @retval None
*/
static char stm8_fm17550_iap_connect(void)
{
	s8_t ret;
	/* create new tcp pcb */
	stm8_fm17550_pcb = tcp_new();
	if (stm8_fm17550_pcb != NULL)
	{
		LocalIPaddr.addr = IPaddress;
		ret = tcp_bind(stm8_fm17550_pcb, &LocalIPaddr, 0); 
		USART_PRINTF_D("tcp_bind ret is: %d\n",ret);
		/* connect to destination address/port */
		ret = tcp_connect(stm8_fm17550_pcb,&DestIPaddr,STM8_FM17550_NNT_REMOTEPORT,http_client_connected);
		USART_PRINTF_D("tcp_connect ret is: %d\n",ret);

		if(ERR_OK != ret)
		{
			memp_free(MEMP_TCP_PCB, stm8_fm17550_pcb);
			stm8_fm17550_pcb = NULL;
		}
		else
		{
			tcp_err(stm8_fm17550_pcb,http_ConnectErr); 
		}
	}
	else
	{
		ret = ERR_MEM;
		/* deallocate the pcb */
		USART_PRINTF_S("下载连接tcp控制块分配失败");
	}
	return ret;
}

/*
	错误回调函数
*/
static void http_ConnectErr(void *arg,err_t err)
{  
	USART_PRINTF_S("下载连接err处理");
    switch(err)  
    {   
        case ERR_MEM:                                            /* Out of memory error.     */  
            USART_PRINTF_S("\r\n ERR_MEM   \r\n");  
        break;    
        case ERR_BUF:                                            /* Buffer error.            */  
            USART_PRINTF_S("\r\n ERR_BUF   \r\n");  
        break;  
        case  ERR_TIMEOUT:                                       /* Timeout.                 */  
            USART_PRINTF_S("\r\n ERR_TIMEOUT   \r\n");  
        break;  
        case ERR_RTE:                                            /* Routing problem.         */        
			USART_PRINTF_S("\r\n ERR_RTE   \r\n");  
        break;  
		case ERR_ISCONN:                                          /* Already connected.       */  
			USART_PRINTF_S("\r\n ERR_ISCONN   \r\n");  
        break;  
        case ERR_ABRT:                                           /* Connection aborted.      */  
            USART_PRINTF_S("\r\n ERR_ABRT   \r\n");  
			stm8_fm17550_pcb = NULL;
        break;  
        case ERR_RST:                                            /* Connection reset.        */       
            USART_PRINTF_S("\r\n ERR_RST   \r\n");  
			stm8_fm17550_pcb = NULL;
        break;  
        case ERR_CONN:                                           /* Not connected.           */  
			USART_PRINTF_S("\r\n ERR_CONN   \r\n");  
            break;  
        case ERR_CLSD:                                           /* Connection closed.       */  
            USART_PRINTF_S("\r\n ERR_CLSD   \r\n");  
			stm8_fm17550_pcb = NULL;
        break;  
        case ERR_VAL:                                            /* Illegal value.           */  
			USART_PRINTF_S("\r\n ERR_VAL   \r\n");  
			return;  
        case ERR_ARG:                                            /* Illegal argument.        */  
            USART_PRINTF_S("\r\n ERR_ARG   \r\n");  
			return;  
        case ERR_USE:                                            /* Address in use.          */  
			USART_PRINTF_S("\r\n ERR_USE   \r\n");  
			return;   
        case ERR_IF:                                             /* Low-level netif error    */  
            USART_PRINTF_S("\r\n ERR_IF   \r\n");  
        break;  
        case ERR_INPROGRESS:                                     /* Operation in progress    */  
            USART_PRINTF_S("\r\n ERR_INPROGRESS   \r\n");  
        break;  
		default:
		break;
    }  
}  


/* 从机响应状态 */
void Setstm8_fm17550_SlaveReplySt(u8_t Le_u_St)
{
	IapFlag.EchoFlag = 1U;
	IapFlag.TxBusyFlag = 0U;
	IapFlag.SlaveRxSt = Le_u_St;
	//Se_w_ConntTimer =(200U/(STM8_IAP_SCHEDULING_CYCLE/5U)) - 1U;//设置数据帧之间的间隔时间
}

/**
* @brief  Disconnects to the TCP echo server
* @param  None
* @retval None
*/
static void stm8_fm17550_iap_disconnect(void)
{
	/* close connection */
	http_client_connection_close(stm8_fm17550_pcb,stm8_fm17550_es);
}

/**
  * @brief Function called when TCP connection established
  * @param tpcb: pointer on the connection contol block
  * @param err: when connection correctly established err should be ERR_OK 
  * @retval err_t: returned error 
  */
static err_t http_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	if(err == ERR_OK)   
	{
		/* allocate structure es to maintain tcp connection informations */
		stm8_fm17550_es = (struct stm8_fm17550 *)mem_malloc(sizeof(struct stm8_fm17550));
		if(stm8_fm17550_es != NULL)
		{
			stm8_fm17550_es->state = ES_CONNECTED;
			stm8_fm17550_es->pcb = NULL;
			stm8_fm17550_es->p_tx = NULL;
			/* pass newly allocated es structure as argument to tpcb */
			tcp_arg(tpcb, stm8_fm17550_es);

			/* initialize LwIP tcp_recv callback function */ 
			tcp_recv(tpcb, http_client_recv);

			/* initialize LwIP tcp_sent callback function */
			//tcp_sent(tpcb, tcp_echoclient_sent);

			/* initialize LwIP tcp_poll callback function */
			tcp_poll(tpcb, http_client_poll, 1);

			return ERR_OK;
		}
		else
		{
			USART_PRINTF_S("下载连接structure es分配失败");
			/* close connection */
			http_client_connection_close(tpcb, stm8_fm17550_es);
			/* return memory allocation error */
			return ERR_MEM;  
		}
	}
	else
	{
		USART_PRINTF_D("下载连接连接回调函数 err: %d\n",err);
		/* close connection */
		http_client_connection_close(tpcb, stm8_fm17550_es);
	}
	return err;
}
    
/**
  * @brief tcp_receiv callback
  * @param arg: argument to be passed to receive callback 
  * @param tpcb: tcp connection control block 
  * @param err: receive error code 
  * @retval err_t: retuned error  
  */
static err_t http_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{ 
	struct stm8_fm17550 *es;
	err_t ret_err;
	u16_t Le_w_i;
	char *position;
	char *endposition;
	u16_t Le_w_temp;
	u8_t Le_u_CnntLngVal[7U] = {0};
	LWIP_ASSERT("arg != NULL",arg != NULL);

	es = (struct stm8_fm17550 *)arg;

	/* if we receive an empty tcp frame from server => close connection */
	if (p == NULL)
	{
		USART_PRINTF_S("下载连接接收函数：p==NULL");
		ret_err = ERR_OK;
	}   
	/* else : a non empty frame was received from echo server but for some reason err != ERR_OK */
	else if(err != ERR_OK)
	{
		USART_PRINTF_S("下载连接接收函数：err!=ERR_OK");
		/* Acknowledge data reception */
		tcp_recved(tpcb, p->tot_len);
		/* free received pbuf*/
		pbuf_free(p);

		ret_err = err;
	}
	else if(es->state == ES_CONNECTED)
	{
		/* Acknowledge data reception */
		tcp_recved(tpcb, p->tot_len);// 必须调用，否则会引起发送窗口为0导致数据不能发送 
		if(STM8_FM17550_NNT_REMOTEPORT == tpcb->remote_port)
		{//从http报文中解析出数据体
			if(p->tot_len < 1460U)
			{
				if(NULL != strstr((char*)p->payload,"HTTP/1.1"))
				{//第一帧数据
					if(NULL != strstr((char*)p->payload,"HTTP/1.1 200 OK"))
					{//无报错时解析出Content-Length，Content-Length字段的值就是下载的数据总长度字节数
						position = strstr((char*)p->payload,"Content-Length: ") + 16U;//Content-Length字段数值的起始位置
						endposition = strchr(&(((char*)p->payload)[position - (char*)p->payload]), '\r\n');//首次出现'\r\n'的地址
						if((endposition - position) <= sizeof(Le_u_CnntLngVal))
						{
							for(Le_w_i = 0;Le_w_i < (endposition - position);Le_w_i++)
							{
								Le_u_CnntLngVal[Le_w_i] = ((char*)p->payload)[(position - (char*)p->payload) + Le_w_i];
							}
							Se_dw_byteCnt = 0U;
							UpdataInfo.Lng = atoi(Le_u_CnntLngVal);/*数据长度*/
							if(UpdataInfo.Lng > STM8_FM17550_DOWNLOAD_DATA_LNG)
							{//数据长度超出下载区大小
								USART_PRINTF_S("数据长度超出下载区存储空间容量");
								IapFlag.downloadFlag = STM8_FM17550_DOWNLOAD_IDLE;
								Sehttp_client_u_cnntSt = STM8_FM17550_DISCNNT;//断开连接	
							}
						}
						else
						{
							USART_PRINTF_S("数据长度有误");
							IapFlag.downloadFlag = STM8_FM17550_DOWNLOAD_IDLE;
							Sehttp_client_u_cnntSt = STM8_FM17550_DISCNNT;//断开连接
						}
				
						USART_PRINTF_D("UpdataInfo.Lng = %d\n",UpdataInfo.Lng);
					}
					else
					{
						USART_PRINTF_S("http请求响应有错误");
						IapFlag.downloadFlag = STM8_FM17550_DOWNLOAD_IDLE;
						Sehttp_client_u_cnntSt = STM8_FM17550_DISCNNT;//断开连接	
					}
				}
				else
				{//用户app数据
					if(IapFlag.downloadFlag == STM8_FM17550_DOWNLOAD_UNDERWAY)
					{
						http_wrFlash((STM8_FM17550_CODE_SEC+Se_dw_byteCnt),(uint8_t*)p->payload,p->tot_len);
						Se_dw_byteCnt += p->tot_len;
						if(Se_dw_byteCnt >= UpdataInfo.Lng)
						{
							if(Se_dw_byteCnt == UpdataInfo.Lng)
							{//接收最后一帧数据，下载完成
								//UpdataInfo.Lng = UpdataInfo.Lng - 4;//有效数据长度 = 接收的数据总长 - 4字节校验
								UpdataInfo.DtFirstAddr = STM8_FM17550_CODE_SEC;/*数据段首地址*/
								//UpdataInfo.DtendAddr =  STM8_FM17550_CODE_SEC + Se_dw_byteCnt -1;/*数据段末地址*/
								//UpdataInfo.CheckSum = 0U;/*升级区数据校验和*/
								//UpdataInfo.IapFlag = 0U;/*在线升级标志*/
								//UpdataInfo.Xor = Gethttpclient_dw_XOR((uint8*)(&UpdataInfo.DtFirstAddr),(sizeof(UpdataInfo)-4));
								//http_wrFlash(HTTP_CLIENT_UPLOAD_INFO_SEC,(uint8_t*)(&UpdataInfo.Xor),sizeof(UpdataInfo));//升级信息写入flash
								IapFlag.downloadFlag = STM8_FM17550_DOWNLOAD_FINISH;
								USART_PRINTF_S("下载完成");
							}
							else
							{//传输字节数有误
								USART_PRINTF_S("接收的数据字节数有误");	
							}
						}
					}
				}
			}
			else
			{
				/*接收数据溢出*/
				USART_PRINTF_S("下载请求接收数据长度溢出");
			}
		}
		/* free received pbuf*/
		pbuf_free(p);
		ret_err = ERR_OK;
	}
	/* data received when connection already closed */
	else
	{
		USART_PRINTF_S("下载连接接收到未知数据");
		/* Acknowledge data reception */
		tcp_recved(tpcb, p->tot_len);
		/* free pbuf and do nothing */
		pbuf_free(p);
		ret_err = ERR_OK;
	}
	return ret_err;
}


/**
  * @brief  This function implements the tcp_poll callback function
  * @param  arg: pointer on argument passed to callback
  * @param  tpcb: tcp connection control block
  * @retval err_t: error code
  */
static err_t http_client_poll(void *arg, struct tcp_pcb *tpcb)
{
	err_t ret_err;
	struct echoclient *es;
	ret_err = ERR_OK;
	es = (struct echoclient*)arg;
	if (es == NULL)
	{
		USART_PRINTF_S("download connection tcp_poll 回调函数 ret_err = ERR_ABRT");
		//if(tpcb != NULL)
		{
			tcp_abort(tpcb);
		}
		ret_err = ERR_ABRT;
	}
	return ret_err;
}


/**
  * @brief This function is used to close the tcp connection with server
  * @param tpcb: tcp connection control block
  * @param es: pointer on echoclient structure
  * @retval None
  */
static void http_client_connection_close(struct tcp_pcb *tpcb, struct stm8_fm17550 * es )
{
	if (es != NULL)
	{
		mem_free(es);
		stm8_fm17550_es = NULL;
		USART_PRINTF_S("free stm8_fm17550_es\n");
	}
	
	if(tpcb != NULL)
	{
		/* remove callbacks */
		tcp_arg(tpcb,NULL);  
		tcp_recv(tpcb,NULL);
		tcp_err(tpcb,NULL);
		//tcp_sent(tpcb, NULL);
		tcp_poll(tpcb, NULL,0);
		/* close tcp connection */
		tcp_close(tpcb);
	}
	else
	{
		//USART_PRINTF_S("stm8_fm17550_pcb == NULL");
	}
}

/*
	发送报文
*/
static void stm8_fm17550_iap_sendMsg(char* data,u16_t len)
{
	tcp_write(stm8_fm17550_pcb,(char*)data ,len, 1);
	//tcp_output(stm8_fm17550_pcb);
}

/**
  * @brief http封装
  * @param 
  * @param
  * @retval None
  */
static void http_client_PostRequest(char* postAddr,char* Le_u_in,u16_t * Le_u_len,char* Le_u_out,u8_t Le_u_ConnectionValue)
{
	strcat(Le_u_out, postAddr);
	strcat(Le_u_out, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n");
	//strcat(Le_u_out, "Accept-Encoding: gzip, deflate\r\n");
	//strcat(Le_u_out, "Accept-Language: zh-CN,zh;q=0.9\r\n");
	if(Le_u_ConnectionValue == 1U)
	{
		strcat(Le_u_out, "Connection: keep-alive\r\n");
	}
	else
	{
		strcat(Le_u_out, "Connection: close\r\n");
	}
	strcat(Le_u_out, "Host: mobile.zzwtec.com\r\n");
	strcat(Le_u_out, "\r\n\r\n");
	*Le_u_len = strlen(Le_u_out);
	USART_PRINTF_S(Le_u_out);
}


/*
//擦除flash扇区
*/
static void http_EraseFlash(uint32_t FLASH_Sector)
{
	__disable_irq() ; //关闭总中断
	//IWDG_Feed();//喂狗
	FLASH_Unlock();
	//下面这个清flash的状态标志很重要，如果没有清可能会导致擦除flash失败或者不能擦除
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | \
					FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
	(void)FLASH_EraseSector(FLASH_Sector,VoltageRange_3);
	FLASH_Lock();
	//IWDG_Feed();//喂狗
	__enable_irq() ; //打开总中断
}


/*
//写flash
*/
static void http_wrFlash(uint32_t FLASH_addr,uint8_t* Le_u_Dt,uint16_t Le_w_lng)
{
	uint16_t Le_w_i;
	//__disable_irq() ; //关闭总中断
	FLASH_Unlock();
	for(Le_w_i = 0;Le_w_i < Le_w_lng;Le_w_i++)
	{
		FLASH_ProgramByte((FLASH_addr+Le_w_i), Le_u_Dt[Le_w_i]);
	}
	FLASH_Lock();
	//__enable_irq() ; //打开总中断
}

/*
//读flash
*/
static void http_rdFlash(uint32_t FLASH_addr,uint8_t* Le_u_Dt,uint16_t Le_w_lng)
{
	uint16_t Le_w_i;
	__disable_irq() ; //关闭总中断
	for(Le_w_i=0; Le_w_i < Le_w_lng; Le_w_i++)
	{
		Le_u_Dt[Le_w_i] = *(__IO uint8*)FLASH_addr;//读FLASH中的数据，直接给出地址就行了。跟从内存中读数据一样
		FLASH_addr++;
	}
	__enable_irq() ; //打开总中断
}

/******************************************************
*函数名：

*形  参：

*返回值：

*描  述：异或取反校验

*备  注：
******************************************************/
static uint8 Gethttpclient_dw_XOR(uint8* Le_u_Dt,uint16 Le_w_Lng)
{
	uint16 Le_w_i;
	uint8 Le_u_Xor = 0U;
	for(Le_w_i = 0U; Le_w_i < Le_w_Lng;Le_w_i++)
	{
		Le_u_Xor ^= Le_u_Dt[Le_w_i];
	}
	return (~Le_u_Xor);
}

#endif /* LWIP_TCP */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
