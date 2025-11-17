#include "videoConfig.h"

#include "..\..\mainSettings.h"

#include "video.h"

#ifdef AN_SDL
#include "SDL.h"
#endif

#include "..\..\mainConfig.h"
#include "..\..\vr\iVR.h"

//

using namespace System;

//

VectorInt2 fit_display_layout(VectorInt2 const& _resolution, DisplayLayout::Type _displayLayout, Optional<VectorInt2> const& _hasToFitIn = NP)
{
	VectorInt2 resolution = _resolution;
	if (_displayLayout == DisplayLayout::Default)
	{
		return resolution;
	}
	if (_displayLayout == DisplayLayout::ForceHorizontal)
	{
		if (resolution.x < resolution.y)
		{
			swap(resolution.x, resolution.y);
		}
	}
	if (_displayLayout == DisplayLayout::ForceVertical)
	{
		if (resolution.x > resolution.y)
		{
			swap(resolution.x, resolution.y);
		}
	}

	if (_hasToFitIn.is_set())
	{
		if (resolution.x > _hasToFitIn.get().x)
		{
			resolution.y = (resolution.y * _hasToFitIn.get().x) / resolution.x;
			resolution.x = _hasToFitIn.get().x;
		}
		if (resolution.y > _hasToFitIn.get().y)
		{
			resolution.x = (resolution.x * _hasToFitIn.get().y) / resolution.y;
			resolution.y = _hasToFitIn.get().y;
		}
	}

	return resolution;
}

//

bool VideoConfig::s_postProcessAllowed = true;

void VideoConfig::general_set_post_process_allowed_for(VectorInt2 const& _resolution)
{
	//general_set_post_process_allowed(_resolution.x < 3000 && _resolution.y < 3000);
	general_set_post_process_allowed(true);
}

VideoConfig::VideoConfig()
{
}

#define LOAD_INT_SETTING(variable) variable = _node->get_int_from_child(TXT(#variable), variable);
#define LOAD_BOOL_SETTING(variable) variable = _node->get_bool_from_child(TXT(#variable), variable);
#define LOAD_FLOAT_SETTING(variable) variable = _node->get_float_from_child(TXT(#variable), variable);
#define LOAD_FLOAT_SETTING_AS(variable, xmlName) variable = _node->get_float_from_child(xmlName, variable);

#define IS_OK_TO_SAVE(variable) if (MainSettings::global().is_ok_to_save(STRINGIZE(PPCAT(mainConfig_videoConfig_, variable))))
#define SAVE_INT_SETTING(variable) IS_OK_TO_SAVE(variable) _node->set_int_to_child(TXT(#variable), variable);
#define SAVE_BOOL_SETTING(variable) IS_OK_TO_SAVE(variable) _node->set_bool_to_child(TXT(#variable), variable);
#define SAVE_FLOAT_SETTING(variable) IS_OK_TO_SAVE(variable) _node->set_float_to_child(TXT(#variable), variable);
#define SAVE_FLOAT_SETTING_AS(variable, xmlName) IS_OK_TO_SAVE(variable) _node->set_float_to_child(xmlName, variable);

bool VideoConfig::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;
	if (!_node)
	{
		return result;
	}
	if (_node->has_attribute_or_child(TXT("resolution")))
	{
		resolutionProvided.load_from_xml_child_node(_node, TXT("resolution"));
		resolution = VectorInt2::zero;
		resolutionFull = VectorInt2::zero;
	}
	if (auto* node = _node->first_child_named(TXT("vrResolution")))
	{
		vrResolutionCoef = node->get_float_attribute(TXT("coef"), vrResolutionCoef);
	}
	if (auto* node = _node->first_child_named(TXT("aspectRatio")))
	{
		aspectRatioCoef = node->get_float_attribute(TXT("coef"), aspectRatioCoef);
	}
	if (auto* node = _node->first_child_named(TXT("overall_vrResolution")))
	{
		overall_vrResolutionCoef = node->get_float_attribute(TXT("coef"), overall_vrResolutionCoef);
	}
	if (auto* node = _node->first_child_named(TXT("overall_aspectRatio")))
	{
		overall_aspectRatioCoef = node->get_float_attribute(TXT("coef"), overall_aspectRatioCoef);
	}
#ifdef FORCE_RENDERING_LOW
	vrResolutionCoef = min(0.9f, vrResolutionCoef);
#endif
#ifdef FORCE_RENDERING_ULTRA_LOW
	vrResolutionCoef = min(0.7f, vrResolutionCoef);
#endif
	LOAD_INT_SETTING(displayIndex);
	LOAD_BOOL_SETTING(maintainDesktopAspectRatioForWindowScaled);
	LOAD_BOOL_SETTING(autoSetupForVR);
	LOAD_BOOL_SETTING(directlyToVR);
	LOAD_FLOAT_SETTING_AS(vrFoveatedRenderingLevel, TXT("vrFoveatedRenderingLevelPct"));
	LOAD_INT_SETTING(vrFoveatedRenderingMaxDepth);
	LOAD_BOOL_SETTING(skipDisplayBufferForVR);
	LOAD_BOOL_SETTING(allowAutoAdjustForVR);
	if (_node->has_attribute_or_child(TXT("displayLayout")))
	{
		displayLayout = DisplayLayout::parse(_node->get_string_from_child(TXT("displayLayout")).to_char());
	}
	if (_node->has_attribute_or_child(TXT("fullScreen")))
	{
		fullScreen = FullScreen::parse(_node->get_string_from_child(TXT("fullScreen")).to_char());
	}
	LOAD_BOOL_SETTING(vsync);
	LOAD_BOOL_SETTING(msaa);
	LOAD_BOOL_SETTING(msaaUI);
	LOAD_BOOL_SETTING(msaaVR);
	LOAD_INT_SETTING(msaaSamples);
	LOAD_BOOL_SETTING(fxaa);
	if (_node->has_attribute_or_child(TXT("setOptions")))
	{
		options.clear();
		options.load_from_xml_attribute_or_child_node(_node, TXT("setOptions"));
	}
	options.load_from_xml_attribute_or_child_node(_node, TXT("options"));
	for_every(contnode, _node->children_named(TXT("useVec4ArraysInsteadOfMat4Arrays")))
	{
		for_every(node, contnode->children_named(TXT("for")))
		{
			String forRenderer = node->get_string_attribute(TXT("renderer"));
			if (!forRenderer.is_empty())
			{
				useVec4ArraysInsteadOfMat4ArraysForRenderers.push_back_unique(forRenderer);
			}
		}
	}
	for_every(contnode, _node->children_named(TXT("limitedSupportForIndexingInShaders")))
	{
		for_every(node, contnode->children_named(TXT("for")))
		{
			String forRenderer = node->get_string_attribute(TXT("renderer"));
			if (!forRenderer.is_empty())
			{
				limitedSupportForIndexingInShadersForRenderers.push_back_unique(forRenderer);
			}
		}
	}
	forcedBlackColour.load_from_xml(_node, TXT("forcedBlackColour"));
	return result;
}

void VideoConfig::save_to_xml(IO::XML::Node * _node, bool _userConfig) const
{
	SAVE_INT_SETTING(displayIndex);
	SAVE_BOOL_SETTING(maintainDesktopAspectRatioForWindowScaled);
	SAVE_BOOL_SETTING(autoSetupForVR);
	SAVE_BOOL_SETTING(directlyToVR);
	SAVE_FLOAT_SETTING_AS(vrFoveatedRenderingLevel, TXT("vrFoveatedRenderingLevelPct"));
	SAVE_INT_SETTING(vrFoveatedRenderingMaxDepth);
	SAVE_BOOL_SETTING(skipDisplayBufferForVR);
	_node->set_string_to_child(TXT("fullScreen"), FullScreen::to_char(fullScreen));
	_node->set_string_to_child(TXT("displayLayout"), DisplayLayout::to_char(displayLayout));
	IS_OK_TO_SAVE(resolutionProvided)
	{
		if (!resolutionProvided.is_zero())
		{
			resolutionProvided.save_to_xml(_node->add_node(TXT("resolution")));
		}
	}
	IS_OK_TO_SAVE(vrResolution)
	{
		if (vrResolutionCoef != 1.0f) // default
		{
			auto* node = _node->add_node(TXT("vrResolution"));
			node->set_float_attribute(TXT("coef"), vrResolutionCoef);
		}
		if (aspectRatioCoef != 1.0f) // default
		{
			auto* node = _node->add_node(TXT("aspectRatio"));
			node->set_float_attribute(TXT("coef"), aspectRatioCoef);
		}
	}
	SAVE_BOOL_SETTING(allowAutoAdjustForVR);
	SAVE_BOOL_SETTING(vsync);
	SAVE_BOOL_SETTING(msaa);
	SAVE_BOOL_SETTING(msaaUI);
	SAVE_BOOL_SETTING(msaaVR);
	SAVE_INT_SETTING(msaaSamples);
	SAVE_BOOL_SETTING(fxaa);
	IS_OK_TO_SAVE(options)
	{
		_node->set_string_to_child(TXT("options"), options.to_string_with_minus());
	}
	if (!_userConfig)
	{
		if (!useVec4ArraysInsteadOfMat4ArraysForRenderers.is_empty())
		{
			auto* contnode = _node->add_node(TXT("useVec4ArraysInsteadOfMat4Arrays"));
			for_every(forRenderer, useVec4ArraysInsteadOfMat4ArraysForRenderers)
			{
				auto* node = contnode->add_node(TXT("for"));
				node->set_attribute(TXT("renderer"), *forRenderer);
			}
		}
		if (!limitedSupportForIndexingInShadersForRenderers.is_empty())
		{
			auto* contnode = _node->add_node(TXT("limitedSupportForIndexingInShaders"));
			for_every(forRenderer, limitedSupportForIndexingInShadersForRenderers)
			{
				auto* node = contnode->add_node(TXT("for"));
				node->set_attribute(TXT("renderer"), *forRenderer);
			}
		}
		if (forcedBlackColour.is_set())
		{
			forcedBlackColour.get().save_to_xml_child_node(_node, TXT("forcedBlackColour"));
		}
	}
	todo_note(TXT("as above!"));
}

void VideoConfig::fill_missing()
{
	if (resolution.is_zero())
	{
		resolution = resolutionProvided;
	}
	VectorInt2 desktopSize = Video::get_desktop_size(displayIndex);
	if (!desktopSize.is_zero())
	{
		if (fullScreen == FullScreen::WindowFull)
		{
			resolution = desktopSize;
			resolutionFull = desktopSize;
			// ignore display layout
		}
		else if (fullScreen == FullScreen::WindowScaled)
		{
			resolutionFull = desktopSize;
			if (resolution.is_zero())
			{
				resolution = desktopSize;
			}
			else
			{
				if (resolution.y > desktopSize.y)
				{
					resolution.x = resolution.x * desktopSize.y / resolution.y;
					resolution.y = desktopSize.y;
				}
				if (resolution.x > desktopSize.x)
				{
					resolution.y = resolution.y * desktopSize.x / resolution.x;
					resolution.x = desktopSize.x;
				}
			}
			if (maintainDesktopAspectRatioForWindowScaled)
			{
				use_desktop_aspect_ratio_for_resolution();
			}
			resolution = fit_display_layout(resolution, displayLayout, desktopSize);
			if (resolution == desktopSize)
			{
				fullScreen = FullScreen::WindowFull;
			}
		}
		else
		{
			if (resolution.is_zero())
			{
				if (fullScreen == FullScreen::Yes)
				{
					resolution = desktopSize;
				}
				else
				{
#ifdef AN_DEVELOPMENT
					// 1/4 of the screen space
					resolution = desktopSize * 6 / 10;
#else
					resolution = desktopSize * 4 / 5;
#endif
				}
			}
			resolution = fit_display_layout(resolution, displayLayout, desktopSize);
		}
	}

	if (autoSetupForVR &&
		resolutionProvided.is_zero() &&
		VR::IVR::get() != nullptr)
	{
		resolution = VectorInt2::zero;
		if (fullScreen != FullScreen::No)
		{
			// if we want fullscreen make it always be window scaled one
			fullScreen = FullScreen::WindowScaled;
		}

		if (VR::IVR::get() && resolution.is_zero())
		{
			VectorInt2 preferredFullScreenResolution = VR::IVR::get()->get_preferred_full_screen_resolution();
			if (!preferredFullScreenResolution.is_zero())
			{
				resolution = preferredFullScreenResolution;
			}
		}

		if (resolution.is_zero())
		{
			// vive
			resolution.x = 2160;
			resolution.y = 1200;
		}

		// if not forced (dev) use smaller, we should avoid big resolutions
		// should we? will it be such a drag?
		if (!MainConfig::access_global().should_force_vr())
		{
			//resolution.x = resolution.x * 2 / 3;
			//resolution.y = resolution.y * 2 / 3;
		}
		
		// maintain aspect ratio, we want nice window
		use_desktop_aspect_ratio_for_resolution();
	}
}

void VideoConfig::use_desktop_aspect_ratio_for_resolution()
{
	VectorInt2 desktopSize = Video::get_desktop_size(displayIndex);
	if (desktopSize.is_zero())
	{
		return;
	}

	float desktopAspect = (float)desktopSize.x / (float)desktopSize.y;
	float resolutionAspect = (float)resolution.x / (float)resolution.y;

	if (resolutionAspect > desktopAspect * 0.7f)
	{
		resolution.x = (int)(desktopAspect * (float)resolution.y);
	}
	else
	{
		resolution.y = (int)((float)resolution.x / desktopAspect);
	}

	if (fullScreen != FullScreen::No)
	{
		// limit to desktop
		if (resolution.y > desktopSize.y)
		{
			resolution.x = (int)(desktopAspect * (float)resolution.y);
		}
		if (resolution.x > desktopSize.x)
		{
			resolution.y = (int)((float)resolution.x / desktopAspect);
		}
	}
	else
	{
		resolution.x = min(resolution.x, desktopSize.x * 7 / 8);
		resolution.y = min(resolution.y, desktopSize.y * 7 / 8);
	}
}

bool VideoConfig::is_option_set(Name const & _option) const
{
	if (_option == TXT("msaa")) return does_use_msaa();
	if (_option == TXT("fxaa")) return fxaa;
	if (_option == TXT("vsync")) return vsync;
	if (options.get_tag(_option)) return true;

	return false;
}

bool VideoConfig::should_use_vec4_arrays_instead_of_mat4_arrays(String const& _renderer) const
{
	return useVec4ArraysInsteadOfMat4ArraysForRenderers.does_contain(_renderer);
}

bool VideoConfig::has_limited_support_for_indexing_in_shaders(String const& _renderer) const
{
	return useVec4ArraysInsteadOfMat4ArraysForRenderers.does_contain(_renderer);
}

#undef LOAD_INT_SETTING
#undef LOAD_BOOL_SETTING
#undef LOAD_FLOAT_SETTING
#undef LOAD_FLOAT_SETTING_AS

#undef SAVE_INT_SETTING
#undef SAVE_BOOL_SETTING
#undef SAVE_FLOAT_SETTING
#undef SAVE_FLOAT_SETTING_AS

