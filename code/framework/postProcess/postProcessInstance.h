#pragma once

#include "..\library\libraryStored.h"
#include "graph\postProcessGraph.h"

namespace Framework
{
	class PostProcess;
	class PostProcessGraphInstance;

	class PostProcessInstance
	{
	public:
		PostProcessInstance(PostProcess* _postProcess);
		~PostProcessInstance();

		void initialise();

		PostProcess* get_post_process() const { return postProcess; }
		PostProcessGraphInstance* get_graph_instance() { return graphInstance; }

	private:
		PostProcess* postProcess;
		PostProcessGraphInstance* graphInstance;
	};

};
