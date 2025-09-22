/*
 * screen.h
 *
 *  Created on: Aug 31, 2024
 *      Author: nicob
 */

#ifndef __SCREEN_H__
#define __SCREEN_H__
#include <stdint.h>

#define SCREEN_BACKGROUND_COLOR ST7735_MAGENTA
#define SCREEN_HEADER_COLOR ST7735_WHITE
#define SCREEN_TEXT_COLOR ST7735_WHITE
#define SCREEN_HEADER_HEIGHT 15

void Screen_Init(void);
void Screen_Header_Draw(const char *title, uint8_t battery);
void Screen_Background_Draw(void);
void Screen_Status_Draw(const char* title, const char* text);

#endif /* __SCREEN_H__ */
