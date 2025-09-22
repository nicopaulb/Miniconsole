#ifndef __STAGE_H__
#define __STAGE_H__

#include "hid_host_app.h"

void Stage_MainMenu_Enter_Handle(HID_Report_t* report, uint8_t battery);
void Stage_MainMenu_Handle(HID_Report_t* report, uint8_t battery);
void Stage_MainMenu_Exit_Handle(HID_Report_t* report, uint8_t battery);
void Stage_Snake_Enter_Handle(HID_Report_t* report, uint8_t battery);
void Stage_Snake_Handle(HID_Report_t* report, uint8_t battery);
void Stage_Snake_Exit_Handle(HID_Report_t* report, uint8_t battery);
void Stage_Test_Enter_Handle(HID_Report_t* report, uint8_t battery);
void Stage_Test_Handle(HID_Report_t* report, uint8_t battery);
void Stage_Test_Exit_Handle(HID_Report_t* report, uint8_t battery);

#endif /* __STAGE_H__ */
