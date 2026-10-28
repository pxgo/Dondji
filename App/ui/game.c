/* SPDX-License-Identifier: Apache-2.0 */
#include "game.h"

#include <string.h>
#include "app/game.h"
#include "driver/st7565.h"
#ifdef ENABLE_CHINESE
#include "driver/py25q16.h"
#endif
#include "external/printf/printf.h"
#include "helper.h"
#include "helper/battery.h"
#include "settings.h"

bool UI_GAME_UseChinese(void)
{
#ifdef ENABLE_CHINESE
    if (gUiLanguage == UI_LANGUAGE_CN) {
        uint8_t version;
        PY25Q16_ReadBuffer(CN_FONT_FLASH_BASE + CN_FONT_VERSION_OFFSET, &version, 1);
        return version == CN_FONT_VERSION && SETTINGS_CNCharToIndex(0x6E38) >= 0;
    }
#endif
    return false;
}

const char *UI_GAME_Title(void)
{
    return UI_GAME_UseChinese() ? "游戏" : "Games";
}

static void DrawLabel(const char *text, uint8_t y)
{
#ifdef ENABLE_CHINESE
    UI_PrintStringSmallAtPixel(text, 0, LCD_WIDTH - 1, y, y + 11u, 0);
#else
    UI_PrintStringSmallNormalAt(text, 0, y);
#endif
}

static void DrawCell(uint8_t cell, bool food)
{
    const uint8_t x = 16u + (cell % SNAKE_WIDTH) * 4u;
    const uint8_t y = 14u + (cell / SNAKE_WIDTH) * 4u;
    for (uint8_t row = 0; row < 3u; ++row)
        for (uint8_t col = 0; col < 3u; ++col)
            if (!food || row == 1u || col == 1u)
                UI_DrawPixelBuffer(gFrameBuffer, x + col, y + row, true);
}

void UI_DisplayGame(void)
{
    char text[22];
    UI_DisplayClear();
    memset(gStatusLine, 0, sizeof(gStatusLine));
    snprintf(text, sizeof(text), "RF OFF       %u%%", (unsigned int)gBatteryIconFillPercent);
    UI_PrintStringSmallBufferNormal(text, gStatusLine);
    ST7565_BlitStatusLine();

    if (gGameInList) {
        DrawLabel(UI_GAME_Title(), 0);
        DrawLabel(UI_GAME_UseChinese() ? "贪吃蛇" : "Snake", 20);
        UI_PrintStringSmallNormal("M:Start E:Back", 0, 127, 5);
        UI_PrintStringSmallNormal("Move:2/4/6/8", 0, 127, 6);
    } else {
        snprintf(text, sizeof(text), "Score:%u", (unsigned int)gSnakeGame.score);
        UI_PrintStringSmallNormalAt(text, 0, 0);
        UI_PrintStringSmallNormalAt("M:Pause", 76, 0);
        UI_DrawRectangleBuffer(gFrameBuffer, 15, 13, 112, 54, true);
        for (uint8_t i = 0; i < gSnakeGame.length; ++i)
            DrawCell(SNAKE_CellAt(&gSnakeGame, i), false);
        if (gSnakeGame.food < SNAKE_CAPACITY)
            DrawCell(gSnakeGame.food, true);
        if (gSnakeGame.status != SNAKE_RUNNING) {
            memset(gFrameBuffer[3], 0, LCD_WIDTH);
            memset(gFrameBuffer[4], 0, LCD_WIDTH);
            const char *title = (gSnakeGame.status == SNAKE_PAUSED) ? "PAUSED"
                              : (gSnakeGame.status == SNAKE_WON) ? "YOU WIN" : "GAME OVER";
            UI_PrintStringSmallNormal(title, 0, 127, 3);
            UI_PrintStringSmallNormal(gSnakeGame.status == SNAKE_PAUSED ? "MENU:Continue" : "MENU:Restart", 0, 127, 4);
        }
    }
    ST7565_BlitFullScreen();
}
