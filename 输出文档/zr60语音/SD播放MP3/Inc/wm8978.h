#ifndef __WM8978_H
#define __WM8978_H
#include "stm32f1xx_hal.h"
#include "gpio.h"

#define WM8978_ADDR	0X34

uint8_t WM8978_Write_Reg(uint8_t reg,uint16_t val);
uint16_t WM8978_Read_Reg(uint8_t reg);
uint8_t WM8978_Init(void);
void WM8978_ADDA_Cfg(uint8_t dacen,uint8_t adcen);
void WM8978_Output_Cfg(uint8_t dacen,uint8_t bpsen);
void WM8978_HPvol_Set(uint8_t voll,uint8_t volr);
void WM8978_SPKvol_Set(uint8_t volx);
void WM8978_I2S_Cfg(uint8_t fmt,uint8_t len);
void WM8978_Input_Cfg(uint8_t micen,uint8_t lineinen,uint8_t auxen);
void WM8978_MIC_Gain(uint8_t gain);
void WM8978_LINEIN_Gain(uint8_t gain);
void WM8978_AUX_Gain(uint8_t gain);
#endif
