#ifndef __LED_H
#define	__LED_H

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

//引脚定义
/*******************************************************/
//R 红色灯
#define LED1_PIN                  GPIO_Pin_6                 
#define LED1_GPIO_PORT            GPIOF                      
#define LED1_GPIO_CLK             RCC_AHB1Periph_GPIOF

//G 绿色灯
#define LED2_PIN                  GPIO_Pin_7                 
#define LED2_GPIO_PORT            GPIOF                      
#define LED2_GPIO_CLK             RCC_AHB1Periph_GPIOF

//B 蓝色灯
#define LED3_PIN                       GPIO_Pin_8                 
#define LED3_GPIO_PORT                 GPIOF                       
#define LED3_GPIO_CLK             	   RCC_AHB1Periph_GPIOF

//读卡器模块电源使能引脚
#define READ_CARD_EN_PIN               GPIO_Pin_5               
#define READ_CARD_EN_GPIO_PORT         GPIOA                   
#define READ_CARD_EN_GPIO_CLK          RCC_AHB1Periph_GPIOA

#define OPEN_DOOR_PIN                  GPIO_Pin_0               
#define OPEN_DOOR_GPIO_PORT            GPIOB              
#define OPEN_DOOR_GPIO_CLK             RCC_AHB1Periph_GPIOB

/************************************************************/


/** 控制LED灯亮灭的宏，
	* LED低电平亮，设置ON=0，OFF=1
	* 若LED高电平亮，把宏设置成ON=1 ，OFF=0 即可
	*/
#define ON  0
#define OFF 1

/* 带参宏，可以像内联函数一样使用 */
#define LED1(a)	if (a)	\
					digitalLo(LED1_GPIO_PORT,LED1_PIN);\
					else		\
					digitalHi(LED1_GPIO_PORT,LED1_PIN)

#define LED2(a)	if (a)	\
					digitalLo(LED2_GPIO_PORT,LED2_PIN);\
					else		\
					digitalHi(LED2_GPIO_PORT,LED2_PIN)

#define LED3(a)	if (a)	\
					digitalLo(LED3_GPIO_PORT,LED3_PIN);\
					else		\
					digitalHi(LED3_GPIO_PORT,LED3_PIN)


/* 直接操作寄存器的方法控制IO */
#define	digitalHi(p,i)			 {p->BSRRL=i;}		//设置为高电平
#define digitalLo(p,i)			 {p->BSRRH=i;}		//输出低电平
#define digitalToggle(p,i)	 	 {p->ODR ^=i;}		//输出反转状态

/* 定义控制IO的宏 */
#define LED1_TOGGLE		digitalToggle(LED1_GPIO_PORT,LED1_PIN)
#define LED1_ON		digitalHi(LED1_GPIO_PORT,LED1_PIN)
#define LED1_OFF			digitalLo(LED1_GPIO_PORT,LED1_PIN)

#define LED2_TOGGLE		digitalToggle(LED2_GPIO_PORT,LED2_PIN)
#define LED2_ON		digitalHi(LED2_GPIO_PORT,LED2_PIN)
#define LED2_OFF			digitalLo(LED2_GPIO_PORT,LED2_PIN)

#define LED3_TOGGLE		digitalToggle(LED3_GPIO_PORT,LED3_PIN)
#define LED3_ON			digitalHi(LED3_GPIO_PORT,LED3_PIN)
#define LED3_OFF		digitalLo(LED3_GPIO_PORT,LED3_PIN)

/* 基本混色，后面高级用法使用PWM可混出全彩颜色,且效果更好 */

//红
#define LED_RED  \
					LED1_ON;\
					LED2_OFF;\
					LED3_OFF

//绿
#define LED_GREEN		\
					LED1_OFF;\
					LED2_ON;\
					LED3_OFF

//蓝
#define LED_BLUE	\
					LED1_OFF;\
					LED2_OFF;\
					LED3_ON

					
//黄(红+绿)					
#define LED_YELLOW	\
					LED1_ON;\
					LED2_ON;\
					LED3_OFF
//紫(红+蓝)
#define LED_PURPLE	\
					LED1_ON;\
					LED2_OFF;\
					LED3_ON

//青(绿+蓝)
#define LED_CYAN \
					LED1_OFF;\
					LED2_ON;\
					LED3_ON
					
//白(红+绿+蓝)
#define LED_WHITE	\
					LED1_ON;\
					LED2_ON;\
					LED3_ON
					
//黑(全部关闭)
#define LED_RGBOFF	\
					LED1_OFF;\
					LED2_OFF;\
					LED3_OFF		




void LED_GPIO_Config(void);

#endif /* __LED_H */
