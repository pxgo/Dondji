/* SPDX-License-Identifier: Apache-2.0 */
#ifndef APP_PIANO_H
#define APP_PIANO_H

#include "driver/keyboard.h"

#define PIANO_NOTE_COUNT 12u
#define PIANO_NO_NOTE UINT8_MAX

uint8_t PIANO_NoteForKey(KEY_Code_t key);
uint16_t PIANO_Frequency(uint8_t note);
uint8_t PIANO_CurrentNote(void);
void PIANO_Open(void);
void PIANO_Play(uint8_t note);
void PIANO_Stop(void);
void PIANO_Close(void);

#endif
