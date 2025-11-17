#pragma once

#include "aiPerceptionRequest.h"

#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\memory\refCountObjectPtr.h"

namespace Framework
{
	class Object;

	namespace AI
	{
		class MindInstance;
		struct PerceptionRequest;

		/**
		 *	Looks for particular objects, places, etc.
		 *	It's fully async! And should be used only as such.
		 *	Takes requests and gives back handles that are filled with results.
		 *	Most requests should be handled immediately in advance_perception.
		 */
		class Perception
		{
		public:
			static bool is_valid_for_perception(Object const* _object);

		public:
			Perception(MindInstance* _mind);
			~Perception();

			bool does_require_advance() const { return !requests.is_empty(); }
			void advance();

			PerceptionRequest* add_request(PerceptionRequest* _request);

		private:
#ifndef AN_CLANG
#ifdef AN_DEVELOPMENT
			MindInstance* mind;
#endif
#endif

			Concurrency::SpinLock requestsAdditionLock = Concurrency::SpinLock(TXT("Framework.AI.Perception.requestsAdditionLock")); // only for addition
			Array<PerceptionRequestPtr> requests;

		};
	};
};
