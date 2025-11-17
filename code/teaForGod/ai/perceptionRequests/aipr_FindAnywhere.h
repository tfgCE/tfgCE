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
			struct FindAnywhere
			: public PooledObject<FindAnywhere>
			, public Framework::AI::PerceptionRequest
			{
				FAST_CAST_DECLARE(FindAnywhere);
				FAST_CAST_BASE(Framework::AI::PerceptionRequest);
				FAST_CAST_END();

				typedef Framework::AI::PerceptionRequest base;
			public:
				typedef std::function<float(Framework::IModulesOwner const * _check)> CheckObject;

				FindAnywhere(Framework::IModulesOwner* _owner, CheckObject _check_object);

				bool has_found() const { return result != nullptr; }
				Framework::IModulesOwner* get_result() const { return result; }
				Framework::PresencePath const & get_path() const { return pathToResult; }

			protected: // PerceptionRequest
				override_ bool process();

			private:
				CheckObject check_object;
				Framework::IModulesOwner* owner = nullptr;
				Framework::IModulesOwner* result = nullptr;
				Framework::PresencePath pathToResult;
			};
		};
	};
};
