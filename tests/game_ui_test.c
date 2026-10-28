/* SPDX-License-Identifier: Apache-2.0 */
#include "game_stubs/runtime.h"
#include "../App/ui/game.h"
#include "../App/ui/helper.h"

static unsigned fullBlits, statusBlits, pixelCalls, cellPixelCalls;
static unsigned cellVisits[56][LCD_WIDTH];
static bool drawingBorder;
static uint8_t fontVersion = CN_FONT_VERSION;
static int16_t fontIndex = 1;
static unsigned fontReads;
typedef struct { char text[64]; size_t x, last, y; } TextCall;
static TextCall textCalls[12];
static size_t textCount;

static void CheckCanaries(void)
{
    for (size_t i = 0; i < sizeof(stubFrame.before); ++i) {
        assert(stubFrame.before[i] == 0xa5 && stubFrame.after[i] == 0x5a);
        assert(stubStatus.before[i] == 0xa5 && stubStatus.after[i] == 0x5a);
    }
}
void UI_DisplayClear(void) { memset(gFrameBuffer, 0, sizeof(gFrameBuffer)); }
void ST7565_BlitFullScreen(void) { CheckCanaries(); ++fullBlits; }
void ST7565_BlitStatusLine(void) { CheckCanaries(); ++statusBlits; }
void PY25Q16_ReadBuffer(uint32_t address, void *buffer, uint32_t size)
{
    assert(address == CN_FONT_FLASH_BASE + CN_FONT_VERSION_OFFSET && size == 1);
    *(uint8_t *)buffer = fontVersion;
    ++fontReads;
}
int16_t SETTINGS_CNCharToIndex(uint16_t character)
{
    assert(character == 0x6e38);
    return fontIndex;
}
void UI_DrawPixelBuffer(uint8_t (*buffer)[128], uint8_t x, uint8_t y, bool black)
{
    assert(buffer == gFrameBuffer && x < LCD_WIDTH && y <= 55);
    assert(black);
    buffer[y / 8u][x] |= (uint8_t)(1u << (y % 8u));
    ++pixelCalls;
    if (!drawingBorder) {
        ++cellPixelCalls;
        ++cellVisits[y][x];
    }
}
void UI_DrawRectangleBuffer(uint8_t (*buffer)[128], int16_t x1, int16_t y1,
                            int16_t x2, int16_t y2, bool black)
{
    assert(x1 == 15 && y1 == 13 && x2 == 112 && y2 == 54);
    drawingBorder = true;
    for (int16_t x = x1; x <= x2; ++x) {
        UI_DrawPixelBuffer(buffer, (uint8_t)x, (uint8_t)y1, black);
        UI_DrawPixelBuffer(buffer, (uint8_t)x, (uint8_t)y2, black);
    }
    for (int16_t y = (int16_t)(y1 + 1); y < y2; ++y) {
        UI_DrawPixelBuffer(buffer, (uint8_t)x1, (uint8_t)y, black);
        UI_DrawPixelBuffer(buffer, (uint8_t)x2, (uint8_t)y, black);
    }
    drawingBorder = false;
}

static void RecordText(const char *text, size_t x, size_t width, size_t y, size_t height)
{
    if (width == 0 || x + width > LCD_WIDTH) {
        fprintf(stderr, "text exceeds LCD: '%s', x=%zu, width=%zu\n", text, x, width);
        assert(width > 0 && x + width <= LCD_WIDTH);
    }
    assert(y + height <= 56);
    assert(textCount < sizeof(textCalls) / sizeof(textCalls[0]));
    TextCall *call = &textCalls[textCount++];
    assert(strlen(text) < sizeof(call->text));
    strcpy(call->text, text);
    call->x = x;
    call->last = x + width - 1u;
    call->y = y;
}
/* Match helper.c: six font columns, leading column +1, stride seven.
 * Deliberately do not clip: clipping would conceal invalid UI call sites. */
static void Latin(const char *text, size_t x, size_t y, uint8_t *row, bool status)
{
    size_t length = strlen(text);
    RecordText(text, x, length * 7u, status ? 0 : y, 7);
    for (size_t i = 0; i < length; ++i) {
        assert((unsigned char)text[i] >= ' ' && (unsigned char)text[i] < 127);
        if (text[i] == ' ')
            continue;
        for (size_t c = 0; c < 6; ++c) {
            size_t column = x + i * 7u + 1u + c;
            assert(column < LCD_WIDTH);
            row[column] = 0x7f; /* geometry only; glyph artwork is not emulated */
        }
    }
}
void UI_PrintStringSmallNormalAt(const char *text, uint8_t x, uint8_t y)
{
    assert(y / 8u < 7);
    Latin(text, x, (y / 8u) * 8u, gFrameBuffer[y / 8u], false);
}
void UI_PrintStringSmallNormal(const char *text, uint8_t start, uint8_t end, uint8_t line)
{
    size_t width = strlen(text) * 7u;
    assert(line < 7 && start <= end);
    /* Check before unsigned centering arithmetic can wrap as in helper.c. */
    RecordText(text, start, width, line * 8u, 7);
    --textCount;
    assert(width <= (size_t)end - start + 1u);
    size_t x = start;
    if (end > start)
        x += ((size_t)end - start - width + 1u) / 2u;
    Latin(text, x, line * 8u, gFrameBuffer[line], false);
}
void UI_PrintStringSmallBufferNormal(const char *text, uint8_t *buffer)
{
    assert(buffer == gStatusLine);
    Latin(text, 0, 0, buffer, true);
}
#ifdef ENABLE_CHINESE
void UI_PrintStringSmallAtPixel(const char *text, uint8_t start, uint8_t end,
                                uint8_t top, uint8_t bottom, uint8_t latinDown)
{
    size_t width = 0;
    assert(start <= end && top <= bottom && bottom <= 55 && latinDown == 0);
    for (const unsigned char *p = (const unsigned char *)text; *p;) {
        if (*p < 128) {
            width += 7;
            ++p;
        } else {
            assert((p[0] & 0xf0u) == 0xe0u && p[1] && p[2]);
            width += 12;
            p += 3;
        }
    }
    assert(width <= (size_t)end - start + 1u);
    RecordText(text, start, width, top, (size_t)bottom - top + 1u);
    /* Exercise the framebuffer writes for a 12px label without pretending
     * to reproduce font decoding/flash access. */
    for (size_t x = start; x < start + width; ++x)
        for (size_t y = top; y <= bottom; ++y)
            gFrameBuffer[y / 8u][x] |= (uint8_t)(1u << (y % 8u));
}
#endif

static bool HasText(const char *text)
{
    for (size_t i = 0; i < textCount; ++i)
        if (strcmp(textCalls[i].text, text) == 0)
            return true;
    return false;
}
static void Render(void)
{
    memset(&stubFrame, 0xcc, sizeof(stubFrame));
    memset(&stubStatus, 0xcc, sizeof(stubStatus));
    memset(stubFrame.before, 0xa5, sizeof(stubFrame.before));
    memset(stubFrame.after, 0x5a, sizeof(stubFrame.after));
    memset(stubStatus.before, 0xa5, sizeof(stubStatus.before));
    memset(stubStatus.after, 0x5a, sizeof(stubStatus.after));
    memset(cellVisits, 0, sizeof(cellVisits));
    textCount = pixelCalls = cellPixelCalls = fullBlits = statusBlits = 0;
    drawingBorder = false;
    UI_DisplayGame();
    CheckCanaries();
    assert(fullBlits == 1 && statusBlits == 1);
    assert(textCount >= 3);
    assert(strncmp(textCalls[0].text, "RF OFF", 6) == 0);
    for (size_t i = 0; i < textCount; ++i)
        assert(textCalls[i].x <= textCalls[i].last && textCalls[i].last < LCD_WIDTH);
}
static void CheckCell(uint8_t cell, bool food)
{
    unsigned x = 16u + (cell % SNAKE_WIDTH) * 4u;
    unsigned y = 14u + (cell / SNAKE_WIDTH) * 4u;
    for (unsigned row = 0; row < 3; ++row)
        for (unsigned col = 0; col < 3; ++col)
            assert(cellVisits[y + row][x + col] == ((!food || row == 1 || col == 1) ? 1u : 0u));
}
static void TestLanguage(void)
{
    gUiLanguage = 0;
    assert(!UI_GAME_UseChinese() && strcmp(UI_GAME_Title(), "Games") == 0);
#ifdef ENABLE_CHINESE
    gUiLanguage = UI_LANGUAGE_CN;
    fontVersion = 0;
    assert(!UI_GAME_UseChinese());
    fontVersion = CN_FONT_VERSION;
    fontIndex = -1;
    assert(!UI_GAME_UseChinese());
    fontIndex = 1;
    assert(UI_GAME_UseChinese() && strcmp(UI_GAME_Title(), "游戏") == 0);
    assert(fontReads >= 3);
#else
    gUiLanguage = UI_LANGUAGE_CN;
    assert(!UI_GAME_UseChinese() && fontReads == 0);
#endif
}
static void TestList(void)
{
    gGameInList = true;
    for (unsigned language = 0; language < 2; ++language) {
        gUiLanguage = (uint8_t)language;
        gBatteryIconFillPercent = 100;
        Render();
        assert(pixelCalls == 0);
        assert(HasText("M:Start E:Back") && HasText("Move:2/4/6/8"));
        assert(HasText(UI_GAME_UseChinese() ? "游戏" : "Games"));
        assert(HasText(UI_GAME_UseChinese() ? "贪吃蛇" : "Snake"));
    }
}
static void TestSnakeAndOverlays(void)
{
    gGameInList = false;
    SNAKE_Init(&gSnakeGame, 0);
    gSnakeGame.food = 239; /* bottom-right extremity */
    static const uint8_t states[] = {SNAKE_RUNNING, SNAKE_PAUSED, SNAKE_LOST, SNAKE_WON};
    static const char *titles[] = {NULL, "PAUSED", "GAME OVER", "YOU WIN"};
    for (unsigned state = 0; state < 4; ++state) {
        gSnakeGame.status = states[state];
        if (states[state] == SNAKE_WON) {
            gSnakeGame.length = SNAKE_CAPACITY;
            gSnakeGame.tail = 239;
            gSnakeGame.score = 237;
            gSnakeGame.food = UINT8_MAX;
            for (uint16_t i = 0; i < SNAKE_CAPACITY; ++i)
                gSnakeGame.body[(239u + i) % SNAKE_CAPACITY] = (uint8_t)i;
        }
        Render();
        unsigned expected = (unsigned)gSnakeGame.length * 9u;
        if (gSnakeGame.food < SNAKE_CAPACITY) {
            expected += 5;
            CheckCell(gSnakeGame.food, true);
        }
        assert(cellPixelCalls == expected && pixelCalls > cellPixelCalls);
        for (uint8_t i = 0; i < gSnakeGame.length; ++i)
            CheckCell(SNAKE_CellAt(&gSnakeGame, i), false);
        assert(HasText("M:Pause"));
        for (size_t i = 0; i < textCount; ++i)
            if (strcmp(textCalls[i].text, "M:Pause") == 0)
                assert(textCalls[i].x == 76 && textCalls[i].last == 124);
        if (titles[state]) {
            assert(HasText(titles[state]));
            assert(HasText(state == 1 ? "MENU:Continue" : "MENU:Restart"));
        } else {
            assert(!HasText("PAUSED") && !HasText("GAME OVER") && !HasText("YOU WIN"));
        }
    }
}

int main(int argc, char **argv)
{
    assert(argc <= 2);
    if (argc == 1 || strcmp(argv[1], "language") == 0)
        TestLanguage();
    if (argc == 1 || strcmp(argv[1], "list") == 0)
        TestList();
    if (argc == 1 || strcmp(argv[1], "board") == 0)
        TestSnakeAndOverlays();
    if (argc == 2)
        assert(strcmp(argv[1], "language") == 0 || strcmp(argv[1], "list") == 0
               || strcmp(argv[1], "board") == 0);
    puts("game UI geometry/canary tests passed");
    return 0;
}
