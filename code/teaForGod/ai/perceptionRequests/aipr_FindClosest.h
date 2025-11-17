#pragma once

#include "..\..\..\framework\ai\aiPerceptionRequest.h"
#include "..\..\..\framework\presence\presencePath.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\core\memory\pooledObject.h"

#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define AN_DEBUG_FIND_CLOSEST
#endif

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace PerceptionRequests
		{
			struct FindClosest
			: public PooledObject<FindClosest>
			, public Framework::AI::PerceptionRequest
			{
				FAST_CAST_DECLARE(FindClosest);
				FAST_CAST_BASE(Framework::AI::PerceptionRequest);
				FAST_CAST_END();

				typedef Framework::AI::PerceptionRequest base;
			public:
				typedef std::function<bool(Framework::IModulesOwner const * _check, REF_ Optional<Transform> & _offsetOS)> CheckObject;

				FindClosest(Framework::IModulesOwner* _owner, Optional<float> const& _maxDistance = NP, Optional<int> const& _maxDepth = NP, CheckObject _check_object = nullptr);

				bool has_found() const { return result.is_set(); }
				Framework::IModulesOwner* get_result() const { return result.get(); }
				Framework::PresencePath const & get_path() const { return pathToResult; }

				FindClosest* add_door_penalty(int _atDoor, float _addDistance);
				FindClosest* only_navigable() { onlyNavigable = true; return this; }

			protected: // PerceptionRequest
				override_ bool process();

			private:
				enum Phase
				{
					P_Setup,
					P_SearchRooms,
					P_Finalise
				};

				Phase phase = P_Setup;

#ifdef AN_DEBUG_FIND_CLOSEST
				int psrNo = 0; // [!@#]
#endif

				CheckObject check_object;
				float maxDistance = 0.0f;
				int maxDepth = 0;
				SafePtr<Framework::IModulesOwner> owner;
				SafePtr<Framework::IModulesOwner> result;
				float resultDistance = 0.0f;
				Framework::PresencePath pathToResult;
				bool onlyNavigable = false;

				struct DoorPenalty
				{
					float addDistance;
					int atDoor;
				};
				ArrayStatic<DoorPenalty, 4> doorPenalties;

				struct SearchRoom
				{
					Framework::RelativeToPresencePlacement toRoom; // to room we search
					int depth = 0; // from the starting room
					float distanceSoFar = 0.0f;
				};
				List<SearchRoom> searchRooms;

				void process_search_room();
			};
		};
	};
};
