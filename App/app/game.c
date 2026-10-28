/* SPDX-License-Identifier: Apache-2.0 */
#include "game.h"
#include "piano.h"

#include "app/menu.h"
#include "app/chFrScanner.h"
#include "app/mdc1200_app.h"
#include "app/scanner.h"
#include "app/yan_id_rf.h"
#ifdef ENABLE_FMRADIO
#include "app/fm.h"
#endif
#include "audio.h"
#include "driver/backlight.h"
#include "driver/bk4819.h"
#include "driver/gpio.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "radio.h"

#include "settings.h"
#include "ui/ui.h"

SnakeState_t gSnakeGame;
bool gGameInList;
uint8_t gGameSelection;
static bool active;
static bool releasePending;
static bool inputReady;
static uint8_t releaseTicks;
static uint8_t keyTicks;
static KEY_Code_t lastKey;
static uint8_t moveTicks;
static uint16_t seedTicks;
#ifdef ENABLE_VOX
static bool voxRestorePending;

static void ResetVox(void)
{
    gVOX_NoiseDetected = false;
    g_VOX_Lost = false;
    gVoxResumeCountdown = 0;
    gVoxPauseCountdown = 0;
    gVoxStopCountdown_10ms = 0;
}
#endif

bool GAME_IsActive(void)
{
    return active;
}

bool GAME_TxBlocked(void)
{
    return active || releasePending;
}

static void ResetInput(void)
{
    gPttIsPressed = false;
    gPttWasPressed = false;
    gPttWasReleased = false;
    gPttDebounceCounter = 0;
    gKeyReading0 = KEY_INVALID;
    gKeyReading1 = KEY_INVALID;
    gKeyBeingHeld = false;
    gDebounceCounter = 0;
    gFlagPrepareTX = false;
#ifdef ENABLE_VOX
    ResetVox();
#endif
}

static void ResetRadioTimers(void)
{
    gSchedulePowerSave = false;
    gPowerSaveCountdownExpired = false;
    gBatterySaveCountdown_10ms = battery_save_count_10ms;
    gScheduleDualWatch = false;
    gDualWatchCountdown_10ms = dual_watch_count_after_rx_10ms;
    gScheduleScanListen = false;
    gRxReceptionMode = RX_MODE_NONE;
    gRxVfoIsActive = false;
#ifdef ENABLE_NOAA
    gScheduleNOAA = false;
    gNOAA_Countdown_10ms = NOAA_countdown_10ms;
#endif
#ifdef ENABLE_FMRADIO
    gScheduleFM = false;
    gFM_RestoreCountdown_10ms = 0;
#endif
}

bool GAME_Enter(void)
{
    /* Only suspend idle radio sessions; scans, FM and reception must end first. */
    if (GAME_TxBlocked() || gScreenToDisplay != DISPLAY_MENU
        || gCurrentFunction != FUNCTION_FOREGROUND || gMonitor
        || gReducedService || gLowBattery || SerialConfigInProgress()
        || gPttIsPressed || GPIO_IsPttPressed() || gFlagPrepareTX
        || gRTTECountdown_10ms || gScanStateDir != SCAN_OFF
        || gCssBackgroundScan || SCANNER_IsScanning()
        || gFlagReconfigureVfos || gVfoConfigureMode != VFO_CONFIGURE_NONE
        || gFlagAcceptSetting || gRequestSaveSettings || gRequestSaveVFO
        || gRequestSaveChannel || gFlagRefreshSetting
#ifdef ENABLE_FEAT_F4HWN
        || gDualPttTxVfo != 0xFF
#endif
#ifdef ENABLE_FMRADIO
        || gFmRadioMode || gFM_ScanState != FM_SCAN_OFF
#endif
#ifdef ENABLE_VOX
        || gVOX_NoiseDetected
#endif
    )
        return false;

    active = true;
    gGameInList = true;
    gGameSelection = GAME_SNAKE;
    inputReady = false;
    releaseTicks = 0;
    keyTicks = 0;
    lastKey = KEY_INVALID;
    moveTicks = 0;
    ResetInput();
    ResetRadioTimers();
    gBeepToPlay = BEEP_NONE;
    gRequestDisplayScreen = DISPLAY_INVALID;
    gMenuCountdown = 0;
    MDC1200_AppDisableRx();
    YAN_RF_AbortRx();
    AUDIO_AudioPathOff();
    gEnableSpeaker = false;
    BK4819_DisableVox();
    BK4819_WriteRegister(BK4819_REG_3F, 0);
    BK4819_WriteRegister(BK4819_REG_02, 0);
    BK4819_SetupPowerAmplifier(0, 0);
    BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);
    BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, false);
    BK4819_Sleep();
    BACKLIGHT_TurnOn();
    GUI_SelectNextDisplay(DISPLAY_GAME);
    return true;
}

void GAME_OnLeave(void)
{
    if (!active)
        return;
    PIANO_Close();
    active = false;
    releasePending = true;
#ifdef ENABLE_VOX
    voxRestorePending = true;
#endif
    releaseTicks = 0;
    ResetInput();
    ResetRadioTimers();
    gBeepToPlay = BEEP_NONE;
    gMenuCountdown = menu_timeout_500ms;
    gKeyLockCountdown = gEeprom.AUTO_KEYPAD_LOCK * 30;
#ifdef ENABLE_FEAT_F4HWN_SLEEP
    gSleepModeCountdown_500ms = gSetting_set_off * 120;
#endif
    /* A critical-battery exit must not wake RF after the battery code slept it. */
    if (!gReducedService && gCurrentFunction != FUNCTION_POWER_SAVE)
        RADIO_SetupRegisters(true);
}

void GAME_Leave(void)
{
    if (!active)
        return;
    gRequestDisplayScreen = DISPLAY_INVALID;
    GUI_SelectNextDisplay(DISPLAY_MENU);
    MENU_ActivateMainPage();
    gUpdateStatus = true;
}

bool GAME_CheckRelease(void)
{
#ifdef ENABLE_VOX
    /* RX may have become INCOMING during release. Restore later, without
       interrupting an ongoing reception or holding the keyboard barrier open. */
    if (voxRestorePending && !GAME_TxBlocked() && !gReducedService
        && gCurrentFunction == FUNCTION_FOREGROUND) {
        ResetVox();
        voxRestorePending = false;
        RADIO_SetupRegisters(true);
    }
#endif
    if (!releasePending)
        return false;
#ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
    /* Injected keys are not evidence that the physical side keys are released. */
    gKeyFromSerial = KEY_INVALID;
#endif
    KEY_Code_t key = KEYBOARD_Poll();
    if (GPIO_IsPttPressed() || key != KEY_INVALID) {
        releaseTicks = 0;
    } else if (++releaseTicks >= 3u) {
        ResetInput();
        releasePending = false;
    }
    return true;
}

void GAME_ProcessKeys(KEY_Code_t key, bool pressed, bool held)
{
    if (!active || !pressed || held || !inputReady)
        return;
    if (!gGameInList && gGameSelection == GAME_PIANO && key != KEY_EXIT)
        return;
    inputReady = false;
    releaseTicks = 0;
    BACKLIGHT_TurnOn();

    if (key == KEY_EXIT) {
        if (gGameInList)
            GAME_Leave();
        else {
            PIANO_Close();
            gGameInList = true;
            gUpdateDisplay = true;
        }
        return;
    }
    if (gGameInList && (key == KEY_UP || key == KEY_DOWN)) {
        gGameSelection = (uint8_t)((gGameSelection + 1u) % GAME_COUNT);
        gUpdateDisplay = true;
        return;
    }
    if (key == KEY_MENU) {
        if (gGameInList || gSnakeGame.status == SNAKE_LOST || gSnakeGame.status == SNAKE_WON) {
            if (gGameSelection == GAME_PIANO)
                PIANO_Open();
            else
                SNAKE_Init(&gSnakeGame, seedTicks ^ gBatteryVoltageAverage);
            gGameInList = false;
        } else {
            gSnakeGame.status = (gSnakeGame.status == SNAKE_RUNNING) ? SNAKE_PAUSED : SNAKE_RUNNING;
        }
        moveTicks = 0;
        gUpdateDisplay = true;
    } else if (!gGameInList) {
        switch (key) {
        case KEY_2: SNAKE_Turn(&gSnakeGame, SNAKE_UP); break;
        case KEY_4: SNAKE_Turn(&gSnakeGame, SNAKE_LEFT); break;
        case KEY_6: SNAKE_Turn(&gSnakeGame, SNAKE_RIGHT); break;
        case KEY_8: SNAKE_Turn(&gSnakeGame, SNAKE_DOWN); break;
        default: break;
        }
    }
}

static void PianoKeyTick(KEY_Code_t key)
{
    const uint8_t oldNote = PIANO_CurrentNote();
    if (GPIO_IsPttPressed()) {
        PIANO_Stop();
        lastKey = KEY_INVALID;
        keyTicks = 0;
    } else {
        /* Mute on the first release/change sample. Start only after debounce;
           piano keys may transition directly without an all-keys-up barrier. */
        if (PIANO_NoteForKey(key) == PIANO_NO_NOTE || key != lastKey)
            PIANO_Stop();
        if (key != lastKey) {
            lastKey = key;
            keyTicks = 0;
        } else if (key != KEY_INVALID && keyTicks < 3u && ++keyTicks == 3u) {
            if (key == KEY_EXIT)
                GAME_ProcessKeys(key, true, false);
            else {
                PIANO_Play(PIANO_NoteForKey(key));
                BACKLIGHT_TurnOn();
            }
        }
    }
    if (oldNote != PIANO_CurrentNote())
        gUpdateDisplay = true;
}

void GAME_Tick10ms(void)
{
    ++seedTicks;
    KEY_Code_t key = KEYBOARD_Poll();
    if (!inputReady) {
        if (key != KEY_INVALID || GPIO_IsPttPressed())
            releaseTicks = 0;
        else if (++releaseTicks >= 3u) {
            inputReady = true;
            keyTicks = 0;
            lastKey = KEY_INVALID;
        }
    } else if (!gGameInList && gGameSelection == GAME_PIANO) {
        PianoKeyTick(key);
    } else if (key != lastKey) {
        lastKey = key;
        keyTicks = 0;
    } else if (key != KEY_INVALID && keyTicks < 3u && ++keyTicks == 3u) {
        GAME_ProcessKeys(key, true, false);
    }

    if (!active)
        return;
    if (!gGameInList && gGameSelection == GAME_SNAKE
            && gSnakeGame.status == SNAKE_RUNNING && ++moveTicks >= 20u) {
        moveTicks = 0;
        SNAKE_Step(&gSnakeGame);
        gUpdateDisplay = true;
    }
}
