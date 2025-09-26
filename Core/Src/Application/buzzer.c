#include "buzzer.h"

#define TONE_BUF_SIZE 255
#define TIMR_FREQ 32000000
#define TIMR_ARR 1000
#define TIMR_PRS(freq) ((TIMR_FREQ/(TIMR_ARR*freq))-1)

typedef struct tone {
	const uint16_t *frequency;
	const uint16_t *duration;
	uint32_t size;
} tone_t;

static tone_t tone_buf;
static uint32_t tone_id;

static const uint16_t boot_freqs[] = { 523, 659, 784, 1047 };
static const uint16_t boot_durations[] = { 150, 150, 150, 300 };

const uint16_t connected_freqs[] = {  880, 988 };
const uint16_t connected_durations[] = { 80, 100 };

const uint16_t menu_move_freqs[] = { 880, 988 };
const uint16_t menu_move_durations[] = { 80, 100 };

const uint16_t menu_back_freqs[] = { 988, 880 };
const uint16_t menu_back_durations[] = { 80, 100 };

const uint16_t game_over_freqs[] = { 784, 659, 523 };
const uint16_t game_over_durations[] = { 150, 150, 300 };

const uint16_t snake_food_freqs[] = { 988};
const uint16_t snake_food_durations[] = { 100};

static void Buzzer_Play_Tone(uint16_t frequency, uint16_t duration) {
	__HAL_TIM_SET_COUNTER(&BUZZER_TIM2_HANDLE, duration);

	if (frequency != 0) {
		__HAL_TIM_SET_PRESCALER(&BUZZER_TIM_HANDLE, TIMR_PRS(frequency));
		HAL_TIM_PWM_Start(&BUZZER_TIM_HANDLE, TIM_CHANNEL_1);
	}

	__HAL_TIM_ENABLE_IT(&BUZZER_TIM2_HANDLE, TIM_IT_UPDATE);
	__HAL_TIM_ENABLE(&BUZZER_TIM2_HANDLE);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM2) {
		HAL_TIM_PWM_Stop(&BUZZER_TIM_HANDLE, TIM_CHANNEL_1);
		tone_id++;
		if (tone_id < tone_buf.size) {
			Buzzer_Play_Tone(tone_buf.frequency[tone_id], tone_buf.duration[tone_id]);
		}
	}
}

void Buzzer_Play(const uint16_t *tone, const uint16_t *duration, uint32_t size) {
	tone_buf.frequency = tone;
	tone_buf.duration = duration;
	tone_buf.size = size;
	tone_id = 0;

	// Play first tone
	Buzzer_Play_Tone(tone_buf.frequency[tone_id], tone_buf.duration[tone_id]);
}

void Buzzer_Play_Boot(void) {
	Buzzer_Play(boot_freqs, boot_durations, sizeof(boot_freqs) / sizeof(uint16_t));
}

void Buzzer_Play_Connected(void) {
	Buzzer_Play(connected_freqs, menu_move_durations, sizeof(connected_freqs) / sizeof(uint16_t));
}

void Buzzer_Play_Menu_Move(void) {
	Buzzer_Play(menu_move_freqs, menu_move_durations, sizeof(menu_move_freqs) / sizeof(uint16_t));
}

void Buzzer_Play_Menu_Back(void) {
	Buzzer_Play(menu_back_freqs, menu_back_durations, sizeof(menu_back_freqs) / sizeof(uint16_t));
}

void Buzzer_Play_Game_Over(void) {
	Buzzer_Play(game_over_freqs, game_over_durations, sizeof(game_over_freqs) / sizeof(uint16_t));
}

void Buzzer_Play_Snake_Food(void) {
	Buzzer_Play(snake_food_freqs, snake_food_durations, sizeof(snake_food_freqs) / sizeof(uint16_t));
}

