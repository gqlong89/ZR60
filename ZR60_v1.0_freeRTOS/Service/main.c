/************************************************
  * @file    main.c
  * @author  liujian
  * @version V1.0
  * @date    2018-3-28
  * @brief
  ***********************************************
  * @attention
  *
  * ƽ̨:����  STM32 F407 ������  
  *
  *
  *
 ************************************************/
#include "Include.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "misc.h"
#include "event_groups.h"
#include "./led/bsp_led.h"

/*************************************************
  *        				�궨��
**************************************************/
//#define RunSt_bits_Task  		(1 << 0)
#define LwipApp5ms_bits_Task  	(1 << 1)
#define LwipApp100ms_bits_Task  (1 << 2)
#define iap_bits_Task  			(1 << 3)
#define Fliter_bits_Task  		(1 << 4)
#define ZR60Ctrl5ms_bits_Task  	(1 << 5)
#define ZR60Ctrl100ms_bits_Task (1 << 6)
#define SleepMng_bits_Task 		(1 << 7)
#define https_bits_Task 		(1 << 8)
#define All_bits_Task  			(LwipApp5ms_bits_Task | LwipApp100ms_bits_Task | iap_bits_Task | SleepMng_bits_Task\
								 | Fliter_bits_Task | ZR60Ctrl5ms_bits_Task | ZR60Ctrl100ms_bits_Task)
static EventGroupHandle_t  		xCreatedEventGroup;

/*************************************************
  *        				�������
**************************************************/

/*************************************************
  *        				���徲̬����
**************************************************/
static void AppTaskCreate(void);
static void Lwip_app_5msTask(void * pvParameters);
static void Lwip_app_100msTask(void * pvParameters);
static void iap_Task(void * pvParameters);
static void Fliter_Task(void * pvParameters);
static void ZR60Ctrl_5msTask(void * pvParameters);
static void ZR60Ctrl_10msTask(void * pvParameters);
#ifdef freeRTOS_RUN_STATUS_DEBUG
static void vTaskRunningStatus(void *pvParameters);
#endif
static void AppObjCreate (void);
static void vTaskFeedDog(void *pvParameters);
static void LowPower_Task(void * pvParameters);
static void https_Task(void * pvParameters);
/*************************************************
  *        				main����
**************************************************/
int main(void)
{
	SCB->VTOR = FLASH_BASE | 0x40000;//�ж�������ƫ��
	Mcu_Init();/* ��ʼ�� */
	
	/* Enable RNG clock source *///polarSSL��Ҫʹ�õ�
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
	/* Enable RNG IP  */
	RNG_Cmd(ENABLE);
	/* Enable CRYP clock */
	//RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_CRYP, ENABLE);
	/* Enable HASH clock */
	//RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_HASH, ENABLE);
	
	AppObjCreate(); /* �����¼���־�� */
	AppTaskCreate();/* �������� */
	
	while(1)
	{}
}


/* �������� */
static void AppTaskCreate(void)
{
#ifdef freeRTOS_RUN_STATUS_DEBUG
	xTaskCreate(vTaskRunningStatus, (const char *)"RunningStatus", 	1024 , \
				STD_NULL_PTR, tskIDLE_PRIORITY + 1, STD_NULL_PTR);
#endif
	xTaskCreate(LowPower_Task, (const char *)"LowPower", 	256 , \
				STD_NULL_PTR, tskIDLE_PRIORITY + 2, STD_NULL_PTR);
	xTaskCreate(iap_Task, 			(const char *)"iap", 			512 , \
				STD_NULL_PTR, tskIDLE_PRIORITY + 3, STD_NULL_PTR);
	xTaskCreate(Lwip_app_100msTask, (const char *)"Lwip_app_100ms", 1024 , \
				STD_NULL_PTR, tskIDLE_PRIORITY + 3, STD_NULL_PTR);
	xTaskCreate(https_Task, (const char *)"https", 1024 , \
				STD_NULL_PTR, tskIDLE_PRIORITY + 1, STD_NULL_PTR);
	xTaskCreate(ZR60Ctrl_5msTask, 	(const char *)"ZR60Ctrl_5ms", 	512 , \
				STD_NULL_PTR, tskIDLE_PRIORITY + 5,STD_NULL_PTR);
	xTaskCreate(ZR60Ctrl_10msTask, 	(const char *)"ZR60Ctrl_10ms", 	512 , \
				STD_NULL_PTR, tskIDLE_PRIORITY + 3,STD_NULL_PTR);
	xTaskCreate(Lwip_app_5msTask, 	(const char *)"Lwip_app_5ms", 	512 , \
				STD_NULL_PTR, tskIDLE_PRIORITY + 4, STD_NULL_PTR);
	xTaskCreate(Fliter_Task,		(const char *)"Fliter", 		128 , \
				STD_NULL_PTR, tskIDLE_PRIORITY + 5,STD_NULL_PTR);
	xTaskCreate(vTaskFeedDog,		(const char *)"FeedDog", 		128 , \
				STD_NULL_PTR, tskIDLE_PRIORITY + 5,STD_NULL_PTR);
	
	vTaskStartScheduler();//����������
}

static void Lwip_app_5msTask(void * pvParameters)
{
	static uint64_t Se_dw_LocalTime = 0U;
	while(1)
	{	
		TskdhcpClient_MainFunction();
		TskdnsAnalysis_MainFunction();
	
		/*********************************
		���½ӿ���Ҫ���ڵ���
		*********************************/
		/* handle periodic timers for LwIP */
		Se_dw_LocalTime = Se_dw_LocalTime + 5U;
		LwIP_Periodic_Handle(Se_dw_LocalTime);
		
		xEventGroupSetBits(xCreatedEventGroup,LwipApp5ms_bits_Task);
		//IWDG_Feed();//ι��
		vTaskDelay(5 / portTICK_RATE_MS);
	}
}


static void https_Task(void * pvParameters)
{
	while(1)
	{
		tcp_ShortConnect_MainFunction();
		
		//xEventGroupSetBits(xCreatedEventGroup, https_bits_Task);
		vTaskDelay(5000 / portTICK_RATE_MS);
	}
}


static void Lwip_app_100msTask(void * pvParameters)
{
	while(1)
	{
		tcp_LngConnect_MainFunction();
		Tskntpclient_MainFunction();
		
		xEventGroupSetBits(xCreatedEventGroup, LwipApp100ms_bits_Task);
		//IWDG_Feed();//ι��
		vTaskDelay(35 / portTICK_RATE_MS);
	}
}

static void iap_Task(void * pvParameters)
{
	while(1)
	{
		//stm8_fm17550_iap_MainFunction();
		http_client_iap_MainFunction();
		
		xEventGroupSetBits(xCreatedEventGroup, iap_bits_Task);
		//IWDG_Feed();//ι��
		vTaskDelay(50 / portTICK_RATE_MS);
	}
}


/* �˲����� */
static void Fliter_Task(void * pvParameters)
{
	while(1)
	{
		MngADFliter_mainFunction();
		TskBtnFltr_MainFunction();//����ɨ��
		TskPSFltr_MainFunction();//��������Ӳ���ź�ɨ��
		
		xEventGroupSetBits(xCreatedEventGroup, Fliter_bits_Task);
		//IWDG_Feed();//ι��
		vTaskDelay(5 / portTICK_RATE_MS);
	}
}

static void ZR60Ctrl_5msTask(void * pvParameters)
{
	while(1)
	{
		/*******************
		*	ZR60�����߼�
		********************/	
		TskZR60Ctrl_MainFunction();
		TskAudioIO_MainFunction();//��������
		
		xEventGroupSetBits(xCreatedEventGroup, ZR60Ctrl5ms_bits_Task);
		//IWDG_Feed();//ι��
		vTaskDelay(20 / portTICK_RATE_MS);
	}
}

static void ZR60Ctrl_10msTask(void * pvParameters)
{
	while(1)
	{
		TskBListMng_MainFunction();
		TskMemIf_MainFunction();//дflash��eeprom
		
		xEventGroupSetBits(xCreatedEventGroup, ZR60Ctrl100ms_bits_Task);
		//IWDG_Feed();//ι��
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}

static void LowPower_Task(void * pvParameters)
{
	while(1)
	{
		TskSleepMng_MainFunction();	
		
		xEventGroupSetBits(xCreatedEventGroup, SleepMng_bits_Task);
		//IWDG_Feed();//ι��
		vTaskDelay(20 / portTICK_RATE_MS);
	}
}

#ifdef freeRTOS_RUN_STATUS_DEBUG
/* ����״̬ͳ�� */
static void vTaskRunningStatus(void *pvParameters)
{
    uint8_t pcWriteBuffer[700];

    while(1)
    {
		printf("\r\n������         ����״̬ ���ȼ� ʣ��ջ   �������\r\n");
		vTaskList((char *)&pcWriteBuffer);
		printf("%s\r\n", pcWriteBuffer);
		
		printf("\r\n������          ���м���        ʹ����\r\n");
		vTaskGetRunTimeStats((char *)&pcWriteBuffer);
		printf("%s\r\n", pcWriteBuffer);
		
		//xEventGroupSetBits(xCreatedEventGroup, RunSt_bits_Task);
        IWDG_Feed();//ι��
        vTaskDelay(30000);
    }
}
#endif

/* �����¼���־�� */
static void AppObjCreate (void)
{
    /* �����¼���־�� */
    xCreatedEventGroup = xEventGroupCreate();
    
    if(xCreatedEventGroup == NULL)
    {
        /* û�д����ɹ����û�������������봴��ʧ�ܵĴ������� */
		USART_PRINTF_S("\r\ncreate event failure!\n");
    }
}

/* ι������ */
static void vTaskFeedDog(void *pvParameters)
{
	static uint16_t Se_w_Timer = 0U;
    EventBits_t uxBits;
    const TickType_t xTicksToWait = 150 / portTICK_PERIOD_MS; /* ����ӳ�150ms */
	
    IWDG_Config(IWDG_Prescaler_256 ,100);//�������Ź���ʼ����50-->320ms
    while(1)
    {
        /* �ȴ������������¼���־ */
        uxBits = xEventGroupWaitBits(xCreatedEventGroup, /* �¼���־���� */
                                     All_bits_Task,       /* �ȴ�TASK_BIT_ALL������ */
                                     pdTRUE,             /* �˳�ǰTASK_BIT_ALL�������������TASK_BIT_ALL�������òű�ʾ���˳���*/
                                     pdTRUE,             /* ����ΪpdTRUE��ʾ�ȴ�TASK_BIT_ALL��������*/
                                     xTicksToWait);      /* �ȴ��ӳ�ʱ�� */
        
        if((uxBits & All_bits_Task) == All_bits_Task)
        {
            IWDG_Feed();
			Se_w_Timer++;
			if(Se_w_Timer >= 200U)
			{
				Se_w_Timer = 0U;
				USART_PRINTF_S("\r\n�û�������������\r\n");
			}
        }
        else
        {
            /* ������ÿxTicksToWait����һ�� */
            /* ͨ������uxBits�򵥵Ŀ����ڴ˴�����Ǹ�������û�з������б�־ */			
			if((uxBits & LwipApp5ms_bits_Task) != LwipApp5ms_bits_Task)
			{
				USART_PRINTF_S("\r\nErrorLogging��Lwip app 5ms����δ��������\r\n");
			}
			
			if((uxBits & LwipApp100ms_bits_Task) != LwipApp100ms_bits_Task)
			{
				USART_PRINTF_S("\r\nErrorLogging��Lwip app 100ms����δ��������\r\n");
			}
			
			if((uxBits & iap_bits_Task) != iap_bits_Task)
			{
				USART_PRINTF_S("\r\nErrorLogging�����߸�������δ��������\r\n");
			}	
			
			if((uxBits & Fliter_bits_Task) != Fliter_bits_Task)
			{
				USART_PRINTF_S("\r\nErrorLogging���˲�����δ��������\r\n");
			}
			
			if((uxBits & ZR60Ctrl5ms_bits_Task) != ZR60Ctrl5ms_bits_Task)
			{
				USART_PRINTF_S("\r\nErrorLogging��ZR60 5ms����δ��������\r\n");
			}
			
			if((uxBits & ZR60Ctrl100ms_bits_Task) != ZR60Ctrl100ms_bits_Task)
			{
				USART_PRINTF_S("\r\nErrorLogging��ZR60 100ms����δ��������\r\n");
			}
			
			if((uxBits & SleepMng_bits_Task) != SleepMng_bits_Task)
			{
				USART_PRINTF_S("\r\nErrorLogging���͹�������δ��������\r\n");
			}
        }
    }
}


/*******************END OF FILE******************/
