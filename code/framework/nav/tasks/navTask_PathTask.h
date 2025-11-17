#pragma once

#include "..\navPath.h"
#include "..\navPlacementAtNode.h"
#include "..\navTask.h"

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
			class PathTask
			: public Nav::Task
			{
				FAST_CAST_DECLARE(PathTask);
				FAST_CAST_BASE(Nav::Task);
				FAST_CAST_END();

				typedef Nav::Task base;
			public:
				PathTask(bool _writer);

				PathPtr const & get_path() const { return path; }

#ifdef AN_LOG_NAV_TASKS
			public: // Nav::Task
				override_ void log_internal(LogInfoContext& _log) const;
#endif

			protected:
				PathPtr path;
			};
		};
	};
};
