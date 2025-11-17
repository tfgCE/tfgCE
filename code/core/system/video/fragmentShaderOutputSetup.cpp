#include "fragmentShaderOutputSetup.h"

#include "renderTarget.h"

using namespace ::System;

//

bool FragmentShaderOutputSetup::load_from_xml(IO::XML::Node const * _node, OutputTextureDefinition const & _defaultOutputTextureDefinition)
{
	bool result = base::load_from_xml(_node, _defaultOutputTextureDefinition);

	allowsDefaultOutput = _node->get_bool_attribute_or_from_child_presence(TXT("allowsDefaultOutput"), allowsDefaultOutput);

	if (outputs.is_empty() && !allowsDefaultOutput)
	{
		warn_loading_xml(_node, TXT("no outputs defined for fragment shader nor \"allowsDefaultOutput\" node provided : <outputs allowsDefaultOutput=\"true\"/>"));
	}

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("sizeMultiplier")))
	{
		float commonSizeMultiplier = attr->get_as_float();
		if (commonSizeMultiplier > 0.0f)
		{
			sizeMultiplier.x = commonSizeMultiplier;
			sizeMultiplier.y = commonSizeMultiplier;
		}
		else
		{
			error_loading_xml(_node, TXT("invalid value for sizeMultiplier"));
			result = false;
		}
	}

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("sizeMultiplierX")))
	{
		float sizeMultiplierX = attr->get_as_float();
		if (sizeMultiplierX > 0.0f)
		{
			sizeMultiplier.x = sizeMultiplierX;
		}
		else
		{
			error_loading_xml(_node, TXT("invalid value for sizeMultiplierX"));
			result = false;
		}
	}

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("sizeMultiplierY")))
	{
		float sizeMultiplierY = attr->get_as_float();
		if (sizeMultiplierY > 0.0f)
		{
			sizeMultiplier.y = sizeMultiplierY;
		}
		else
		{
			error_loading_xml(_node, TXT("invalid value for sizeMultiplierY"));
			result = false;
		}
	}

	return result;
}

RenderTarget* FragmentShaderOutputSetup::create_render_target(SIZE_ VectorInt2 _size) const
{
	RenderTargetSetup setup(SIZE_ _size * sizeMultiplier);
	for_every(output, outputs)
	{
		setup.add_output_texture(OutputTextureDefinition(output->get_name(), output->get_video_format(), output->get_base_video_format()));
	}

	RENDER_TARGET_CREATE_INFO(TXT("fragment shader output"));
	return new RenderTarget(setup);
}

void FragmentShaderOutputSetup::have_at_least_one_default_output(OutputTextureDefinition const & _default)
{
	if (get_output_texture_count() == 0)
	{
		add_output_texture(_default);
	}
}
