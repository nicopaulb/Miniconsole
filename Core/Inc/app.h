#ifndef __APP_H__
#define __APP_H__

typedef enum {
	STAGE_START_ENTER = 0,
	STAGE_START,
	STAGE_START_EXIT,
	STAGE_MAINMENU_ENTER,
	STAGE_MAINMENU,
	STAGE_MAINMENU_EXIT,
	STAGE_SNAKE_ENTER,
	STAGE_SNAKE,
	STAGE_SNAKE_EXIT,
	STAGE_TEST_ENTER,
	STAGE_TEST,
	STAGE_TEST_EXIT,
	STAGE_RESET,
} App_Stage_t;

void App_Init(void);
void App_Loop(void);
void App_Set_Stage(App_Stage_t stage_new);

#endif /* __APP_H__ */
