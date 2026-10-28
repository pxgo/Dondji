/* SPDX-License-Identifier: Apache-2.0 */
#include "game.h"

#include <string.h>
#include "app/game.h"
#include "app/piano.h"
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

static void DrawLabel(const char *text, uint8_t y, uint8_t left)
{
#ifdef ENABLE_CHINESE
    /* This legacy text API takes LCD coordinates, including the 8px top bar. */
    UI_PrintStringSmallAtPixel(text, left, LCD_WIDTH - 1, y + 8u, y + 19u, 0);
#else
    UI_PrintStringSmallNormal(text, left, LCD_WIDTH - 1, y / 8u);
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

static void DrawPiano(void)
{
    static const char notes[PIANO_NOTE_COUNT][3] = {
        "C4", "D4", "E4", "F4", "G4", "A4", "B4",
        "C5", "D5", "E5", "F5", "G5"
    };
    DrawLabel(UI_GAME_UseChinese() ? "电子琴" : "Piano", 0, 0);
    UI_PrintStringSmallNormal("1:do 2:ri 3:mi", 0, 127, 2);
    UI_PrintStringSmallNormal("4:so 5:la 6:xi", 0, 127, 3);
    UI_PrintStringSmallNormal("7:ri+ 8:mi+ 9:fa+", 0, 127, 4);
    UI_PrintStringSmallNormal("*:fa 0:do+ #:so+", 0, 127, 5);
    const uint8_t note = PIANO_CurrentNote();
    if (note < PIANO_NOTE_COUNT) {
        char text[20];
        snprintf(text, sizeof(text), "%s %uHz E:Back", notes[note], (unsigned int)PIANO_Frequency(note));
        UI_PrintStringSmallNormal(text, 0, 127, 6);
    } else {
        UI_PrintStringSmallNormal("Hold key E:Back", 0, 127, 6);
    }
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
        DrawLabel(UI_GAME_Title(), 0, 0);
        DrawLabel(UI_GAME_UseChinese() ? "贪吃蛇" : "Snake", 16, 16);
        DrawLabel(UI_GAME_UseChinese() ? "电子琴" : "Piano", 32, 16);
        const uint8_t selectedY = gGameSelection == GAME_SNAKE ? 16 : 32;
#ifdef ENABLE_CHINESE
        UI_PrintStringSmallAtPixel(">", 4, 4, selectedY + 8u, selectedY + 19u, 0);
#else
        UI_PrintStringSmallNormalAt(">", 4, selectedY);
#endif
        UI_PrintStringSmallNormal("M:Start E:Back", 0, 127, 6);
    } else if (gGameSelection == GAME_PIANO) {
        DrawPiano();
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
