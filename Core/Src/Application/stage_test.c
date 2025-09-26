#include "app.h"
#include "screen.h"
#include "hid_host_app.h"
#include "st7735.h"
#include "icons.h"
#include "buzzer.h"

#define STAGE_TITLE "TEST"

static HID_Report_t report_old;

void Stage_Test_Enter_Handle(HID_Report_t *report, uint8_t battery) {
	Screen_Header_Draw(STAGE_TITLE, battery);
	Screen_Background_Draw();
	ST7735_FillRectangle(ST7735_WIDTH / 2 - 40, ST7735_HEIGHT / 2 - 40, 80, 80, ST7735_WHITE);
	ST7735_FillRectangle(ST7735_WIDTH / 2 - 38, ST7735_HEIGHT / 2 - 38, 76, 76, ST7735_BLACK);
	ST7735_WriteString(ST7735_CENTERED, ST7735_HEIGHT - 15, "Press X/Y/A/B to test", Font_7x10, SCREEN_TEXT_COLOR,
	SCREEN_BACKGROUND_COLOR);
	App_Set_Stage(STAGE_TEST);
}

void Stage_Test_Handle(HID_Report_t *report, uint8_t battery) {
	if (memcmp(report, &report_old, sizeof(HID_Report_t)) != 0) {
		if (report->BTN_A) {
			ST7735_DrawImage(ST7735_WIDTH / 2 - 32, ST7735_HEIGHT / 2 - 32, 64, 64, xbox_button_a);
		} else if (report->BTN_B) {
			ST7735_DrawImage(ST7735_WIDTH / 2 - 32, ST7735_HEIGHT / 2 - 32, 64, 64, xbox_button_b);
		} else if (report->BTN_X) {
			ST7735_DrawImage(ST7735_WIDTH / 2 - 32, ST7735_HEIGHT / 2 - 32, 64, 64, xbox_button_x);
		} else if (report->BTN_Y) {
			ST7735_DrawImage(ST7735_WIDTH / 2 - 32, ST7735_HEIGHT / 2 - 32, 64, 64, xbox_button_y);
		} else if (report->BTN_Xbox) {
			App_Set_Stage(STAGE_TEST_EXIT);
		} else {
			ST7735_FillRectangle(ST7735_WIDTH / 2 - 38, ST7735_HEIGHT / 2 - 38, 76, 76, ST7735_BLACK);
		}
		memcpy(&report_old, report, sizeof(HID_Report_t));
	}
}

void Stage_Test_Exit_Handle(HID_Report_t *report, uint8_t battery) {
	Buzzer_Play_Menu_Back();
	App_Set_Stage(STAGE_MAINMENU_ENTER);
}
