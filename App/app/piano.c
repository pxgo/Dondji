/* SPDX-License-Identifier: Apache-2.0 */
#include "piano.h"

#include "app/game.h"
#include "audio.h"
#include "driver/bk4819.h"
#include "misc.h"

/* C4..G5, diatonic major scale, rounded to integer Hz (no floating point). */
static const uint16_t frequencies[PIANO_NOTE_COUNT] = {
    262, 294, 330, 349, 392, 440, 494, 523, 587, 659, 698, 784
};
static const uint8_t keys[PIANO_NOTE_COUNT] = {
    KEY_1, KEY_2, KEY_3, KEY_STAR, KEY_4, KEY_5, KEY_6,
    KEY_0, KEY_7, KEY_8, KEY_9, KEY_F
};
static uint16_t savedToneFrequency;
static uint16_t savedMute;
static uint8_t currentNote;
static bool opened;

uint8_t PIANO_NoteForKey(KEY_Code_t key)
{
    for (uint8_t i = 0; i < PIANO_NOTE_COUNT; ++i)
        if (key == keys[i])
            return i;
    return PIANO_NO_NOTE;
}

uint16_t PIANO_Frequency(uint8_t note)
{
    return note < PIANO_NOTE_COUNT ? frequencies[note] : 0;
}

uint8_t PIANO_CurrentNote(void)
{
    return opened ? currentNote : PIANO_NO_NOTE;
}

void PIANO_Open(void)
{
    if (opened || !GAME_IsActive())
        return;
    savedToneFrequency = BK4819_ReadRegister(BK4819_REG_71);
    savedMute = BK4819_ReadRegister(BK4819_REG_50);
    currentNote = PIANO_NO_NOTE;
    opened = true;
}

void PIANO_Stop(void)
{
    if (!opened || currentNote == PIANO_NO_NOTE)
        return;
    AUDIO_AudioPathOff();
    gEnableSpeaker = false;
    BK4819_EnterTxMute();
    BK4819_WriteRegister(BK4819_REG_70, 0);
    BK4819_SetAF(BK4819_AF_MUTE);
    /* Never use TurnsOffTones_TurnsOnRX while inside a game session. */
    BK4819_Sleep();
    currentNote = PIANO_NO_NOTE;
}

void PIANO_Play(uint8_t note)
{
    if (!opened || !GAME_IsActive() || note >= PIANO_NOTE_COUNT || currentNote == note)
        return;
    AUDIO_AudioPathOff();
    BK4819_SetupPowerAmplifier(0, 0);
    BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);
    BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, false);
    BK4819_WriteRegister(BK4819_REG_30, 0);
    /* Reuse the driver's known power configuration after Sleep; PlayTone
       enables only AF/DISC/TX-DSP, never PLL, RX link, or PA gain. */
    BK4819_WriteRegister(BK4819_REG_37, 0x9D1F);
    BK4819_PlayTone(frequencies[note], true);
    BK4819_ExitTxMute();
    AUDIO_AudioPathOn();
    gEnableSpeaker = true;
    currentNote = note;
}

void PIANO_Close(void)
{
    if (!opened)
        return;
    PIANO_Stop();
    /* Restore only tone/mute state, not an old RF power or link snapshot. */
    BK4819_WriteRegister(BK4819_REG_71, savedToneFrequency);
    BK4819_WriteRegister(BK4819_REG_50, savedMute);
    opened = false;
}
