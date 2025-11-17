#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\functionParamsStruct.h"
#include "..\..\core\math\math.h"

#include <functional>

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	struct Damage;
	struct DamageInfo;

	namespace LightningDischarge
	{
		struct Params
		{
			ADD_FUNCTION_PARAM_TRUE(Params, ignoreNarrative, ignore_narrative);
			ADD_FUNCTION_PARAM_TRUE(Params, ignoreViolenceDisallowed, ignore_violence_disallowed);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(Params, Framework::IModulesOwner*, imo, for_imo, nullptr);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(Params, Framework::IModulesOwner*, instigator, for_instigator, nullptr);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(Params, Transform, startPlacementOS, with_start_placement_OS, Transform::identity);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(Params, Vector3, endPlacementOffset, with_end_placement_offset, Vector3::zero); // relative to starting placement
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(Params, float, maxDist, with_max_dist, 2.0f);
			ADD_FUNCTION_PARAM_PLAIN(Params, std::function<void(Damage &, DamageInfo &)>, setup_damage, with_setup_damage);
			ADD_FUNCTION_PARAM(Params, int, lightningCount, with_lightning_count);
			// this order of search/checks - explicitly switch off these
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(Params, bool, rayCastSearchForObjects, with_ray_cast_search_for_objects, true, true);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(Params, bool, wideSearchForObjects, with_wide_search_for_objects, true, true);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(Params, bool, fallbackRayCastSearchForObjects, with_fallback_ray_cast_search_for_objects, true, true);
				// ^- by default it is set to true, if won't hit anything and didn't do raycast in first place, will do this
				//    this is to always get the proper hit place for shields
			// order of search/checks
			ADD_FUNCTION_PARAM_DEF(Params, bool, singleRoom, in_single_room, true);
			// both used for lightning spawner and sound
			ADD_FUNCTION_PARAM(Params, Name, hitID, with_hit_id);
			ADD_FUNCTION_PARAM(Params, Name, missID, with_miss_id);
			// temporary object id
			ADD_FUNCTION_PARAM(Params, Name, lightningHitTOID, with_lightning_hit_to_id);
		};
		bool perform(Params const & _params); // returns true if hit
	};
};
