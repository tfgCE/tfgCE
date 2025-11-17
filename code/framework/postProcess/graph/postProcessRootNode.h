#pragma once

#include "postProcessNode.h"

namespace Framework
{
	class PostProcessInputNode;
	class PostProcessOutputNode;

	class PostProcessRootNode
	: public PostProcessNode
	{
		FAST_CAST_DECLARE(PostProcessRootNode);
		FAST_CAST_BASE(PostProcessNode);
		FAST_CAST_END();

		typedef PostProcessNode base;
	public:
		PostProcessRootNode();
		virtual ~PostProcessRootNode();

		PostProcessInputNode const * get_input_node() const { return inputNode; }
		PostProcessOutputNode const * get_output_node() const { return outputNode; }

	public: // PostProcessNode
		override_ bool load_shader_program(Library* _library);
		override_ bool for_every_shader_program_instance(InstanceData & _instanceData, ForShaderProgramInstance _for_every_shader_program_instance) const;
		override_ void manage_activation_using(InstanceData & _instanceData, SimpleVariableStorage const & _storage) const;

	public: // ::Graph::Node
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool ready_to_use();

	private:
		NodeContainer* nodeContainer;

		PostProcessInputNode* inputNode;
		PostProcessOutputNode* outputNode;

		bool find_input_and_output();
	};

};
