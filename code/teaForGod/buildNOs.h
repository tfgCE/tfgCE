#pragma once

#include "..\core\globalDefinitions.h"

// remove all game slots that were stored before this version (if we actually have the number!)
// currently disabled to allow keeping them
#define BUILD_NO__INVALIDATE_GAME_SLOTS -230

// propose restart at the first pilgrimage (only if not at first)
#define BUILD_NO__RESTART_GAME_SLOT_PROPOSAL 237

#define BUILD_NO__ACK_FLAG__RESTART_ADVENTURE bit(0)