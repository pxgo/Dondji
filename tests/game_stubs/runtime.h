#ifndef TEST_GAME_RUNTIME_H
#define TEST_GAME_RUNTIME_H
/* SPDX-License-Identifier: Apache-2.0 */
#define GAME_STUB_DEFINE
#include "common.h"
#include "../../App/app/game.h"
#include "../../App/app/piano.h"

#include <assert.h>
#include <string.h>

static KEY_Code_t physicalKey = KEY_INVALID;
static bool physicalPtt, serialBusy, scanning;
static unsigned polls, sleeps, paOff, rxOff, audioOff, mdcOff, yanOff;
static unsigned voxOff, registerWrites, restores, backlights, menuRestores;
static bool paEnabled, rxEnabled, voxEnabled;
static unsigned savedMenuIndex = 5, menuIndex = 5;
static uint16_t registers[128], toneHistory[256];
static unsigned readsByRegister[128], writesByRegister[128];
static unsigned tonePlays, toneStops, audioOn, muteEnters, muteExits;
static bool audioEnabled, toneEnabled;
static uint8_t audioFilter;

KEY_Code_t KEYBOARD_Poll(void)
{
    ++polls;
#ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
    if (gKeyFromSerial != KEY_INVALID) {
        KEY_Code_t key = gKeyFromSerial;
        gKeyFromSerial = KEY_INVALID;
        return key;
    }
#endif
    return physicalKey;
}
bool GPIO_IsPttPressed(void) { return physicalPtt; }
bool SerialConfigInProgress(void) { return serialBusy; }
bool SCANNER_IsScanning(void) { return scanning; }
void MDC1200_AppDisableRx(void) { ++mdcOff; }
void YAN_RF_AbortRx(void) { ++yanOff; }
void AUDIO_AudioPathOff(void) { ++audioOff; audioEnabled = false; }
void AUDIO_AudioPathOn(void)
{
    assert(!paEnabled && !rxEnabled && toneEnabled);
    ++audioOn;
    audioEnabled = true;
}
void BK4819_DisableVox(void) { ++voxOff; voxEnabled = false; }
void BK4819_WriteRegister(uint8_t reg, uint16_t value)
{
    assert(reg < 128);
    assert(reg == BK4819_REG_3F || reg == BK4819_REG_02
           || reg == BK4819_REG_30 || reg == BK4819_REG_37
           || reg == BK4819_REG_50 || reg == BK4819_REG_70 || reg == BK4819_REG_71);
    if (reg == BK4819_REG_3F || reg == BK4819_REG_02 || reg == BK4819_REG_30)
        assert(value == 0);
    if (reg == BK4819_REG_37)
        assert(value == 0x9d1f);
    if (reg == BK4819_REG_70) {
        assert(value == 0 && !audioEnabled);
        if (toneEnabled)
            ++toneStops;
        toneEnabled = false;
    }
    registers[reg] = value;
    ++writesByRegister[reg];
    ++registerWrites;
}
uint16_t BK4819_ReadRegister(uint8_t reg)
{
    assert(reg == BK4819_REG_50 || reg == BK4819_REG_71);
    ++readsByRegister[reg];
    return registers[reg];
}
void BK4819_EnterTxMute(void)
{
    ++muteEnters;
    registers[BK4819_REG_50] = 0xbb18;
}
void BK4819_ExitTxMute(void)
{
    ++muteExits;
    registers[BK4819_REG_50] = 0x3b18;
}
void BK4819_SetAF(uint8_t af)
{
    assert(af == BK4819_AF_MUTE || af == BK4819_AF_BEEP);
    audioFilter = af;
}
void BK4819_PlayTone(uint16_t frequency, bool tuningGainSwitch)
{
    assert(GAME_IsActive() && GAME_TxBlocked());
    assert(!paEnabled && !rxEnabled && !voxEnabled && !audioEnabled);
    assert(registers[BK4819_REG_30] == 0 && registers[BK4819_REG_37] == 0x9d1f);
    assert(tuningGainSwitch && tonePlays < sizeof(toneHistory) / sizeof(toneHistory[0]));
    toneHistory[tonePlays++] = frequency;
    /* Driver boundary model: frequency is recorded in Hz, not its chip encoding. */
    registers[BK4819_REG_71] = frequency;
    registers[BK4819_REG_70] = 0x8000 | (28u << 8);
    BK4819_EnterTxMute();
    BK4819_SetAF(BK4819_AF_BEEP);
    toneEnabled = true;
}
void BK4819_SetupPowerAmplifier(uint8_t bias, uint32_t frequency)
{
    assert(bias == 0 && frequency == 0);
    ++paOff;
}
void BK4819_ToggleGpioOut(uint8_t pin, bool enabled)
{
    assert(!enabled);
    if (pin == BK4819_GPIO1_PIN29_PA_ENABLE)
        paEnabled = enabled;
    else {
        assert(pin == BK4819_GPIO0_PIN28_RX_ENABLE);
        rxEnabled = enabled;
        ++rxOff;
    }
}
void BK4819_Sleep(void)
{
    assert(!paEnabled && !rxEnabled && !voxEnabled);
    assert(audioOff && mdcOff && yanOff && paOff && rxOff);
    assert(!audioEnabled);
    registers[BK4819_REG_30] = 0;
    registers[BK4819_REG_37] = 0x1d00;
    ++sleeps;
}
void BACKLIGHT_TurnOn(void) { ++backlights; }
void RADIO_SetupRegisters(bool switching)
{
    assert(switching);
    ++restores;
    /* Model only radio.c's documented VOX suppression, not hardware RF. */
    voxEnabled = !GAME_TxBlocked();
}
void MENU_ActivateMainPage(void)
{
    ++menuRestores;
    menuIndex = savedMenuIndex;
}
void GUI_SelectNextDisplay(GUI_DisplayType_t display)
{
    /* Simulated dispatcher's production contract: every real departure cleans up. */
    if (display == DISPLAY_INVALID)
        return;
    if (display != gScreenToDisplay && GAME_IsActive() && display != DISPLAY_GAME)
        GAME_OnLeave();
    gScreenToDisplay = display;
}

#endif
