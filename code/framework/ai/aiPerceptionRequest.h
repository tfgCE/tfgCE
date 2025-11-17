#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\memory\refCountObject.h"

namespace Framework
{
	namespace AI
	{
		class Perception;
		struct PerceptionRequest;

		typedef RefCountObjectPtr<PerceptionRequest> PerceptionRequestPtr;

		struct PerceptionRequest
		: public RefCountObject
		{
			FAST_CAST_DECLARE(PerceptionRequest);
			FAST_CAST_END();
		public:
			virtual ~PerceptionRequest() {}

			bool is_processed() const { return processed; }

		protected: friend class Perception;
			virtual bool process(); // return true if processed (call base to end processing)

		private:
			bool processed = false;
		};
	};
};
