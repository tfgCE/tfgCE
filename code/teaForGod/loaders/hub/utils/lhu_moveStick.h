#pragma once

#include "..\..\..\..\core\math\math.h"

namespace Loader
{
	namespace Utils
	{
		struct MoveStick
		{
		public:
			void reset();

			VectorInt2 const& update(Vector2 const& _moveStick, float _deltaTime);

		private:
			static const float c_firstRepeat;
			static const float c_nextRepeat;

			Optional<Vector2> currMoveStick;
			Optional<float> timeToRepeat;
			VectorInt2 outMove;
		};
	};
};
