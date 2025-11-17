#include "postProcessProcessorNode.h"

#include "..\..\..\core\mainSettings.h"
#include "..\..\..\core\graph\graphImplementation.h"
#include "..\..\..\core\types\names.h"
#include "..\..\..\core\system\core.h"
#include "..\..\..\core\system\video\shader.h"
#include "..\..\..\core\system\video\fragmentShaderOutputSetup.h"
#include "..\..\..\core\system\video\renderTarget.h"
#include "..\..\..\core\system\video\camera.h"

#include "..\..\library\library.h"

#include "postProcessInputNode.h"
#include "postProcessGraphProcessingContext.h"
#include "..\postProcessRenderTargetManager.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(PostProcessProcessorNode);

PostProcessProcessorNode::PostProcessProcessorNode()
: distanceFromInput(0)
, sizeMultiplierRelativeToPrev(Vector2::one)
, sizeMultiplierLoopGuard(false)
, sizeMultiplier(1.0f)
, processIfInputSizeAtLeast(0.0f)
, loadData(nullptr)
{
}

PostProcessProcessorNode::~PostProcessProcessorNode()
{
	delete_and_clear(loadData);
}

bool PostProcessProcessorNode::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	if (!loadData)
	{
		loadData = new LoadData();
	}

	loadData->shaderProgram.load_from_xml(_node, TXT("shaderProgram"), _lc);

	activationParam = _node->get_name_attribute(TXT("activationParam"), activationParam);
	systemTagRequired.load_from_xml_attribute(_node, TXT("systemTagRequired"));

	useProvidedResolution = _node->get_name_attribute(TXT("useProvidedResolution"), useProvidedResolution);
	sizeMultiplier = _node->get_float_attribute(TXT("sizeMultiplier"), sizeMultiplier);
	processIfInputSizeAtLeast = _node->get_float_attribute(TXT("processIfInputSizeAtLeast"), processIfInputSizeAtLeast);

	for_every(node, _node->children_named(TXT("shaderParams")))
	{
		result &= shaderParams.load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("map")))
	{
		Name shaderOutput = node->get_name_attribute(TXT("shaderOutput"));
		Name outputConnector = node->get_name_attribute(TXT("asOutputConnector"));
		if (shaderOutput.is_valid() && outputConnector.is_valid())
		{
			mapShaderOutputs[shaderOutput] = outputConnector;
		}
		else
		{
			error_loading_xml(node, TXT("shaderOutput and asOutputConnector have to be defined"));
			result = false;
		}
	}

	return result;
}

bool PostProcessProcessorNode::on_input_loaded_from_xml(int _inputIndex, IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::on_input_loaded_from_xml(_inputIndex, _node, _lc);

	// make sure there is such input data
	additionalInputData.set_size(max(_inputIndex + 1, additionalInputData.get_size()));
	
	result &= additionalInputData[_inputIndex].load_from_xml(_node, _lc);

	return result;
}

bool PostProcessProcessorNode::for_every_shader_program_instance(InstanceData & _instanceData, ForShaderProgramInstance _for_every_shader_program_instance) const
{
	bool result = base::for_every_shader_program_instance(_instanceData, _for_every_shader_program_instance);

	result &= _for_every_shader_program_instance(this, _instanceData[iShaderProgramInstance]);
	
	return result;
}

void PostProcessProcessorNode::manage_activation_using(InstanceData & _instanceData, SimpleVariableStorage const & _storage) const
{
	base::manage_activation_using(_instanceData, _storage);

	if (activationParam.is_valid() || !systemTagRequired.is_empty())
	{
		bool isActive = true;

		if (isActive && activationParam.is_valid())
		{
			TypeId id;
			void const* value;
			if (_storage.get_existing_of_any_type_id(activationParam, OUT_ id, value))
			{
				if (id == type_id<bool>())
				{
					isActive = *((bool*)value);
				}
				if (id == type_id<float>())
				{
					isActive &= *((float*)value) != 0.0f;
				}
				if (id == type_id<int>())
				{
					isActive &= *((int*)value) != 0;
				}
			}
		}

		if (isActive && !systemTagRequired.is_empty())
		{
			if (!systemTagRequired.check(::System::Core::get_system_tags()))
			{
				isActive = false;
			}
		}

		_instanceData[iIsActive] = isActive;
	}
}

bool PostProcessProcessorNode::load_shader_program(Library* _library)
{
	bool result = base::load_shader_program(_library);

	// clear everything
	clear_output_connectors();
	shaderProgram = nullptr;
	fragmentShaderOutputSetup = ::System::FragmentShaderOutputSetup();

	if (loadData)
	{
		// setup shader program instance
		ShaderProgram* shaderProgramToLoad = _library->get_shader_programs().find(loadData->shaderProgram);
		if (shaderProgramToLoad)
		{
			shaderProgram = shaderProgramToLoad->get_shader_program();

			// create outputs based on fragment shader definition (output setup)
			if (::System::ShaderProgram const * actualShaderProgram = shaderProgramToLoad->get_shader_program())
			{
				if (::System::Shader const * fragmentShader = actualShaderProgram->get_fragment_shader())
				{
					// copy fragment shader output setup
					fragmentShaderOutputSetup = fragmentShader->get_fragment_shader_output_setup();
					int outputTextureCount = fragmentShaderOutputSetup.get_output_texture_count();
					for_count(int, outputTextureIndex, outputTextureCount)
					{
						::System::OutputTextureDefinition const & outputTextureDefinition = fragmentShaderOutputSetup.get_output_texture_definition(outputTextureIndex);
						// remap outputs, we use index anyway
						if (auto * mapToConnector = mapShaderOutputs.get_existing(outputTextureDefinition.get_name()))
						{
							add_output_connector(OutputConnector(*mapToConnector).temp_ref());
						}
						else
						{
							add_output_connector(OutputConnector(outputTextureDefinition.get_name()).temp_ref());
						}
					}
				}
			}
		}
		else if (loadData->shaderProgram.is_valid())
		{
			error(TXT("could not find shader program \"%S\" for post process node"), loadData->shaderProgram.to_string().to_char());
			result = false;
		}
		// delete loading data
		delete_and_clear(loadData);
	}
	return result;
}

bool PostProcessProcessorNode::calculate_size_multiplier()
{
	if (sizeMultiplierLoopGuard)
	{
		error(TXT("post process processor looped"));
		return false;
	}
	bool result = true;
	sizeMultiplierRelativeToPrev = Vector2::zero;
	sizeMultiplierLoopGuard = true;
	distanceFromInput = 0;
	// find maximum size of input - we don't want to go any smaller unless shader defines it should be smaller
	for_every(inputConnector, all_input_connectors())
	{
		if (PostProcessInputNode* inputNode = fast_cast<PostProcessInputNode>(inputConnector->get_connection().node))
		{
			if (distanceFromInput <= 0)
			{
				distanceFromInput = 1;
				sizeMultiplierRelativeToPrev.x = max(sizeMultiplierRelativeToPrev.x, fragmentShaderOutputSetup.get_size_multiplier().x);
				sizeMultiplierRelativeToPrev.y = max(sizeMultiplierRelativeToPrev.y, fragmentShaderOutputSetup.get_size_multiplier().y);
			}
		}
		if (PostProcessProcessorNode* inputProcessor = fast_cast<PostProcessProcessorNode>(inputConnector->get_connection().node))
		{
			result &= inputProcessor->calculate_size_multiplier();
			if (distanceFromInput <= inputProcessor->distanceFromInput)
			{
				distanceFromInput = inputProcessor->distanceFromInput + 1;
				sizeMultiplierRelativeToPrev.x = max(sizeMultiplierRelativeToPrev.x, fragmentShaderOutputSetup.get_size_multiplier().x);
				sizeMultiplierRelativeToPrev.y = max(sizeMultiplierRelativeToPrev.y, fragmentShaderOutputSetup.get_size_multiplier().y);
			}
		}
	}
	if (sizeMultiplierRelativeToPrev.x == 0.0f)
	{
		sizeMultiplierRelativeToPrev.x = 1.0f;
	}
	if (sizeMultiplierRelativeToPrev.y == 0.0f)
	{
		sizeMultiplierRelativeToPrev.y = 1.0f;
	}
	sizeMultiplierLoopGuard = false;
	return result;
}

void PostProcessProcessorNode::handle_instance_data(REF_ InstanceDataHandler const & _dataHandler) const
{
	base::handle_instance_data(REF_ _dataHandler);

	_dataHandler.handle(iRenderTarget);
	_dataHandler.handle(iInputRenderTargets);
	_dataHandler.handle(iOutputRenderTargets);
	_dataHandler.handle(iShaderProgramInstance);
	_dataHandler.handle(iIsActive);
}

void PostProcessProcessorNode::initialise(InstanceData & _instanceData) const
{
	base::initialise(_instanceData);

	_instanceData[iRenderTarget] = nullptr;
	_instanceData[iInputRenderTargets].set_size(all_input_connectors().get_size(), true);
	_instanceData[iOutputRenderTargets].set_size(all_output_connectors().get_size(), true);
	_instanceData[iShaderProgramInstance].set_shader_program(shaderProgram);
	_instanceData[iShaderProgramInstance].set_uniforms_from(shaderParams);
	_instanceData[iIsActive] = true;
}

void PostProcessProcessorNode::release_render_targets(InstanceData & _instanceData) const
{
	PostProcessRenderTarget*& renderTarget = _instanceData[iRenderTarget];
	if (renderTarget)
	{
		renderTarget->release_usage();
		renderTarget = nullptr;
	}
	for_every(inputRenderTarget, _instanceData[iInputRenderTargets])
	{
		inputRenderTarget->clear();
	}
	for_every(outputRenderTarget, _instanceData[iOutputRenderTargets])
	{
		outputRenderTarget->clear();
	}
}

void PostProcessProcessorNode::do_post_process(InstanceData & _instanceData, PostProcessGraphProcessingContext const& _processingContext) const
{
	if (was_post_processing_done(_instanceData))
	{
		// already processed
		return;
	}

	an_assert(_processingContext.video3D);

	base::do_post_process(_instanceData, _processingContext.for_previous());

	// at this point inputs (processed through base call) should propagate outputs and we should have them stored in our "inputRenderTargets" array

	// get shader program
	::System::ShaderProgramInstance & shaderProgramInstance = _instanceData[iShaderProgramInstance];
	::System::ShaderProgram* shaderProgram = shaderProgramInstance.get_shader_program();

	// get render target (basing on shader program's fragment shader's setup and processing context's size)
	PostProcessRenderTarget*& renderTarget = _instanceData[iRenderTarget];

	VectorInt2 inputSize = VectorInt2::zero;

	{	// get valid input size
		int index = 0;
		Array<PostProcessRenderTargetPointer>& inputRenderTargets = _instanceData[iInputRenderTargets];
		for_every(inputConnector, all_input_connectors())
		{
			an_assert(additionalInputData.is_index_valid(inputConnector->get_connector_index()));
			auto& inputRenderTarget = inputRenderTargets[index];
			if (auto* rt = inputRenderTarget.get_render_target())
			{
				VectorInt2 rtSize = rt->get_size();
				inputSize.x = max(inputSize.x, rtSize.x);
				inputSize.y = max(inputSize.y, rtSize.y);
			}
			++index;
		}
	}


	bool isActive = _instanceData[iIsActive];
	if (processIfInputSizeAtLeast > 0.0f &&
		min(inputSize.x, inputSize.y) < processIfInputSizeAtLeast)
	{
		isActive = false;
	}
	if (isActive)
	{
		an_assert(renderTarget == nullptr);
		an_assert(_processingContext.renderTargetManager);
		an_assert(inputSize != VectorInt2::zero);
		VectorInt2 requestedSize = inputSize;
		float requestedScale = 1.0f;
		bool withMipMaps = _processingContext.withMipMaps;
		if (_processingContext.resolutionProvider && useProvidedResolution.is_valid())
		{
			VectorInt2 providedSize = _processingContext.resolutionProvider->get_resolution_for_processing(useProvidedResolution);
			if (providedSize.x != 0)
			{
				requestedSize.x = providedSize.x;
			}
			if (providedSize.y != 0)
			{
				requestedSize.y = providedSize.y;
			}
		}
		requestedSize *= (sizeMultiplierRelativeToPrev * sizeMultiplier);
		renderTarget = _processingContext.renderTargetManager->get_available_render_target_for(requestedSize, requestedScale, withMipMaps, shaderProgram->get_fragment_shader()->get_fragment_shader_output_setup());

		// setup outputs to use renderTarget
		Array<PostProcessRenderTargetPointer>& outputRenderTargets = _instanceData[iOutputRenderTargets];
		{
			int outputRenderTargetIdx = 0;
			for_every(outputRenderTarget, outputRenderTargets)
			{
				outputRenderTarget->set(renderTarget, outputRenderTargetIdx);
				++outputRenderTargetIdx;
			}
		}

		// we will be rendering to our render target
		renderTarget->get_render_target()->bind();

		// setup viewport - before binding shader program
		// we need to render in space of our render target
		_processingContext.video3D->set_viewport(VectorInt2::zero, renderTarget->get_render_target()->get_size());
		_processingContext.video3D->set_2d_projection_matrix_ortho();

		{	// set filtering for input textures
			int index = 0;
			Array<PostProcessRenderTargetPointer>& inputRenderTargets = _instanceData[iInputRenderTargets];
			for_every(inputConnector, all_input_connectors())
			{
				an_assert(additionalInputData.is_index_valid(inputConnector->get_connector_index()));
				auto& inputRenderTarget = inputRenderTargets[index];
				if (auto* rt = inputRenderTarget.get_render_target())
				{
					AdditionalInputData const & aid = additionalInputData[inputConnector->get_connector_index()];
					rt->change_filtering(inputRenderTarget.get_data_texture_index(), aid.inputFilteringMag.get(::System::TextureFiltering::linearMipMapLinear), aid.inputFilteringMin.get(::System::TextureFiltering::linearMipMapLinear));
				}
				++index;
			}
		}

		// bind shader program (directly - do everything that binding of shaderProgramInstance does and we may need)
		shaderProgram->bind();
		shaderProgram->apply(shaderProgramInstance);
		if (_processingContext.camera != nullptr)
		{
			// bind projection matrix
			int projectionMatrix3DuniformIndex = shaderProgram->find_uniform_index(Names::projectionMatrix3D);
			if (projectionMatrix3DuniformIndex != NONE)
			{
#ifdef AN_ASSERT
				an_assert(_processingContext.camera->has_projection_matrix_calculated(), TXT("maybe camera was not updated after rendering?"));
#endif

				shaderProgram->set_uniform(projectionMatrix3DuniformIndex, _processingContext.camera->get_projection_matrix());
			}
		}
		// and set colour (if present) to white
		todo_note(TXT("this might be not needed anymore"));
		if (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes())
		{
			shaderProgram->bind_vertex_attrib(_processingContext.video3D, shaderProgram->get_in_colour_attribute_index(), Colour::white);
		}

		// bind textures from inputs - do it directly in shader program - we don't care about storing that in instance
		int index = 0;
		Array<PostProcessRenderTargetPointer>& inputRenderTargets = _instanceData[iInputRenderTargets];
		for_every(inputConnector, all_input_connectors())
		{
			shaderProgram->set_uniform(inputConnector->get_name(), inputRenderTargets[index].get_texture_id());
			++index;
		}

		// render now! use render target's VBO to render on that render target
		Vector2 renderTargetPureSize = renderTarget->get_render_target()->get_size(true).to_vector2();

		// set build-in params
		if (inputRenderTargets.is_index_valid(0))
		{
			::System::RenderTarget* inRT0 = inputRenderTargets[0].get_render_target();
			an_assert(inRT0 != nullptr);
			shaderProgram->set_build_in_uniform_in_texture_size_related_uniforms(inRT0->get_size().to_vector2(), renderTargetPureSize);
		}

		if (_processingContext.camera != nullptr)
		{
			renderTarget->get_render_target()->render_for_shader_program(_processingContext.video3D, Vector2::zero, renderTarget->get_render_target()->get_size().to_vector2(), NP, *_processingContext.camera, shaderProgram);
		}
		else
		{
			renderTarget->get_render_target()->render_for_shader_program(_processingContext.video3D, Vector2::zero, renderTarget->get_render_target()->get_size().to_vector2(), NP, shaderProgram);
		}

		// unbind shader program and render target
		shaderProgramInstance.unbind();
		renderTarget->get_render_target()->unbind();
	}
	else
	{
		// very simple pass through, all or just the first one
		Array<PostProcessRenderTargetPointer>& inputRenderTargets = _instanceData[iInputRenderTargets];
		Array<PostProcessRenderTargetPointer>& outputRenderTargets = _instanceData[iOutputRenderTargets];
		for_count(int, index, min(inputRenderTargets.get_size(), outputRenderTargets.get_size()))
		{
			outputRenderTargets[index].set(inputRenderTargets[index]);
		}
		an_assert(inputRenderTargets.get_size() == outputRenderTargets.get_size() || outputRenderTargets.get_size() == 1, TXT("for %S, input: %i output: %i"), get_name().to_char(), inputRenderTargets.get_size(), outputRenderTargets.get_size()); // handle other cases?
	}

	// we have post processed outputs now, propagate them
	propagate_outputs(_instanceData);

	if (isActive)
	{
		// we no longer need render target, release them
		renderTarget->release_usage();
		renderTarget = nullptr;
	}

	// we no longer need inputs too, release them
	for_every(inputRenderTarget, _instanceData[iInputRenderTargets])
	{
		inputRenderTarget->clear();
	}
}

void PostProcessProcessorNode::gather_input(InstanceData & _instanceData, int _inputIndex, PostProcessRenderTargetPointer const & _renderTargetPointer) const
{
	_instanceData[iInputRenderTargets][_inputIndex] = _renderTargetPointer;
}

PostProcessRenderTargetPointer const & PostProcessProcessorNode::get_output(InstanceData & _instanceData, int _outputIndex) const
{
	return _instanceData[iOutputRenderTargets][_outputIndex];
}
