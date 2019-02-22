/******************************************************
�ļ�����	IcUnlock

������		

Data			  Vasion			author
2018/04/7		  V1.0			    liujian
*******************************************************/
#ifndef		_IC_UNLOCK_H
#define		_IC_UNLOCK_H
/*******************************************************
description�� include the header file
*******************************************************/
#include "IcCardUnlock/IcUnlock_cfg.h"

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/
#define IC_UNLOCK_DEBUG
#ifdef 	IC_UNLOCK_DEBUG
#define IC_UNLOCK_PRINTF_S(x)   	 printf("%s\n",x)
#define IC_UNLOCK_PRINTF_D(x,d)   	 printf(x,d)
#define IC_UNLOCK_PRINTF_CARD_NUM(x,d1,d2,d3,d4)    	printf(x,d1,d2,d3,d4)
#else
#define IC_UNLOCK_PRINTF_S(x)    
#define IC_UNLOCK_PRINTF_D(x,d)  
#define IC_UNLOCK_PRINTF_CARD_NUM(x,d1,d2,d3,d4)    	printf(x,d1,d2,d3,d4)
#endif

/**********�곣������*********/
#define  IC_UNLOCK_SCHEDULING_CYCLE		  20U//��������


#define IC_UNLOCK_DEVICE_TYPE  1

#define OK  			0x00//��ȡ��������Ч
/**
* ��ȡ������
*/
#define GetCard_Num  0x00
/**
* ����key
*/
#define Load_Key  0x01
/**
* ���Ƭ����ȡ��Ƭ��
*/
#define Get_CardSnr  0x02
/**
* ����Ϊ������鿨Ƭģʽ IDLEģʽ
*/
#define SetDetectCard_ALL  0x03
/**
* ����0x14 �洢��С��ID��ϢcommunityId
*/
#define Block_0x14  0x14
/**
* ����0x15 �洢��¥�������ϢbuildNum1
*/
#define Block_0x15  0x15
/**
* ����0x16 �����¥�������ϢbuildNum2
*/
#define Block_0x16  0x16
/**
 * ����0x18 ¥��id
 */
#define Block_0x18  0x18
/**
 * ����0x19 ��id
 */
#define Block_0x19  0x19
/**
 * ����0x20 �±�
 */
#define Block_0x1A  0x1a
/**
* ���ݿ�10, ��IC���洢�û�ID��
*/
#define Block_0x10  0x10
/**
* ���ݿ�11, ��IC���洢�ֻ�����Ŀ�
*/
#define Block_0x11 0x11

/***********�꺯��***********/
#define GetIcUnlock_u_TxIdle()  1//GetUartCmn_u_TxIdle()


/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/*****struct definitions*****/


/******enum definitions******/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/
extern struct sm3_info sm;
extern struct rtc_time Ve_h_tm;

/*******************************************************
description�� function External declaration
*******************************************************/
extern void  InitIcUnlock_parameter(void);
extern void  TskIcUnlock_MainFunction(void);
extern uint8_t	GetIcUnlock_u_CardSetSt(void);
extern char IcUnlock_u_ArrayCmp(unsigned char *a,unsigned char *b, unsigned int len);
extern void SetIcUnlock_u_CardReadSuccess(void);
#endif 