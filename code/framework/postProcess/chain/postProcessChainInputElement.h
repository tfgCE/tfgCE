#pragma once

#include "postProcessChainElement.h"

namespace Framework
{
	class PostProcessChain;

	class PostProcessChainInputElement
	: public PostProcessChainElement
	{
		FAST_CAST_DECLARE(PostProcessChainInputElement);
			FAST_CAST_BASE(PostProcessChainElement);
		FAST_CAST_END();

		typedef PostProcessChainElement base;
	private: friend class PostProcessChain;
		PostProcessChainInputElement();
		virtual ~PostProcessChainInputElement();

		void use_input(PostProcessChain* _postProcessChain, ::System::RenderTarget* _renderTarget, Name const * _nameOverride); // name will override_ all outputs, so it should be used only with render targets with one texture

		void clear_input_usage();
		void add_input_usage(int _index); // input/output

		void setup_render_target_output_usage(::System::RenderTarget* _inputRenderTarget); // as input usage is

	private: // PostProcessChainElement
		override_ int get_output_render_target_count() const;
		override_ PostProcessRenderTargetPointer const & get_output_render_target(int _index) const;
		override_ void do_post_process(PostProcessChainProcessingContext const & _processingContext);
		override_ void release_render_targets();

	private:
		struct Input
		{
		public:
			PostProcessRenderTargetPointer renderTarget;

			Input();
			void clear_usage();
			void add_usage();
			bool is_used() const { return usageCount != 0; }

		private:
			int usageCount;
		};
		// those are inputs for this element from different render target (or post process chain maybe too?)
		// and those are also outputs of this element in this post process chain
		Array<Input> inputs;
	};
};
