#pragma once

#include "iVR.h"

namespace System
{
	class Font;
};

namespace VR
{
	class VRSystemImpl;

	/**
	 *	Advancing sensor is done pre rendering
	 */
	class VRSystem
	: public IVR
	{
		typedef IVR base;
	protected: // IVR
		implement_ void get_ready_to_exit();

		implement_ void handle_vr_mode_changes();

		implement_ void set_processing_levels(Optional<float> const& _cpuLevel = NP, Optional<float> const& _gpuLevel = NP, bool _temporarily = false);
		implement_ void I_am_perf_thread_main();
		implement_ void I_am_perf_thread_other();
		implement_ void I_am_perf_thread_render();
		implement_ void log_perf_threads(LogInfoContext& _log);

		implement_ bool is_ok(bool _justCheckDevice = false) const;
		implement_ AvailableFunctionality get_available_functionality() const;

		implement_ tchar const* get_prompt_controller_suffix(Input::DeviceID _source) const;

		implement_ VectorInt2 const & get_preferred_full_screen_resolution();
		implement_ void force_bounds_rendering(bool _force);

		implement_ void set_render_mode(RenderMode::Type _mode);
		implement_ RenderInfo const & get_render_info() const;

		implement_ void advance();
		implement_ void game_sync();

		implement_ DecideHandsResult::Type decide_hands_by_system(OUT_ int & _leftHand, OUT_ int & _rightHand);

		implement_ void update_hands_controls_source();

		implement_ bool load_play_area_rect(bool _loadAnything = false);
		implement_ bool has_loaded_play_area_rect() const;

		implement_ bool is_wireless() const;
		implement_ bool may_have_invalid_boundary() const;
		implement_ bool is_boundary_unavailable() const;

		// rendering/presenting
		implement_ bool begin_render_internal(System::Video3D * _v3d);
		implement_ bool end_render_internal(System::Video3D* _v3d);

		implement_ Meshes::Mesh3D* load_mesh(Name const & _modelName) const;
		implement_ int translate_bone_index(VRHandBoneIndex::Type _index) const;

		implement_ float update_expected_frame_time() const;
		implement_ float calculate_ideal_expected_frame_time() const;

		implement_ void draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font);

		implement_ void update_render_scaling(REF_ float & _scale);

		implement_ void update_hands_available();

	protected:
		void setup_fallback_distortion_shader(Eye::Type _eye, bool _chromAb = true);

	protected:
		VRSystem(Name const & _name, VRSystemImpl* _impl);
		virtual ~VRSystem();

	private:
		VRSystemImpl* impl;

		implement_ void enter_vr_mode();
		implement_ void init_video(::System::Video3D* _v3d);
		implement_ void create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget);
		implement_ VectorInt2 get_direct_to_vr_render_size(int _eyeIdx);
		implement_ float get_direct_to_vr_render_aspect_ratio_coef(int _eyeIdx);
		implement_ ::System::RenderTarget* get_direct_to_vr_render_target(int _eyeIdx);
		implement_ void on_render_targets_created();

		friend class IVR;
	};

};
