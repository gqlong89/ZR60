#ifndef __BEEP_H_
#define	__BEEP_H_

#include "stm32f4xx.h"



/* 定义蜂鸣器连接的GPIO端口, 用户只需要修改下面的代码即可改变控制的蜂鸣器引脚 */
#define BEEP_GPIO_PORT    	GPIOG			              /* GPIO端口 */
#define BEEP_GPIO_CLK 	    RCC_AHB1Periph_GPIOG		/* GPIO端口时钟 */
#define BEEP_GPIO_PIN		GPIO_Pin_7			        /* 连接到蜂鸣器的GPIO */

#define LOCK_GPIO_PORT    	GPIOB			              /* GPIO端口 */
#define LOCK_GPIO_CLK 	    RCC_AHB1Periph_GPIOB		/* GPIO端口时钟 */
#define LOCK_GPIO_PIN		GPIO_Pin_0			        /* 连接到开门的GPIO */
				
/* 直接操作寄存器的方法控制IO */
#define	digitalHi(p,i)		{p->BSRRL=i;}			  //设置为高电平		
#define digitalLo(p,i)		{p->BSRRH=i;}				//输出低电平
#define digitalToggle(p,i)	{p->ODR ^=i;}			//输出反转状态


/* 定义控制IO的宏 */
#define BEEP_TOGGLE			digitalToggle(BEEP_GPIO_PORT,BEEP_GPIO_PIN)
#define BEEP_OFF			digitalLo(BEEP_GPIO_PORT,BEEP_GPIO_PIN)
#define BEEP_ON				digitalHi(BEEP_GPIO_PORT,BEEP_GPIO_PIN)

/* 定义控制IO的宏 */
#define LOCK_TOGGLE			digitalToggle(LOCK_GPIO_PORT,LOCK_GPIO_PIN)
#define LOCK_OFF			digitalLo(LOCK_GPIO_PORT,LOCK_GPIO_PIN)
#define LOCK_ON				digitalHi(LOCK_GPIO_PORT,LOCK_GPIO_PIN)

void BEEP_GPIO_Config(void);
#endif /* __BEEP_H_ */
