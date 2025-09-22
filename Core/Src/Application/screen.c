#include "screen.h"
#include "st7735.h"

void Screen_Init(void) {
	ST7735_Init();
	Screen_Background_Draw();
	Screen_Header_Draw("INIT", 0);
}

void Screen_Header_Draw(const char *title, uint8_t battery) {
	ST7735_FillRectangle(0, 0, ST7735_WIDTH, SCREEN_HEADER_HEIGHT, SCREEN_HEADER_COLOR);
	ST7735_WriteString(ST7735_CENTERED, (SCREEN_HEADER_HEIGHT-7)/2, title, Font_7x10, ST7735_BLACK, SCREEN_HEADER_COLOR);
	if (battery > 0) {
		char battery_str[6];
		snprintf(battery_str, 6, "%02u%%", battery);
		ST7735_WriteString(130, (SCREEN_HEADER_HEIGHT-7)/2, battery_str, Font_7x10, ST7735_BLACK, SCREEN_HEADER_COLOR);
	}
}

void Screen_Background_Draw(void) {
	ST7735_FillRectangle(0, SCREEN_HEADER_HEIGHT, ST7735_WIDTH, ST7735_HEIGHT - SCREEN_HEADER_HEIGHT,
			SCREEN_BACKGROUND_COLOR);
}

void Screen_Status_Draw(const char *title, const char *text) {
	static const char *drawed_string = NULL;
	if (drawed_string == NULL || strcmp(title, drawed_string) != 0) {
		Screen_Background_Draw();
		Screen_Header_Draw("INIT", 0);
		ST7735_WriteString(ST7735_CENTERED, 4 * 10, title, Font_11x18, SCREEN_TEXT_COLOR, SCREEN_BACKGROUND_COLOR);
		ST7735_WriteString(ST7735_CENTERED, 8 * 10, text, Font_7x10, SCREEN_TEXT_COLOR, SCREEN_BACKGROUND_COLOR);
		drawed_string = title;
	}
}
