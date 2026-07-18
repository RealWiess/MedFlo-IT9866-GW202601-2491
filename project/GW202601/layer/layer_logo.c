#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scene.h"
#include "ctrlboard.h"
#include "ite/itp.h"

#if !defined(_WIN32) && !CFG_BL_SHOW_LOGO
//#define ITU_PLAY_VIDEO_ON_BOOTING
#endif

static ITUVideo *logoVideo = 0;
static ITULayer *startUpLayer = 0;

static int gShowDelayCnt = 0;
static gInit = false;

static void LogoVideoOnStop(ITUVideo* video)
{
    ituLayerGoto(startUpLayer);
}

bool LogoOnEnter(ITUWidget* widget, char* param)
{
    printf(" >>> %s\n", __func__);
    if (!logoVideo)
    {
        logoVideo = ituSceneFindWidget(&theScene, "logoVideo");
        assert(logoVideo);

#ifdef CFG_PLAY_DEMO_VIDEO
        startUpLayer = ituSceneFindWidget(&theScene, "demoLayer");//fakeLayer
#else
        startUpLayer = ituSceneFindWidget(&theScene, "fakeLayer");//fakeLayer
#endif // CFG_PLAY_DEMO_VIDEO

        assert(startUpLayer);
    }

#ifdef ITU_PLAY_VIDEO_ON_BOOTING
    ituVideoPlay(logoVideo, 0);
    ituVideoSetOnStop(logoVideo, LogoVideoOnStop);
#else
    ituLayerGoto(startUpLayer);
#endif
    return true;
}

bool LogoOnTimer(ITUWidget* widget, char* param) {
    return true;
}

bool LogoOnLeave(ITUWidget* widget, char* param) {
    return true;
}

void LogoReset(void)
{

}
