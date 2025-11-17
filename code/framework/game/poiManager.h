#pragma once

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\memory\safeObject.h"

#include "..\world\pointOfInterestInstance.h"

//

namespace Framework
{
	class POIManager
	{
	public:
		void reset();
		void update(float _deltaTime);

		bool check(Framework::PointOfInterestInstance const * _poi, Name const& _usage); // not definite!
		bool check_and_occupy(Framework::PointOfInterestInstance const * _poi, Name const& _usage, Optional<float> const& _forTime);

	private:
		Concurrency::SpinLock modifyLock; // used only if modifying, otherwise, when reading, we're good as it is

		struct Entry
		{
			Name usage;
			SafePtr<Framework::PointOfInterestInstance> poi;
			Optional<float> timeLeft;
		};
		ArrayStatic<Entry, 128> entries;
	};
};
