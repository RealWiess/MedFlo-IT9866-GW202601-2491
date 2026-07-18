/** @file
 * ITE Display Control Board Scene Definition.
 *
 * @author Jim Tan
 * @version 1.0
 * @date 2015
 * @copyright ITE Tech. Inc. All Rights Reserved.
 */
/** @defgroup ctrlboard ITE Display Control Board Modules
 *  @{
 */
#ifndef SCENE_H
#define SCENE_H

#include "ite/itu.h"
#include "ctrlboard.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ctrlboard_scene Scene
 *  @{
 */

#define MS_PER_FRAME                50             ///< Drawing delay per frame (20 FPS, down from 58 to reduce CPU load)

extern bool IsDelay;

typedef enum
{
	EVENT_CUSTOM_SCREENSAVER = ITU_EVENT_CUSTOM,    ///< Ready to enter screensaver mode. Custom0 event on GUI Designer.
    //EVENT_CUSTOM_SD_INSERTED,                       ///< #1: SD card inserted.
    //EVENT_CUSTOM_SD_REMOVED,                        ///< #2: SD card removed.
	//EVENT_CUSTOM_USB_INSERTED,                      ///< #3: USB drive inserted.
    //EVENT_CUSTOM_USB_REMOVED,                       ///< #4: USB drive removed.
    //EVENT_CUSTOM_KEY0,                              ///< #5: Key #0 pressed.
    //EVENT_CUSTOM_KEY1,                              ///< #6: Key #1 pressed.
	//EVENT_CUSTOM_KEY2,                              ///< #7: Key #2 pressed.
    //EVENT_CUSTOM_KEY3,                              ///< #8: Key #3 pressed.
    EVENT_CUSTOM_CUART,                        ///< #1: CANBUS UART message.
    EVENT_CUSTOM_MUART,                        ///< #2: MCU UART message.
    EVENT_CUSTOM_PERIPHERAL,                   ///< #3: Peripheral message
    EVENT_CUSTOM_BUTTON,

    EVENT_CUSTOM_KEY_UP = ITU_EVENT_CUSTOM + 10, //RESET
    EVENT_CUSTOM_KEY_DOWN,                      //MILE
    EVENT_CUSTOM_KEY_BOOST,

    EVENT_CUSTOM_MAX,

} CustomEvent;

// scene
/**
 * Initializes scene module.
 */
void SceneInit(void);

/**
 * Exits scene module.
 */
void SceneExit(void);

/**
 * Loads ITU file.
 */
void SceneLoad(void);

/**
 * Runs the main loop to receive events, update and draw scene.
 *
 * @return The QuitValue.
 */
int SceneRun(void);

/**
 * Gotos main menu layer.
 */
void SceneGotoMainMenu(void);

void SceneGotoLayer(uint32_t layer);

/**
 * Sets the status of scene.
 *
 * @param ready true for ready, false for not ready yet.
 */
void SceneSetReady(bool ready);

/**
 * Quits the scene.
 *
 * @param value The reason to quit the scene.
 */
void SceneQuit(QuitValue value);

/**
 * Gets the current quit value.
 *
 * @return The current quit value.
 */
QuitValue SceneGetQuitValue(void);

void SceneEnterVideoState(int timePerFrm);
void SceneLeaveVideoState(void);

/**
 * Changes language file.
 */
void SceneChangeLanguage(void);

/**
 * Predraw scene.
 *
 * @param arg Unused.
 */
extern void itv_set_video_window(uint32_t startX, uint32_t startY, uint32_t width, uint32_t height);
extern void BackupSave(void);
extern void BackupInit(void);
extern void BackupRestore(void);
extern void BackupSyncFile(void);
extern void BackupDestroy(void);
extern void malloc_stats(void);
extern int ugRestoreDir(const char* dest, const char* src);

void ScenePredraw(int arg);
extern void ScreenSetDoubleClick(void);
extern int ScreenSaverCheckForDoubleClick(void);
extern void ScreenOffContinue(void);
#ifdef CFG_MIRROR_TEST
extern ITUButton* projectChangeUIButton;
extern ITUButton* projectVChangeUIButton;
extern bool ProjectChangeUIBtOnMouseUp(ITUWidget* widget, char* param);
extern bool ProjectVChangeUIBtOnMouseUp(ITUWidget* widget, char* param);
extern ITULayer* projectLayer;
//extern ITULayer* projectVLayer;
extern bool mirror_initial;
//extern bool showingPhoneContainer;
extern bool showingHorizontalBackground;
extern bool is_central_pos_p;
extern bool is_central_pos_v;
extern bool mirrorReady;
#endif
extern bool showMirror;
extern bool showReverse;

void show_mirror_window(int w, int h);

extern ITULayer* dashboardLayer;
extern ITULayer* topLayer;
extern ITULayer* callLayer;
extern ITUBackground* callBackground;
extern ITUText* callPhoneText;
extern ITUText* callNameText;
extern ITUText* callTimeText;
extern ITUSprite* callSprite;
extern ITUText* callVTimeText;
extern ITUSprite* callVSprite;
//void unshow_mirror_window(int w, int h);

/**
 * Global instance variable of scene.
 */
extern ITUScene theScene;

/** @} */ // end of ctrlboard_scene

#ifdef __cplusplus
}
#endif

#endif /* SCENE_H */
/** @} */ // end of ctrlboard