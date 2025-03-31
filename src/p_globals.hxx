#pragma once

#include <Dolphin/types.h>

#include "p_warp.hxx"

namespace BetterSMS {
    extern void *sPRMFile;
    extern Collision::TWarpCollisionList *sWarpColArray;
    extern Collision::TWarpCollisionList *sWarpColPreserveArray;
    extern bool sIsAudioStreaming;
    extern bool sIsDebugMode;
}  // namespace BetterSMS