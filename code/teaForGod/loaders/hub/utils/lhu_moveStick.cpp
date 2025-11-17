#include "lhu_moveStick.h"

//

using namespace Loader;
using namespace Utils;

//

const float MoveStick::c_firstRepeat = 0.5f;
const float MoveStick::c_nextRepeat = 0.2f;

void MoveStick::reset()
{
	currMoveStick.clear();
	timeToRepeat.clear();
	outMove = VectorInt2::zero;
}

VectorInt2 const & MoveStick::update(Vector2 const& _moveStick, float _deltaTime)
{
	outMove = VectorInt2::zero;
	if (max(abs(_moveStick.x), abs(_moveStick.y)) < 0.5f)
	{
		currMoveStick.clear();
		timeToRepeat.clear();
	}
	else
	{
		bool provideOutputNow = false;
		if (!currMoveStick.is_set() || ! timeToRepeat.is_set() ||
			(currMoveStick.is_set() && Vector2::dot(currMoveStick.get(), _moveStick) < 0.7f))
		{
			provideOutputNow = true;
			timeToRepeat = c_firstRepeat;
			currMoveStick.clear();
		}
		else
		{
			if (timeToRepeat.is_set())
			{
				timeToRepeat = timeToRepeat.get() - _deltaTime;
				if (timeToRepeat.get() <= 0.0f)
				{
					timeToRepeat.clear();
				}
			}
			if (!timeToRepeat.is_set())
			{
				timeToRepeat = c_nextRepeat;
				provideOutputNow = true;
			}
		}
		if (provideOutputNow)
		{
			outMove = _moveStick.to_vector_int2();
			if (!currMoveStick.is_set())
			{
				currMoveStick = outMove.to_vector2();
			}
		}
	}
	return outMove;
}
