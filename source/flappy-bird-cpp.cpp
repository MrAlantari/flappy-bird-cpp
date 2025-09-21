/*
 * Copyright 2016-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    flappy-bird-cpp.cpp
 * @brief   Application entry point.
 */
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <ctime>

#include <algorithm>

#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

#include "lcd.h"

#include "images/pipe_green.h"
#include "images/yellowbird_midflap.h"
#include "images/title_screen.h"
#include "images/game_over.h"
#include "images/background.h"

enum GameState {
    titleScreen,
    playing,
    gameOver,
};

enum PipeInteraction {
    none,
    colliding,
    passingThrough,
};

class Player {
public:
	void draw() const {
		s_playerImage.draw({ m_xPos, m_yPos }, {}, false, true);
	}

	void fall() {
		if (m_yPos < LCD_HEIGHT - s_playerImage.getHeight()) {
			m_yPos += s_gravity;
		}
	}

	void fly() {
		if (m_yPos > s_gravity) {
			m_yPos -= s_gravity;
		}
	}

    void reset() {
        m_yPos = s_initialYPos;
    }

    uint16_t getX() const { return m_xPos; }
    uint16_t getY() const { return m_yPos; }
    uint16_t getWidth() const { return s_playerImage.getWidth(); }
    uint16_t getHeight() const { return s_playerImage.getHeight(); }

private:
    static constexpr Image    s_playerImage{ yellowbird_midflap_25x17, 25, 17 };
	static constexpr uint16_t s_gravity{ 3 };
    static constexpr uint16_t s_initialYPos{ 47 };

    uint16_t m_xPos{ 20 };
	uint16_t m_yPos{ s_initialYPos };
};

class PipeGate {
public:
	PipeGate(const int16_t xOffset = LCD_WIDTH) {
		reset(xOffset);
	}

	void update() {
		constexpr int16_t moveSpeed{ 2 };
		m_xOffset -= moveSpeed;

		if (m_xOffset <= -static_cast<int16_t>(s_pipeImage.getWidth())) {
			reset(LCD_WIDTH);
		}
	}

	void draw() const {
		drawUpperPipe();
		drawLowerPipe();
	}

	PipeInteraction checkInteraction(const Player& player) {
        const uint16_t pipeRight{ static_cast<uint16_t>(m_xOffset + s_pipeImage.getWidth()) };
        const uint16_t pipeLeft { static_cast<uint16_t>(m_xOffset) };

        if (player.getX() > pipeRight || (player.getX() + player.getWidth()) < pipeLeft)
            return PipeInteraction::none;

        const bool upperCollision{ player.getY() < m_gateY };
        const bool lowerCollision{ player.getY() + player.getHeight() > m_gateY + s_gapHeight };
        if (upperCollision || lowerCollision)
            return PipeInteraction::colliding;

        if (m_isScored)
            return PipeInteraction::none;

        m_isScored = true;
        return PipeInteraction::passingThrough;
    }

	void reset(const int16_t startX = LCD_WIDTH) {
		const uint16_t maxValidY{ LCD_HEIGHT - s_gapHeight - s_minMargin };
		m_gateY    = getRandomInRange(s_minMargin, maxValidY);
		m_xOffset  = startX;
		m_isScored = false;
	}

private:
    static constexpr Image    s_pipeImage{ pipe_green_21x128, 21, 128 };
	static constexpr uint16_t s_gapHeight{ 45 };
	static constexpr uint16_t s_minMargin{ 20 };

    static uint16_t getRandomInRange(uint16_t min, uint16_t max) {
		return static_cast<uint16_t>(min + (std::rand() % (max - min + 1)));
	}

	void drawUpperPipe() const {
		const uint16_t upperPipeHeight{ m_gateY };
		const uint16_t visibleHeight  { std::min(upperPipeHeight, s_pipeImage.getHeight()) };

		const SourceRegion clip{
			.left   = static_cast<uint16_t>(std::max<int16_t>(-m_xOffset, 0)),
            .right  = static_cast<uint16_t>(std::max<int16_t>((m_xOffset + s_pipeImage.getWidth()) - LCD_WIDTH, 0)),
            .top    = static_cast<uint16_t>(s_pipeImage.getHeight() - visibleHeight),
            .bottom = 0,
		};

		const ScreenPosition pos{
			.x = static_cast<uint16_t>(std::max<int16_t>(m_xOffset, 0)),
            .y = 0,
		};

		s_pipeImage.draw(pos, clip, true);
	}

	void drawLowerPipe() const {
		const uint16_t lowerPipeStart { m_gateY + s_gapHeight };
		const uint16_t lowerPipeHeight{ LCD_HEIGHT - lowerPipeStart };
		const uint16_t visibleHeight  { std::min(lowerPipeHeight, s_pipeImage.getHeight()) };

		const SourceRegion clip{
			.left   = static_cast<uint16_t>(std::max<int16_t>(-m_xOffset, 0)),
            .right  = static_cast<uint16_t>(std::max<int16_t>((m_xOffset + s_pipeImage.getWidth()) - LCD_WIDTH, 0)),
            .top    = 0,
            .bottom = s_pipeImage.getHeight() - visibleHeight,
		};

		const ScreenPosition pos{
			.x = static_cast<uint16_t>(std::max<int16_t>(m_xOffset, 0)),
			.y = lowerPipeStart,
		};

		s_pipeImage.draw(pos, clip);
	}

    int16_t  m_xOffset{};
	uint16_t m_gateY{};
    bool     m_isScored{};
};

class ScrollingBackground {
public:
    void update() {
        constexpr int16_t scrollSpeed{ 1 };
        m_offset -= scrollSpeed;

        if (m_offset <= -static_cast<int16_t>(s_bgImage.getWidth() / 2)) {
            m_offset += s_bgImage.getWidth() / 2;
        }
    }

    void draw() const {
        const ScreenPosition pos1{ static_cast<uint16_t>(m_offset), 0 };
        s_bgImage.draw(pos1);
    }

private:
    static constexpr Image s_bgImage{ background_320x128, 320, 128 };
    int16_t m_offset{};
};

int main() {
	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	/* Init FSL debug console. */
	BOARD_InitDebugConsole();
#endif
    LCD_Init(LP_FLEXCOMM0_PERIPHERAL);

    uint16_t score{};
    char scoreString[4]{};

    GameState gameState{ GameState::titleScreen };
    ScrollingBackground background{};

	Player player{};
	PipeGate pipeGate1{ 70 };
	PipeGate pipeGate2{};

    Image titleScreenImage{ title_screen_160x128, 160, 128 };
    Image gameOverImage   { game_over_160x128, 160, 128 };

	while(1) {
        const bool buttonPressed{ GPIO_PinRead(BOARD_INITBUTTONSPINS_SW3_GPIO, BOARD_INITBUTTONSPINS_SW3_GPIO_PIN) == 0 };

        LCD_Clear(0x0000);
        switch (gameState) {
            case GameState::titleScreen: {
                background.draw();
                titleScreenImage.draw({}, {}, false, true);
                if (buttonPressed) {
                    gameState = GameState::playing;
                }
            } break;
            case GameState::playing: {
                if (buttonPressed) {
                    player.fly();
                } else {
                    player.fall();
                }

                background.draw();
                player.draw();
                pipeGate1.draw();
                pipeGate2.draw();

                pipeGate1.update();
                pipeGate2.update();
                background.update();

                const uint16_t scoreX{ (LCD_WIDTH - (3 * 11)) / 2 };
                snprintf(scoreString, sizeof(scoreString), "%03u", score);
                LCD_Puts(scoreX, 10, scoreString, Font_11x18, 0xffff);

                const auto gateInteraction{ pipeGate1.checkInteraction(player) | pipeGate2.checkInteraction(player) };
                switch (gateInteraction) {
                    case PipeInteraction::colliding:
                        gameState = GameState::gameOver;
                        break;
                    case PipeInteraction::passingThrough:
                        ++score;
                        break;
                    case PipeInteraction::none:
                    default:
                        break;
                }
            } break;
            case GameState::gameOver: {
                background.draw();
                gameOverImage.draw({}, {}, false, true);

                const uint16_t scoreX{ (LCD_WIDTH - (3 * 11)) / 2 };
                snprintf(scoreString, sizeof(scoreString), "%03u", score);
                LCD_Puts(scoreX, 10, scoreString, Font_11x18, 0xffff);

                if (buttonPressed) {
                    gameState = GameState::playing;
                    score = 0;
                    player.reset();
                    pipeGate1.reset(70);
                    pipeGate2.reset();
                }
            } break;
            default:
                break;
        }
        LCD_GramRefresh();
	}
	return 0 ;
}
