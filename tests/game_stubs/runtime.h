#ifndef TEST_GAME_RUNTIME_H
#define TEST_GAME_RUNTIME_H
/* SPDX-License-Identifier: Apache-2.0 */
#define GAME_STUB_DEFINE
#include "common.h"
#include "../../App/app/game.h"

#include <assert.h>
#include <string.h>

static KEY_Code_t physicalKey = KEY_INVALID;
static bool physicalPtt, serialBusy, scanning;
static unsigned polls, sleeps, paOff, rxOff, audioOff, mdcOff, yanOff;
static unsigned voxOff, registerWrites, restores, backlights, menuRestores;
static bool paEnabled, rxEnabled, voxEnabled;
static unsigned savedMenuIndex = 5, menuIndex = 5;

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
void AUDIO_AudioPathOff(void) { ++audioOff; }
void BK4819_DisableVox(void) { ++voxOff; voxEnabled = false; }
void BK4819_WriteRegister(uint8_t reg, uint16_t value)
{
    assert(reg == BK4819_REG_3F || reg == BK4819_REG_02);
    assert(value == 0);
    ++registerWrites;
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
