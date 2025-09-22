#include "app.h"
#include "screen.h"
#include "hid_host_app.h"
#include "st7735.h"
#include "icons.h"

#define STAGE_TITLE "SNAKE"

#define BASE_TICK_TIME 150

#define SCREEN_BORDER_SIZE 4
#define SCREEN_CELL_SIZE 5
#define SCREEN_HEIGHT 90
#define SCREEN_WIDTH 140

#define GRID_WIDTH (SCREEN_WIDTH / SCREEN_CELL_SIZE)
#define GRID_HEIGHT (SCREEN_HEIGHT / SCREEN_CELL_SIZE)
#define GRID_OFFSET_X(VAL) (((ST7735_WIDTH - SCREEN_WIDTH) / 2) + (VAL*SCREEN_CELL_SIZE))
#define GRID_OFFSET_Y(VAL) (((ST7735_HEIGHT + SCREEN_HEADER_HEIGHT - SCREEN_HEIGHT) / 2) + (VAL * SCREEN_CELL_SIZE))

#define COLOR_SNAKE ST7735_WHITE
#define COLOR_FOOD  ST7735_GREEN

#define INVALID_COORD ((coord_t){UINT8_MAX, UINT8_MAX})
#define DIR_UP DIRS[0]
#define DIR_DOWN DIRS[1]
#define DIR_LEFT DIRS[2]
#define DIR_RIGHT DIRS[3]

typedef enum cell {
	CELL_SNAKE_MIN = 0, CELL_SNAKE_MAX = GRID_WIDTH * GRID_HEIGHT, CELL_FOOD, CELL_EMPTY
} cell_t;

typedef struct coord {
	uint8_t x;
	uint8_t y;
} coord_t;

static const coord_t DIRS[4] = { { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } };
static cell_t grid[GRID_HEIGHT][GRID_WIDTH];
static bool round_running;
static uint32_t tick_count;
static coord_t head;
static coord_t tail;
static uint32_t body_length;

static cell_t get_cell(coord_t *coord) {
	return grid[coord->y][coord->x];
}

static void set_cell(coord_t *coord, cell_t value) {
	grid[coord->y][coord->x] = value;
}

static coord_t spawn_food(void) {
	coord_t spawn_candidate = { GRID_WIDTH / 2 + 2, GRID_HEIGHT / 2 };
	if (body_length < GRID_WIDTH * GRID_HEIGHT) {
		do {
			spawn_candidate = (coord_t ) { rand() % GRID_WIDTH, rand() % GRID_HEIGHT };
		} while (get_cell(&spawn_candidate) != CELL_EMPTY);
		set_cell(&spawn_candidate, CELL_FOOD);
	} else {
		spawn_candidate = INVALID_COORD;
	}

	return spawn_candidate;
}

static bool is_wall_collided(coord_t *new_head) {
	return new_head->x < 0 || new_head->x >= GRID_WIDTH || new_head->y < 0 || new_head->y >= GRID_HEIGHT;
}

static bool is_body_collided(coord_t *new_head) {
	cell_t cell = get_cell(new_head);
	return cell >= CELL_SNAKE_MIN && cell <= CELL_SNAKE_MAX;
}

static coord_t get_direction(HID_Report_t *report) {
	static const coord_t *cur_dir = &DIR_RIGHT;
	const coord_t* new_dir = cur_dir;

	// Handle HAT switch
	switch (report->HAT_Switch) {
		case HATSWITCH_UP:
		new_dir = &DIR_UP;
		case HATSWITCH_UPRIGHT:
		case HATSWITCH_RIGHT:
		case HATSWITCH_DOWNRIGHT:
		new_dir = &DIR_UP;
		case HATSWITCH_DOWN:
		new_dir = &DIR_DOWN;
		case HATSWITCH_DOWNLEFT:
		case HATSWITCH_LEFT:
		case HATSWITCH_UPLEFT:
		new_dir = &DIR_DOWN;
	}

	// Also handles Y/B/A/X buttons
	if (report->BTN_A) {
		new_dir = &DIR_DOWN;
	} else if (report->BTN_B) {
		new_dir = &DIR_RIGHT;
	} else if (report->BTN_X) {
		new_dir = &DIR_LEFT;
	} else if (report->BTN_Y) {
		new_dir = &DIR_UP;
	}

	// Also handles both joysticks
	if (report->JOY_LeftAxisY < 0x007F || report->JOY_RightAxisY < 0x007F) {
		new_dir = &DIR_UP;
	} else if (report->JOY_LeftAxisY > 0xCFFF || report->JOY_RightAxisY > 0xCFFF) {
		new_dir = &DIR_DOWN;
	} else if (report->JOY_LeftAxisX < 0x007F || report->JOY_RightAxisX < 0x007F) {
		new_dir = &DIR_LEFT;
	} else if (report->JOY_LeftAxisX > 0xCFFF || report->JOY_RightAxisX > 0xCFFF) {
		new_dir = &DIR_RIGHT;
	}

	// Prevent going to opposite direction
	if ((cur_dir == &DIR_UP && new_dir == &DIR_DOWN ) || (cur_dir == &DIR_DOWN && new_dir == &DIR_UP )
			|| (cur_dir == &DIR_LEFT && new_dir == &DIR_RIGHT ) || (cur_dir == &DIR_RIGHT && new_dir == &DIR_LEFT)) {
		return *cur_dir;
	}

	cur_dir = new_dir;
	return *cur_dir;
}

static void move_snake(coord_t *new_head) {
	uint32_t old_tail_id = get_cell(&tail);

	// If food is not eaten, tail is removed
	if (get_cell(new_head) != CELL_FOOD) {
		set_cell(&tail, CELL_EMPTY);
		ST7735_FillRectangle(GRID_OFFSET_X(tail.x), GRID_OFFSET_Y(tail.y), SCREEN_CELL_SIZE, SCREEN_CELL_SIZE,
		ST7735_BLACK);
		if (body_length == 1) {
			tail = *new_head;
		} else {
			// Find new tail based on tick count marker
			for (int i = 0; i < 4; i++) {
				coord_t new_tail = { tail.x + DIRS[i].x, tail.y + DIRS[i].y };
				if (new_tail.x < 0 || new_tail.x >= GRID_WIDTH || new_tail.y < 0 || new_tail.y >= GRID_HEIGHT) {
					// Out of bound
					continue;
				}
				if (get_cell(&new_tail) == ((old_tail_id + 1))) {
					// New tail found
					tail = new_tail;
					break;
				}
			}
		}
	} else {
		body_length++;
		coord_t food = spawn_food();
		ST7735_FillRectangle(GRID_OFFSET_X(food.x), GRID_OFFSET_Y(food.y), SCREEN_CELL_SIZE, SCREEN_CELL_SIZE,
		ST7735_GREEN);
	}

	// Update head to new cell
	head = *new_head;
	set_cell(&head, tick_count++);
	ST7735_FillRectangle(GRID_OFFSET_X(head.x), GRID_OFFSET_Y(head.y), SCREEN_CELL_SIZE, SCREEN_CELL_SIZE,
	ST7735_WHITE);
}

static void update_screen(void) {
	for (int y = 0; y < GRID_HEIGHT; y++) {
		for (int x = 0; x < GRID_WIDTH; x++) {
			switch (get_cell(&(coord_t ) { x, y })) {
			case CELL_EMPTY:
				ST7735_FillRectangle(GRID_OFFSET_X(x), GRID_OFFSET_Y(y), SCREEN_CELL_SIZE, SCREEN_CELL_SIZE,
				ST7735_BLACK);
				break;
			case CELL_FOOD:
				ST7735_FillRectangle(GRID_OFFSET_X(x), GRID_OFFSET_Y(y), SCREEN_CELL_SIZE, SCREEN_CELL_SIZE,
				COLOR_FOOD);
				break;
			default:
				ST7735_FillRectangle(GRID_OFFSET_X(x), GRID_OFFSET_Y(y), SCREEN_CELL_SIZE, SCREEN_CELL_SIZE,
				COLOR_SNAKE);
				break;
			}
		}
	}
}

static void start_round(void) {
	// Initialize empty grid
	for (int i = 0; i < GRID_WIDTH; i++)
		for (int j = 0; j < GRID_HEIGHT; j++)
			grid[j][i] = CELL_EMPTY;

	body_length = 1;
	tick_count = 0;
	round_running = true;

	// Set snake in the middle
	head.x = GRID_WIDTH / 2;
	head.y = GRID_HEIGHT / 2;
	set_cell(&head, CELL_SNAKE_MIN);
	tail = head;

	spawn_food();
	update_screen();
}

static void end_round(void) {
	char score[20];
	snprintf(score, 20, "Score : %lu", body_length);
	ST7735_WriteString(ST7735_CENTERED, (ST7735_HEIGHT - 7) / 2 - 10, "GAME OVER", Font_11x18, ST7735_WHITE,
	ST7735_RED);
	ST7735_WriteString(ST7735_CENTERED, (ST7735_HEIGHT - 7) / 2 + 10, score, Font_7x10, ST7735_WHITE, ST7735_RED);
	ST7735_WriteString(ST7735_CENTERED, (ST7735_HEIGHT - 7) / 2 + 30, "Press B to restart", Font_7x10, ST7735_WHITE,
	ST7735_BLACK);
	round_running = false;
}

void Stage_Snake_Enter_Handle(HID_Report_t *report, uint8_t battery) {
	Screen_Header_Draw(STAGE_TITLE, battery);
	Screen_Background_Draw();
	ST7735_FillRectangle((ST7735_WIDTH - (SCREEN_WIDTH + SCREEN_BORDER_SIZE)) / 2,
			((ST7735_HEIGHT + SCREEN_HEADER_HEIGHT) - (SCREEN_HEIGHT + SCREEN_BORDER_SIZE)) / 2,
			SCREEN_WIDTH + SCREEN_BORDER_SIZE, SCREEN_HEIGHT + SCREEN_BORDER_SIZE, ST7735_WHITE);
	ST7735_FillRectangle((ST7735_WIDTH - SCREEN_WIDTH) / 2, (ST7735_HEIGHT + SCREEN_HEADER_HEIGHT - SCREEN_HEIGHT) / 2,
	SCREEN_WIDTH, SCREEN_HEIGHT, ST7735_BLACK);
	ST7735_WriteString(ST7735_CENTERED, (ST7735_HEIGHT + SCREEN_HEADER_HEIGHT) / 2, "Press B to start", Font_7x10, ST7735_WHITE,
	ST7735_BLACK);
	App_Set_Stage(STAGE_SNAKE);
}

void Stage_Snake_Handle(HID_Report_t *report, uint8_t battery) {
	if (report->BTN_B && !round_running) {
		start_round();
	}

	if (report->BTN_Xbox) {
		round_running = false;
		App_Set_Stage(STAGE_SNAKE_EXIT);
	}

	if (round_running) {
		static long long last_tick = 0;
		long long cur_tick = HAL_GetTick();
		coord_t dir = get_direction(report);

		if (cur_tick - last_tick > BASE_TICK_TIME) {
			last_tick = cur_tick;
			coord_t new_head = { head.x + dir.x, head.y + dir.y };
			// Check for collision
			if (is_wall_collided(&new_head) || is_body_collided(&new_head)) {
				end_round();
			} else {
				move_snake(&new_head);
			}
		}
	}
}

void Stage_Snake_Exit_Handle(HID_Report_t *report, uint8_t battery) {
	App_Set_Stage(STAGE_MAINMENU_ENTER);
}
