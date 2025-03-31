#include <Dolphin/GX.h>
#include <Dolphin/OS.h>

#include <JSystem/JDrama/JDRNameRef.hxx>

#include <SMS/Map/MapCollisionData.hxx>
#include <SMS/macros.h>

#include "libs/constmath.hxx"
#include "logging.hxx"
#include "module.hxx"
#include "p_globals.hxx"
#include "p_warp.hxx"

/*This works by taking the target id and matching it to the
/ ID of the first entry to have the same home ID as the target.
/
/ This makes a half decent linking system for the collision
/ triangles for functionalities like linked warping!
*/

#if BETTER_SMS_EXTRA_COLLISION

using namespace BetterSMS;
using namespace BetterSMS::Collision;

static void parseWarpLinks(TMapCollisionData *col, TWarpCollisionList *links, u32 validID,
                           u32 idGroupSize = 0) {
    u32 curDataIndex = 0;

    for (u32 i = 0; i < col->mCheckDataCount; ++i) {
        if (TCollisionLink::isValidWarpCol(&col->mCollisionTris[i])) {

            TCollisionLink link(&col->mCollisionTris[i], (u8)(col->mCollisionTris[i].mValue >> 8),
                                (u8)col->mCollisionTris[i].mValue,
                                TCollisionLink::getSearchModeFrom(&col->mCollisionTris[i]));

            links->addLink(link);
            if (curDataIndex > 0xFF)
                break;
        }
    }
}

// 0x802B8B20
static u32 initCollisionWarpLinks(const char *name) {
    TWarpCollisionList *warpDataArray         = new TWarpCollisionList(2048);
    TWarpCollisionList *warpDataPreserveArray = new TWarpCollisionList(1);
    BetterSMS::sWarpColArray                  = warpDataArray;
    BetterSMS::sWarpColPreserveArray          = warpDataPreserveArray;

    if (warpDataArray) {
        parseWarpLinks(gpMapCollisionData, warpDataArray, 16040, 4);
    }

    return JDrama::TNameRef::calcKeyCode(name);
}
SMS_PATCH_BL(SMS_PORT_REGION(0x802B8B20, 0x802B0AF0, 0, 0), initCollisionWarpLinks);

#endif