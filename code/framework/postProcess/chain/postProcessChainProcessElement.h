#pragma once

#include "..\..\..\core\fastCast.h"
#include "..\..\..\core\tags\tag.h"
#include "postProcessChainElement.h"
#include "..\graph\iPostProcessGraphProcessingInputRenderTargetsProvider.h"

namespace Framework
{
	class PostProcess;
	class PostProcessInstance;
	class PostProcessInputNode;
	class PostProcessOutputNode;

	class PostProcessChainProcessElement
	: public PostProcessChainElement
	, public IPostProcessGraphProcessingInputRenderTargetsProvider
	{
		FAST_CAST_DECLARE(PostProcessChainProcessElement);
			FAST_CAST_BASE(PostProcessChainElement);
		FAST_CAST_END();

		typedef PostProcessChainElement base;
	public:
		PostProcessChainProcessElement(PostProcess* _postProcess, Tags const & _requires, Tags const & _disallows);
		virtual ~PostProcessChainProcessElement();

		PostProcessInstance & access_post_process_instance() { return *postProcessInstance; }
		PostProcessInstance const & get_post_process_instance() const { return *postProcessInstance; }

	public: // PostProcessChainElement
		override_ int get_output_render_target_count() const;
		override_ PostProcessRenderTargetPointer const & get_output_render_target(int _index) const;
		override_ void for_every_shader_program_instance(ForShaderProgramInstance _for_every_shader_program_instance);
		override_ void do_post_process(PostProcessChainProcessingContext const & _processingContext);
		override_ void release_render_targets();
		override_ bool is_active() const;

	public: // IPostProcessGraphProcessingInputRenderTargetsProvider
		implement_ PostProcessRenderTargetPointer const & get_input_render_target(Name const & _inputName) const;

	private:
		Tags requires;
		Tags disallows;
		PostProcessInstance* postProcessInstance; // for this element only, only valid during do_post_process
		PostProcessInputNode const * inputNode;
		PostProcessOutputNode const * outputNode;

	};
};
