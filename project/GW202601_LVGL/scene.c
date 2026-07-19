#include <sys/ioctl.h>
#include <sys/time.h>
#include <assert.h>
#include <math.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "SDL/SDL.h"
#include "ite/itp.h"
#include "scene.h"
#include "ctrlboard.h"

#include "scene_utility.h"
#include "peripheral.h"
#include "bd/bt_main.h"
#include "bd/bt_wifi.h"

#if defined (CFG_AMP_TEST) && defined (CFG_AMPLIFIER_ENABLE) && defined (CFG_AUDIO_ENABLE)
#include "module_audio.h"

#define WAIT_TIME  10 //rough 1 second
#endif

#ifdef _WIN32
    #include <crtdbg.h>

#ifndef CFG_VIDEO_ENABLE
    #define DISABLE_SWITCH_VIDEO_STATE
#endif
#endif // _WIN32

#ifndef CFG_POWER_WAKEUP_DOUBLE_CLICK_INTERVAL
	#define DOUBLE_KEY_INTERVAL 200
#endif

#if defined(__OPENRTOS__) && defined(CFG_BTA_ENABLE)
extern bool gSleepModeBT;
#endif

#define FPS_ENABLE
#define DOUBLE_KEY_ENABLE

#define GESTURE_THRESHOLD           40
#define MAX_COMMAND_QUEUE_SIZE      8
#define MOUSEDOWN_LONGPRESS_DELAY   1000

extern ITUActionFunction actionFunctions[];
extern void resetScene(void);

// status
static QuitValue quitValue;
static bool      inVideoState;

// command
typedef enum
{
    CMD_NONE,
    CMD_LOAD_SCENE,
    CMD_CALL_CONNECTED,
    CMD_GOTO_MAINMENU,
    CMD_GOTO_SETCLOCK,
    CMD_GOTO_ENGINEERMODE,
    CMD_CHANGE_LANG,
    CMD_PREDRAW,
	CMD_GOTO_PROJECTV,
	CMD_GOTO_PROJECT,
	CMD_MIRROR_H,
	CMD_MIRROR_V
} CommandID;

#define MAX_STRARG_LEN 32

typedef struct
{
    CommandID   id;
    int         arg1;
    int         arg2;
    char        strarg1[MAX_STRARG_LEN];
} Command;

static mqd_t commandQueue = -1;

// scene
ITUScene theScene;
static SDL_Window *window;
static ITUSurface* screenSurf;
static int screenWidth;
static int screenHeight;
static float screenDistance;
static bool isReady;
static int periodPerFrame;

#if defined(CFG_USB_MOUSE) || defined(_WIN32)
static ITUIcon* cursorIcon;
#endif

bool IsDelay = true;
bool showMirror = false;
extern ITULayer* CurrentShowLayer;

#if defined (CFG_AMP_TEST) && defined (CFG_AMPLIFIER_ENABLE) && defined (CFG_AUDIO_ENABLE)
static uint32_t audioTick = 0 , lastAudioTick = 0;
static int play_wait_time = WAIT_TIME;

#endif

void SceneInit(void)
{
    struct mq_attr attr;
	ITURotation rot;

#ifdef CFG_LCD_ENABLE
    screenWidth = ithLcdGetWidth();
    screenHeight = ithLcdGetHeight();

    window = SDL_CreateWindow("Display Control Board", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, 0);
    if (!window)
    {
        printf("Couldn't create window: %s\n", SDL_GetError());
        return;
    }

    // init itu
    ituLcdInit();

#ifdef CFG_M2D_ENABLE
    ituM2dInit();
#else
    ituSWInit();
#endif // CFG_M2D_ENABLE

    ituSceneInit(&theScene, NULL);
#ifndef _WIN32
#ifdef CFG_ENABLE_ROTATE
#ifdef CFG_MIRROR_TEST
    ituSetRotation(ITU_ROT_0);
#else
    ituSetRotation(ITU_ROT_0);
#endif
#endif
#endif
#ifdef CFG_VIDEO_ENABLE
    ituFrameFuncInit();
#endif // CFG_VIDEO_ENABLE

#ifdef CFG_PLAY_VIDEO_ON_BOOTING
#ifndef CFG_BOOT_VIDEO_ENABLE_WINDOW_MODE
    rot = itv_get_rotation();

    if (rot == ITU_ROT_90 || rot == ITU_ROT_270)
        PlayVideo(0, 0, ithLcdGetHeight(), ithLcdGetWidth(), CFG_BOOT_VIDEO_BGCOLOR, CFG_BOOT_VIDEO_VOLUME);
    else
    	PlayVideo(0, 0, ithLcdGetWidth(), ithLcdGetHeight(), CFG_BOOT_VIDEO_BGCOLOR, CFG_BOOT_VIDEO_VOLUME);
#else
    PlayVideo(CFG_VIDEO_WINDOW_X_POS, CFG_VIDEO_WINDOW_Y_POS, CFG_VIDEO_WINDOW_WIDTH, CFG_VIDEO_WINDOW_HEIGHT, CFG_BOOT_VIDEO_BGCOLOR, CFG_BOOT_VIDEO_VOLUME);
#endif
#endif

#ifdef CFG_PLAY_MJPEG_ON_BOOTING
#ifndef CFG_BOOT_VIDEO_ENABLE_WINDOW_MODE
    rot = itv_get_rotation();

    if (rot == ITU_ROT_90 || rot == ITU_ROT_270)
        PlayMjpeg(0, 0, ithLcdGetHeight(), ithLcdGetWidth(), CFG_BOOT_VIDEO_BGCOLOR, 0);
    else
    	PlayMjpeg(0, 0, ithLcdGetWidth(), ithLcdGetHeight(), CFG_BOOT_VIDEO_BGCOLOR, 0);
#else
	PlayMjpeg(CFG_VIDEO_WINDOW_X_POS, CFG_VIDEO_WINDOW_Y_POS, CFG_VIDEO_WINDOW_WIDTH, CFG_VIDEO_WINDOW_HEIGHT, CFG_BOOT_VIDEO_BGCOLOR, 0);
#endif
#endif


    screenSurf = ituGetDisplaySurface();

    ituFtInit();
    ituFtLoadFont(0, CFG_PRIVATE_DRIVE ":/font/" CFG_FONT_FILENAME, ITU_GLYPH_8BPP);
	ituFtLoadFont(1, CFG_PRIVATE_DRIVE ":/font/" "IBMPlexMono-Bold.ttf", ITU_GLYPH_8BPP);

    //ituSceneInit(&theScene, NULL);
	ituSceneSetFunctionTable(&theScene, actionFunctions);

    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_COMMAND_QUEUE_SIZE;
    attr.mq_msgsize = sizeof(Command);

    commandQueue = mq_open("scene", O_CREAT | O_NONBLOCK, 0644, &attr);
    assert(commandQueue != -1);

    screenDistance = sqrtf(screenWidth * screenWidth + screenHeight * screenHeight);

    isReady = false;
    periodPerFrame = MS_PER_FRAME;
#endif

}

void SceneExit(void)
{
#ifdef CFG_LCD_ENABLE
    mq_close(commandQueue);
    commandQueue = -1;

    resetScene();

    if (theScene.root)
    {
        ituSceneExit(&theScene);
    }
    ituFtExit();

#ifdef CFG_M2D_ENABLE
    ituM2dExit();
#ifdef CFG_VIDEO_ENABLE
    ituFrameFuncExit();
#endif // CFG_VIDEO_ENABLE
#else
    ituSWExit();
#endif // CFG_M2D_ENABLE

    SDL_DestroyWindow(window);
#endif
}

void SceneLoad(void)
{
    Command cmd;

    if (commandQueue == -1)
        return;

    isReady  = false;

    cmd.id = CMD_LOAD_SCENE;

    mq_send(commandQueue, (const char*)&cmd, sizeof (Command), 0);
}

void SceneGotoMainMenu(void)
{
    Command cmd;

    if (commandQueue == -1)
        return;

    cmd.id     = CMD_GOTO_MAINMENU;
    mq_send(commandQueue, (const char*)&cmd, sizeof (Command), 0);
}

void SceneGotoLayer(uint32_t layer)
{
    Command cmd;

    if (commandQueue == -1)
        return;

    cmd.id     = layer;
    mq_send(commandQueue, (const char*)&cmd, sizeof (Command), 0);
}

void SceneChangeLanguage(void)
{
    Command cmd;

    if (commandQueue == -1)
        return;

    cmd.id     = CMD_CHANGE_LANG;
    mq_send(commandQueue, (const char*)&cmd, sizeof (Command), 0);
}

void ScenePredraw(int arg)
{
    Command cmd;

    if (commandQueue == -1)
        return;

    cmd.id     = CMD_PREDRAW;
    mq_send(commandQueue, (const char*)&cmd, sizeof (Command), 0);
}

void SceneSetReady(bool ready)
{
    isReady = ready;
}

static void LoadScene(void)
{
#ifdef CFG_LCD_ENABLE
    uint32_t tick1, tick2;

	resetScene();
    if (theScene.root)
    {
        ituSceneExit(&theScene);
    }

    // load itu file
    tick1 = SDL_GetTicks();

#ifdef CFG_LCD_MULTIPLE
    {
        char filepath[PATH_MAX];

        sprintf(filepath, CFG_PRIVATE_DRIVE ":/itu/%ux%u/ctrlboard.itu", ithLcdGetWidth(), ithLcdGetHeight());
        ituSceneLoadFileCore(&theScene, filepath);
    }
#else
    ituSceneLoadFileCore(&theScene, CFG_PRIVATE_DRIVE ":/automotive_display.itu");
#endif // CFG_LCD_MULTIPLE

    tick2 = SDL_GetTicks();
    printf("itu loading time: %dms\n", tick2 - tick1);

    /* 清除預設文字 + 全螢幕 + 黑底淺藍字，避免白色閃現 */
    ITUTextBox* tb = (ITUTextBox*)ituSceneFindWidget(&theScene, "TextBox1");
    if (tb) {
        ituWidgetSetX(tb, 0);
        ituWidgetSetY(tb, 0);
        ituWidgetSetWidth(tb, 480);
        ituWidgetSetHeight(tb, 480);
        tb->textboxFlags |= ITU_TEXTBOX_MULTILINE;
        tb->lineHeight = 30;
        tb->text.layout = ITU_LAYOUT_TOP_LEFT;
        ituSetColor(&tb->fgColor,   255, 100, 180, 255);
        ituSetColor(&tb->defColor,  255, 100, 180, 255);
        ituSetColor(&tb->text.bgColor, 255, 0, 0, 0);
        ituTextBoxSetString(tb, "FW: " FW_BUILD_TIME);
    }
    ITUBackground* bg = (ITUBackground*)ituSceneFindWidget(&theScene, "fakeBackground");
    if (bg) {
        ituWidgetSetColor((ITUIcon*)bg, 255, 0, 0, 0);
    }

    /* TODO: logoLayer 的 OnEnter 會顯示版本頁，需要從 .itu 檔修改 */

    /* Load external PNG images for ring UI icons */
    {
        static const char* icon_names[] = {
            "bg_image",
            "wifi_status_ring_icon",
            "wifi_warn_ring_icon",
            "ble_scan_ring_icon",
            "ble_err_ring_icon",
            "gateway_center_icon",
            "warn_center_icon",
            NULL
        };
        for (int i = 0; icon_names[i]; i++) {
            ITUIcon* icon = (ITUIcon*)ituSceneFindWidget(&theScene, icon_names[i]);
            if (icon) {
                char path[64];
                snprintf(path, sizeof(path), CFG_PRIVATE_DRIVE ":/%s.png", icon_names[i]);
                ITUSurface* surf = ituPngLoadFile(0, 0, path, 0);
                if (surf) {
                    icon->surf = surf;
                    printf("[RingUI] loaded %s\n", path);
                } else {
                    printf("[RingUI] FAILED to load %s\n", path);
                }
            }
        }
    }

    if (theConfig.lang != LANG_ENG)
        ituSceneUpdate(&theScene, ITU_EVENT_LANGUAGE, theConfig.lang, 0, 0);

#ifdef CFG_ENABLE_ROTATE
    ituSetRotation(ITU_ROT_90);
    //ituSceneSetRotation(&theScene, ITU_ROT_90, ithLcdGetHeight(), ithLcdGetWidth()); //NTUT Hardy
    //ituSceneSetRotation(&theScene, ITU_ROT_90, ithLcdGetWidth(), ithLcdGetHeight());
#endif

    tick1 = tick2;

#if defined(CFG_USB_MOUSE) || defined(_WIN32)
    cursorIcon = ituSceneFindWidget(&theScene, "cursorIcon");
    if (cursorIcon)
    {
        ituWidgetSetVisible(cursorIcon, true);
    }
#endif // defined(CFG_USB_MOUSE) || defined(_WIN32)

    tick2 = SDL_GetTicks();
    printf("itu init time: %dms\n", tick2 - tick1);

    //ExternalProcessInit();
#endif
}

void SceneEnterVideoState(int timePerFrm)
{
    if (inVideoState)
    {
        return;
    }

#ifndef DISABLE_SWITCH_VIDEO_STATE
    #ifdef CFG_VIDEO_ENABLE
    ituFrameFuncInit();
    #endif
    screenSurf   = ituGetDisplaySurface();
    inVideoState = true;
    if (timePerFrm != 0)
        periodPerFrame = timePerFrm;
#endif
}

void SceneLeaveVideoState(void)
{
    if (!inVideoState)
    {
        return;
    }

#ifndef DISABLE_SWITCH_VIDEO_STATE
    #ifdef CFG_VIDEO_ENABLE
    ituFrameFuncExit();
    #endif
    #ifdef CFG_LCD_ENABLE
    ituLcdInit();
    #endif
    #ifdef CFG_M2D_ENABLE
    ituM2dInit();
    #else
    ituSWInit();
    #endif

    screenSurf   = ituGetDisplaySurface();
    periodPerFrame = MS_PER_FRAME;
#endif
    inVideoState = false;
}

static void GotoMainMenu(void)
{
    ITULayer* mainMenuLayer = ituSceneFindWidget(&theScene, "mainMenuLayer");
    assert(mainMenuLayer);
    ituLayerGoto(mainMenuLayer);
}

static void GotoLayer(char* layername)
{
    ITULayer* layer = ituSceneFindWidget(&theScene, layername);
    assert(layer);
    ituLayerGoto(layer);
}

static void ProcessCommand(void)
{
    Command cmd;
    ITULayer* projectLayer;
    ITULayer* layer = (ITULayer*)ituGetVarTarget(0);
    while (mq_receive(commandQueue, (char*)&cmd, sizeof(Command), 0) > 0)
    {
        switch (cmd.id)
        {
        case CMD_LOAD_SCENE:
            LoadScene();
#if defined(CFG_PLAY_VIDEO_ON_BOOTING)
            ituScenePreDraw(&theScene, screenSurf);
            WaitPlayVideoFinish();
#elif defined(CFG_PLAY_MJPEG_ON_BOOTING)
            ituScenePreDraw(&theScene, screenSurf);
			WaitPlayMjpegFinish();
#else
			ituScenePreDraw(&theScene, screenSurf); //for main Layer
#endif
            ituSceneStart(&theScene);
            break;

        case CMD_GOTO_MAINMENU:
            GotoLayer("dashboardLayer");
            break;

        case CMD_GOTO_SETCLOCK:
            GotoLayer("setClockLayer");
            break;
        case CMD_GOTO_ENGINEERMODE:
            GotoLayer("engineerModeLayer");
            break;

        case CMD_CHANGE_LANG:
            ituSceneUpdate(&theScene, ITU_EVENT_LANGUAGE, theConfig.lang, 0, 0);
            ituSceneUpdate(&theScene, ITU_EVENT_LAYOUT, 0, 0, 0);
            break;

#if !defined(CFG_PLAY_VIDEO_ON_BOOTING) && !defined(CFG_PLAY_MJPEG_ON_BOOTING)
        case CMD_PREDRAW:
            ituScenePreDraw(&theScene, screenSurf);
            break;
#endif
#if 0
        case CMD_GOTO_PROJECTV:
            ituLayerGoto(projectVLayer);
            break;
#endif
        case CMD_GOTO_PROJECT:
            printf("CMD_GOTO_PROJECT\n");
            projectLayer = ituSceneFindWidget(&theScene, "projectLayer");
            ituLayerGoto(projectLayer);
            break;

        case CMD_MIRROR_H:
#ifdef CFG_MIRROR_TEST
            projectLayer = ituSceneFindWidget(&theScene, "projectLayer");
            dashboardLayer = ituSceneFindWidget(&theScene, "dashboardLayer");
            printf("[CMD_MIRROR_H](CurrentShowLayer == projectLayer):%d\n",(CurrentShowLayer == projectLayer));
            if (CurrentShowLayer == projectLayer) {
                printf("mirror h project\n");
                is_central_pos_p = false;
                showingHorizontalBackground = false;
                ituWidgetEnable(projectChangeUIButton);
                ituSceneExecuteCommand(&theScene, 1, (ITUCommandFunc)ProjectChangeUIBtOnMouseUp, 0);

            } else {
                printf("mirror h projectV\n");
                //is_central_pos_v = false;
                //showingHorizontalBackground = false;
                //ituWidgetDisable(projectVChangeUIButton);
                //ituSceneExecuteCommand(&theScene, 1, (ITUCommandFunc)ProjectVChangeUIBtOnMouseUp, 0);
            }
#endif
            break;
        case CMD_MIRROR_V:
#ifdef CFG_MIRROR_TEST
            projectLayer = ituSceneFindWidget(&theScene, "projectLayer");
            dashboardLayer = ituSceneFindWidget(&theScene, "dashboardLayer");
            printf("[CMD_MIRROR_V](CurrentShowLayer == projectLayer):%d\n",(CurrentShowLayer == projectLayer));
            if (CurrentShowLayer == projectLayer ) {
                printf("mirror v project\n");
                is_central_pos_p = true;
                ituWidgetDisable(projectChangeUIButton);
                ituSceneExecuteCommand(&theScene, 1, (ITUCommandFunc)ProjectChangeUIBtOnMouseUp, 0);
            } else  {
                printf("mirror v projectV\n");
                //is_central_pos_v = true;
                //ituWidgetEnable(projectVChangeUIButton);
                //ituSceneExecuteCommand(&theScene, 1, (ITUCommandFunc)ProjectVChangeUIBtOnMouseUp, 0);
            }
#endif
            break;
        }
    }
}

static bool CheckQuitValue(void)
{
    if (quitValue)
    {
        if (ScreenSaverIsScreenSaving() && theConfig.screensaver_type == SCREENSAVER_BLANK)
            ScreenSaverRefresh();

        return true;
    }
    return false;
}

static void CheckStorage(void)
{
#if 0
    StorageAction action = StorageCheck();

    switch (action)
    {
    case STORAGE_SD_INSERTED:
        ituSceneSendEvent(&theScene, EVENT_CUSTOM_SD_INSERTED, NULL);
        break;

    case STORAGE_SD_REMOVED:
        ituSceneSendEvent(&theScene, EVENT_CUSTOM_SD_REMOVED, NULL);
        break;

    case STORAGE_USB_INSERTED:
        ituSceneSendEvent(&theScene, EVENT_CUSTOM_USB_INSERTED, NULL);
        break;

    case STORAGE_USB_REMOVED:
        ituSceneSendEvent(&theScene, EVENT_CUSTOM_USB_REMOVED, NULL);
        break;

    case STORAGE_USB_DEVICE_INSERTED:
        {
            ITULayer* usbDeviceModeLayer = ituSceneFindWidget(&theScene, "usbDeviceModeLayer");
            assert(usbDeviceModeLayer);

            ituLayerGoto(usbDeviceModeLayer);
        }
        break;

    case STORAGE_USB_DEVICE_REMOVED:
        {
            ITULayer* mainMenuLayer = ituSceneFindWidget(&theScene, "mainMenuLayer");
            assert(mainMenuLayer);

            ituLayerGoto(mainMenuLayer);
        }
        break;
    }
#endif
}

//static void CheckExternal(void)
//{
//    ExternalEvent ev;
//    int ret = ExternalReceive(&ev);
//
//    if (ret)
//    {
//        ScreenSaverRefresh();
//        ExternalProcessEvent(&ev);
//    }
//}

#if defined(CFG_USB_MOUSE) || defined(_WIN32)

static void CheckMouse(void)
{
    if (ioctl(ITP_DEVICE_USBMOUSE, ITP_IOCTL_IS_AVAIL, NULL))
    {
        if (!ituWidgetIsVisible(cursorIcon))
            ituWidgetSetVisible(cursorIcon, true);
    }
    else
    {
        if (ituWidgetIsVisible(cursorIcon))
            ituWidgetSetVisible(cursorIcon, false);
    }
}
#endif // defined(CFG_USB_MOUSE) || defined(_WIN32)

#if defined (CFG_AMP_TEST) && defined (CFG_AMPLIFIER_ENABLE) && defined (CFG_AUDIO_ENABLE)
static void CheckAudioPlayStatus(void)
{
    audioTick = SDL_GetTicks();

    if (audioTick - lastAudioTick < 100) {
        return;
    }

    lastAudioTick = audioTick;

    if (AudioStatus() == false || getAudioIndex() == No_Audio) {

        if ((--play_wait_time) == 0) {
            initAudioIndex();
            SetAudioVolume(theConfig.audiolevel);
            PlayAudio(KillBill);
            play_wait_time = WAIT_TIME;
        }
    }

}

#endif

/* ================================================================ */
/*  Status Display — 在 demoLayer 的 TextBox1 顯示系統狀態           */
/* ================================================================ */

static uint32_t s_status_last_update = 0;
#define STATUS_UPDATE_MS 1000

static void StatusDisplay_Update(void)
{
    uint32_t now = SDL_GetTicks();
    if (now - s_status_last_update < STATUS_UPDATE_MS)
        return;
    s_status_last_update = now;

    ITUTextBox* tb = (ITUTextBox*)ituSceneFindWidget(&theScene, "TextBox1");
    if (!tb) return;

    uint8_t flags = nmgw_status_get_flags();

    /* WiFi info */
    char ip[16] = "---";
    char ssid[64] = "---";
    bt_wifi_get_ip(ip, sizeof(ip));
    // 顯示目前連線目標的 SSID（非 config 中的 SSID1）
    extern char g_current_ssid[64];
    if (g_current_ssid[0])
        strncpy(ssid, g_current_ssid, sizeof(ssid) - 1);
    else if (theConfig.ssid[0])
        strncpy(ssid, theConfig.ssid, sizeof(ssid) - 1);

    /* Build status text (left-aligned, no leading spaces) */
    int medflo_count = nmgw_medflo_device_count();
    char buf[512];

    // WiFi driver ready?
    const char* hw = (flags & NMGW_FLAG_WIFI_READY) ? "OK" : "FAIL";

    // WiFi connected?
    const char* conn = (flags & NMGW_FLAG_WIFI_OK) ? "OK" : "--";

    // Disconnect reason 轉成文字
    uint8_t disc_reason = nmgw_get_disconnect_reason();
    const char* reason_str;
    switch (disc_reason) {
        case 1: reason_str = "TEMP_DISC"; break;
        case 2: reason_str = "DISC_30S";  break;
        case 3: reason_str = "CONN_FAIL"; break;
        default: reason_str = "-"; break;
    }

    /* BLE init step: 1=waiting WiFi, 2-4=BLE init, 5=ready */
    extern int g_ble_init_step;
    const char* ble_status;
    if (!(flags & NMGW_FLAG_BLE_OK)) {
        switch (g_ble_init_step) {
            case 0:  ble_status = "[wait]"; break;
            case 1:  ble_status = "[wait WiFi]"; break;
            case 2:
            case 3:
            case 4:  ble_status = "[init...]"; break;
            default: ble_status = "[--]"; break;
        }
    } else {
        ble_status = "[OK]";
    }

    /* WiFi status: show scanning/connecting during boot */
    const char* wifi_status;
    if (g_ble_init_step <= 1 && !(flags & NMGW_FLAG_WIFI_OK)) {
        wifi_status = (flags & NMGW_FLAG_WIFI_READY) ? "scanning..." : "init...";
    } else {
        wifi_status = conn;
    }

    /* AP whitelist: 顯示兩個 SSID，標示目前連上的 */
    extern char g_current_ssid[64];
    const char* ap1_mark = "[--]";
    const char* ap2_mark = "[--]";
    if (flags & NMGW_FLAG_WIFI_OK) {
        if (strcmp(g_current_ssid, theConfig.ssid) == 0)
            ap1_mark = "[OK]";
        else if (strlen(theConfig.ssid2) > 0 && strcmp(g_current_ssid, theConfig.ssid2) == 0)
            ap2_mark = "[OK]";
    }

    char ap_list[128] = "";
    snprintf(ap_list, sizeof(ap_list), "%s %s", ap1_mark, theConfig.ssid);
    if (strlen(theConfig.ssid2) > 0) {
        int len = strlen(ap_list);
        snprintf(ap_list + len, sizeof(ap_list) - len, "\n       %s %s", ap2_mark, theConfig.ssid2);
    }

    /* Scan results: 顯示前 6 個 AP */
    extern char g_scan_ssid[20][33];
    extern int  g_scan_rssi[20];
    extern int  g_scan_count;
    char scan_buf[256] = "";
    if (g_scan_count > 0) {
        int n = (g_scan_count < 6) ? g_scan_count : 6;
        int off = 0;
        for (int i = 0; i < n && off < (int)sizeof(scan_buf) - 30; i++) {
            off += snprintf(scan_buf + off, sizeof(scan_buf) - off,
                           "  %s (%d)\n", g_scan_ssid[i], g_scan_rssi[i]);
        }
        // remove trailing newline
        int slen = strlen(scan_buf);
        if (slen > 0 && scan_buf[slen-1] == '\n') scan_buf[slen-1] = '\0';
    }

    snprintf(buf, sizeof(buf),
        "MedFlo Gateway Status\n"
        "\n"
        "WiFi : [HW:%s] [%s]\n"
        "       %s\n"
        "       %s\n"
        "\n"
        "APs  :\n%s\n"
        "\n"
        "BLE  : %s  (%d)\n"
        "\n"
        "MQTT : %s\n"
        "OTA  : %s\n"
        "\n"
        "FW: " FW_BUILD_TIME "\n"
        "DC: %d  T:%d D:%d C:%d",
        hw, wifi_status,
        ap_list,
        ip,
        scan_buf,
        ble_status,
        medflo_count,
        (flags & NMGW_FLAG_MQTT_OK) ? "[OK]" : "[--]",
        (flags & NMGW_FLAG_OTA_OK)  ? "[OK]" : "[--]",
        nmgw_get_disconnect_count(),
        nmgw_get_disc_count_temp(),
        nmgw_get_disc_count_30s(),
        nmgw_get_disc_count_fail()
    );

    ituTextBoxSetString(tb, buf);
}

int SceneRun(void)
{
    SDL_Event ev;
    int delay, frames, lastx, lasty;
    uint32_t tick, dblclk, lasttick, mouseDownTick;
    static bool first_screenSurf = true , sleepModeDoubleClick = false;
#if defined(CFG_POWER_WAKEUP_IR)
    static bool sleepModeIR = false;
#endif

    /* Watch keystrokes */
    dblclk = frames = lasttick = lastx = lasty = mouseDownTick = 0;

    for (;;)
    {
        bool result = false;

        if (CheckQuitValue())
            break;

#ifdef CFG_LCD_ENABLE
        ProcessCommand();
#endif
        //updateDashboardData();
        updatePeripheral();
        StatusDisplay_Update();

        //CheckExternal();
        CheckStorage();

    #if defined(CFG_USB_MOUSE) || defined(_WIN32)
        if (cursorIcon)
            CheckMouse();
    #endif // defined(CFG_USB_MOUSE) || defined(_WIN32)

        tick = SDL_GetTicks();

    #ifdef FPS_ENABLE
        frames++;
        if (tick - lasttick >= 1000)
        {
            printf("fps: %d\n", frames);
            frames = 0;
            lasttick = tick;
        }
    #endif // FPS_ENABLE


#if defined (CFG_AMP_TEST) && defined (CFG_AMPLIFIER_ENABLE) && defined (CFG_AUDIO_ENABLE)
        CheckAudioPlayStatus();
#endif

#ifdef CFG_LCD_ENABLE
        while (SDL_PollEvent(&ev))
        {
            switch (ev.type)
            {
            case SDL_KEYDOWN:
                ScreenSaverRefresh();
                result = ituSceneUpdate(&theScene, ITU_EVENT_KEYDOWN, ev.key.keysym.sym, 0, 0);
                switch (ev.key.keysym.sym)
                {
                #if 0
                case SDLK_LEFT:
                    ituSceneSendEvent(&theScene, EVENT_CUSTOM_KEY2, NULL);
                    break;
                #endif
                case SDLK_INSERT:
                    break;

            #ifdef _WIN32
                case SDLK_UP:
                    ituSceneSendEvent(&theScene, EVENT_CUSTOM_KEY_UP, NULL);
                    //printf("EVENT_CUSTOM_KEY_UP\n");
                    break;

                case SDLK_DOWN:
                    ituSceneSendEvent(&theScene, EVENT_CUSTOM_KEY_DOWN, NULL);
                    //printf("EVENT_CUSTOM_KEY_DOWN\n");
                    break;

                case SDLK_RIGHT:
                    ituSceneSendEvent(&theScene, EVENT_CUSTOM_KEY_BOOST, NULL);
                    break;
                case SDLK_d:
                    GotoLayer("dashboardLayer");
                    break;
                case SDLK_s:
                    GotoLayer("setClockLayer");
                break;
                case SDLK_e:
                    GotoLayer("engineerModeLayer");
                break;
                case SDLK_l:
                    {
                        ITULayer* Layer1 = ituSceneFindWidget(&theScene, "Layer1");
                        assert(Layer1);

                        ituLayerGoto(Layer1);
                    }
                break;
                //case SDLK_e:
                //    result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHPINCH, 20, 30, 40);
                //    break;

                case SDLK_f:
                    {
                        ITULayer* usbDeviceModeLayer = ituSceneFindWidget(&theScene, "usbDeviceModeLayer");
                        assert(usbDeviceModeLayer);

                        ituLayerGoto(usbDeviceModeLayer);
                    }
                    break;

                /*case SDLK_g:
                    {
                        ExternalEvent ev;

                        ev.type = EXTERNAL_SHOW_MSG;
                        strcpy(ev.buf1, "test");

                        ScreenSaverRefresh();
                        ExternalProcessEvent(&ev);
                    }
                    break;*/

            #endif // _WIN32
                }
                if (result && !ScreenIsOff() && !StorageIsInUsbDeviceMode())
                    AudioPlayKeySound();

                break;

            case SDL_KEYUP:
                result = ituSceneUpdate(&theScene, ITU_EVENT_KEYUP, ev.key.keysym.sym, 0, 0);
                break;

            case SDL_MOUSEMOTION:
                ScreenSaverRefresh();
                #if defined(CFG_USB_MOUSE) || defined(_WIN32)
                if (cursorIcon)
                {
                    ituWidgetSetX(cursorIcon, ev.button.x);
                    ituWidgetSetY(cursorIcon, ev.button.y);
                    ituWidgetSetDirty(cursorIcon, true);
            	    //printf("mouse: move %d, %d\n", ev.button.x, ev.button.y);
                }
                #endif // defined(CFG_USB_MOUSE) || defined(_WIN32)
                result = ituSceneUpdate(&theScene, ITU_EVENT_MOUSEMOVE, ev.button.button, ev.button.x, ev.button.y);
                break;

            case SDL_MOUSEBUTTONDOWN:
                ScreenSaverRefresh();
                printf("mouse: down %d, %d\n", ev.button.x, ev.button.y);
                {
                    mouseDownTick = SDL_GetTicks();
                #ifdef DOUBLE_KEY_ENABLE
                    if (mouseDownTick - dblclk <= 200)
                    {
                        result = ituSceneUpdate(&theScene, ITU_EVENT_MOUSEDOUBLECLICK, ev.button.button, ev.button.x, ev.button.y);
                        dblclk = 0;
                    }
                    else
                #endif // DOUBLE_KEY_ENABLE
                    {
                        result = ituSceneUpdate(&theScene, ITU_EVENT_MOUSEDOWN, ev.button.button, ev.button.x, ev.button.y);
                        dblclk = mouseDownTick;
                        lastx = ev.button.x;
                        lasty = ev.button.y;
                    }
                    if (result && !ScreenIsOff() && !StorageIsInUsbDeviceMode())
                        AudioPlayKeySound();

                #ifdef CFG_SCREENSHOT_ENABLE
                    if (ev.button.x < 50 && ev.button.y > CFG_LCD_HEIGHT - 50)
                        Screenshot(screenSurf);
                #endif // CFG_SCREENSHOT_ENABLE
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (SDL_GetTicks() - dblclk <= 200)
                {
                    int xdiff = abs(ev.button.x - lastx);
                    int ydiff = abs(ev.button.y - lasty);

                    if (xdiff >= GESTURE_THRESHOLD && xdiff > ydiff)
                    {
                        if (ev.button.x > lastx)
                        {
                            printf("mouse: slide to right\n");
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDERIGHT, xdiff, ev.button.x, ev.button.y);
                        }
                        else
                        {
                            printf("mouse: slide to left\n");
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDELEFT, xdiff, ev.button.x, ev.button.y);
                        }
                    }
                    else if (ydiff >= GESTURE_THRESHOLD)
                    {
                        if (ev.button.y > lasty)
                        {
                            printf("mouse: slide to down\n");
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDEDOWN, ydiff, ev.button.x, ev.button.y);
                        }
                        else
                        {
                            printf("mouse: slide to up\n");
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDEUP, ydiff, ev.button.x, ev.button.y);
                        }
                    }
                }
                result |= ituSceneUpdate(&theScene, ITU_EVENT_MOUSEUP, ev.button.button, ev.button.x, ev.button.y);
                mouseDownTick = 0;
                break;

            case SDL_FINGERMOTION:
                ScreenSaverRefresh();
                printf("touch: move %d, %d\n", ev.tfinger.x, ev.tfinger.y);
                result = ituSceneUpdate(&theScene, ITU_EVENT_MOUSEMOVE, 1, ev.tfinger.x, ev.tfinger.y);
                break;

            case SDL_FINGERDOWN:
                ScreenSaverRefresh();
                printf("touch: down %d, %d\n", ev.tfinger.x, ev.tfinger.y);
                {
                    mouseDownTick = SDL_GetTicks();
                #ifdef DOUBLE_KEY_ENABLE
					#ifdef CFG_POWER_WAKEUP_DOUBLE_CLICK_INTERVAL
					if (mouseDownTick - dblclk <= CFG_POWER_WAKEUP_DOUBLE_CLICK_INTERVAL)
					#else
                    if (mouseDownTick - dblclk <= 200)
					#endif
                    {
	                 	printf("double touch!\n");
						if(sleepModeDoubleClick)
						{
							ScreenSetDoubleClick();
							ScreenSaverRefresh();
							sleepModeDoubleClick = false;
						}
                        result = ituSceneUpdate(&theScene, ITU_EVENT_MOUSEDOUBLECLICK, 1, ev.tfinger.x, ev.tfinger.y);
                        dblclk = mouseDownTick = 0;
                    }
                    else
                #endif // DOUBLE_KEY_ENABLE
                    {
                        result = ituSceneUpdate(&theScene, ITU_EVENT_MOUSEDOWN, 1, ev.tfinger.x, ev.tfinger.y);
                        dblclk = mouseDownTick;
                        lastx = ev.tfinger.x;
                        lasty = ev.tfinger.y;
                    }
                    if (result && !ScreenIsOff() && !StorageIsInUsbDeviceMode())
                        AudioPlayKeySound();

                #ifdef CFG_SCREENSHOT_ENABLE
                    if (ev.tfinger.x < 50 && ev.tfinger.y > CFG_LCD_HEIGHT - 50)
                        Screenshot(screenSurf);
                #endif // CFG_SCREENSHOT_ENABLE
                    //if (ev.tfinger.x < 50 && ev.tfinger.y > CFG_LCD_HEIGHT - 50)
                    //    SceneQuit(QUIT_UPGRADE_WEB);
                }
                break;

            case SDL_FINGERUP:
                printf("touch: up %d, %d\n", ev.tfinger.x, ev.tfinger.y);
                if (SDL_GetTicks() - dblclk <= 300)
                {
                    int xdiff = abs(ev.tfinger.x - lastx);
                    int ydiff = abs(ev.tfinger.y - lasty);

                    if (xdiff >= GESTURE_THRESHOLD && xdiff > ydiff)
                    {
                        if (ev.tfinger.x > lastx)
                        {
                            printf("touch: slide to right %d %d\n", ev.tfinger.x, ev.tfinger.y);
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDERIGHT, xdiff, ev.tfinger.x, ev.tfinger.y);
                        }
                        else
                        {
                            printf("touch: slide to left %d %d\n", ev.tfinger.x, ev.tfinger.y);
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDELEFT, xdiff, ev.tfinger.x, ev.tfinger.y);
                        }
                    }
                    else if (ydiff >= GESTURE_THRESHOLD)
                    {
                        if (ev.tfinger.y > lasty)
                        {
                            printf("touch: slide to down %d %d\n", ev.tfinger.x, ev.tfinger.y);
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDEDOWN, ydiff, ev.tfinger.x, ev.tfinger.y);
                        }
                        else
                        {
                            printf("touch: slide to up %d %d\n", ev.tfinger.x, ev.tfinger.y);
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDEUP, ydiff, ev.tfinger.x, ev.tfinger.y);
                        }
                    }
                }
                result |= ituSceneUpdate(&theScene, ITU_EVENT_MOUSEUP, 1, ev.tfinger.x, ev.tfinger.y);
                mouseDownTick = 0;
                break;

            case SDL_MULTIGESTURE:
                printf("touch: multi %d, %d\n", ev.mgesture.x, ev.mgesture.y);
                if (ev.mgesture.dDist > 0.0f)
                {
                    int dist = (int)(screenDistance * ev.mgesture.dDist);
                    int x = (int)(screenWidth * ev.mgesture.x);
                    int y = (int)(screenHeight * ev.mgesture.y);
                    result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHPINCH, dist, x, y);
                }
                break;
            }
        }
        if (!ScreenIsOff())
        {
            if (mouseDownTick > 0 && (SDL_GetTicks() - mouseDownTick >= MOUSEDOWN_LONGPRESS_DELAY))
            {
                printf("long press: %d %d\n", lastx, lasty);
                result |= ituSceneUpdate(&theScene, ITU_EVENT_MOUSELONGPRESS, 1, lastx, lasty);
                mouseDownTick = 0;
            }
            result |= ituSceneUpdate(&theScene, ITU_EVENT_TIMER, 0, 0, 0);
            //printf("%d\n", result);
            if (result)
            {
                ituSceneDraw(&theScene, screenSurf);
                ituFlip(screenSurf);
                if (first_screenSurf) {
                    ioctl(ITP_DEVICE_BACKLIGHT, ITP_IOCTL_RESET, NULL);//Add Backlight turn on when first frame display.
                    ScreenSetBrightness(theConfig.brightness - 1);
                    printf("first frame display, turn on backlight, brightness=%d\n", theConfig.brightness);
                    first_screenSurf = false;
                }
            }

            if (theConfig.screensaver_type != SCREENSAVER_NONE &&
                ScreenSaverCheck())
            {
                ituSceneSendEvent(&theScene, EVENT_CUSTOM_SCREENSAVER, "0");

                if (theConfig.screensaver_type == SCREENSAVER_BLANK)
                {
                    // have a change to flush action commands
                    ituSceneUpdate(&theScene, ITU_EVENT_TIMER, 0, 0, 0);

                    // draw black screen
                    ituSceneDraw(&theScene, screenSurf);
                    ituFlip(screenSurf);

                    ScreenOff();

                    #if defined(CFG_POWER_WAKEUP_IR)
                        sleepModeIR = true;
                    #endif
					#if defined(CFG_POWER_WAKEUP_TOUCH_DOUBLE_CLICK)
						sleepModeDoubleClick = true;
					#endif
                }
            }
        }
#if defined(CFG_BTA_ENABLE)
		if (gSleepModeBT)
		{
			if (!callBackground)
			{
				callBackground = ituSceneFindWidget(&theScene, "callBackground");
				assert(callBackground);
			}
			if(!ituWidgetIsVisible(callBackground))
			{
				ScreenSaverRefresh();
				ituSceneUpdate(&theScene, ITU_EVENT_MOUSEDOWN, 1, 0, 0);
				ituSceneDraw(&theScene, screenSurf);
				ituFlip(screenSurf);
			}
			gSleepModeBT = false;
		}
#endif
#if defined(CFG_POWER_WAKEUP_IR)
        if (ScreenIsOff() && sleepModeIR)
        {
            printf("Wake up by remote IR!\n");
            ScreenSaverRefresh();
            ituSceneUpdate(&theScene, ITU_EVENT_MOUSEDOWN, 1, 0, 0);
            ituSceneDraw(&theScene, screenSurf);
            ituFlip(screenSurf);
            sleepModeIR = false;
        }
#endif

		if(sleepModeDoubleClick)
		{
			if(theConfig.screensaver_type != SCREENSAVER_NONE &&
                ScreenSaverCheckForDoubleClick())
            {
                if (theConfig.screensaver_type == SCREENSAVER_BLANK)
                    ScreenOffContinue();
            }
		}
#endif
		if (IsDelay == true)
			delay = periodPerFrame - (SDL_GetTicks() - tick);
		else
			delay = 0;

        //printf("scene loop delay=%d\n", delay);
        if (delay > 0)
        {
            SDL_Delay(delay);
        }
        else
            sched_yield();
    }

    return quitValue;
}

void SceneQuit(QuitValue value)
{
    quitValue = value;
}

QuitValue SceneGetQuitValue(void)
{
    return quitValue;
}


void show_mirror_window(int w, int h)
{
	printf("[%s] w%d h%d\n", __func__, w, h);

	Command cmd;

	showMirror = true;
	//mirrorReady = true;
	if (w > h)
	{
		//ituWidgetSetDraw(projectHorizontalPhoneContainer, PreviewVideoBackgroundDraw);
		//ituWidgetGetGlobalPosition(projectHorizontalPhoneContainer, &x, &y);
		//width = ituWidgetGetWidth(projectHorizontalPhoneContainer);
		//height = ituWidgetGetHeight(projectHorizontalPhoneContainer);
/*		is_central_pos_p = false;

		showingHorizontalBackground = false;
*/

		//ituLayerGoto(projectLayer);
		if (commandQueue == -1)
			return;

		//cmd.id = CMD_GOTO_PROJECT;
		cmd.id = CMD_MIRROR_H;
		mq_send(commandQueue, (const char*)&cmd, sizeof (Command), 0);
	}
	else
	{
		//ituWidgetSetDraw(projectPhoneContainer, PreviewVideoBackgroundDraw);
		//ituWidgetGetGlobalPosition(projectPhoneContainer, &x, &y);
		//width = ituWidgetGetWidth(projectPhoneContainer);
		//height = ituWidgetGetHeight(projectPhoneContainer);
		//if (is_central_pos_p != true)
		//	ProjectChangeUIBtOnMouseUp(projectChangeUIButton, NULL);
		//is_central_pos_p = true;

		//if (is_central_pos_p)
		//	is_central_pos_p = false;
		//else

		////showingPhoneContainer = true;
		//ProjectChangeUIBtOnMouseUp(projectChangeUIButton, NULL);

		//ituLayerGoto(projectVLayer);

		if (commandQueue == -1)
			return;

		//cmd.id = CMD_GOTO_PROJECTV;
		cmd.id = CMD_MIRROR_V;
		mq_send(commandQueue, (const char*)&cmd, sizeof (Command), 0);
	}
	//itv_set_video_window(x, y, width, height);
	//itv_set_pb_mode(1);
}

//void unshow_mirror_window(int w, int h)
//{
//	printf("[%s]\n", __func__);
//
//#ifdef CFG_VIDEO_ENABLE
//	network_mirror_stop();
//	itv_set_pb_mode(0);
//#endif
//	SceneLeaveVideoState();
//
////	showingPhoneContainer = false;
//	//ituWidgetSetVisible(projectPhoneContainer, false);
//
//	if (w > h)
//	{
//		ProjectChangeUIBtOnMouseUp(NULL, NULL);
//	}
//	else
//	{
//		ProjectVChangeUIBtOnMouseUp(NULL, NULL);
//	}
//
//
//}