#include <Dolphin/DVD.h>
#include <Dolphin/types.h>

#include <SMS/GC2D/SelectDir.hxx>
#include <SMS/Manager/FlagManager.hxx>
#include <SMS/Manager/RumbleManager.hxx>
#include <SMS/Player/MarioGamePad.hxx>
#include <SMS/System/Application.hxx>
#include <SMS/System/CardManager.hxx>
#include <SMS/System/GCLogoDir.hxx>
#include <SMS/System/MenuDirector.hxx>
#include <SMS/System/MovieDirector.hxx>
#include <SMS/System/RenderModeObj.hxx>
#include <SMS/System/Resolution.hxx>

#include "application.hxx"
#include "level_select/p_level_select.hxx"
#include "libs/container.hxx"
#include "libs/global_unordered_map.hxx"
#include "module.hxx"
#include "music.hxx"
#include "p_settings.hxx"

using namespace BetterSMS;

static TGlobalVector<Application::ContextCallback> sContextCBs(256);
static bool sIsAdditionalMovie  = false;
static bool sShowSettingsOnBoot = false;
static u8 sIntroArea = 15, sIntroEpisode = 0;

extern FirstBootSetting gFirstBootSetting;

#define SMS_CHECK_RESET_FLAG(gamepad) (((1 << (gamepad)->mPort) & TMarioGamePad::mResetFlag) == 1)

BETTER_SMS_FOR_EXPORT bool BetterSMS::Application::registerContextCallback(u8 context,
                                                                           ContextCallback cb) {
    if (sContextCBs[context] != nullptr)
        return false;
    sContextCBs[context] = cb;
    return true;
}

BETTER_SMS_FOR_EXPORT void BetterSMS::Application::setIntroStage(u8 area, u8 episode) {
    sIntroArea    = area;
    sIntroEpisode = episode;
}

bool BetterSMS::Application::isFirstBoot() { return gFirstBootSetting.getBool(); }

BETTER_SMS_FOR_EXPORT void BetterSMS::Application::showSettingsOnFirstBoot(bool show_on_boot) {
    sShowSettingsOnBoot = show_on_boot;
}

BETTER_SMS_FOR_CALLBACK bool BetterAppContextGameBoot(TApplication *app) {
    SMSSetupGCLogoRenderingInfo(app->mDisplay);
    return false;
}

BETTER_SMS_FOR_CALLBACK bool BetterAppContextGameBootLogo(TApplication *app) {
    SMSSetupGCLogoRenderingInfo(app->mDisplay);

    auto *director = new TGCLogoDir();
    app->mDirector = director;

    director->setup(app->mDisplay, app->mGamePads[0]);
    return false;
}

BETTER_SMS_FOR_CALLBACK bool BetterAppContextDirectMovie(TApplication *app) {
    SMSSetupMovieRenderingInfo(app->mDisplay);

    app->mFader->setDisplaySize(SMSGetGameRenderWidth(), SMSGetGameRenderHeight());

    auto *director = new TMovieDirector();
    app->mDirector = director;

    director->setup(app->mDisplay, app->mGamePads[0]);
    return false;
}

BETTER_SMS_FOR_CALLBACK bool BetterAppContextGameBootIntro(TApplication *app) {
    app->mCutSceneID = 9;
    app->mNextScene.set(sIntroArea, sIntroEpisode, 0);
    BetterAppContextDirectMovie(app);
    return false;
}

BETTER_SMS_FOR_CALLBACK bool BetterAppContextDirectStage(TApplication *app) {
    sIsAdditionalMovie = app->checkAdditionalMovie();
    if (!sIsAdditionalMovie) {
        SMSSetupGameRenderingInfo(app->mDisplay, (app->_44 & 1) != 0);

        app->mFader->setDisplaySize(SMSGetGameRenderWidth(), SMSGetGameRenderHeight());

        auto *director = new TMarDirector();
        app->mDirector = director;

        bool skipLoop =
            director->setup(app->mDisplay, &app->mGamePads[0], app->mCurrentScene.mAreaID,
                            app->mCurrentScene.mEpisodeID) != 0;

        if (skipLoop)
            app->mContext = TApplication::CONTEXT_GAME_INTRO;
        return skipLoop;
    } else {
        BetterAppContextDirectMovie(app);
        return false;
    }
}

BETTER_SMS_FOR_CALLBACK bool BetterAppContextDirectShineSelect(TApplication *app) {
    SMSSetupTitleRenderingInfo(app->mDisplay);

    sIsAdditionalMovie = false;

    app->mFader->setDisplaySize(SMSGetTitleRenderWidth(), SMSGetTitleRenderHeight());

    auto *director = new TSelectDir();
    app->mDirector = director;

    director->setup(app->mDisplay, app->mGamePads[0], app->mCurrentScene.mAreaID);
    return false;
}

BETTER_SMS_FOR_CALLBACK bool BetterAppContextDirectLevelSelect(TApplication *app) {
    SMSSetupTitleRenderingInfo(app->mDisplay);

    sIsAdditionalMovie = false;

    app->mFader->setDisplaySize(SMSGetTitleRenderWidth(), SMSGetTitleRenderHeight());

#if 1
    auto *director = new LevelSelectDirector();
#else
    auto *director = new TMenuDirector();
#endif

    app->mDirector = director;

    director->setup(app->mDisplay, app->mGamePads[0]);

    TFlagManager::smInstance->setFlag(0x20001, 3);

    app->mCurrentScene.set(sIntroArea, sIntroEpisode, 0);
    app->mNextScene.set(sIntroArea, sIntroEpisode, 0);
    return false;
}

BETTER_SMS_FOR_CALLBACK bool BetterAppContextDirectSettingsMenu(TApplication *app) {
    SMSSetupTitleRenderingInfo(app->mDisplay);

    sIsAdditionalMovie = false;

    app->mFader->setDisplaySize(SMSGetTitleRenderWidth(), SMSGetTitleRenderHeight());

    auto *director = new SettingsDirector();
    app->mDirector = director;

    director->setup(app->mDisplay, app->mGamePads[0]);

    app->mCurrentScene.set(sIntroArea, sIntroEpisode, 0);
    app->mNextScene.set(sIntroArea, sIntroEpisode, 0);
    return false;
}

extern void gameInitCallbackHandler(TApplication *app);
extern void gameBootCallbackHandler(TApplication *app);
extern void gameChangeCallbackHandler(TApplication *app);

// SETTINGS
extern void initAllSettings(TApplication *);

void BetterApplicationProcess(TApplication *app) {
    initAllSettings(app);

    bool exitLoop   = false;
    u8 delayContext = 1;
    do {
        Application::ContextCallback cb = sContextCBs[app->mContext];
        SMS_ASSERT(cb, "Application attempted to fetch context handler %u but it wasn't found!",
                   app->mContext);

        exitLoop = (*cb)(app);
        if (!exitLoop) {
            delayContext = app->gameLoop();
            gameChangeCallbackHandler(app);
        }

        if (app->mDirector)
            delete app->mDirector;
        app->mDirector = nullptr;

        if (app->mContext == TApplication::CONTEXT_GAME_BOOT_LOGO) {
            if (!SMS_CHECK_RESET_FLAG(app->mGamePads[0])) {
                app->initialize_nlogoAfter();

                bool introSceneMatchesExpected = app->mNextScene.mAreaID == sIntroArea &&
                                                 app->mNextScene.mEpisodeID == sIntroEpisode;

                if (BetterSMS::isDebugMode()) {
                    delayContext = TApplication::CONTEXT_DIRECT_LEVEL_SELECT;
                } else if (sShowSettingsOnBoot && Application::isFirstBoot()) {
                    delayContext = CONTEXT_DIRECT_SETTINGS_MENU;
                }

                // If external tools like Junior's Toolbox hijack the scene, we should permiss
                // it.
                if (!introSceneMatchesExpected) {
                    delayContext = TApplication::CONTEXT_DIRECT_STAGE;
                }
            }
        } else if (app->mContext == TApplication::CONTEXT_GAME_BOOT) {
            app->mNextScene.set(sIntroArea, sIntroEpisode, 0);
            if (!SMS_CHECK_RESET_FLAG(app->mGamePads[0])) {
                gameInitCallbackHandler(app);
                app->initialize_bootAfter();
                gameBootCallbackHandler(app);
            }
        } else {
            app->mCurrentHeap->freeAll();
        }

        SMSRumbleMgr->reset();

        if (SMS_CHECK_RESET_FLAG(app->mGamePads[0])) {
            TMarioGamePad::mResetFlag = 0;
            JUTGamePad::recalibrate(0xF0000000);

            exitLoop = DVDCheckDisk();
            if (!exitLoop) {
                delayContext = TApplication::CONTEXT_GAME_SHUTDOWN;
                app->_44 |= 2;
            } else {
                if (app->mContext == TApplication::CONTEXT_GAME_BOOT ||
                    app->mContext == TApplication::CONTEXT_GAME_BOOT_LOGO) {
                    app->mContext = TApplication::CONTEXT_GAME_SHUTDOWN;
                } else if (app->mContext != TApplication::CONTEXT_GAME_SHUTDOWN) {
                    delayContext = BetterSMS::isDebugMode()
                                       ? TApplication::CONTEXT_DIRECT_LEVEL_SELECT
                                       : TApplication::CONTEXT_GAME_INTRO;

                    gpCardManager->unmount();
                }
            }
        }

        OSReport("Context: %d\n", app->mContext);
        OSReport("Delay Context: %d\n", delayContext);

        // This fixes the secret area movies destroying previous scene data
        if (!sIsAdditionalMovie && app->mContext <= TApplication::CONTEXT_DIRECT_LEVEL_SELECT) {
            if (app->mNextScene.mAreaID != app->mCurrentScene.mAreaID ||
                app->mNextScene.mEpisodeID != app->mCurrentScene.mEpisodeID) {
                if (app->mCurrentScene.mEpisodeID != 0xFF) {
                    app->mPrevScene = app->mCurrentScene;
                }
                app->mCurrentScene = app->mNextScene;
            }
        }

        app->mContext = delayContext;
    } while (app->mContext != TApplication::CONTEXT_GAME_SHUTDOWN);
}
SMS_PATCH_BL(SMS_PORT_REGION(0x80005624, 0, 0, 0), BetterApplicationProcess);

#undef SMS_CHECK_RESET_FLAG