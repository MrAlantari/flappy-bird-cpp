/*
 * lcd.h
 *
 *  Created on: 12 paz 2024
 *      Author: daniel
 */

#ifndef LCD_H_
#define LCD_H_

#include <math.h>
#include "fsl_lpspi.h"
#include "fsl_gpio.h"
#include "pin_mux.h"

#define LCD_WIDTH  160
#define LCD_HEIGHT 128

#define LCD_RST_0 GPIO_PinWrite(BOARD_INITLCDPINS_LCD_RST_GPIO, BOARD_INITLCDPINS_LCD_RST_GPIO_PIN, 0)
#define LCD_RST_1 GPIO_PinWrite(BOARD_INITLCDPINS_LCD_RST_GPIO, BOARD_INITLCDPINS_LCD_RST_GPIO_PIN, 1)

#define LCD_DC_0 GPIO_PinWrite(BOARD_INITLCDPINS_LCD_DC_GPIO, BOARD_INITLCDPINS_LCD_DC_GPIO_PIN, 0)
#define LCD_DC_1 GPIO_PinWrite(BOARD_INITLCDPINS_LCD_DC_GPIO, BOARD_INITLCDPINS_LCD_DC_GPIO_PIN, 1)

typedef struct {
	const uint8_t width;
	uint8_t height;
	const uint16_t *data;
} FontDef;

extern FontDef Font_7x10;
extern FontDef Font_11x18;
extern FontDef Font_16x26;

struct ScreenPosition {
    uint16_t x{};
    uint16_t y{};
};

struct SourceRegion {
    uint16_t left{};
    uint16_t right{};
    uint16_t top{};
    uint16_t bottom{};
};

class Image {
public:
    constexpr Image(const uint16_t* data, const uint16_t width, const uint16_t height)
    	: m_data{ data }, m_width{ width }, m_height{ height }
	{}

    void draw(
		const ScreenPosition& dest,
        const SourceRegion clip = {},
		const bool vFlip = false,
		const bool clipBlack = false
    ) const;

    uint16_t getWidth() const { return m_width; }
    uint16_t getHeight() const { return m_height; }

private:
    const uint16_t* const m_data{};
	const uint16_t m_width{};
  	const uint16_t m_height{};
};

void LCD_Init(LPSPI_Type *base);
void LCD_Clear(uint16_t color);
void LCD_GramRefresh(void);
void LCD_Draw_Point(uint16_t x, uint16_t y, uint16_t color);
void LCD_Puts(uint16_t x, uint16_t y, const char *text, FontDef font, uint16_t color);

#endif /* LCD_H_ */
