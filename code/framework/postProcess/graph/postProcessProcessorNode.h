#pragma once

#include "..\..\..\core\instance\instanceDatum.h"
#include "..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\core\system\video\fragmentShaderOutputSetup.h"
#include "..\..\..\core\system\video\shaderProgramInstance.h"
#include "..\..\..\core\system\video\videoEnums.h"

#include "..\postProcessRenderTargetPointer.h"

#include "postProcessNode.h"

namespace Framework
{
	/**
	 *	Processor holds shader and is responsible for doing actual post processing.
	 *
	 *	Inputs are defined as usual, outputs are taken from shader definition in "load_shader_program".
	 *	Outputs have same names and are in same order as in shader definition.
	 *
	 *	After nodes are connected (and long after shader is loaded) size multiplier is calculated - 1.0 is exactly as input/output render target used with post process chain.
	 *	Rule of thumb is to have output with size multiplier 1.0 but if can be bigger or smaller (if user knows what they're doing).
	 *
	 */
	class PostProcessProcessorNode
	: public PostProcessNode
	{
		FAST_CAST_DECLARE(PostProcessProcessorNode);
		FAST_CAST_BASE(PostProcessNode)
		FAST_CAST_END();

		typedef PostProcessNode base;
	public:
		PostProcessProcessorNode();
		virtual ~PostProcessProcessorNode();

		bool calculate_size_multiplier();

	public: // PostProcessNode
		override_ bool load_shader_program(Library* _library);

		override_ void initialise(InstanceData & _instanceData) const;
		override_ void release_render_targets(InstanceData & _instanceData) const;
		override_ void do_post_process(InstanceData & _instanceData, PostProcessGraphProcessingContext const& _processingContext) const;
		override_ bool for_every_shader_program_instance(InstanceData & _instanceData, ForShaderProgramInstance _for_every_shader_program_instance) const;
		override_ void manage_activation_using(InstanceData & _instanceData, SimpleVariableStorage const & _storage) const;

		override_ void gather_input(InstanceData & _instanceData, int _inputIndex, PostProcessRenderTargetPointer const & _renderTargetPointer) const;
		override_ PostProcessRenderTargetPointer const & get_output(InstanceData & _instanceData, int _outputIndex) const;

	public: // ::Graph::Node
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool on_input_loaded_from_xml(int _inputIndex, IO::XML::Node const * _node, LibraryLoadingContext & _lc);

		override_ void handle_instance_data(REF_ InstanceDataHandler const & _dataHandler) const;

	private:
		::System::FragmentShaderOutputSetup fragmentShaderOutputSetup;
		::System::ShaderProgram* shaderProgram;
		int distanceFromInput; // used to calculate from furthest
		Vector2 sizeMultiplierRelativeToPrev; // to previous, not to main, we want to remain relative
		bool sizeMultiplierLoopGuard;
		Name useProvidedResolution;
		float sizeMultiplier; // provided by user in xml
		float processIfInputSizeAtLeast; // provided by user in xml, lower of two (x, y)
		Array<AdditionalInputData> additionalInputData;
		SimpleVariableStorage shaderParams;
		Map<Name, Name> mapShaderOutputs; // map actual shader output (key) to graph's connector (value)
		// conditions to make it active
		// if not active, it should have either matching number of inputs and outputs or just a single output where first input will be passed through
		Name activationParam;
		TagCondition systemTagRequired;

		// instance data
		::Instance::Datum<PostProcessRenderTarget*> iRenderTarget;
		::Instance::Datum<Array<PostProcessRenderTargetPointer> > iInputRenderTargets;
		::Instance::Datum<Array<PostProcessRenderTargetPointer> > iOutputRenderTargets;
		::Instance::Datum<::System::ShaderProgramInstance> iShaderProgramInstance;
		::Instance::Datum<bool> iIsActive;

	private:
		struct LoadData
		{
			LibraryName shaderProgram;
		};
		LoadData* loadData;
	};

};
