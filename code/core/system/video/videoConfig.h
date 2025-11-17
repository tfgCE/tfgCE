#pragma once

#include "..\..\math\math.h"
#include "..\..\tags\tag.h"

namespace System
{
	namespace FullScreen
	{
		enum Type
		{
			No,
			Yes,
			WindowScaled, // window as big as screen but render to smaller render target and upscale
			WindowFull, // window as big as screen and rendering as whole - this is special case of window scaled when resolution is equal to desktop
		};

		inline Type parse(tchar const * _text)
		{
			if (String::compare_icase(_text, TXT("no")) ||
				String::compare_icase(_text, TXT("false"))) return No;
			if (String::compare_icase(_text, TXT("yes")) ||
				String::compare_icase(_text, TXT("true"))) return Yes;
			if (String::compare_icase(_text, TXT("windowScaled")) ||
				String::compare_icase(_text, TXT("windowed"))) return WindowScaled;
			if (String::compare_icase(_text, TXT("windowFull"))) return WindowFull;
			return WindowFull; // default
		}

		inline tchar const * to_char(Type _type)
		{
			if (_type == No) return TXT("false");
			if (_type == Yes) return TXT("true");
			return TXT("windowed"); // default
		}
	};

	namespace DisplayLayout
	{
		enum Type
		{
			Default,
			ForceHorizontal,
			ForceVertical,
		};

		inline Type parse(tchar const * _text)
		{
			if (String::compare_icase(_text, TXT("forceHorizontal"))) return ForceHorizontal;
			if (String::compare_icase(_text, TXT("forceVertical"))) return ForceVertical;
			return Default; // default
		}

		inline tchar const * to_char(Type _type)
		{
			if (_type == ForceHorizontal) return TXT("forceHorizontal");
			if (_type == ForceVertical) return TXT("forceVertical");
			return TXT("default"); // default
		}
	};

	struct VideoConfig
	{
	private:
		static bool s_postProcessAllowed;

	public:
		static bool general_is_post_process_allowed() { return s_postProcessAllowed; }
		static void general_set_post_process_allowed(bool _allowed) { s_postProcessAllowed = _allowed; }
		static void general_set_post_process_allowed_for(VectorInt2 const& _resolution);

	public:
		int displayIndex = 0;
		bool maintainDesktopAspectRatioForWindowScaled = true;
		bool autoSetupForVR = true; // if no other parameters provided, will choose the best for auto setup
		bool directlyToVR = false; // if this is set, no post process is available (bloom, fxaa etc) and the game renders stuff directly to vr system, no auto adjustment too
		float vrFoveatedRenderingLevel = 0.0f; // depends on vr implementation/system
		int vrFoveatedRenderingMaxDepth = 4; // 1=1.5? 2=1.0 3=0.5 4=0.25 5=0.125 6=0.0625
		bool skipDisplayBufferForVR = false; // skips displaying on screen to get better framerate
		VectorInt2 resolutionProvided = VectorInt2::zero; // provided via config or by user
		VectorInt2 resolution = VectorInt2::zero;
		VectorInt2 resolutionFull = VectorInt2::zero; // resolution of full screen, used only for window scaled, read from desktop size
		DisplayLayout::Type displayLayout = DisplayLayout::Default;
		float vrResolutionCoef = 1.0f;
		float aspectRatioCoef = 1.0f; // if > 1.0 we're lowering Y, if < 1.0 we're lowering X
		// overall - applied at the end
		float overall_vrResolutionCoef = 1.0f;
		float overall_aspectRatioCoef = 1.0f; // if > 1.0 we're lowering Y, if < 1.0 we're lowering X
		bool allowAutoAdjustForVR = true; // allows VR to adjust scale
		FullScreen::Type fullScreen = FullScreen::No; // windowed by default
		bool vsync = true;
		bool msaa = false; // if it is on in general
			// check above functions to see how it is being used
			bool msaaUI = true;
			bool msaaVR = true;
		int msaaSamples = 2; // do they have to share number of samples?
		bool fxaa = false;

		Tags options; // various options, if something should be turned on by default, it should not be mentioned here

		Array<String> useVec4ArraysInsteadOfMat4ArraysForRenderers;
		Array<String> limitedSupportForIndexingInShadersForRenderers;

		Optional<Colour> forcedBlackColour;

		VideoConfig();

		bool is_option_set(Name const & _option) const;

		bool does_use_msaa() const { return msaa; }
		bool does_use_ui_msaa() const { return msaa && msaaUI; }
		bool does_use_vr_msaa() const { return msaa && msaaVR; }
		int get_msaa_samples() const { return does_use_msaa()? msaaSamples : 0; }
		int get_ui_msaa_samples() const { return does_use_ui_msaa() ? msaaSamples : 0; }
		int get_vr_msaa_samples() const { return does_use_vr_msaa() ? msaaSamples : 0; }

		void fill_missing();
		void use_desktop_aspect_ratio_for_resolution();

		bool should_use_vec4_arrays_instead_of_mat4_arrays(String const& _renderer) const;
		bool has_limited_support_for_indexing_in_shaders(String const& _renderer) const;

		bool load_from_xml(IO::XML::Node const * _node);
		void save_to_xml(IO::XML::Node * _node, bool _userConfig = false) const;
	};

};

