#ifndef INC_BUZZER_H_
#define INC_BUZZER_H_

#include "main.h"

extern TIM_HandleTypeDef BUZZER_TIM_HANDLE;
extern TIM_HandleTypeDef BUZZER_TIM2_HANDLE;

void Buzzer_Play_Boot(void);
void Buzzer_Play_Connected(void);
void Buzzer_Play_Menu_Move(void);
void Buzzer_Play_Menu_Back(void);
void Buzzer_Play_Game_Over(void);
void Buzzer_Play_Snake_Food(void);

#endif /* INC_BUZZER_H_ */
