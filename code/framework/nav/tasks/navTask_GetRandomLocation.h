#pragma once

#include "..\navPath.h"
#include "..\navPlacementAtNode.h"
#include "..\navTask.h"

namespace Framework
{
	namespace Nav
	{
		class Mesh;
		
		namespace Tasks
		{
			/**
			 */
			class GetRandomLocation
			: public Nav::Task
			{
				FAST_CAST_DECLARE(GetRandomLocation);
				FAST_CAST_BASE(Nav::Task);
				FAST_CAST_END();

				typedef Nav::Task base;
			public:
				static GetRandomLocation* new_task(PlacementAtNode const & _start, float _distanceLeft);

				PlacementAtNode const & get_location() const { return location; }

			protected:
				implement_ void perform();

				implement_ void cancel_if_related_to(Room* _room);

				implement_ tchar const* get_log_name() const { return TXT("get random location"); }

			private:
				GetRandomLocation(PlacementAtNode const & _start, float _distanceLeft);

			private:
				PlacementAtNode start;
				PlacementAtNode location;
				float distanceLeft;
			};
		};
	};
};
