#include <SMS/Manager/FlagManager.hxx>
#include <SMS/SPC/SpcBinary.hxx>
#include <SMS/macros.h>

#include "logging.hxx"
#include "module.hxx"
#include "p_settings.hxx"
#include "sunscript.hxx"

using namespace BetterSMS;

static void initBinaryNullptrPatch(TSpcBinary *binary) {
    if (binary || !BetterSMS::areBugsPatched()) {
        binary->init();
        return;
    }
    Console::debugLog("Warning: SPC binary is nullptr!\n");
}
SMS_PATCH_BL(SMS_PORT_REGION(0x80289098, 0x80280E24, 0, 0), initBinaryNullptrPatch);
