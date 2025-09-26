#include "app.h"
#include "screen.h"
#include "main.h"
#include "stage.h"
#include "hid_host_app.h"
#include "st7735.h"
#include "buzzer.h"

static App_Stage_t stage;

static void Stage_Start_Enter_Handle(HID_Report_t* report, uint8_t battery);
static void Stage_Start_Handle(HID_Report_t* report, uint8_t battery);
static void Stage_Start_Exit_Handle(HID_Report_t* report, uint8_t battery);



void App_Init(void) {
	Screen_Init();
	Buzzer_Play_Boot();
}

void App_Loop(void) {
	HID_Report_t* report = HID_Host_Get_Report();
	HID_HOST_Status_t status =  HID_Host_Get_State();
	uint8_t battery = HID_Host_Get_Battery_Level();

	switch(status) {
		case HID_HOST_IDLE:
			Screen_Status_Draw("Scanning...", "Searching a controller");
			stage = STAGE_START;
			break;
		case HID_HOST_CONNECTED:
			Screen_Status_Draw("Pairing...", "Trying to bound");
			stage = STAGE_START_ENTER;
			break;
		case HID_HOST_DONE:
			switch(stage) {
			case STAGE_START_ENTER:
				Stage_Start_Enter_Handle(report, battery);
				break;
			case STAGE_START:
				Stage_Start_Handle(report, battery);
				break;
			case STAGE_START_EXIT:
				Stage_Start_Exit_Handle(report, battery);
				break;
			case STAGE_MAINMENU_ENTER:
				Stage_MainMenu_Enter_Handle(report, battery);
				break;
			case STAGE_MAINMENU:
				Stage_MainMenu_Handle(report, battery);
				break;
			case STAGE_MAINMENU_EXIT:
				Stage_MainMenu_Exit_Handle(report, battery);
				break;
			case STAGE_SNAKE_ENTER:
				Stage_Snake_Enter_Handle(report, battery);
				break;
			case STAGE_SNAKE:
				Stage_Snake_Handle(report, battery);
				break;
			case STAGE_SNAKE_EXIT:
				Stage_Snake_Exit_Handle(report, battery);
				break;
			case STAGE_TEST_ENTER:
				Stage_Test_Enter_Handle(report, battery);
				break;
			case STAGE_TEST:
				Stage_Test_Handle(report, battery);
				break;
			case STAGE_TEST_EXIT:
				Stage_Test_Exit_Handle(report, battery);
				break;
			case STAGE_RESET:
				NVIC_SystemReset();
				break;
			default:
				break;
			}
			break;
		default:
//			Screen_Status_Draw("ERROR", "Reset the device");
//			stage = STAGE_START;
			break;
	}
}

void App_Set_Stage(App_Stage_t stage_new) {
	stage = stage_new;
}

static void Stage_Start_Enter_Handle(HID_Report_t* report, uint8_t battery) {
	Buzzer_Play_Connected();
	Screen_Header_Draw("INIT", battery);
	Screen_Status_Draw("Connected !", "Press B to start");
	stage = STAGE_START;
}

static void Stage_Start_Handle(HID_Report_t* report, uint8_t battery) {
	if(report->BTN_B) {
		stage = STAGE_START_EXIT;
	}
}

static void Stage_Start_Exit_Handle(HID_Report_t* report, uint8_t battery) {
	stage = STAGE_MAINMENU_ENTER;
}
