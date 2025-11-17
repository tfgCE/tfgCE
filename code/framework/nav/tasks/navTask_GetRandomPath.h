#pragma once

#include "navTask_PathTask.h"

namespace Framework
{
	namespace Nav
	{
		class Mesh;
		struct PathRequestInfo;
		
		namespace Tasks
		{
			/**
			 */
			class GetRandomPath
			: public PathTask
			{
				FAST_CAST_DECLARE(GetRandomPath);
				FAST_CAST_BASE(PathTask);
				FAST_CAST_END();

				typedef PathTask base;
			public:
				static GetRandomPath* new_task(PlacementAtNode const & _start, float _distanceLeft, Optional<Vector3> const & _startInDir, PathRequestInfo const & _requestInfo);

				PlacementAtNode const & get_location() const { return location; }

			protected:
				implement_ void perform();

				implement_ void cancel_if_related_to(Room* _room);

				implement_ tchar const* get_log_name() const { return TXT("get random path"); }

			private:
				GetRandomPath(PlacementAtNode const & _start, float _distanceLeft, Optional<Vector3> const & _startInDir, PathRequestInfo const & _requestInfo);

			private:
				PathRequestInfo requestInfo;

				PlacementAtNode start;
				PlacementAtNode location;
				float distanceLeft;
				Optional<Vector3> startInDir;
			};
		};
	};
};
