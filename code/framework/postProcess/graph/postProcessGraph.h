#pragma once

#include "..\..\..\core\graph\graph.h"
#include "..\..\library\libraryLoadingContext.h"
#include "postProcessNode.h" // it has to be here
#include "postProcessRootNode.h" // it has to be here
#include "postProcessGraphInstance.h"

namespace Framework
{
	class Library;
	class PostProcessGraph;
	class PostProcessGraphInstance;
	class PostProcessRenderTargetManager;

	class PostProcessGraph
	: public ::Graph::Graph<PostProcessGraph, PostProcessNode, PostProcessGraphInstance, LibraryLoadingContext>
	{
		typedef ::Graph::Graph<PostProcessGraph, PostProcessNode, PostProcessGraphInstance, LibraryLoadingContext> base;

	public:
		PostProcessGraph();

		bool load_shader_programs(Library* _library);

		PostProcessRootNode const * get_root_node() const { return postProcessRootNode; }

	public: // Graph
		override_ PostProcessNode* create_node(String const & _name, NodeContainer* _nodeContainer);
		override_ PostProcessNode* create_root_node();
		override_ void set_root_node(PostProcessNode* _rootNode);

	private:
		PostProcessRootNode* postProcessRootNode;
	};

};
