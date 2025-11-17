#pragma once

#include "postProcessNode.h"

#include "..\..\..\core\graph\graphInstance.h"
#include "..\..\library\libraryLoadingContext.h"

namespace Framework
{
	class Library;
	class PostProcessGraph;
	class PostProcessNode;
	class PostProcessInstance;
	class PostProcessGraphInstance;
	class PostProcessOutputNode;
	struct PostProcessGraphProcessingContext;

	class PostProcessGraphInstance
	: public ::Graph::Instance<PostProcessGraph, PostProcessNode, PostProcessGraphInstance, LibraryLoadingContext>
	{
		typedef ::Graph::Instance<PostProcessGraph, PostProcessNode, PostProcessGraphInstance, LibraryLoadingContext> base;

	public:
		PostProcessGraphInstance(PostProcessGraph const * _graph);
		virtual ~PostProcessGraphInstance();

		void initialise(PostProcessInstance* _postProcessInstance);
		void release_render_targets();

		void do_post_process(PostProcessGraphProcessingContext & _processingContext);
		void for_every_shader_program_instance(ForShaderProgramInstance _for_every_shader_program_instance);
		void manage_activation_using(SimpleVariableStorage const & _storage);

		PostProcessRenderTargetPointer const & get_output_render_target(int _outputIndex, OUT_ OPTIONAL_ Name * _outputRenderTargetName = nullptr) const;
		PostProcessRenderTargetPointer const & get_output_render_target() const;

		PostProcessOutputNode const * get_output_node() const { return outputNode; }

	private:
		PostProcessInstance* postProcessInstance;
		PostProcessOutputNode const * outputNode;
		bool initialised;
	};

};
