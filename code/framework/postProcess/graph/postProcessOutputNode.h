#pragma once

#include "postProcessNode.h"
#include "..\postProcessRenderTargetPointer.h"

namespace Framework
{
	/**
	 *	This node grabs output render targets of nodes from the graph to provide them outside
	 */
	class PostProcessOutputNode
	: public PostProcessNode
	{
		FAST_CAST_DECLARE(PostProcessOutputNode);
		FAST_CAST_BASE(PostProcessNode)
		FAST_CAST_END();

		typedef PostProcessNode base;
	public:
		PostProcessOutputNode();
		virtual ~PostProcessOutputNode();

		int get_output_render_target_count(InstanceData * _data) const;
		PostProcessRenderTargetPointer const & get_output_render_target(InstanceData * _data, int _outputIndex, OUT_ OPTIONAL_ Name * _outputRenderTargetName = nullptr) const;
		Name const & get_main_output_render_target_name() const { return mainOutputRenderTargetName; }
		int get_main_output_render_target_index() const { return mainOutputRenderTargetIndex; }

	public: // PostProcessNode
		override_ void do_post_process(InstanceData & _instanceData, PostProcessGraphProcessingContext const& _processingContext) const;
		override_ void initialise(InstanceData & _instanceData) const;
		override_ void release_render_targets(InstanceData & _instanceData) const;
		override_ void gather_input(InstanceData & _instanceData, int _inputIndex, PostProcessRenderTargetPointer const & _renderTargetPointer) const;

	public: // ::Graph::Node
		override_ void handle_instance_data(REF_ InstanceDataHandler const & _dataHandler) const;

		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool on_input_loaded_from_xml(int _inputIndex, IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	private:
		Name mainOutputRenderTargetName;
		CACHED_ int mainOutputRenderTargetIndex = NONE;
		Array<Name> outputRenderTargetNames; // defined for names
		Array<AdditionalInputData> additionalInputData;
		::Instance::Datum<Array<PostProcessRenderTargetPointer> > iOutputRenderTargets;
	};

};
