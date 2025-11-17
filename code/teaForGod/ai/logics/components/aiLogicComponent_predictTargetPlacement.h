#pragma once

#include "..\..\..\..\core\math\math.h"

namespace Framework
{
	struct RelativeToPresencePlacement;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class NPCBase;

			struct PredictTargetPlacement
			{
				void set_in_use(bool _inUse) { inUse = _inUse; }

				void set_projectile_speed(float _speed);
				void set_max_off_angle(Optional<float> const & _angle = NP);

				// targetWS is in OUR space
				Vector3 predict(Vector3 const& _targetWS, Framework::IModulesOwner* _owner, Framework::RelativeToPresencePlacement const& _target);

			private:
				bool inUse = true;
				Optional<float> projectileSpeed;
				Optional<float> maxOffAngle;

				Optional<Vector3> predictedForTargetWS;
				Vector3 predictedTargetWS;
			};

		};
	};
};