#include "foveatedRenderingSetup.h"

#include "..\..\mainConfig.h"

#include "..\..\system\timeStamp.h"

#include "..\..\concurrency\scopedSpinLock.h"

#include "..\..\system\core.h"
#include "..\..\system\video\renderTarget.h"
#include "..\..\system\video\renderTargetUtils.h"
#include "..\..\system\video\video3d.h"
#include "..\..\system\video\video3dPrimitives.h"
#include "..\..\tags\tagCondition.h"
#include "..\..\vr\iVR.h"

//

//#define DEBUG_FOVEATED_RENDERING_SETUP

//#define DEBUG_LOADING_FOVEATED_RENDERING_SETUPS

//

using namespace System;

//

Concurrency::SpinLock g_foveatedRenderingSetupPresetsLoadingLock;
ArrayStatic<FoveatedRenderingSetupPreset, 10> g_foveatedRenderingSetupPresets;
Optional<float> g_temporaryFoveationLevelOffset;
Optional<float> g_temporaryMinDensity;
bool g_temporaryNoFoveation = false;

//

::System::TimeStamp g_forceSetFoveation;

#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
bool g_tweakingTemplateInUse = false;
FoveatedRenderingSetupPreset g_tweakingTemplate;

bool FoveatedRenderingSetup::testAtAnyTime =
#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS_AT_ALL_TIMES
	true;
#else
	false;
#endif

System::TimeStamp g_renderingTestGridTS;

FoveatedRenderingSetupPreset& FoveatedRenderingSetup::access_tweaking_template()
{
	g_tweakingTemplateInUse = true;
	return g_tweakingTemplate;
}

void FoveatedRenderingSetup::dont_use_tweaking_template()
{
	g_tweakingTemplateInUse = false;
}

void FoveatedRenderingSetup::set_tweaking_template_from_current()
{
	if (auto* frs = choose_best_preset())
	{
		g_tweakingTemplate.set_from(*frs);
	}
}

void FoveatedRenderingSetup::store_tweaking_template_as_current()
{
	if (auto* frs = choose_best_preset())
	{
		frs->set_from(g_tweakingTemplate);
	}
}

bool FoveatedRenderingSetup::is_rendering_test_grid()
{
	return g_renderingTestGridTS.get_time_since() < 0.2f;
}

void FoveatedRenderingSetup::handle_rendering_test_grid(int _eyeIdx)
{
	if (auto* vr = VR::IVR::get())
	{
		bool allowedNow = testAtAnyTime;
#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS_AT_ALL_TIMES
		allowedNow = true;
#endif
		if (allowedNow)
		{
			VR::Input::Button::WithSource anyGripButton;
			anyGripButton.type = VR::Input::Button::Grip;
			if (vr->get_controls().is_button_pressed(anyGripButton))
			{
				render_test_grid(_eyeIdx);
			}
		}
	}
}
#endif

void FoveatedRenderingSetup::render_test_grid(int _eyeIdx)
{
#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
	g_renderingTestGridTS.reset();
#endif
	if (auto* vr = VR::IVR::get())
	{
		if (auto* rt = vr->get_render_render_target((VR::Eye::Type)_eyeIdx))
		{
			rt->bind();

			VectorInt2 renderSize = rt->get_size();

			auto* v3d = ::System::Video3D::get();

			v3d->set_viewport(VectorInt2::zero, renderSize);
			v3d->set_near_far_plane(0.02f, 100.0f);

			v3d->set_2d_projection_matrix_ortho();
			v3d->access_model_view_matrix_stack().clear();

			{
				Colour colourPaper = Colour::white;
				Colour colourInk = Colour::black;
				Vector2 vs = renderSize.to_vector2();
				::System::Video3DPrimitives::fill_rect_2d(colourPaper, Vector2::zero, vs, false);

#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS__CHECKERBOARD
				float gridSize = 64.0f;
				for (float y = 0.0f; y < vs.y + gridSize; y += gridSize)
				{
					for (float x = 0.0f; x < vs.x + gridSize; x += gridSize)
					{
						Vector2 c(x, y);
						Vector2 g = Vector2::one * gridSize;
						Vector2 hg = g * 0.5f;
						Vector2 l = c + hg * Vector2(-1.0f,  0.0f);
						Vector2 u = c + hg * Vector2( 0.0f,  1.0f);
						Vector2 r = c + hg * Vector2( 1.0f,  0.0f);
						Vector2 d = c + hg * Vector2( 0.0f, -1.0f);
						::System::Video3DPrimitives::triangle_2d(colourInk, l, u, r, false);
						::System::Video3DPrimitives::triangle_2d(colourInk, r, d, l, false);
					}
				}
#else
				/*
				Colour colours[] = { colourPaper, colourInk };
				int colourIdx = 0;
				for (float y = 0.0f; y < vs.y + vs.x; y += 1.0f)
				{
					::System::Video3DPrimitives::line_2d(colours[colourIdx], Vector2(0.0f, y), Vector2(vs.x, y - vs.x), false);
					colourIdx = mod(colourIdx + 1, 2);
				}
				*/
				for (float y = 0.0f; y < vs.y + vs.x; y += 5.0f)
				{
					::System::Video3DPrimitives::line_2d(colourInk, Vector2(0.0f, y), Vector2(vs.x, y - vs.x), false);
				}
				for (float y = 2.0f; y < vs.y + vs.x; y += 30.0f)
				{
					::System::Video3DPrimitives::line_2d(colourPaper, Vector2(0.0f, y), Vector2(vs.x, y - vs.x), false);
				}
				for (float y = 0.0f; y < vs.y + vs.x; y += 7.0f)
				{
					::System::Video3DPrimitives::line_2d(colourInk, Vector2(vs.x, y), Vector2(0.0f, y - vs.x * 0.4f), false);
				}
#endif
			}

#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
			RenderTargetUtils::render_last_foveated_rendering_setups(_eyeIdx);
#endif

			rt->unbind();
		}
	}
}

void FoveatedRenderingSetup::force_set_foveation()
{
	g_forceSetFoveation.reset();
}

bool FoveatedRenderingSetup::should_force_set_foveation()
{
	return g_forceSetFoveation.get_time_since() < 0.1f;
}

bool FoveatedRenderingSetup::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	if (_node->has_attribute(TXT("systemTagRequired")))
	{
		TagCondition systemTagRequired;
		if (systemTagRequired.load_from_xml_attribute(_node, TXT("systemTagRequired")))
		{
			if (!systemTagRequired.check(System::Core::get_system_tags()))
			{
				return result;
			}
		}
	}

	Concurrency::ScopedSpinLock lock(g_foveatedRenderingSetupPresetsLoadingLock);

	g_foveatedRenderingSetupPresets.clear();

	for_every(n, _node->children_named(TXT("foveatedRenderingSetup")))
	{
		FoveatedRenderingSetupPreset frs;
		frs.forFoveatedRenderingLevel = n->get_float_attribute_or_from_child(TXT("for"), 0.0f);
		frs.minDensity = n->get_float_attribute_or_from_child(TXT("minDensity"), 1.0f);
		frs.fixed = n->get_bool_attribute_or_from_child_presence(TXT("fixed"), frs.fixed);
		frs.fixed = !n->get_bool_attribute_or_from_child_presence(TXT("atFocal"), ! frs.fixed);
		for_every(fpn, n->children_named(TXT("focalPoint")))
		{
			FocalPoint fp;
			fp.at.load_from_xml(fpn, TXT("atX"), TXT("atY"));
			fp.offsetXLeft = fpn->get_float_attribute(TXT("offsetXLeft"), fp.offsetXLeft);
			fp.offsetXRight = fpn->get_float_attribute(TXT("offsetXRight"), fp.offsetXRight);
			fp.gain.load_from_xml(fpn, TXT("gainX"), TXT("gainY"));
			fp.foveArea = fpn->get_float_attribute(TXT("foveArea"), fp.foveArea);
			if (frs.focalPoints.has_place_left())
			{
				frs.focalPoints.push_back(fp);
			}
			else
			{
				error_loading_xml(fpn, TXT("too many focal points"));
				result = false;
			}
		}
		if (g_foveatedRenderingSetupPresets.has_place_left())
		{
			g_foveatedRenderingSetupPresets.push_back(frs);
		}
		else
		{
			error_loading_xml(n, TXT("too many focal points"));
			result = false;
		}
	}

	sort(g_foveatedRenderingSetupPresets);

	output(TXT("loaded foveated rendering setups (%i)"), g_foveatedRenderingSetupPresets.get_size());
#ifdef DEBUG_LOADING_FOVEATED_RENDERING_SETUPS
	{
		for_every(frs, g_foveatedRenderingSetupPresets)
		{
			frs->debug_log();
		}
	}
#endif

	return result;
}

int FoveatedRenderingSetup::get_nominal_setup()
{
	float closestDist = 0.0f;
	int closest = NONE;
	for_every(frs, g_foveatedRenderingSetupPresets)
	{
		float dist = abs(frs->forFoveatedRenderingLevel - 1.0f);
		if (closest == NONE ||
			closestDist > dist)
		{
			closest = for_everys_index(frs);
			closestDist = dist;
		}
	}
	return max(0, closest);
}

int FoveatedRenderingSetup::get_max_setup()
{
	return max(0, g_foveatedRenderingSetupPresets.get_size() - 1);
}

float FoveatedRenderingSetup::get_max_vr_foveated_rendering_level()
{
	return g_foveatedRenderingSetupPresets.is_empty()? 1.0f : g_foveatedRenderingSetupPresets.get_last().forFoveatedRenderingLevel;
}

void FoveatedRenderingSetup::setup_using(FoveatedRenderingSetup const& _frs, VR::Eye::Type _eye, Vector2 const& _centre, Vector2 const& _resolution, float _renderScaling)
{
	set_from(_frs);
	for_every(fp, focalPoints)
	{
		fp->at = (_frs.fixed? Vector2::zero : _centre) + Vector2(fp->at.x * (_eye == VR::Eye::Left ? -1.0f : 1.0f), fp->at.y);
		if (_eye == VR::Eye::Left)
		{
			fp->at.x += fp->offsetXLeft;
		}
		else
		{
			fp->at.x += fp->offsetXRight;
		}
		if (fp->gain.y == 0.0f && fp->gain.x != 0.0f)
		{
			if (_resolution.y > _resolution.x)
			{
				fp->gain = fp->gain.x * Vector2(1.0f, _resolution.y / _resolution.x);
			}
			else
			{
				fp->gain = fp->gain.x * Vector2(_resolution.x / _resolution.y, 1.0f);
			}
		}
		{
			float aspectRatioCoef = MainConfig::global().get_video().aspectRatioCoef * MainConfig::global().get_video().overall_aspectRatioCoef;
			fp->gain = apply_aspect_ratio_coef(fp->gain, aspectRatioCoef);
		}
		fp->foveArea *= _renderScaling;
		Vector2 bl = -Vector2::one;
		fp->at = bl + (fp->at - bl) * _renderScaling;
		fp->gain /= _renderScaling;
	}
#ifdef DEBUG_FOVEATED_RENDERING_SETUP
	output(TXT("[foveated-rendering-setup] post setup (using)"));
	debug_log();
#endif
}

FoveatedRenderingSetup * FoveatedRenderingSetup::choose_best_preset()
{
	float level = MainConfig::global().get_video().vrFoveatedRenderingLevel + g_temporaryFoveationLevelOffset.get(0.0f);

	if (!g_foveatedRenderingSetupPresets.is_empty())
	{
		int closest = NONE;
		float closestDist = 0.0f;
		for_every(frs, g_foveatedRenderingSetupPresets)
		{
			float dist = abs(frs->forFoveatedRenderingLevel - level);
			if (closest == NONE ||
				closestDist > dist)
			{
				closest = for_everys_index(frs);
				closestDist = dist;
			}
		}

		if (closest != NONE)
		{
#ifdef DEBUG_FOVEATED_RENDERING_SETUP
			output(TXT("[foveated-rendering-setup] chose best %i %.3f"), closest, g_foveatedRenderingSetupPresets[closest].forFoveatedRenderingLevel);
#endif
			return &g_foveatedRenderingSetupPresets[closest];
		}
	}

	return nullptr;
}

void FoveatedRenderingSetup::setup(VR::Eye::Type _eye, Vector2 const& _centre, Vector2 const& _resolution, float _renderScaling)
{
#ifdef DEBUG_FOVEATED_RENDERING_SETUP
	output(TXT("[foveated-rendering-setup] %i centre %S"), _eye, _centre.to_string().to_char());
	g_tweakingTemplate.debug_log();
#endif

#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
	if (g_tweakingTemplateInUse)
	{
#ifdef DEBUG_FOVEATED_RENDERING_SETUP
		output(TXT("[foveated-rendering-setup] setup using tweaking template"));
		g_tweakingTemplate.debug_log();
#endif
		setup_using(g_tweakingTemplate, _eye, _centre, _resolution, _renderScaling);
		return;
	}
#endif

	if (auto* frs = choose_best_preset())
	{
#ifdef DEBUG_FOVEATED_RENDERING_SETUP
		output(TXT("[foveated-rendering-setup] setup using best chosen"));
		frs->debug_log();
#endif
		setup_using(*frs, _eye, _centre, _resolution, _renderScaling);
		return;
	}

	focalPoints.clear();

	{
		FocalPoint fp;
		fp.at = _centre;
		
		if (_resolution.is_zero())
		{
			fp.gain = Vector2::one;
		}
		else
		{
			if (_resolution.y > _resolution.x)
			{
				fp.gain = Vector2(1.0f, _resolution.y / _resolution.x);
			}
			else
			{
				fp.gain = Vector2(_resolution.x / _resolution.y, 1.0f);
			}
		}

		{
			float aspectRatioCoef = MainConfig::global().get_video().aspectRatioCoef * MainConfig::global().get_video().overall_aspectRatioCoef;
			fp.gain = apply_aspect_ratio_coef(fp.gain, aspectRatioCoef);
		}

		{
			float level = MainConfig::global().get_video().vrFoveatedRenderingLevel;

			float gainLevel = powf(12.0f, level);
			fp.gain *= gainLevel;
		}

		Vector2 bl = -Vector2::one;
		fp.at = bl + (fp.at - bl) * _renderScaling;
		fp.gain /= _renderScaling;

		fp.foveArea = 0.0f;

		while (focalPoints.has_place_left())
		{
			focalPoints.push_back(fp);
		}
	}

#ifdef DEBUG_FOVEATED_RENDERING_SETUP
	output(TXT("[foveated-rendering-setup] post setup (as is)"));
	debug_log();
#endif
}

bool FoveatedRenderingSetup::should_use()
{
	return MainConfig::global().get_video().vrFoveatedRenderingLevel > 0.0f;
}

void FoveatedRenderingSetup::debug_log() const
{
	LogInfoContext log;
	log.log(TXT("foveated rendering setup"));
	{
		LOG_INDENT(log);
		log.log(TXT("minDensity: %.4f"), minDensity);
		for_every(fp, focalPoints)
		{
			log.log(TXT(" fp %i at %.5fx%.5f (l%.5f r%.5f) gain %.5fx%.5f foveArea %.3f"), for_everys_index(fp), fp->at.x, fp->at.y, fp->offsetXLeft, fp->offsetXRight, fp->gain.x, fp->gain.y, fp->foveArea);
		}
		for_every(fp, focalPoints)
		{
			log.log(TXT(" <focalPoint atX=\"%.5f\" atY=\"%.5f\" offsetXLeft=\"%.5f\" offsetXRight=\"%.5f\" gainX=\"%.5f\" gainY=\"%.5f\" foveArea=\"%.2f\"/>"),
				fp->at.x, fp->at.y, 
				fp->offsetXLeft, fp->offsetXRight,
				fp->gain.x, fp->gain.y, fp->foveArea);
		}
	}
	log.output_to_output();
}

bool FoveatedRenderingSetup::set_temporary_foveation_level_offset(Optional<float> const& _offset)
{
	bool changed = (g_temporaryFoveationLevelOffset != _offset);
	g_temporaryFoveationLevelOffset = _offset;
	return changed;
}

bool FoveatedRenderingSetup::set_temporary_no_foveation(bool _noFoveation)
{
	bool changed = (g_temporaryNoFoveation != _noFoveation);
	g_temporaryNoFoveation = _noFoveation;
	return changed;
}

bool FoveatedRenderingSetup::is_temporary_no_foveation()
{
	return g_temporaryNoFoveation;
}

float FoveatedRenderingSetup::adjust_min_density(float _density)
{
	return max(g_temporaryNoFoveation? 1.0f : g_temporaryMinDensity.get(_density), _density);
}

#ifdef USE_LESS_PIXEL_FOR_FOVEATED_RENDERING
Optional<float> g_testUseLessPixelForFoveatedRendering;
Optional<int> g_testUseLessPixelForFoveatedRenderingMaxDepth;

void ::System::FoveatedRenderingSetup::test_use_less_pixel_for_foveated_rendering(Optional<float> const& _level, Optional<int> const & _maxDepth)
{
	g_testUseLessPixelForFoveatedRendering = _level;
	g_testUseLessPixelForFoveatedRenderingMaxDepth = _maxDepth;
}

bool ::System::FoveatedRenderingSetup::should_test_use_less_pixel_for_foveated_rendering()
{
	return g_testUseLessPixelForFoveatedRendering.is_set() && g_testUseLessPixelForFoveatedRenderingMaxDepth.is_set();
}

Optional<float> const& ::System::FoveatedRenderingSetup::get_test_use_less_pixel_for_foveated_rendering()
{
	return g_testUseLessPixelForFoveatedRendering;
}

Optional<int> const& ::System::FoveatedRenderingSetup::get_test_use_less_pixel_for_foveated_rendering_maxDepth()
{
	return g_testUseLessPixelForFoveatedRenderingMaxDepth;
}
#endif

bool g_hiResScene = false;

void ::System::FoveatedRenderingSetup::set_hi_res_scene(bool _hiRes)
{
	g_hiResScene = _hiRes;
}

bool ::System::FoveatedRenderingSetup::is_hi_res_scene()
{
	return g_hiResScene;
}
