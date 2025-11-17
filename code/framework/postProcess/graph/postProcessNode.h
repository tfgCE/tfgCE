#pragma once

#include <functional>

#include "..\..\..\core\graph\graph.h"
#include "..\..\..\core\system\video\shaderProgramInstance.h"
#include "..\..\..\core\instance\instanceDatum.h"
#include "..\..\library\libraryLoadingContext.h"
#include "..\..\video\shaderProgram.h"

namespace System
{
	struct ShaderProgramInstance;
};

namespace Framework
{
	class PostProcessGraph;
	class PostProcessNode;
	class PostProcessGraphInstance;
	class PostProcessRenderTargetManager;
	struct PostProcessRenderTargetPointer;
	struct PostProcessGraphProcessingContext;

	typedef std::function< bool(PostProcessNode const * _node, ::System::ShaderProgramInstance & _shaderProgramInstance) > ForShaderProgramInstance; // return true if to continue searching, false to stop

	class PostProcessNode
	: public ::Graph::Node<PostProcessGraph, PostProcessNode, PostProcessGraphInstance, LibraryLoadingContext>
	{
		FAST_CAST_DECLARE(PostProcessNode);
		FAST_CAST_END();

		typedef ::Graph::Node<PostProcessGraph, PostProcessNode, PostProcessGraphInstance, LibraryLoadingContext> base;
	public:
		PostProcessNode();
		virtual ~PostProcessNode();

		virtual bool load_shader_program(Library* _library);

		virtual void initialise(InstanceData & _instanceData) const;
		virtual void release_render_targets(InstanceData & _instanceData) const;
		virtual void do_post_process(InstanceData & _instanceData, PostProcessGraphProcessingContext const& _processingContext) const;
		virtual bool for_every_shader_program_instance(InstanceData & _instanceData, ForShaderProgramInstance _for_every_shader_program_instance) const;
		virtual void manage_activation_using(InstanceData & _instanceData, SimpleVariableStorage const & _storage) const;

		virtual PostProcessRenderTargetPointer const & get_output(InstanceData & _instanceData, int _outputIndex) const;

		void ready_for_post_processing(InstanceData & _instanceData) const;

	protected:
		void propagate_outputs(InstanceData & _instanceData) const;
		virtual void gather_input(InstanceData & _instanceData, int _inputIndex, PostProcessRenderTargetPointer const & _renderTargetPointer) const;

	public: // ::Graph::Node
		override_ void handle_instance_data(REF_ InstanceDataHandler const & _dataHandler) const;

	protected:
		void mark_post_processing_done(InstanceData & _instanceData) const;
		bool was_post_processing_done(InstanceData & _instanceData) const;

	protected:
		struct AdditionalInputData
		{
			Optional<::System::TextureFiltering::Type> inputFilteringMag;
			Optional<::System::TextureFiltering::Type> inputFilteringMin;

			AdditionalInputData();
			bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		};

	private:
		::Instance::Datum<bool> iPostProcessingDone;
	};

};
