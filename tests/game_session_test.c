/* SPDX-License-Identifier: Apache-2.0 */
#include "game_stubs/runtime.h"

static void Tick(unsigned count)
{
    while (count--)
        GAME_Tick10ms();
}
static void ReleaseEntry(void)
{
    physicalKey = KEY_INVALID;
    physicalPtt = false;
    Tick(3);
}
static void Press(KEY_Code_t key)
{
    physicalKey = key;
    Tick(4); /* one changed sample followed by three stable samples */
}
static void ReleaseExit(void)
{
    physicalKey = KEY_INVALID;
    physicalPtt = false;
    assert(GAME_CheckRelease());
    assert(GAME_TxBlocked());
    assert(GAME_CheckRelease());
    assert(GAME_TxBlocked());
    assert(GAME_CheckRelease());
    assert(!GAME_TxBlocked());
    assert(!GAME_CheckRelease());
}
static void Fixture(void)
{
    if (GAME_IsActive())
        GAME_Leave();
    if (GAME_TxBlocked())
        ReleaseExit();
    physicalKey = KEY_INVALID;
    physicalPtt = serialBusy = scanning = false;
    gScreenToDisplay = DISPLAY_MENU;
    gCurrentFunction = FUNCTION_FOREGROUND;
    gDualPttTxVfo = 0xff;
    gKeyReading0 = gKeyReading1 = KEY_INVALID;
#ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
    gKeyFromSerial = KEY_INVALID;
#endif
    gEeprom.AUTO_KEYPAD_LOCK = 2;
    gSetting_set_off = 3;
    gBatteryVoltageAverage = 780;
    gRequestDisplayScreen = DISPLAY_MAIN;
    gMenuCountdown = 77;
    gMonitor = gReducedService = gLowBattery = gCssBackgroundScan = false;
    gFlagReconfigureVfos = gFlagAcceptSetting = gRequestSaveSettings = false;
    gRequestSaveVFO = gFlagRefreshSetting = gFmRadioMode = false;
    gFlagPrepareTX = gPttIsPressed = gVOX_NoiseDetected = false;
    gRTTECountdown_10ms = gVfoConfigureMode = gFM_ScanState = 0;
    gRequestSaveChannel = 0;
    gScanStateDir = SCAN_OFF;
    polls = sleeps = paOff = rxOff = audioOff = mdcOff = yanOff = 0;
    voxOff = registerWrites = restores = backlights = menuRestores = 0;
    paEnabled = rxEnabled = voxEnabled = true;
    savedMenuIndex = menuIndex = 5;
}
static void CheckRejected(void)
{
    assert(!GAME_Enter());
    assert(!GAME_IsActive() && !GAME_TxBlocked());
    assert(sleeps == 0 && paOff == 0 && restores == 0 && backlights == 0);
    assert(registerWrites == 0 && audioOff == 0 && mdcOff == 0 && yanOff == 0);
    assert(gMenuCountdown == 77);
}
#define REJECT(setup, cleanup) do { setup; CheckRejected(); cleanup; } while (0)

static void TestEntryGuards(void)
{
    Fixture();
    REJECT(gScreenToDisplay = DISPLAY_MAIN, gScreenToDisplay = DISPLAY_MENU);
    for (FUNCTION_Type_t fn = FUNCTION_TRANSMIT; fn < FUNCTION_N_ELEM; ++fn) {
        gCurrentFunction = fn;
        CheckRejected();
    }
    gCurrentFunction = FUNCTION_FOREGROUND;
    bool *busy[] = {&gMonitor, &gReducedService, &gLowBattery, &serialBusy,
        &gPttIsPressed, &physicalPtt, &gFlagPrepareTX, &gCssBackgroundScan,
        &scanning, &gFlagReconfigureVfos, &gFlagAcceptSetting,
        &gRequestSaveSettings, &gRequestSaveVFO, &gFlagRefreshSetting};
    for (size_t i = 0; i < sizeof(busy) / sizeof(busy[0]); ++i) {
        *busy[i] = true;
        CheckRejected();
        *busy[i] = false;
    }
    REJECT(gRTTECountdown_10ms = 1, gRTTECountdown_10ms = 0);
    REJECT(gScanStateDir = 1, gScanStateDir = SCAN_OFF);
    REJECT(gScanStateDir = -1, gScanStateDir = SCAN_OFF);
    REJECT(gVfoConfigureMode = 1, gVfoConfigureMode = VFO_CONFIGURE_NONE);
    REJECT(gRequestSaveChannel = 1, gRequestSaveChannel = 0);
#ifdef ENABLE_FEAT_F4HWN
    REJECT(gDualPttTxVfo = 0, gDualPttTxVfo = 0xff);
#endif
#ifdef ENABLE_FMRADIO
    REJECT(gFmRadioMode = true, gFmRadioMode = false);
    REJECT(gFM_ScanState = 1, gFM_ScanState = FM_SCAN_OFF);
#endif
#ifdef ENABLE_VOX
    REJECT(gVOX_NoiseDetected = true, gVOX_NoiseDetected = false);
#endif
    assert(GAME_Enter());
    assert(!GAME_Enter() && sleeps == 1);
}

static void TestEntryShutdownAndMenuGate(void)
{
    Fixture();
    physicalKey = KEY_MENU;
    gKeyReading0 = gKeyReading1 = KEY_MENU;
    gPttWasPressed = gPttWasReleased = gKeyBeingHeld = true;
    gPttDebounceCounter = 7;
    gDebounceCounter = 42;
    gSchedulePowerSave = gPowerSaveCountdownExpired = gScheduleDualWatch = true;
    gScheduleScanListen = gRxVfoIsActive = gScheduleNOAA = gScheduleFM = true;
    gEnableSpeaker = true;
    gBeepToPlay = 9;
#ifdef ENABLE_VOX
    g_VOX_Lost = true;
    gVoxResumeCountdown = gVoxPauseCountdown = gVoxStopCountdown_10ms = 99;
#endif
    assert(GAME_Enter());
    assert(GAME_IsActive() && GAME_TxBlocked() && gGameInList);
    assert(gScreenToDisplay == DISPLAY_GAME && gRequestDisplayScreen == DISPLAY_INVALID);
    assert(gMenuCountdown == 0 && !gEnableSpeaker && gBeepToPlay == BEEP_NONE);
    assert(!gPttIsPressed && !gPttWasPressed && !gPttWasReleased);
    assert(gKeyReading0 == KEY_INVALID && gKeyReading1 == KEY_INVALID);
    assert(!gKeyBeingHeld && gPttDebounceCounter == 0 && gDebounceCounter == 0);
    assert(!gSchedulePowerSave && !gScheduleDualWatch && !gScheduleScanListen);
    assert(!gPowerSaveCountdownExpired && !gRxVfoIsActive);
    assert(gBatterySaveCountdown_10ms == battery_save_count_10ms);
    assert(gDualWatchCountdown_10ms == dual_watch_count_after_rx_10ms);
    assert(sleeps == 1 && paOff == 1 && rxOff == 1 && registerWrites == 2);
    assert(voxOff == 1 && !voxEnabled && !paEnabled && !rxEnabled);
#ifdef ENABLE_VOX
    assert(!g_VOX_Lost && !gVOX_NoiseDetected);
    assert(gVoxResumeCountdown == 0 && gVoxPauseCountdown == 0 && gVoxStopCountdown_10ms == 0);
#endif
    GAME_ProcessKeys(KEY_MENU, true, false);
    Tick(30);
    assert(gGameInList); /* launcher MENU must not start Snake */
    physicalKey = KEY_INVALID;
    Tick(2);
    Press(KEY_MENU);
    assert(gGameInList); /* two released samples are not enough */
    ReleaseEntry();
    GAME_ProcessKeys(KEY_MENU, false, false);
    GAME_ProcessKeys(KEY_MENU, true, true);
    assert(gGameInList);
    Press(KEY_MENU);
    assert(!gGameInList && gSnakeGame.status == SNAKE_RUNNING);
    /* Continued MENU holding cannot pause/restart the game. */
    Tick(8);
    assert(gSnakeGame.status == SNAKE_RUNNING);
}

static void TestEntryPhysicalRelease(void)
{
    Fixture();
    assert(GAME_Enter());
    static const KEY_Code_t keys[] = {KEY_SIDE1, KEY_SIDE2, KEY_2};
    for (unsigned i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        physicalKey = KEY_INVALID;
        Tick(2);
        physicalKey = keys[i];
        Tick(1);
        GAME_ProcessKeys(KEY_MENU, true, false);
        assert(gGameInList);
    }
    physicalKey = KEY_INVALID;
    Tick(2);
    physicalPtt = true;
    Tick(1);
    GAME_ProcessKeys(KEY_MENU, true, false);
    assert(gGameInList);
    physicalPtt = false;
    Tick(2);
    GAME_ProcessKeys(KEY_MENU, true, false);
    assert(gGameInList);
    Tick(1);
    Press(KEY_MENU);
    assert(!gGameInList);
}

static void StartGame(void)
{
    Fixture();
    assert(GAME_Enter());
    ReleaseEntry();
    Press(KEY_MENU);
    assert(!gGameInList);
    gSnakeGame.food = 0;
}
static void TestDirectionsAndMovement(void)
{
    StartGame();
    static const KEY_Code_t keys[] = {KEY_2, KEY_4, KEY_8, KEY_6};
    static const uint8_t dirs[] = {SNAKE_UP, SNAKE_LEFT, SNAKE_DOWN, SNAKE_RIGHT};
    for (unsigned i = 0; i < 4; ++i) {
        ReleaseEntry();
        uint8_t head = SNAKE_CellAt(&gSnakeGame, 2);
        Press(keys[i]);
        assert(gSnakeGame.turnPending && gSnakeGame.nextDirection == dirs[i]);
        /* No second event, even if another direction is submitted without release. */
        GAME_ProcessKeys(keys[(i + 1u) % 4u], true, false);
        assert(gSnakeGame.nextDirection == dirs[i]);
        unsigned ticks = 0;
        while (SNAKE_CellAt(&gSnakeGame, 2) == head && ticks++ < 20)
            Tick(1);
        assert(ticks <= 20 && gSnakeGame.direction == dirs[i]);
        assert(!gSnakeGame.turnPending && gSnakeGame.status == SNAKE_RUNNING);
        Tick(2);
        assert(!gSnakeGame.turnPending); /* held physical key did not queue again */
    }
}
static void TestPauseRestartAndExit(void)
{
    StartGame();
    ReleaseEntry();
    Press(KEY_MENU);
    assert(gSnakeGame.status == SNAKE_PAUSED);
    SnakeState_t paused = gSnakeGame;
    Tick(80);
    assert(memcmp(&paused, &gSnakeGame, sizeof(paused)) == 0);
    ReleaseEntry();
    Press(KEY_2);
    assert(!gSnakeGame.turnPending);
    ReleaseEntry();
    Press(KEY_MENU);
    assert(gSnakeGame.status == SNAKE_RUNNING);
    static const uint8_t terminal[] = {SNAKE_LOST, SNAKE_WON};
    for (unsigned i = 0; i < 2; ++i) {
        gSnakeGame.status = terminal[i];
        gSnakeGame.score = 237;
        ReleaseEntry();
        Press(KEY_MENU);
        assert(gSnakeGame.status == SNAKE_RUNNING && gSnakeGame.score == 0);
        assert(gSnakeGame.length == 3 && !gGameInList);
    }
    ReleaseEntry();
    Press(KEY_EXIT);
    assert(gGameInList && GAME_IsActive() && restores == 0);
    GAME_ProcessKeys(KEY_EXIT, true, false);
    assert(GAME_IsActive()); /* same physical press cannot also leave the list */
    ReleaseEntry();
    Press(KEY_EXIT);
    assert(!GAME_IsActive() && gScreenToDisplay == DISPLAY_MENU);
    assert(menuRestores == 1 && menuIndex == savedMenuIndex);
    assert(gUpdateStatus && gMenuCountdown == menu_timeout_500ms);
    assert(gKeyLockCountdown == 60);
#ifdef ENABLE_FEAT_F4HWN_SLEEP
    assert(gSleepModeCountdown_500ms == 360);
#endif
    assert(GAME_TxBlocked());
    ReleaseExit();
}

static void TestForcedExitsAndPhysicalRelease(void)
{
    static const GUI_DisplayType_t pages[] = {DISPLAY_MAIN, DISPLAY_MENU, DISPLAY_SCANNER};
    for (unsigned i = 0; i < sizeof(pages) / sizeof(pages[0]); ++i) {
        Fixture();
        assert(GAME_Enter());
        GUI_SelectNextDisplay(DISPLAY_GAME);
        GUI_SelectNextDisplay(DISPLAY_INVALID);
        assert(GAME_IsActive());
        physicalPtt = true;
        gPttIsPressed = gPttWasPressed = gFlagPrepareTX = true;
#ifdef ENABLE_VOX
        gVOX_NoiseDetected = g_VOX_Lost = true;
        gVoxResumeCountdown = gVoxPauseCountdown = gVoxStopCountdown_10ms = 99;
#endif
        GUI_SelectNextDisplay(pages[i]);
        assert(!GAME_IsActive() && GAME_TxBlocked() && !voxEnabled);
        assert(!gPttIsPressed && !gPttWasPressed && !gFlagPrepareTX);
#ifdef ENABLE_VOX
        assert(!gVOX_NoiseDetected && !g_VOX_Lost);
        assert(gVoxResumeCountdown == 0 && gVoxPauseCountdown == 0 && gVoxStopCountdown_10ms == 0);
#endif
        unsigned before = restores;
        GAME_OnLeave();
        GAME_Leave();
        assert(restores == before && menuRestores == 0);
        for (unsigned n = 0; n < 10; ++n) {
            assert(GAME_CheckRelease());
            assert(GAME_TxBlocked() && !voxEnabled);
        }
        assert(!GAME_Enter());
        physicalPtt = false;
        /* Side keys and matrix keys independently reset the three-sample barrier. */
        static const KEY_Code_t held[] = {KEY_SIDE1, KEY_SIDE2, KEY_2, KEY_MENU, KEY_PTT};
        for (unsigned k = 0; k < sizeof(held) / sizeof(held[0]); ++k) {
            physicalKey = KEY_INVALID;
            assert(GAME_CheckRelease());
            assert(GAME_CheckRelease());
            physicalKey = held[k];
            assert(GAME_CheckRelease());
            assert(GAME_TxBlocked());
        }
#ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
        gKeyFromSerial = KEY_6;
        assert(GAME_CheckRelease());
        assert(gKeyFromSerial == KEY_INVALID && GAME_TxBlocked());
#endif
        ReleaseExit();
#ifdef ENABLE_VOX
        assert(voxEnabled);
#endif
    }
}
static void TestDeferredVoxRestore(void)
{
    static const FUNCTION_Type_t modes[] = {
        FUNCTION_INCOMING, FUNCTION_TRANSMIT, FUNCTION_RECEIVE, FUNCTION_FOREGROUND
    };
    for (unsigned i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        Fixture();
        assert(GAME_Enter());
        GAME_Leave();
        assert(!GAME_IsActive() && GAME_TxBlocked() && !voxEnabled);
        unsigned before = restores;
        assert(before == 1); /* initial leave restore runs with TX still blocked */
        gCurrentFunction = modes[i];
        physicalKey = KEY_INVALID;
        physicalPtt = false;
        for (unsigned sample = 0; sample < 3; ++sample) {
            assert(GAME_CheckRelease());
            assert(GAME_TxBlocked() == (sample < 2));
            assert(restores == before && !voxEnabled);
        }
        /* The physical barrier is now open even if RF is still busy. */
        unsigned releasedPolls = polls;
        if (modes[i] != FUNCTION_FOREGROUND) {
            for (unsigned call = 0; call < 3; ++call) {
                assert(!GAME_CheckRelease());
                assert(!GAME_TxBlocked() && restores == before && !voxEnabled);
                assert(polls == releasedPolls);
            }
        }
#ifdef ENABLE_VOX
        gVOX_NoiseDetected = g_VOX_Lost = true;
        gVoxResumeCountdown = gVoxPauseCountdown = gVoxStopCountdown_10ms = 99;
#endif
        gCurrentFunction = FUNCTION_FOREGROUND;
        assert(!GAME_CheckRelease());
        assert(!GAME_TxBlocked() && polls == releasedPolls);
#ifdef ENABLE_VOX
        assert(restores == before + 1 && voxEnabled);
        assert(!gVOX_NoiseDetected && !g_VOX_Lost);
        assert(gVoxResumeCountdown == 0 && gVoxPauseCountdown == 0 && gVoxStopCountdown_10ms == 0);
#else
        assert(restores == before && !voxEnabled);
#endif
        unsigned restored = restores;
        for (unsigned call = 0; call < 3; ++call)
            assert(!GAME_CheckRelease());
        gCurrentFunction = modes[i];
        assert(!GAME_CheckRelease());
        gCurrentFunction = FUNCTION_FOREGROUND;
        assert(!GAME_CheckRelease());
        assert(restores == restored && polls == releasedPolls);
    }
}

static void TestExternalExitConditions(void)
{
    /* app.c owns battery/config detection; exercise only the resulting GAME API. */
    for (unsigned mode = 0; mode < 4; ++mode) {
        Fixture();
        assert(GAME_Enter());
        if (mode == 0)
            gLowBattery = true;
        else if (mode == 1)
            gReducedService = true;
        else if (mode == 2)
            gCurrentFunction = FUNCTION_POWER_SAVE;
        else
            serialBusy = true;
        GAME_Leave();
        assert(!GAME_IsActive() && GAME_TxBlocked());
        if (mode == 1 || mode == 2)
            assert(restores == 0);
        ReleaseExit();
        if (mode == 1 || mode == 2)
            assert(restores == 0 && !voxEnabled);
        assert(!GAME_Enter());
    }
}

int main(void)
{
    TestEntryGuards();
    TestEntryShutdownAndMenuGate();
    TestEntryPhysicalRelease();
    TestDirectionsAndMovement();
    TestPauseRestartAndExit();
    TestForcedExitsAndPhysicalRelease();
    TestDeferredVoxRestore();
    TestExternalExitConditions();
    Fixture();
    puts("game session tests passed");
    return 0;
}
