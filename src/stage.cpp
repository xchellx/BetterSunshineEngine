#include <Dolphin/DVD.h>
#include <JSystem/J2D/J2DOrthoGraph.hxx>
#include <JSystem/JDrama/JDRNameRef.hxx>

#include <SMS/Manager/FlagManager.hxx>
#include <SMS/Manager/ModelWaterManager.hxx>
#include <SMS/MapObj/MapObjInit.hxx>
#include <SMS/System/Application.hxx>
#include <SMS/System/GameSequence.hxx>
#include <SMS/System/MarDirector.hxx>
#include <SMS/macros.h>

#include "libs/container.hxx"
#include "libs/global_vector.hxx"
#include "libs/profiler.hxx"
#include "libs/string.hxx"

#include "loading.hxx"
#include "logging.hxx"
#include "module.hxx"
#include "stage.hxx"

using namespace BetterSMS;

static TGlobalVector<Stage::InitCallback> sStageInitCBs;
static TGlobalVector<Stage::UpdateCallback> sStageUpdateCBs;
static TGlobalVector<Stage::Draw2DCallback> sStageDrawCBs;
static TGlobalVector<Stage::ExitCallback> sStageExitCBs;

Stage::TStageParams *Stage::TStageParams::sStageConfig = nullptr;

BETTER_SMS_FOR_EXPORT Stage::TStageParams *BetterSMS::Stage::getStageConfiguration() {
    return TStageParams::sStageConfig;
}

BETTER_SMS_FOR_EXPORT bool BetterSMS::Stage::addInitCallback(InitCallback cb) {
    sStageInitCBs.push_back(cb);
    return true;
}

BETTER_SMS_FOR_EXPORT bool BetterSMS::Stage::addUpdateCallback(UpdateCallback cb) {
    sStageUpdateCBs.push_back(cb);
    return true;
}

BETTER_SMS_FOR_EXPORT bool BetterSMS::Stage::addDraw2DCallback(Draw2DCallback cb) {
    sStageDrawCBs.push_back(cb);
    return true;
}

BETTER_SMS_FOR_EXPORT bool BetterSMS::Stage::addExitCallback(ExitCallback cb) {
    sStageExitCBs.push_back(cb);
    return true;
}

BETTER_SMS_FOR_EXPORT const char *BetterSMS::Stage::getStageName(u8 area, u8 episode) {
    const auto *areaAry = gpApplication.mStageArchiveAry;
    if (!areaAry || area >= areaAry->mChildren.size())
        return nullptr;

    auto *episodeAry =
        reinterpret_cast<TNameRefAryT<TScenarioArchiveName> *>(areaAry->mChildren[area]);

    if (!episodeAry || episode >= episodeAry->mChildren.size())
        return nullptr;

    TScenarioArchiveName &stageName = episodeAry->mChildren[episode];
    return stageName.mArchiveName;
}

BETTER_SMS_FOR_EXPORT bool BetterSMS::Stage::isDivingStage(u8 area, u8 episode) {
    Stage::TStageParams params;
    if (const char *stageName = Stage::getStageName(area, episode)) {
        params.load(stageName);
    }
    return params.mIsDivingStage.get();
}

BETTER_SMS_FOR_EXPORT bool BetterSMS::Stage::isExStage(u8 area, u8 episode) {
    Stage::TStageParams params;
    if (const char *stageName = Stage::getStageName(area, episode)) {
        params.load(stageName);
    }
    if (params.isCustomConfig()) {
        return params.mIsExStage.get();
    } else {
        return (area >= TGameSequence::AREA_DOLPICEX0 && area <= TGameSequence::AREA_COROEX6);
    }
}

void BetterSMS::Stage::TStageParams::reset() {
    mIsExStage.set(false);
    mIsDivingStage.set(false);
    mIsOptionStage.set(false);
    mIsMultiplayerStage.set(false);
    mIsEggFree.set(true);
    mPlayerHasFludd.set(true);
    mPlayerHasHelmet.set(false);
    mPlayerHasGlasses.set(false);
    mPlayerHasShirt.set(false);
    mPlayerCanRideYoshi.set(true);
    mMusicEnabled.set(true);
    mMusicSetCustom.set(false);
    mMusicID.set(1);
    mMusicAreaID.set(1);
    mMusicEpisodeID.set(1);
    mMusicPitch.set(1.0f);
    mMusicSpeed.set(1.0f);
    mMusicVolume.set(0.75f);
    mIsCustomConfigLoaded = false;
}

static int findNumber(const char *string, size_t max) {
    for (int i = 0; i < max; ++i) {
        const char chr = string[i];
        if (chr == '\0')
            return -1;
        if (chr >= '0' && chr <= '9')
            return i;
    }
    return -1;
}

static int findExtension(const char *string, size_t max) {
    for (int i = 0; i < max; ++i) {
        const char chr = string[i];
        if (chr == '\0')
            return -1;
        if (chr == '.')
            return i;
    }
    return -1;
}

char *BetterSMS::Stage::TStageParams::stageNameToParamPath(char *dst, const char *stage,
                                                           bool generalize) {
    strncpy(dst, "/data/scene/params/", 20);

    const int numIDPos = findNumber(stage, 60);
    if (generalize && numIDPos != -1) {
        strncpy(dst + 19, stage, numIDPos);
        dst[19 + numIDPos] = '\0';
        strcat(dst, "+.prm");
    } else {
        const int extensionPos = findExtension(stage, 60);
        if (extensionPos == -1)
            strcat(dst, stage);
        else
            strncpy(dst + 19, stage, extensionPos);
        dst[19 + extensionPos] = '\0';
        strcat(dst, ".prm");
    }

    return dst;
}

void BetterSMS::Stage::TStageParams::load(const char *stageName) {
    DVDFileInfo fileInfo;
    s32 entrynum;

    char path[64];
    stageNameToParamPath(path, stageName, false);

    entrynum = DVDConvertPathToEntrynum(path);
    if (entrynum >= 0) {
        DVDFastOpen(entrynum, &fileInfo);
        void *buffer = JKRHeap::alloc(fileInfo.mLen, 32, nullptr);

        DVDReadPrio(&fileInfo, buffer, fileInfo.mLen, 0, 2);
        DVDClose(&fileInfo);
        {
            JSUMemoryInputStream stream(buffer, fileInfo.mLen);
            TParams::load(stream);
            JKRHeap::free(buffer, nullptr);
        }
        mIsCustomConfigLoaded = true;
        return;
    }

    stageNameToParamPath(path, stageName, true);

    entrynum = DVDConvertPathToEntrynum(path);
    if (entrynum >= 0) {
        DVDFastOpen(entrynum, &fileInfo);
        void *buffer = JKRHeap::alloc(fileInfo.mLen, 32, nullptr);

        DVDReadPrio(&fileInfo, buffer, fileInfo.mLen, 0, 2);
        DVDClose(&fileInfo);
        {
            JSUMemoryInputStream stream(buffer, fileInfo.mLen);
            TParams::load(stream);
            JKRHeap::free(buffer, nullptr);
        }
        mIsCustomConfigLoaded = true;
        return;
    }

    reset();
}

void initStageLoading(TMarDirector *director) {
    Loading::setLoading(true);
    director->loadResource();
}
SMS_PATCH_BL(SMS_PORT_REGION(0x80296DE0, 0x80291750, 0, 0), initStageLoading);

static bool sIsStageInitialized = false;

// Extern to stage init
BETTER_SMS_FOR_CALLBACK void loadStageConfig(TMarDirector *) {
    // Stage::TStageParams::sStageConfig = new (JKRHeap::sSystemHeap, 4) Stage::TStageParams;
    Stage::TStageParams::sStageConfig = new Stage::TStageParams;

    Stage::TStageParams *config = Stage::getStageConfiguration();
    config->reset();
    config->load(Stage::getStageName(gpApplication.mCurrentScene.mAreaID,
                                     gpApplication.mCurrentScene.mEpisodeID));
}

// Extern to stage init
BETTER_SMS_FOR_CALLBACK void resetStageConfig(TApplication *) {
    // delete Stage::TStageParams::sStageConfig;

    waterColor[0].set(0x3C, 0x46, 0x78, 0x14);  // Water rgba
    waterColor[1].set(0xFE, 0xA8, 0x02, 0x6E);  // Yoshi Juice rgba
    waterColor[2].set(0x9B, 0x01, 0xFD, 0x6E);
    waterColor[3].set(0xFD, 0x62, 0xA7, 0x6E);
    bodyColor[0].set(0x40, 0xA1, 0x24, 0xFF);  // Yoshi rgba
    bodyColor[1].set(0xFF, 0x8C, 0x1C, 0xFF);
    bodyColor[2].set(0xAA, 0x4C, 0xFF, 0xFF);
    bodyColor[3].set(0xFF, 0xA0, 0xBE, 0xFF);
    gAudioVolume = 0.75f;
    gAudioPitch  = 1.0f;
    gAudioSpeed  = 1.0f;

    if (Stage::TStageParams *config = Stage::getStageConfiguration()) {
        config->reset();
    }
}

void initStageCallbacks(TMarDirector *director) {
    TFlagManager::smInstance->resetStage();

    loadStageConfig(director);

    for (auto &item : sStageInitCBs) {
        item(director);
    }

    director->setupObjects();
    Loading::setLoading(false);

    sIsStageInitialized = true;
}
SMS_PATCH_BL(SMS_PORT_REGION(0x802998B8, 0x80291750, 0, 0), initStageCallbacks);
SMS_WRITE_32(SMS_PORT_REGION(0x802B7720, 0, 0, 0), 0x60000000);

void updateStageCallbacks(TApplication *app) {
    u32 func;
    SMS_FROM_GPR(12, func);

    if (!sIsStageInitialized)
        return;

    if (gpMarDirector && app->mContext == TApplication::CONTEXT_DIRECT_STAGE) {
        for (auto &item : sStageUpdateCBs) {
            item(gpMarDirector);
        }
    }
}

void drawStageCallbacks(J2DOrthoGraph *ortho) {
    ortho->setup2D();

    for (auto &item : sStageDrawCBs) {
        item(gpMarDirector, ortho);
    }
}
SMS_PATCH_BL(SMS_PORT_REGION(0x80143F14, 0x80138B50, 0, 0), drawStageCallbacks);

extern void resetPlayerDatas(TMarDirector *);

void exitStageCallbacks(TApplication *app) {
    if (app->mContext != TApplication::CONTEXT_DIRECT_STAGE)
        return;

    for (auto &item : sStageExitCBs) {
        item(app);
    }

    resetStageConfig(app);
    sIsStageInitialized = false;
}

#pragma region MapIdentifiers

static bool isExMap() {
    auto config = Stage::getStageConfiguration();
    if (config && config->isCustomConfig())
        return config->mIsExStage.get();
    else
        return (gpApplication.mCurrentScene.mAreaID >= TGameSequence::AREA_DOLPICEX0 &&
                gpApplication.mCurrentScene.mAreaID <= TGameSequence::AREA_COROEX6);
}
SMS_PATCH_B(SMS_PORT_REGION(0x802A8B58, 0x802A0C00, 0, 0), isExMap);

static bool isMultiplayerMap() {
    auto config = Stage::getStageConfiguration();
    if (config && config->isCustomConfig())
        return config->mIsMultiplayerStage.get();
    else
        return (gpMarDirector->mAreaID == TGameSequence::AREA_TEST10 &&
                gpMarDirector->mEpisodeID == 0);
}
SMS_PATCH_B(SMS_PORT_REGION(0x802A8B30, 0x802A0BD8, 0, 0), isMultiplayerMap);

static bool isDivingMap() {
    auto config = Stage::getStageConfiguration();
    if (config && config->isCustomConfig())
        return config->mIsDivingStage.get();
    else
        return (gpMarDirector->mAreaID == TGameSequence::AREA_MAREBOSS ||
                gpMarDirector->mAreaID == TGameSequence::AREA_MAREEX0 ||
                gpMarDirector->mAreaID == TGameSequence::AREA_MAREUNDERSEA);
}
SMS_PATCH_B(SMS_PORT_REGION(0x802A8AFC, 0x802A0BA4, 0, 0), isDivingMap);

static bool isOptionMap() {
    auto config = Stage::getStageConfiguration();
    if (config && config->isCustomConfig())
        return config->mIsOptionStage.get();
    else
        return (gpMarDirector->mAreaID == 15);
}
SMS_PATCH_B(SMS_PORT_REGION(0x802A8AE0, 0x802A0B88, 0, 0), isOptionMap);

#pragma endregion