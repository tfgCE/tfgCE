#pragma once

#include "..\..\core\collision\collisionFlags.h"
#include "..\..\core\globalDefinitions.h"
#include "..\..\core\math\math.h"

namespace Framework
{
	interface_class IModulesOwner;
	class Room;
	struct CheckSegmentResult;
	struct RelativeToPresencePlacement;

	namespace AI
	{
		class Message;
		class MindInstance;
		struct LatentTaskInfoWithParams;
	};
};

struct Transform;
struct Vector3;

namespace TeaForGodEmperor
{
	namespace AI
	{
		struct CastInfo
		{
			Optional<bool> atTarget; // at target socket or anywhere within target
			Optional<bool> atCentre; //
			Optional<Collision::Flags> collisionFlags;

			CastInfo & at_target() { atTarget = true; return *this; }
			CastInfo & at_centre() { atCentre = true; return *this; }
			CastInfo & at_within_target() { atTarget = false; return *this; }
			CastInfo & with_collision_flags(Collision::Flags const & _flags) { collisionFlags = _flags; return *this; }
		};
		bool do_ray_cast(CastInfo const & _castInfo, Vector3 const & _startWS, Framework::IModulesOwner* _owner, Framework::IModulesOwner* _enemy, Framework::Room* _enemyRoom, Transform const & _enemyPlacementInOwnerRoom, OUT_ Framework::CheckSegmentResult & _result, Framework::RelativeToPresencePlacement * _fillRelativeToPresencePlacement = nullptr); // returns result etc
		bool check_clear_ray_cast(CastInfo const & _castInfo, Vector3 const & _startWS, Framework::IModulesOwner* _owner, Framework::IModulesOwner* _enemy, Framework::Room* _enemyRoom, Transform const & _enemyPlacementInOwnerRoom, Optional<Transform> const& _enemyOffsetOS = NP); // ignores actors items etc
		bool check_clear_perception_ray_cast(CastInfo const & _castInfo, Transform const & _perceptionSocketWS, Range const & _perceptionFOV, Range const& _perceptionVerticalFOV, Framework::IModulesOwner const * _owner, Framework::IModulesOwner const * _enemy, Framework::Room const * _enemyRoom, Transform const & _enemyPlacementInOwnerRoom, Optional<Transform> const & _enemyOffsetOS = NP);
		bool check_clear_perception_ray_cast(CastInfo const & _castInfo, Transform const & _perceptionSocketWS, Optional<Transform> const& _secondaryPerceptionSocketWS, Range const & _perceptionFOV, Range const& _perceptionVerticalFOV, Framework::IModulesOwner const * _owner, Framework::IModulesOwner const * _enemy, Framework::Room const * _enemyRoom, Transform const & _enemyPlacementInOwnerRoom, Optional<Transform> const & _enemyOffsetOS = NP); // chooses better socket
	};
};
