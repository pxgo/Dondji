/* SPDX-License-Identifier: Apache-2.0 */
#ifndef APP_GAME_H
#define APP_GAME_H

#include "driver/keyboard.h"
#include "app/snake.h"

enum {
    GAME_SNAKE,
    GAME_PIANO,
    GAME_COUNT
};

extern SnakeState_t gSnakeGame;
extern bool gGameInList;
extern uint8_t gGameSelection;

bool GAME_Enter(void);
bool GAME_IsActive(void);
bool GAME_TxBlocked(void);
/* Called by the display dispatcher for every exit, including forced exits. */
void GAME_OnLeave(void);
void GAME_Leave(void);
void GAME_Tick10ms(void);
void GAME_ProcessKeys(KEY_Code_t key, bool pressed, bool held);
/* Suppress normal keyboard processing until all physical keys are released. */
bool GAME_CheckRelease(void);

#endif
