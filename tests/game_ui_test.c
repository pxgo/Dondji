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
typedef struct { char text[64]; size_t x, last, y, bottom; } TextCall;
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
    call->bottom = y + height - 1u;
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
    bool hasChinese = false;
    assert(start <= end && top <= bottom);
    for (const unsigned char *p = (const unsigned char *)text; *p;) {
        if (*p >= 0xe4 && *p <= 0xef) {
            assert((p[1] & 0xc0u) == 0x80u && (p[2] & 0xc0u) == 0x80u);
            hasChinese = true;
            width += 13;
            p += 3;
        } else {
            assert(*p >= ' ' && *p < 127);
            width += 7;
            ++p;
        }
    }
    assert(width > 0);
    --width; /* no trailing inter-character space */
    size_t x = start;
    if (end > start) {
        assert(width <= (size_t)end - start + 1u);
        if (width < (size_t)end - start)
            x += ((size_t)end - start - width) / 2u;
    }
    assert(x + width <= LCD_WIDTH);
    const size_t firstX = x;
    size_t firstY = SIZE_MAX, lastY = 0;
    const size_t bandHeight = (size_t)bottom - top + 1u;
    /* Match helper.c's LCD coordinates, glyph-specific centering and y-8
     * framebuffer mapping. Fill glyph rectangles only, never their spacing. */
    for (const unsigned char *p = (const unsigned char *)text; *p;) {
        const bool chinese = *p >= 0xe4 && *p <= 0xef;
        const size_t glyphWidth = chinese ? 12u : 6u;
        const size_t glyphHeight = chinese ? 12u : 7u;
        size_t lcdY = top;
        if (bandHeight >= glyphHeight)
            lcdY += (bandHeight - glyphHeight) / 2u;
        if (!chinese && hasChinese) {
            const unsigned add = (unsigned)latinDown + 1u;
            if (lcdY <= 255u - add)
                lcdY += add;
        }
        if (lcdY < 8u)
            lcdY = 8u;
        const size_t fbY = lcdY - 8u;
        assert(fbY + glyphHeight <= 56);
        if (fbY < firstY)
            firstY = fbY;
        if (fbY + glyphHeight - 1u > lastY)
            lastY = fbY + glyphHeight - 1u;
        if (chinese || *p != ' ')
            for (size_t col = 0; col < glyphWidth; ++col)
                for (size_t row = 0; row < glyphHeight; ++row) {
                    assert(x + col < LCD_WIDTH);
                    const size_t y = fbY + row;
                    gFrameBuffer[y / 8u][x + col] |= (uint8_t)(1u << (y % 8u));
                }
        x += glyphWidth + 1u;
        p += chinese ? 3u : 1u;
    }
    RecordText(text, firstX, width, firstY, lastY - firstY + 1u);
}
#endif

static bool HasText(const char *text)
{
    for (size_t i = 0; i < textCount; ++i)
        if (strcmp(textCalls[i].text, text) == 0)
            return true;
    return false;
}
static const TextCall *FindText(const char *text)
{
    const TextCall *found = NULL;
    for (size_t i = 0; i < textCount; ++i)
        if (strcmp(textCalls[i].text, text) == 0) {
            assert(found == NULL);
            found = &textCalls[i];
        }
    assert(found != NULL);
    return found;
}
static void ResetDrawing(void)
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
}
static void Render(void)
{
    ResetDrawing();
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
#ifdef ENABLE_CHINESE
static void TestPixelTextGeometry(void)
{
    ResetDrawing();
    UI_DisplayClear();
    UI_PrintStringSmallAtPixel("AB", 0, 127, 8, 19, 0);
    const TextCall *call = FindText("AB");
    assert(call->x == 57 && call->last == 69 && call->y == 2 && call->bottom == 8);
    assert(gFrameBuffer[0][57] == 0xfc && gFrameBuffer[1][57] == 1);
    assert(gFrameBuffer[0][56] == 0 && gFrameBuffer[0][63] == 0);
    assert(gFrameBuffer[0][64] == 0xfc && gFrameBuffer[0][70] == 0);
    CheckCanaries();

    ResetDrawing();
    UI_DisplayClear();
    UI_PrintStringSmallAtPixel("中A", 0, 127, 8, 19, 0);
    call = FindText("中A");
    assert(call->x == 54 && call->last == 72 && call->y == 0 && call->bottom == 11);
    assert(gFrameBuffer[0][54] == 0xff && gFrameBuffer[1][54] == 0x0f);
    assert(gFrameBuffer[0][66] == 0 && gFrameBuffer[0][67] == 0xf8);
    assert(gFrameBuffer[1][67] == 3); /* mixed Latin baseline adds one pixel */
    CheckCanaries();

    ResetDrawing();
    UI_DisplayClear();
    UI_PrintStringSmallAtPixel(">", 4, 4, 56, 62, 0);
    call = FindText(">");
    assert(call->x == 4 && call->last == 9 && call->y == 48 && call->bottom == 54);
    assert(gFrameBuffer[6][4] == 0x7f && gFrameBuffer[6][10] == 0);
    CheckCanaries(); /* LCD y > 55 is legal when framebuffer y remains <= 55 */

    ResetDrawing();
    UI_DisplayClear();
    UI_PrintStringSmallAtPixel("A", 0, 0, 0, 11, 0);
    call = FindText("A");
    assert(call->y == 0 && call->bottom == 6 && gFrameBuffer[0][0] == 0x7f);
    CheckCanaries();

    ResetDrawing();
    UI_DisplayClear();
    UI_PrintStringSmallAtPixel("中", 0, 0, 0, 11, 0);
    call = FindText("中");
    assert(call->x == 0 && call->last == 11 && call->y == 0 && call->bottom == 11);
    assert(gFrameBuffer[0][0] == 0xff && gFrameBuffer[1][0] == 0x0f);
    assert(gFrameBuffer[0][12] == 0);
    CheckCanaries();
}
#endif
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
    TestPixelTextGeometry();
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
        for (unsigned selection = 0; selection < GAME_COUNT; ++selection) {
            gGameSelection = (uint8_t)selection;
            gBatteryIconFillPercent = 100;
            Render();
            assert(pixelCalls == 0);
            assert(HasText("M:Start E:Back"));
            assert(HasText(UI_GAME_UseChinese() ? "游戏" : "Games"));
            assert(HasText(UI_GAME_UseChinese() ? "贪吃蛇" : "Snake"));
            assert(HasText(UI_GAME_UseChinese() ? "电子琴" : "Piano"));
            const bool chinese = UI_GAME_UseChinese();
            const TextCall *title = FindText(chinese ? "游戏" : "Games");
            const TextCall *snake = FindText(chinese ? "贪吃蛇" : "Snake");
            const TextCall *piano = FindText(chinese ? "电子琴" : "Piano");
            const TextCall *marker = FindText(">");
            const TextCall *selected = selection == GAME_SNAKE ? snake : piano;
            const TextCall *labels[] = {title, snake, piano};
            static const size_t bandTops[] = {0, 16, 32};
            for (unsigned i = 0; i < 3; ++i) {
                size_t width, height, offset, expectedX;
#ifdef ENABLE_CHINESE
                width = chinese ? (i == 0 ? 25u : 38u) : 34u;
                height = chinese ? 12u : 7u;
                offset = chinese ? 0u : 2u;
                const size_t left = i == 0 ? 0u : 16u;
                expectedX = left + (127u - left - width) / 2u;
#else
                width = 35u;
                height = 7u;
                offset = 0u;
                const size_t left = i == 0 ? 0u : 16u;
                expectedX = left + (128u - left - width) / 2u;
#endif
                assert(labels[i]->x == expectedX && labels[i]->last == expectedX + width - 1u);
                assert(labels[i]->y == bandTops[i] + offset);
                assert(labels[i]->bottom == labels[i]->y + height - 1u);
                assert(labels[i]->bottom < bandTops[i] + 12u);
            }
            assert(title->bottom < snake->y && snake->bottom < piano->y);
            assert(marker->x == 4 && marker->last < selected->x);
            const size_t selectedTop = selection == GAME_SNAKE ? 16u : 32u;
#ifdef ENABLE_CHINESE
            assert(marker->last == 9 && marker->y == selectedTop + 2u);
#else
            assert(marker->last == 10 && marker->y == selectedTop);
#endif
            assert(marker->bottom == marker->y + 6u);
            assert(marker->y >= selectedTop && marker->bottom < selectedTop + 12u);
            assert(marker->y <= selected->bottom && selected->y <= marker->bottom);
            /* Exclude the separate status buffer; all main-page text rectangles
             * must be disjoint, including marker, labels and footer. */
            for (size_t i = 1; i < textCount; ++i)
                for (size_t j = i + 1; j < textCount; ++j) {
                    const TextCall *a = &textCalls[i], *b = &textCalls[j];
                    assert(a->last < b->x || b->last < a->x
                           || a->bottom < b->y || b->bottom < a->y);
                }
        }
    }
}
static void TestSnakeAndOverlays(void)
{
    gGameInList = false;
    gGameSelection = GAME_SNAKE;
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

static void TestPiano(void)
{
    static const char *notes[] = {"C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5", "D5", "E5", "F5", "G5"};
    static const unsigned hz[] = {262, 294, 330, 349, 392, 440, 494, 523, 587, 659, 698, 784};
    gScreenToDisplay = DISPLAY_MENU;
    gCurrentFunction = FUNCTION_FOREGROUND;
    gDualPttTxVfo = 0xff;
    assert(GAME_Enter());
    gGameInList = false;
    gGameSelection = GAME_PIANO;
    PIANO_Open();
    for (unsigned language = 0; language < 2; ++language) {
        gUiLanguage = (uint8_t)language;
        for (unsigned note = 0; note <= PIANO_NOTE_COUNT; ++note) {
            PIANO_Stop();
            if (note < PIANO_NOTE_COUNT)
                PIANO_Play((uint8_t)note);
            Render();
            assert(pixelCalls == 0 && cellPixelCalls == 0);
            assert(HasText(UI_GAME_UseChinese() ? "电子琴" : "Piano"));
            assert(HasText("1:do 2:ri 3:mi") && HasText("4:so 5:la 6:xi"));
            assert(HasText("7:ri+ 8:mi+ 9:fa+") && HasText("*:fa 0:do+ #:so+"));
            char expected[32];
            if (note < PIANO_NOTE_COUNT)
                snprintf(expected, sizeof(expected), "%s %uHz E:Back", notes[note], hz[note]);
            else
                strcpy(expected, "Hold key E:Back");
            assert(HasText(expected));
            assert(!HasText("M:Pause") && !HasText("M:Start E:Back"));
        }
    }
    GAME_Leave();
    assert(!GAME_IsActive() && PIANO_CurrentNote() == PIANO_NO_NOTE);
}

int main(int argc, char **argv)
{
    assert(argc <= 2);
#ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
    gKeyFromSerial = KEY_INVALID;
#endif
    if (argc == 1 || strcmp(argv[1], "language") == 0)
        TestLanguage();
    if (argc == 1 || strcmp(argv[1], "list") == 0)
        TestList();
    if (argc == 1 || strcmp(argv[1], "board") == 0)
        TestSnakeAndOverlays();
    if (argc == 1 || strcmp(argv[1], "piano") == 0)
        TestPiano();
    if (argc == 2)
        assert(strcmp(argv[1], "language") == 0 || strcmp(argv[1], "list") == 0
               || strcmp(argv[1], "board") == 0 || strcmp(argv[1], "piano") == 0);
    puts("game UI geometry/canary tests passed");
    return 0;
}
