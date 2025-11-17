#pragma once

#include "..\..\math\math.h"

#include "..\..\vr\vrEnums.h"

#ifdef BUILD_NON_RELEASE
#define TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
// this option defines default behaviour (which can be changed in options)
//#define TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS_AT_ALL_TIMES
// render lines or checkerboard
//#define TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS__CHECKERBOARD
#endif

#ifndef AN_ANDROID
// doesn't help anything
//#define USE_LESS_PIXEL_FOR_FOVEATED_RENDERING
#endif

namespace System
{
	struct FoveatedRenderingSetupPreset;

	/**
	 *   min_pixel_density=0.;
	 *	for(int i=0;i<focalPointsPerLayer;++i){
	 *	focal_point_density = 1./max((focalX[i]-px)^2*gainX[i]^2+
	 *						(focalY[i]-py)^2*gainY[i]^2-foveaArea[i],1.);
	 *	min_pixel_density=max(min_pixel_density,focal_point_density);
	 *	min_pixel_density=max(min_pixel_density,
	 *						   TEXTURE_FOVEATED_MIN_PIXEL_DENSITY_QCOM);
	 */
	struct FoveatedRenderingSetup
	{
		static int const MAX_FOCAL_POINTS = 2;

#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
		static bool testAtAnyTime;
		static FoveatedRenderingSetupPreset& access_tweaking_template();
		static void set_tweaking_template_from_current();
		static void store_tweaking_template_as_current();
		static void dont_use_tweaking_template();
		static void handle_rendering_test_grid(int _eyeIdx);
		static bool is_rendering_test_grid();
#endif
		static void render_test_grid(int _eyeIdx);

		static void force_set_foveation(); // use this post changing foveation rendering setup
		static bool should_force_set_foveation();

		static bool load_from_xml(IO::XML::Node const* _node);

		static int get_nominal_setup();
		static int get_max_setup();
		static float get_max_vr_foveated_rendering_level();

		static bool set_temporary_foveation_level_offset(Optional<float> const & _offset = NP); // don't forget to call force_set_foveation afterwards, returns true if changed
		static bool set_temporary_no_foveation(bool _noFoveation); // don't forget to call force_set_foveation afterwards, returns true if changed
		static bool is_temporary_no_foveation(); // maybe we just want to disable it at all for a moment?
		// user
		static float adjust_min_density(float _density);

#ifdef USE_LESS_PIXEL_FOR_FOVEATED_RENDERING
		static void test_use_less_pixel_for_foveated_rendering(Optional<float> const& _level, Optional<int> const & _maxDepth);
		static bool should_test_use_less_pixel_for_foveated_rendering();
		static Optional<float> const& get_test_use_less_pixel_for_foveated_rendering();
		static Optional<int> const& get_test_use_less_pixel_for_foveated_rendering_maxDepth();
		static int get_use_less_pixel_for_foveated_rendering_max_depth() { return hardcoded 3; }
#endif

		static void set_hi_res_scene(bool _hiRes);
		static bool is_hi_res_scene();

		struct FocalPoint
		{
			Vector2 at = Vector2::zero; // -1 to 1
			float offsetXLeft = 0.0f; // -1 to 1
			float offsetXRight = 0.0f; // -1 to 1
			Vector2 gain = Vector2::zero; // the more the better, if only x is set, will auto scale
			float foveArea = 0.0f;
		};
		ArrayStatic<FocalPoint, MAX_FOCAL_POINTS> focalPoints;
		//float minDensity = 0.25f; // pixels can't be just 4x4
		//float minDensity = 0.125f; // pixels can't be just 8x8
		float minDensity = 0.0625f; // pixels can't be just 16x16
		bool fixed = false;

		void setup(VR::Eye::Type _eye, Vector2 const& _centre, Vector2 const & _resolution, float _renderScaling); // uses foveated rendering from main config
		void setup_using(FoveatedRenderingSetup const & _frs, VR::Eye::Type _eye, Vector2 const& _centre, Vector2 const & _resolution, float _renderScaling);

		static bool should_use(); // basing on main config

		void debug_log() const;

		void set_from(FoveatedRenderingSetup const& _from) { minDensity = _from.minDensity; focalPoints = _from.focalPoints; fixed = _from.fixed; }

	private:
		static FoveatedRenderingSetup * choose_best_preset();
	};

	struct FoveatedRenderingSetupPreset
	: public FoveatedRenderingSetup
	{
		float forFoveatedRenderingLevel = 0.0f;

		static inline int compare(void const* _a, void const* _b)
		{
			FoveatedRenderingSetupPreset const & a = *plain_cast<FoveatedRenderingSetupPreset>(_a);
			FoveatedRenderingSetupPreset const & b = *plain_cast<FoveatedRenderingSetupPreset>(_b);
			if (a.forFoveatedRenderingLevel < b.forFoveatedRenderingLevel) return A_BEFORE_B;
			if (a.forFoveatedRenderingLevel > b.forFoveatedRenderingLevel) return B_BEFORE_A;
			return A_AS_B;
		}
	};
};
