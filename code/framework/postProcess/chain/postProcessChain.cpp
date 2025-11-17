#include "postProcessChain.h"
#include "postProcessChainElement.h"
#include "postProcessChainInputElement.h"
#include "postProcessChainOutputElement.h"
#include "postProcessChainProcessingContext.h"
#include "postProcessChainProcessElement.h"

#include "..\..\..\core\system\video\video3d.h"
#include "..\..\..\core\system\video\renderTarget.h"

#ifdef AN_DEVELOPMENT
#include "..\postProcess.h"
#include "..\postProcessInstance.h"
#include "..\..\..\core\system\input.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

PostProcessChain::PostProcessChain(PostProcessRenderTargetManager* _renderTargetManager)
: renderTargetManager(_renderTargetManager)
, isValid(false)
, requiresLinking(true)
#ifdef AN_ASSERT
, cameraUsedToRenderProvided(false)
#endif
, outputElement(nullptr)
{
	arrange_input_and_output();
}

PostProcessChain::~PostProcessChain()
{
	clear();
}

PostProcessChainProcessElement* PostProcessChain::add(PostProcess* _postProcess, Tags const & _requires, Tags const & _disallows)
{
	PostProcessChainProcessElement* element = _postProcess ? new PostProcessChainProcessElement(_postProcess, _requires, _disallows) : nullptr;
	if (element)
	{
		add(element);
	}
	return element;
}

void PostProcessChain::add(PostProcessChainElement* _element)
{
	elements.push_back(_element);
	isValid = false;
	requiresLinking = true;
}

void PostProcessChain::clear()
{
	// first disconnect, then delete all
	disconnect_elements();
	activeElements.clear();
	for_every_ptr(element, elements)
	{
		delete element;
	}
	elements.clear();
	inputElements.clear();
	outputElement = nullptr;
	isValid = false;
	requiresLinking = true;
}

void PostProcessChain::disconnect_elements()
{
	for_every_ptr(element, elements)
	{
		element->clear_connections();
	}
}

void PostProcessChain::arrange_input_and_output(int _inputCount)
{
	if (_inputCount == NONE)
	{
		_inputCount = inputElements.get_size();
	}
	_inputCount = max(1, _inputCount);

	disconnect_elements();
	inputElements.clear();
	outputElement = nullptr;

	// remove all extra inputs, store only as much as we need
	for (int index = 0; index < elements.get_size(); ++index)
	{
		if (PostProcessChainInputElement* input = fast_cast<PostProcessChainInputElement>(elements[index]))
		{
			if (inputElements.get_size() < _inputCount)
			{
				// store as many as we need
				inputElements.push_back(input);
			}
			else
			{
				// delete following ones
				delete input;
			}
			// but remove all of them from the array
			elements.remove_at(index);
			--index;
		}
	}

	// remove all outputs, except the last one
	for (int index = 0; index < elements.get_size(); ++index)
	{
		if (PostProcessChainOutputElement* output = fast_cast<PostProcessChainOutputElement>(elements[index]))
		{
			if (outputElement)
			{
				// delete previous one
				delete outputElement;
			}
			// store last one found
			outputElement = output;
			// but remove all of them from the array
			elements.remove_at(index);
			--index;
		}
	}

	// create input if not present
	if (inputElements.get_size() < _inputCount)
	{
		inputElements.push_back(new PostProcessChainInputElement());
	}
	// add input at the beginning
	int insertAt = 0;
	for_every_ptr(inputElement, inputElements)
	{
		elements.insert_at(insertAt, inputElement);
		++insertAt;
	}

	// create output if not present
	if (outputElement == nullptr)
	{
		outputElement = new PostProcessChainOutputElement();
	}
	outputElement->use_output(get_output_definition());
	// add output at the end
	elements.push_back(outputElement);

	requiresLinking = true;
}

void PostProcessChain::link()
{
	arrange_input_and_output(inputElements.get_size());

	// disconnect all elements (although arrange_input_and_output does that)
	disconnect_elements();

	// do actual linking
	isValid = true;

	// ready elements for linking and add to active elements
	activeElements.clear();
	for_every_ptr(element, elements)
	{
		if (PostProcessChainInputElement* input = fast_cast<PostProcessChainInputElement>(element))
		{
			input->clear_input_usage();
		}
		if (element->is_active())
		{
			activeElements.push_back(element);
		}
	}

	// start at the end, look by input for outputs in former elements
	// if input is not found, mark as invalid
	for_every_reverse_ptr(element, activeElements)
	{
		for_every(inputConnector, element->all_inputs())
		{
			bool connected = false;
			for_every_reverse_ptr_continue_after(prevElement, element)
			{
				for_every(outputConnector, prevElement->all_outputs())
				{
					if (inputConnector->get_name() == outputConnector->get_name())
					{
						inputConnector->connection = PostProcessChainConnection(prevElement, outputConnector->get_index());
						outputConnector->connections.push_back(PostProcessChainConnection(element, inputConnector->get_index()));
						if (PostProcessChainInputElement* input = fast_cast<PostProcessChainInputElement>(prevElement))
						{
							input->add_input_usage(outputConnector->get_index());
						}
						connected = true;
						break;
					}
				}
				if (connected)
				{
					break;
				}
			}
		}
	}

	// linking not required anymore
	requiresLinking = false;
}

void PostProcessChain::release_render_targets()
{
	for_every_ptr(element, elements)
	{
		element->release_render_targets();
	}
}

void PostProcessChain::setup_render_target_output_usage(::System::RenderTarget* _inputRenderTarget)
{
	internal_setup_render_target_output_usage(&_inputRenderTarget, nullptr, 1);
}

void PostProcessChain::setup_render_target_output_usage(Array<::System::RenderTarget*> const & _inputRenderTargets, Array<Name> const * _nameOverrides)
{
	internal_setup_render_target_output_usage(_inputRenderTargets.get_data(), _nameOverrides? _nameOverrides->get_data() : nullptr, _inputRenderTargets.get_size());
}

void PostProcessChain::internal_setup_render_target_output_usage(::System::RenderTarget*const* _pInputRenderTargets, Name const* _nameOverrides, int _count)
{
	use_as_input(_pInputRenderTargets, _nameOverrides, _count); // may mark linking as required

	if (requiresLinking)
	{
		link();
	}

	// set render target usage
	::System::RenderTarget*const* pInputRenderTarget = _pInputRenderTargets;
	for(int i = 0; i < _count; ++ i, ++pInputRenderTarget)
	{
		(*pInputRenderTarget)->mark_all_outputs_required();
	}

	int idx = 0;
	for_every_ptr(inputElement, inputElements)
	{
		if (_count == 1)
		{
			inputElement->setup_render_target_output_usage(*_pInputRenderTargets);
		}
		else
		{
			inputElement->setup_render_target_output_usage(_pInputRenderTargets[idx]);
			idx = (idx + 1) % _count;
		}
	}
}

void PostProcessChain::use_as_input(::System::RenderTarget*const* _pInputRenderTargets, Name const* _nameOverrides, int _count)
{
	if (inputElements.get_size() != _count)
	{
		arrange_input_and_output(_count);
	}
	an_assert(inputElements.get_size() > 0, TXT("should have input element created"));
	inputRenderTargets.clear();
	::System::RenderTarget*const* pInputRenderTarget = _pInputRenderTargets;
	for (int i = 0; i < _count; ++i, ++pInputRenderTarget)
	{
		inputRenderTargets.push_back(RefCountObjectPtr<System::RenderTarget>(*pInputRenderTarget));
	}
	int idx = 0;
	for_every_ptr(inputElement, inputElements)
	{
		inputElement->use_input(this, inputRenderTargets[idx].get(), _nameOverrides ? &_nameOverrides[idx] : nullptr);
		if (inputRenderTargets.get_size() != 1)
		{
			idx = (idx + 1) % _count;
		}
	}
}

void PostProcessChain::set_camera_that_was_used_to_render(::System::Camera const & _camera)
{
	cameraUsedToRender = _camera;
#ifdef AN_ASSERT
	cameraUsedToRenderProvided = true;
#endif
}

void PostProcessChain::do_post_process(::System::RenderTarget* _inputRenderTarget, ::System::Video3D* _v3d)
{
	internal_do_post_process(&_inputRenderTarget, nullptr, 1, _v3d);
}

void PostProcessChain::do_post_process(Array<::System::RenderTarget*> const & _inputRenderTargets, Array<Name> const * _nameOverrides, ::System::Video3D* _v3d)
{
	internal_do_post_process(_inputRenderTargets.get_data(), _nameOverrides ? _nameOverrides->get_data() : nullptr, _inputRenderTargets.get_size(), _v3d);
}

void PostProcessChain::internal_do_post_process(::System::RenderTarget*const* _pInputRenderTargets, Name const* _nameOverrides, int _count, ::System::Video3D* _v3d)
{
#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
	if (System::Input::get()->has_key_been_pressed(System::Key::L))
	{
		output_colour(1, 1, 1, 1);
		output(TXT("POST PROCESS CHAIN"));
		output_colour();
	}
#endif
#endif

#ifdef AN_DEVELOPMENT
	an_assert(calledForEveryShaderProgramInstance, TXT("you need to call for_every_shader_program_instance to at least setup default values for shader programs -> _shaderProgramInstance.set_uniforms_from(_shaderProgramInstance.get_shader_program()->get_default_values());"));
#endif

	use_as_input(_pInputRenderTargets, _nameOverrides, _count); // may mark linking as required

	if (requiresLinking)
	{
		link();
	}

	// ready to render
	_v3d->set_default_viewport();
	_v3d->set_near_far_plane(0.02f, 100.0f);

	_v3d->set_2d_projection_matrix_ortho();
	_v3d->access_model_view_matrix_stack().clear();

	_v3d->setup_for_2d_display();

	// setup processing context
	PostProcessChainProcessingContext processingContext;
	processingContext.video3D = _v3d;
	processingContext.renderTargetManager = renderTargetManager;
#ifdef AN_ASSERT
	an_assert(cameraUsedToRenderProvided, TXT("camera was not provided"));
#endif
	processingContext.camera = &cameraUsedToRender;

#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
	if (System::Input::get()->has_key_been_pressed(System::Key::L))
	{
		for_every_ptr(element, elements)
		{
			if (! activeElements.does_contain(element))
			{
				output_colour(0,0,0,1);
			}
			if (PostProcessChainProcessElement* ppe = fast_cast<PostProcessChainProcessElement>(element))
			{
				output(TXT("  %S"), ppe->get_post_process_instance().get_post_process()->get_name().to_string().to_char());
			}
			output_colour();
		}
	}
#endif
#endif

	for_every_ptr(element, activeElements)
	{
		element->do_post_process(processingContext);
	}

	::System::RenderTarget::unbind_to_none();

#ifdef AN_DEVELOPMENT
	calledForEveryShaderProgramInstance = true;
#endif
}

void PostProcessChain::render_output(::System::Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size)
{
	// ready to render
	PostProcessRenderTargetPointer const & outputRenderTarget = get_output_render_target();
	::System::RenderTarget* renderTarget = outputRenderTarget.get_render_target();
	int renderTargetDataTextureIndex = outputRenderTarget.get_data_texture_index();
	an_assert(renderTarget);
#ifdef AN_ASSERT
	an_assert(cameraUsedToRenderProvided, TXT("camera was not provided"));
#endif
	renderTarget->render(renderTargetDataTextureIndex, _v3d, _leftBottom, _size);
}

void PostProcessChain::define_output(::System::FragmentShaderOutputSetup const & _outputDefinition)
{
	outputDefinition = _outputDefinition;
	requiresLinking = true;
	arrange_input_and_output(inputElements.get_size()); // will redefine output
}

int PostProcessChain::get_output_render_target_count() const
{
	an_assert(outputElement, TXT("should have output element created"));
	return outputElement->get_output_render_target_count();
}

PostProcessRenderTargetPointer const & PostProcessChain::get_output_render_target(int _index) const
{
	an_assert(outputElement, TXT("should have output element created"));
	return outputElement->get_output_render_target(_index);
}

void PostProcessChain::for_every_shader_program_instance(ForShaderProgramInstance _for_every_shader_program_instance)
{
#ifdef AN_DEVELOPMENT
	calledForEveryShaderProgramInstance = true;
#endif
	for_every_ptr(element, elements)
	{
		element->for_every_shader_program_instance(_for_every_shader_program_instance);
	}
}
