#include "st7735.h"
#include "malloc.h"
#include "string.h"

#define DELAY 0x80
#define ST7735_SELECT() 	HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_RESET)
#define ST7735_UNSELECT() 	HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_SET)

extern SPI_HandleTypeDef ST7735_SPI_PORT;

const uint8_t memoryAccessDirectionInit = ST7735_MADCTL_MY | ST7735_MADCTL_MV;
const uint8_t colorModeInit = ST7735_COLMOD_16BIT;
const uint8_t columnAddressInit[4] = {0x00, 0x00, /* XSTART */ 0x00, ST7735_WIDTH /* XEND */};
const uint8_t rowAddressInit[4] = {0x00, 0x00, /* YSTART */ 0x00, ST7735_HEIGHT /* YEND */};

static void ST7735_Reset() {
    HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_SET);
}

static void ST7735_WriteCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&ST7735_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}

static void ST7735_WriteData(uint8_t* buff, size_t buff_size) {
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(&ST7735_SPI_PORT, buff, buff_size, HAL_MAX_DELAY);
}

static void ST7735_ExecuteCommand(uint8_t cmd, const uint8_t *args,  size_t numArgs) {
	 ST7735_WriteCommand(cmd);
	 if(numArgs > 0) {
		 ST7735_WriteData((uint8_t*)args, numArgs);
	 }
}

static void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    uint8_t columnAddress[4] = {0x00, x0 + ST7735_XSTART, 0x00, x1 + ST7735_XSTART };
    uint8_t rowAddress[4] = {0x00, y0 + ST7735_YSTART, 0x00, y1 + ST7735_YSTART };

    // Set RAM pointer
    ST7735_ExecuteCommand(ST7735_CASET, columnAddress, sizeof(columnAddress));
    ST7735_ExecuteCommand(ST7735_RASET, rowAddress, sizeof(rowAddress));

    // Write to RAM
    ST7735_ExecuteCommand(ST7735_RAMWR, NULL, 0);
}

void ST7735_Init() {
	ST7735_SELECT();

    ST7735_Reset();
    ST7735_ExecuteCommand(ST7735_SWRESET, NULL, 0);
    HAL_Delay(150);
    ST7735_ExecuteCommand(ST7735_SLPOUT, NULL, 0);
    HAL_Delay(500);
    ST7735_ExecuteCommand(ST7735_MADCTL, &memoryAccessDirectionInit, sizeof(memoryAccessDirectionInit));
    ST7735_ExecuteCommand(ST7735_COLMOD, &colorModeInit, sizeof(colorModeInit));
    ST7735_ExecuteCommand(ST7735_CASET, columnAddressInit, sizeof(columnAddressInit));
    ST7735_ExecuteCommand(ST7735_RASET, rowAddressInit, sizeof(rowAddressInit));
    ST7735_ExecuteCommand(ST7735_NORON, NULL, 0);
    HAL_Delay(10);
    ST7735_ExecuteCommand(ST7735_DISPON, NULL, 0);
    HAL_Delay(100);

    ST7735_UNSELECT();
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT))
        return;

    ST7735_SELECT();

    ST7735_SetAddressWindow(x, y, x+1, y+1);
    uint8_t data[] = { color >> 8, color & 0xFF };
    ST7735_WriteData(data, sizeof(data));

    ST7735_UNSELECT();
}

static void ST7735_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
    uint32_t i, b, j;

    ST7735_SetAddressWindow(x, y, x+font.width-1, y+font.height-1);

    for(i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < font.width; j++) {
            if((b << j) & 0x8000)  {
                uint8_t data[] = { color >> 8, color & 0xFF };
                ST7735_WriteData(data, sizeof(data));
            } else {
                uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF };
                ST7735_WriteData(data, sizeof(data));
            }
        }
    }
}

void ST7735_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor) {
	size_t str_len = strlen(str);

	ST7735_SELECT();

	if(x == ST7735_CENTERED) {
		x = (ST7735_WIDTH - str_len*font.width)/2;
	}

	if(y == ST7735_CENTERED) {
		y = (ST7735_HEIGHT - str_len*font.height)/2;
	}

    while(*str) {
        if(x + font.width >= ST7735_WIDTH) {
            x = 0;
            y += font.height;
            if(y + font.height >= ST7735_HEIGHT) {
                break;
            }

            if(*str == ' ') {
                str++;
                continue;
            }
        }

        ST7735_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }

    ST7735_UNSELECT();
}

void ST7735_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	uint16_t line[ST7735_WIDTH] = {0};
	uint8_t pixel[] = { color >> 8, color & 0xFF };

	if((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
	if((x + w - 1) >= ST7735_WIDTH) w = ST7735_WIDTH - x;
	if((y + h - 1) >= ST7735_HEIGHT) h = ST7735_HEIGHT - y;

	ST7735_SELECT();
	ST7735_SetAddressWindow(x, y, x+w-1, y+h-1);

	for(x = 0; x < w; ++x)
		memcpy(line + x, pixel, sizeof(pixel));

	HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
	for(y = h; y > 0; y--)
		HAL_SPI_Transmit(&ST7735_SPI_PORT, (uint8_t*) line, w * sizeof(pixel), HAL_MAX_DELAY);

	ST7735_UNSELECT();
}

void ST7735_FillScreen(uint16_t color) {
    ST7735_FillRectangle(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
    if(x == ST7735_CENTERED) {
    	x = (ST7735_WIDTH - w)/2;
    }

	if(y == ST7735_CENTERED) {
		y = (ST7735_HEIGHT - h)/2;
	}

	if((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
    if((x + w - 1) >= ST7735_WIDTH) return;
    if((y + h - 1) >= ST7735_HEIGHT) return;

    ST7735_SELECT();
    ST7735_SetAddressWindow(x, y, x+w-1, y+h-1);
    ST7735_WriteData((uint8_t*)data, sizeof(uint16_t)*w*h);
    ST7735_UNSELECT();
}

void ST7735_InvertColors(bool invert) {
	ST7735_SELECT();
	ST7735_ExecuteCommand(invert ? ST7735_INVON : ST7735_INVOFF, NULL, 0);
    ST7735_UNSELECT();
}

void ST7735_SetGamma(GammaDef gamma)
{
	ST7735_SELECT();
	ST7735_ExecuteCommand(ST7735_GAMSET, (uint8_t *) &gamma, sizeof(gamma));
	ST7735_UNSELECT();
}



