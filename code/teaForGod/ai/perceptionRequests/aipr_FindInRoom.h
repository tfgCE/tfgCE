#pragma once

#include "..\..\..\framework\ai\aiPerceptionRequest.h"
#include "..\..\..\framework\presence\presencePath.h"
#include "..\..\..\core\memory\pooledObject.h"

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
			struct FindInRoom
			: public PooledObject<FindInRoom>
			, public Framework::AI::PerceptionRequest
			{
				FAST_CAST_DECLARE(FindInRoom);
				FAST_CAST_BASE(Framework::AI::PerceptionRequest);
				FAST_CAST_END();

				typedef Framework::AI::PerceptionRequest base;
			public:
				typedef std::function<float(Framework::IModulesOwner const * _check)> CheckObject;

				FindInRoom(Framework::IModulesOwner* _owner, CheckObject _check_object);

				bool has_found() const { return pathToResult.has_target(); }
				Framework::PresencePath const & get_path() const { return pathToResult; }

			protected: // PerceptionRequest
				override_ bool process();

			private:
				CheckObject check_object;
				Framework::IModulesOwner* owner = nullptr;
				Framework::PresencePath pathToResult;
			};
		};
	};
};
