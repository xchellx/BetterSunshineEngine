#include <Dolphin/MTX.h>
#include <Dolphin/math.h>
#include <Dolphin/types.h>

#include <SMS/Enemy/EnemyMario.hxx>
#include <SMS/Player/Mario.hxx>
#include <SMS/macros.h>

#include "module.hxx"
#include "player.hxx"

using namespace BetterSMS;

static void rescaleHeldObj(Mtx holderMatrix, Mtx destMatrix) {
    TMapObjBase *heldObj;
    SMS_FROM_GPR(31, heldObj);

    Vec holderSize;
    heldObj->mHolder->JSGGetScaling(&holderSize);

    PSMTXCopy(holderMatrix, destMatrix);
    PSMTXScaleApply(destMatrix, destMatrix, 1 / holderSize.x, 1 / holderSize.y, 1 / holderSize.z);
}
SMS_PATCH_BL(SMS_PORT_REGION(0x801E4118, 0x801DBFF0, 0, 0), rescaleHeldObj);