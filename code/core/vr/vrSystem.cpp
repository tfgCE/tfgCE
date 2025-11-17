#include "vrSystem.h"

#include "vrSystemImpl.h"

#include "..\mainConfig.h"

#include "..\system\input.h"
#include "..\system\video\shader.h"
#include "..\system\video\shaderProgram.h"
#include "..\system\video\video3d.h"
#include "..\system\video\renderTarget.h"

#include "..\performance\performanceUtils.h"

using namespace VR;

VRSystem::VRSystem(Name const & _name, VRSystemImpl* _impl)
: base(_name)
, impl(_impl)
{
	impl->init();
	eyeRenderRenderTargets[Eye::Left] = nullptr;
	eyeRenderRenderTargets[Eye::Right] = nullptr;
	eyeOutputRenderTargets[Eye::Left] = nullptr;
	eyeOutputRenderTargets[Eye::Right] = nullptr;
}

void VRSystem::get_ready_to_exit()
{
	impl->get_ready_to_exit_impl();
}

void VRSystem::handle_vr_mode_changes()
{
	impl->handle_vr_mode_changes_impl();
}

void VRSystem::set_processing_levels(Optional<float> const& _cpuLevel, Optional<float> const& _gpuLevel, bool _temporarily)
{
	base::set_processing_levels(_cpuLevel, _gpuLevel, _temporarily);
	impl->set_processing_levels_impl(_cpuLevel, _gpuLevel, _temporarily);
}

void VRSystem::I_am_perf_thread_main()
{
	impl->I_am_perf_thread_main_impl();
}

void VRSystem::I_am_perf_thread_other()
{
	impl->I_am_perf_thread_other_impl();
}

void VRSystem::I_am_perf_thread_render()
{
	impl->I_am_perf_thread_render_impl();
}

void VRSystem::log_perf_threads(LogInfoContext& _log)
{
	impl->log_perf_threads_impl(_log);
}

void VRSystem::enter_vr_mode()
{
	impl->enter_vr_mode();
}

void VRSystem::init_video(::System::Video3D* _v3d)
{
	impl->init_video(_v3d);
}

AvailableFunctionality VRSystem::get_available_functionality() const
{
	return impl->get_available_functionality();
}

tchar const * VRSystem::get_prompt_controller_suffix(Input::DeviceID _source) const
{
	return impl->get_prompt_controller_suffix(_source);
}

void VRSystem::create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget)
{
	impl->create_internal_render_targets(_source, _fallbackEyeResolution, OUT_ _useDirectToVRRendering, OUT_ _noOutputRenderTarget);
}

VectorInt2 VRSystem::get_direct_to_vr_render_size(int _eyeIdx)
{
	return impl->get_direct_to_vr_render_size(_eyeIdx);
}

float VRSystem::get_direct_to_vr_render_aspect_ratio_coef(int _eyeIdx)
{
	return impl->get_direct_to_vr_render_aspect_ratio_coef(_eyeIdx);
}

::System::RenderTarget* VRSystem::get_direct_to_vr_render_target(int _eyeIdx)
{
	auto* rt = impl->get_direct_to_vr_render_target(_eyeIdx);
	//an_assert(rt, TXT("if using \"direct to vr\" as it is set during create_internal_render_targets, please provide a render target"));
	return rt;
}

void VRSystem::on_render_targets_created()
{
	impl->on_render_targets_created();
}

VRSystem::~VRSystem()
{
	eyeRenderRenderTargets[Eye::Left] = nullptr;
	eyeRenderRenderTargets[Eye::Right] = nullptr;
	eyeOutputRenderTargets[Eye::Left] = nullptr;
	eyeOutputRenderTargets[Eye::Right] = nullptr;
	impl->shutdown();
	delete impl;
}

bool VRSystem::is_ok(bool _justCheckDevice) const
{
	return impl->is_ok();
}

VectorInt2 const & VRSystem::get_preferred_full_screen_resolution()
{
	return impl->get_preferred_full_screen_resolution();
}

void VRSystem::set_render_mode(RenderMode::Type _mode)
{
	base::set_render_mode(_mode);
	impl->set_render_mode(_mode);
}

RenderInfo const & VRSystem::get_render_info() const
{
	return impl->get_render_info();
}

float VRSystem::update_expected_frame_time() const
{
	return impl->update_expected_frame_time();
}

float VRSystem::calculate_ideal_expected_frame_time() const
{
	return impl->calculate_ideal_expected_frame_time();
}

void VRSystem::update_render_scaling(REF_ float & _scale)
{
	if (is_ok())
	{
		impl->update_render_scaling(REF_ _scale);
	}
}

void VRSystem::advance()
{
	MEASURE_PERFORMANCE_COLOURED(vrSystemAdvance, Colour::zxGreen);

	// call impl first to advance frame and system
	if (is_ok())
	{
		{
			MEASURE_PERFORMANCE_COLOURED(vrSystemNextFrame, Colour::zxWhite);
			// advance frame first
			impl->next_frame();
		}
		{
			MEASURE_PERFORMANCE_COLOURED(vrSystemAdvanceImpl, Colour::zxWhiteBright);
			// now advance the actual system
			impl->advance();
		}
	}

	// provide results to the game
	base::advance();
}

void VRSystem::game_sync()
{
	MEASURE_PERFORMANCE_COLOURED(vrSystemGameSync, Colour::zxRed);

	base::game_sync();

	if (is_ok())
	{
		MEASURE_PERFORMANCE_COLOURED(vrSystemGameSyncImpl, Colour::zxRedBright);
		impl->game_sync();
	}
}

bool VRSystem::begin_render_internal(System::Video3D * _v3d)
{
	MEASURE_PERFORMANCE_COLOURED(vrSystemBeginRender, Colour::zxMagenta);
	return impl->begin_render(_v3d);
}

bool VRSystem::end_render_internal(System::Video3D * _v3d)
{
	MEASURE_PERFORMANCE_COLOURED(vrSystemEndRender, Colour::zxMagentaBright);
	return impl->end_render(_v3d, eyeOutputRenderTargets);
}

Meshes::Mesh3D* VRSystem::load_mesh(Name const & _modelName) const
{
	return impl->load_mesh(_modelName);
}

int VRSystem::translate_bone_index(VRHandBoneIndex::Type _index) const
{
	return impl->translate_bone_index(_index);
}

DecideHandsResult::Type VRSystem::decide_hands_by_system(OUT_ int & _leftHand, OUT_ int & _rightHand)
{
	return impl->decide_hands(OUT_ _leftHand, OUT_ _rightHand);
}

void VRSystem::force_bounds_rendering(bool _force)
{
	impl->force_bounds_rendering(_force);
}

void VRSystem::update_hands_controls_source()
{
	// clear device related input tags
	if (auto* input = ::System::Input::get())
	{
		for_every(device, Input::Devices::get_all())
		{
			input->remove_usage_tags(device->inputTags);
		}
	}
	impl->update_hands_controls_source();
	// and set input tags using existing devices
	if (auto* input = ::System::Input::get())
	{
		for_count(int, i, Hands::Count)
		{
			auto & handsControls = get_controls().hands[i];
			if (auto* device = VR::Input::Devices::get(handsControls.source))
			{
				input->set_usage_tags_from(device->inputTags);
			}
		}
	}
}

void VRSystem::draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font)
{
	impl->draw_debug_controls(_hand, _at, _width, _font);
}

void VRSystem::update_hands_available()
{
	impl->update_hands_available();
}

bool VRSystem::load_play_area_rect(bool _loadAnything)
{
	return impl->load_play_area_rect(_loadAnything);
}

bool VRSystem::has_loaded_play_area_rect() const
{
	return impl->has_loaded_play_area_rect();
}

bool VRSystem::is_wireless() const
{
	return impl->is_wireless();
}

bool VRSystem::may_have_invalid_boundary() const
{
	return impl->may_have_invalid_boundary();
}

bool VRSystem::is_boundary_unavailable() const
{
	return impl->is_boundary_unavailable();
}
