#pragma once

#include "..\..\..\core\fastCast.h"
#include "postProcessChainElement.h"

namespace System
{
	struct FragmentShaderOutputSetup;
};

namespace Framework
{
	class PostProcessChain;

	class PostProcessChainOutputElement
	: public PostProcessChainElement
	{
		FAST_CAST_DECLARE(PostProcessChainOutputElement);
			FAST_CAST_BASE(PostProcessChainElement);
		FAST_CAST_END();

		typedef PostProcessChainElement base;
	private: friend class PostProcessChain;
		PostProcessChainOutputElement();
		virtual ~PostProcessChainOutputElement();

		void use_output(::System::FragmentShaderOutputSetup const & _outputDefinition);

	private: // PostProcessChainElement
		override_ void do_post_process(PostProcessChainProcessingContext const & _processingContext);
		override_ void release_render_targets();

		override_ int get_output_render_target_count() const;
		override_ PostProcessRenderTargetPointer const & get_output_render_target(int _index) const;

	private:
		Array<PostProcessRenderTargetPointer> outputRenderTargets;
	};
};
