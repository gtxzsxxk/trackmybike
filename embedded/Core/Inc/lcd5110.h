/*
 * lcd5110.h
 *
 *  Created on: 2023年4月16日
 *      Author: hanyuan
 */

#ifndef LCD5110_H_
#define LCD5110_H_
#include "main.h"
#define SCE_H HAL_GPIO_WritePin(LCD_CE_GPIO_Port,LCD_CE_Pin,GPIO_PIN_SET)
#define SCE_L HAL_GPIO_WritePin(LCD_CE_GPIO_Port,LCD_CE_Pin,GPIO_PIN_RESET)

#define DC_H HAL_GPIO_WritePin(LCD_DC_GPIO_Port,LCD_DC_Pin,GPIO_PIN_SET)
#define DC_L HAL_GPIO_WritePin(LCD_DC_GPIO_Port,LCD_DC_Pin,GPIO_PIN_RESET)

#define RST_H HAL_GPIO_WritePin(LCD_RST_GPIO_Port,LCD_RST_Pin,GPIO_PIN_SET)
#define RST_L HAL_GPIO_WritePin(LCD_RST_GPIO_Port,LCD_RST_Pin,GPIO_PIN_RESET)

#define DIN_H HAL_GPIO_WritePin(LCD_DIN_GPIO_Port,LCD_DIN_Pin,GPIO_PIN_SET)
#define DIN_L HAL_GPIO_WritePin(LCD_DIN_GPIO_Port,LCD_DIN_Pin,GPIO_PIN_RESET)

#define SCLK_H HAL_GPIO_WritePin(LCD_SCK_GPIO_Port,LCD_SCK_Pin,GPIO_PIN_SET)
#define SCLK_L HAL_GPIO_WritePin(LCD_SCK_GPIO_Port,LCD_SCK_Pin,GPIO_PIN_RESET)

#define CMD 0
#define DAT 1

#define WORD_TOWER_SIM 92+32
#define WORD_TOWER_GPS 93+32
#define WORD_SIGNAL(x) x+94+32
enum StrLocation{
	Left=0,
	Middle,
	Right
};
void LCD_WriteByte(uint8_t dc,uint8_t b);
void LCD_Init(void);
void LCD_Clean(void);
void LCD_SetXY(uint8_t x,uint8_t y);
extern const uint8_t font6x8[][6];
extern const uint8_t font_big[][24];
extern const uint8_t font_big_number[][24];
extern const uint8_t font_freertos[][24];
uint8_t GetFont(uint8_t dat);
void g_puts(uint8_t ch);
void g_print(char* dat);
void SetLine(uint8_t line);
void l_print(const char* dat,uint8_t line,enum StrLocation loc);
void GUI_SetLine(uint8_t line);
void f_print(char* dat);
void LCD_draw_big(uint8_t x,uint8_t y,uint8_t which);
void LCD_draw_big_num(uint8_t x,uint8_t y,uint8_t which);
void LCD_draw_freertos(uint8_t x,uint8_t y);
#endif

