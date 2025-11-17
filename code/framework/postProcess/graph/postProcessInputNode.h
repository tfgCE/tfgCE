#pragma once

#include "postProcessNode.h"

#include "..\postProcessRenderTargetPointer.h"

namespace Framework
{
	class PostProcessInputNode
	: public PostProcessNode
	{
		FAST_CAST_DECLARE(PostProcessInputNode);
			FAST_CAST_BASE(PostProcessNode)
		FAST_CAST_END();

		typedef PostProcessNode base;
	public:
		PostProcessInputNode();
		virtual ~PostProcessInputNode();

	public: // PostProcessNode
		override_ void do_post_process(InstanceData & _instanceData, PostProcessGraphProcessingContext const & _processingContext) const;
		override_ void initialise(InstanceData & _instanceData) const;
		override_ void release_render_targets(InstanceData & _instanceData) const;
		override_ PostProcessRenderTargetPointer const & get_output(InstanceData & _instanceData, int _outputIndex) const;

	public: // ::Graph::Node
		override_ void handle_instance_data(REF_ InstanceDataHandler const & _dataHandler) const;

	private:
		::Instance::Datum<Array<PostProcessRenderTargetPointer> > iInputRenderTargets;

	};

};
