#include "app.h"
#include "screen.h"
#include "hid_host_app.h"
#include "st7735.h"

#define STAGE_TITLE "MENU"

typedef enum {
	STATE_IDLE = 0, STATE_UP_PRESSED, STATE_DOWN_PRESSED, STATE_ENTER_PRESSED,
} mainmenu_state_t;

typedef enum {
	MAINMENU_SNAKE = 0, MAINMENU_TEST, MAINMENU_EXIT, MAINMENU_MAX,
} mainmenu_item_t;

static mainmenu_item_t selected = MAINMENU_SNAKE;
static mainmenu_state_t state = STATE_IDLE;

static void Screen_MainMenu_Select(mainmenu_item_t selection) {
	switch (selection) {
	case MAINMENU_SNAKE:
		ST7735_WriteString(ST7735_CENTERED, 3 * 10, "PLAY", Font_11x18, SCREEN_BACKGROUND_COLOR, SCREEN_TEXT_COLOR);
		ST7735_WriteString(ST7735_CENTERED, 6 * 10, "TEST", Font_11x18, SCREEN_TEXT_COLOR, SCREEN_BACKGROUND_COLOR);
		ST7735_WriteString(ST7735_CENTERED, 9 * 10, "RESET", Font_11x18, SCREEN_TEXT_COLOR, SCREEN_BACKGROUND_COLOR);
		break;

	case MAINMENU_TEST:
		ST7735_WriteString(ST7735_CENTERED, 3 * 10, "PLAY", Font_11x18, SCREEN_TEXT_COLOR, SCREEN_BACKGROUND_COLOR);
		ST7735_WriteString(ST7735_CENTERED, 6 * 10, "TEST", Font_11x18, SCREEN_BACKGROUND_COLOR, SCREEN_TEXT_COLOR);
		ST7735_WriteString(ST7735_CENTERED, 9 * 10, "RESET", Font_11x18, SCREEN_TEXT_COLOR, SCREEN_BACKGROUND_COLOR);
		break;

	case MAINMENU_EXIT:
		ST7735_WriteString(ST7735_CENTERED, 3 * 10, "PLAY", Font_11x18, SCREEN_TEXT_COLOR, SCREEN_BACKGROUND_COLOR);
		ST7735_WriteString(ST7735_CENTERED, 6 * 10, "TEST", Font_11x18, SCREEN_TEXT_COLOR, SCREEN_BACKGROUND_COLOR);
		ST7735_WriteString(ST7735_CENTERED, 9 * 10, "RESET", Font_11x18, SCREEN_BACKGROUND_COLOR, SCREEN_TEXT_COLOR);
		break;

	default:
		ST7735_WriteString(ST7735_CENTERED, 3 * 10, "PLAY", Font_11x18, SCREEN_TEXT_COLOR, SCREEN_BACKGROUND_COLOR);
		ST7735_WriteString(ST7735_CENTERED, 6 * 10, "TEST", Font_11x18, SCREEN_TEXT_COLOR, SCREEN_BACKGROUND_COLOR);
		ST7735_WriteString(ST7735_CENTERED, 9 * 10, "RESET", Font_11x18, SCREEN_TEXT_COLOR, SCREEN_BACKGROUND_COLOR);
		break;
	}
}

void Stage_MainMenu_Enter_Handle(HID_Report_t *report, uint8_t battery) {
	selected = MAINMENU_SNAKE;
	state = STATE_IDLE;
	Screen_Background_Draw();
	Screen_Header_Draw(STAGE_TITLE, battery);
	Screen_MainMenu_Select(selected);
	App_Set_Stage(STAGE_MAINMENU);
}

void Stage_MainMenu_Handle(HID_Report_t *report, uint8_t battery) {

	switch (state) {
	case STATE_IDLE:
		if (report->BTN_A) {
			state = STATE_ENTER_PRESSED;
		} else if (report->HAT_Switch == HATSWITCH_DOWN) {
			Buzzer_Play_Menu_Move();
			if (selected < MAINMENU_MAX - 1) {
				selected++;
				Screen_MainMenu_Select(selected);
			}
			state = STATE_DOWN_PRESSED;
		} else if (report->HAT_Switch == HATSWITCH_UP) {
			Buzzer_Play_Menu_Move();
			if (selected > 0) {
				selected--;
				Screen_MainMenu_Select(selected);
			}
			state = STATE_UP_PRESSED;
		}
		break;

	case STATE_UP_PRESSED:
		if (report->HAT_Switch != HATSWITCH_UP) {
			state = STATE_IDLE;
		}
		break;

	case STATE_DOWN_PRESSED:
		if (report->HAT_Switch != HATSWITCH_DOWN) {
			state = STATE_IDLE;
		}
		break;

	case STATE_ENTER_PRESSED:
		state = STATE_IDLE;
		App_Set_Stage(STAGE_MAINMENU_EXIT);
		break;
	}
}

void Stage_MainMenu_Exit_Handle(HID_Report_t *report, uint8_t battery) {
	switch (selected) {
	case MAINMENU_SNAKE:
		App_Set_Stage(STAGE_SNAKE_ENTER);
		break;

	case MAINMENU_TEST:
		App_Set_Stage(STAGE_TEST_ENTER);
		break;

	case MAINMENU_EXIT:
		App_Set_Stage(STAGE_RESET);
		break;

	default:
		break;
	}
}
