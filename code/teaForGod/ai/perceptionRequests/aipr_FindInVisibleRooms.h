#pragma once

#include "..\..\..\framework\ai\aiPerceptionRequest.h"
#include "..\..\..\framework\presence\presencePath.h"
#include "..\..\..\core\memory\pooledObject.h"
#include "..\..\..\core\system\video\viewFrustum.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class NPCBase;
		};

		namespace PerceptionRequests
		{
			struct FindInVisibleRooms
			: public PooledObject<FindInVisibleRooms>
			, public Framework::AI::PerceptionRequest
			{
				FAST_CAST_DECLARE(FindInVisibleRooms);
				FAST_CAST_BASE(Framework::AI::PerceptionRequest);
				FAST_CAST_END();

				typedef Framework::AI::PerceptionRequest base;
			public:
				typedef std::function<float(Framework::IModulesOwner const * _check)> CheckObject; // check if is ok to consider
				typedef std::function<float(Framework::IModulesOwner const * _check, Framework::PresencePath& _path)> RateObject;

				FindInVisibleRooms(Framework::IModulesOwner* _owner, CheckObject _check_object, RateObject _rate_object = nullptr, int _notVisibleRoomDepth = 2);

				bool has_found_anything() const { an_assert(is_processed()); return bestIdx != NONE; }
				Framework::PresencePath const & get_path_to_best() const { an_assert(has_found_anything()); return found[bestIdx].path; }

			protected: // PerceptionRequest
				override_ bool process();

			private:
				CheckObject check_object;
				RateObject rate_object;
				int notVisibleRoomDepth;
				SafePtr<Framework::IModulesOwner> owner;
				AI::Logics::NPCBase* npcBase = nullptr;

				struct Found
				{
					bool isInVisibleRoom = true;
					SafePtr<Framework::IModulesOwner> object;
					Framework::PresencePath path;
					float score;
				};
				ArrayStatic<Found, 64> found;
				int checkFoundIdx = NONE; // if none, gets all into found array
				int bestIdx = NONE;
				float bestScore = 0.0f;

				static int const MAX_DEPTH = 8;
				struct LookInto
				{
					System::ViewFrustum viewFrustum;
					Transform viewpointPlacement;
					Framework::DoorInRoom const * throughDoor = nullptr;
				};
				ArrayStatic<LookInto, MAX_DEPTH> lookInto;

				void look_into(Framework::Room const * _room, Transform const & _viewpointPlacement, Framework::DoorInRoom const * _throughDoor, Optional<int> const & _notVisibleRoomIdx);

				float own_rate_object(Framework::IModulesOwner const * _check, Framework::PresencePath& _path, bool _isInVisibleRoom) const;
			};
		};
	};
};
