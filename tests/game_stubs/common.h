/* SPDX-License-Identifier: Apache-2.0 */
#ifndef TEST_GAME_STUBS_H
#define TEST_GAME_STUBS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
/* Use the real key/display/function enums, not copies of game logic. */
#include "../../App/driver/keyboard.h"
#include "../../App/ui/ui.h"
#include "../../App/functions.h"

#ifdef GAME_STUB_DEFINE
#define STORAGE
#else
#define STORAGE extern
#endif

STORAGE bool gPttIsPressed, gPttWasPressed, gPttWasReleased, gKeyBeingHeld;
STORAGE bool gFlagPrepareTX, gVOX_NoiseDetected, g_VOX_Lost;
STORAGE bool gSchedulePowerSave, gPowerSaveCountdownExpired, gScheduleDualWatch;
STORAGE bool gScheduleScanListen, gRxVfoIsActive, gScheduleNOAA, gScheduleFM;
STORAGE bool gMonitor, gReducedService, gLowBattery, gCssBackgroundScan;
STORAGE bool gFlagReconfigureVfos, gFlagAcceptSetting, gRequestSaveSettings;
STORAGE bool gRequestSaveVFO, gFlagRefreshSetting, gFmRadioMode;
STORAGE bool gEnableSpeaker, gUpdateStatus, gUpdateDisplay;
STORAGE uint8_t gPttDebounceCounter, gRTTECountdown_10ms, gVfoConfigureMode;
STORAGE uint8_t gDualPttTxVfo, gSetting_set_off, gRxReceptionMode;
STORAGE uint8_t gFM_ScanState, gBeepToPlay, gBatteryIconFillPercent;
STORAGE int8_t gScanStateDir;
STORAGE uint16_t gVoxResumeCountdown, gVoxPauseCountdown, gVoxStopCountdown_10ms;
STORAGE uint16_t gBatterySaveCountdown_10ms, gDualWatchCountdown_10ms;
STORAGE uint16_t gNOAA_Countdown_10ms, gFM_RestoreCountdown_10ms;
STORAGE uint16_t gMenuCountdown, gKeyLockCountdown, gSleepModeCountdown_500ms;
STORAGE uint16_t gRequestSaveChannel, gBatteryVoltageAverage;
STORAGE uint8_t gUiLanguage;
typedef struct { uint8_t AUTO_KEYPAD_LOCK; } StubEeprom;
STORAGE StubEeprom gEeprom;

/* Definitions for declarations coming from the real headers above. */
#ifdef GAME_STUB_DEFINE
KEY_Code_t gKeyReading0, gKeyReading1;
uint16_t gDebounceCounter;
#ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
volatile KEY_Code_t gKeyFromSerial;
#endif
GUI_DisplayType_t gScreenToDisplay, gRequestDisplayScreen;
FUNCTION_Type_t gCurrentFunction;
#endif

#define SCAN_OFF 0
#define FM_SCAN_OFF 0
#define VFO_CONFIGURE_NONE 0
#define RX_MODE_NONE 0
#define BEEP_NONE 0
#define BK4819_REG_3F 0x3f
#define BK4819_REG_02 0x02
#define BK4819_GPIO1_PIN29_PA_ENABLE 1
#define BK4819_GPIO0_PIN28_RX_ENABLE 0
#define battery_save_count_10ms 100u
#define dual_watch_count_after_rx_10ms 200u
#define NOAA_countdown_10ms 300u
#define menu_timeout_500ms 60u

bool SerialConfigInProgress(void);
bool SCANNER_IsScanning(void);
bool GPIO_IsPttPressed(void);
void MENU_ActivateMainPage(void);
void MDC1200_AppDisableRx(void);
void YAN_RF_AbortRx(void);
void AUDIO_AudioPathOff(void);
void BK4819_DisableVox(void);
void BK4819_WriteRegister(uint8_t reg, uint16_t value);
void BK4819_SetupPowerAmplifier(uint8_t bias, uint32_t frequency);
void BK4819_ToggleGpioOut(uint8_t pin, bool enabled);
void BK4819_Sleep(void);
void BACKLIGHT_TurnOn(void);
void RADIO_SetupRegisters(bool switching);

#define LCD_WIDTH 128
#define FRAME_LINES 8
#define UI_LANGUAGE_CN 1
#define CN_FONT_FLASH_BASE 0x024000u
#define CN_FONT_VERSION_OFFSET 205366u
#define CN_FONT_VERSION 2u

typedef struct {
    uint8_t before[16];
    uint8_t data[FRAME_LINES][LCD_WIDTH];
    uint8_t after[16];
} StubFrame;
typedef struct {
    uint8_t before[16];
    uint8_t data[LCD_WIDTH];
    uint8_t after[16];
} StubStatus;
STORAGE StubFrame stubFrame;
STORAGE StubStatus stubStatus;
#define gFrameBuffer (stubFrame.data)
#define gStatusLine (stubStatus.data)
void ST7565_BlitFullScreen(void);
void ST7565_BlitStatusLine(void);
void PY25Q16_ReadBuffer(uint32_t address, void *buffer, uint32_t size);
int16_t SETTINGS_CNCharToIndex(uint16_t character);

#undef STORAGE
#endif
