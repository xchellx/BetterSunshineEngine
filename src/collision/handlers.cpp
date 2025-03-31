#include <Dolphin/GX.h>
#include <Dolphin/OS.h>

#include <SMS/MSound/MSoundSESystem.hxx>
#include <SMS/Map/MapCollisionData.hxx>
#include <SMS/macros.h>

#include "logging.hxx"
#include "module.hxx"
#include "p_warp.hxx"
#include "player.hxx"

using namespace BetterSMS;

/* Collision State Resetters */

#define EXPAND_WARP_SET(base)                                                                      \
    (base) : case ((base) + 10):                                                                   \
    case ((base) + 20):                                                                            \
    case ((base) + 30)
#define EXPAND_WARP_CATEGORY(base)                                                                 \
    (base) : case ((base) + 1):                                                                    \
    case ((base) + 2):                                                                             \
    case ((base) + 3):                                                                             \
    case ((base) + 4)

static void resetValuesOnStateChange(TMario *player) {
    if (gpMarDirector->mCurState != TMarDirector::STATE_NORMAL)
        return;

    auto playerData = Player::getData(player);

    switch (player->mPrevState) {
    case static_cast<u32>(TMario::STATE_TRIPLE_J):
        playerData->mCollisionFlags.mIsDisableInput = false;
        break;
    default:
        break;
    }

    switch (player->mState) {}

    if ((player->mState != static_cast<u32>(TMario::STATE_JUMPSPINR) &&
         player->mState != static_cast<u32>(TMario::STATE_JUMPSPINL))) {
        playerData->mCollisionFlags.mIsSpinBounce = false;
    }

    player->mController->mState.mReadInput = !playerData->mCollisionFlags.mIsDisableInput;
    player->mController->mState.mDisable   = playerData->mCollisionFlags.mIsDisableInput;
}

static void resetValuesOnGroundContact(TMario *player) {
    auto playerData = Player::getData(player);

    if ((player->mPrevState & static_cast<u32>(TMario::STATE_AIRBORN)) != 0 &&
        (player->mState & static_cast<u32>(TMario::STATE_AIRBORN)) == 0 &&
        playerData->mCollisionFlags.mIsAirborn) {
        playerData->mCollisionFlags.mIsAirborn      = false;
        playerData->mCollisionFlags.mIsDisableInput = false;
    }
}

static void resetValuesOnAirborn(TMario *player) {
    auto playerData = Player::getData(player);

    if ((player->mPrevState & static_cast<u32>(TMario::STATE_AIRBORN)) == 0 &&
        (player->mState & static_cast<u32>(TMario::STATE_AIRBORN)) != 0 &&
        !playerData->mCollisionFlags.mIsAirborn) {
        playerData->mCollisionFlags.mIsAirborn = true;
    }
}

static void resetValuesOnCollisionChange(TMario *player) {
    auto playerData = Player::getData(player);

    if (!player->mFloorTriangle)
        return;

    switch (player->mFloorTriangle->mType) {
    case EXPAND_WARP_SET(16042):
    case EXPAND_WARP_SET(17042):
        playerData->setColliding(false);
        break;
    // Callback collision
    case 16081:
    case 17081: {
        if (!playerData->mPrevCollisionFloor) {
            break;
        }

        u8 index = playerData->mPrevCollisionFloor->mValue >> 8;
        if (index == 2) {
            player->mDirtyParams = playerData->mDefaultDirtyParams;
        }
    }
    default:
        playerData->setColliding(true);
        break;
    }
}

#if BETTER_SMS_EXTRA_COLLISION

using namespace BetterSMS;

// static void slipperyCatchingSoundCheck(u32 sound, const Vec *pos, u32 unk_1, JAISound **out,
//                                        u32 unk_2, u8 unk_3) {
//     TMario *player;
//     SMS_FROM_GPR(31, player);
//
//     if (player->mFloorTriangle->mType == 16081 ||
//         player->mFloorTriangle->mType == 17081)
//         sound = 4105;
//
//     MSoundSE::startSoundActor(sound, pos, unk_1, out, unk_2, unk_3);
// }
// SMS_PATCH_BL(SMS_PORT_REGION(0x8025932C, 0x802510B8, 0, 0), slipperyCatchingSoundCheck);
// SMS_WRITE_32(SMS_PORT_REGION(0x802596C0, 0x8025144C, 0, 0), 0x60000000);

static inline bool isColTypeWater(u16 type) { return (type > 255 && type < 261) || type == 16644; }

// extern -> generic.cpp
BETTER_SMS_FOR_CALLBACK void updateCollisionContext(TMario *player, bool isMario) {
    constexpr s16 CrushTimeToDie = 0;

    Player::TPlayerData *playerData = Player::getData(player);

    resetValuesOnStateChange(player);
    resetValuesOnGroundContact(player);
    resetValuesOnAirborn(player);
    resetValuesOnCollisionChange(player);

    if (player->mRoofTriangle) {
        if (player->mRoofTriangle->mType != playerData->mPrevCollisionRoofType) {
            playerData->mPrevCollisionRoofType = player->mRoofTriangle->mType;
            playerData->mPrevCollisionRoof     = player->mRoofTriangle;
        }
    } else {
        playerData->mPrevCollisionRoofType = 0;
        playerData->mPrevCollisionRoof     = nullptr;
    }

    TBGCheckData *roofTri;

    Vec playerPos;
    player->JSGGetTranslation(&playerPos);

    f32 roofHeight =
        player->checkRoofPlane(playerPos, playerPos.y, (const TBGCheckData **)&roofTri);

    const f32 currentRelRoofHeight = roofHeight - playerPos.y;

    playerData->mLastRelRoofHeight = currentRelRoofHeight;
}

#endif