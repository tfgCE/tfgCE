#pragma once

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObject.h"

#include "overlayInfoEnums.h"

namespace TeaForGodEmperor
{
	namespace OverlayInfo
	{
		class System;

		struct Element
		: public RefCountObject
		{
			FAST_CAST_DECLARE(Element);
			FAST_CAST_END();

		public:
			enum Relativity
			{
				Normal,					// starting CP + this (follows camera)
				RelativeToCamera,		// current CP + this
				RelativeToCameraFlat,	// (starting CP pitch/roll/Z + current CP yaw/XY) + this
				RelativeToAnchor,		// anchor + this
				RelativeToAnchorPY,		// as above, pitch+yaw
				RelativeToAnchorHUD,	// anchor (hud) + this
				RelativeToAnchorHUDPY,	// as above, pitch+yaw
				RelativeToVRAnchor,		// vranchor + this
			};

			enum RenderLayer
			{
				Meshes,
				Flat,
				Subtitle,

				Default = Flat
			};
		public:
			Name const& get_id() const { return id; }

			float get_faded_active() const { return active * fadedIn; }
			bool is_active() const { return activeTarget == 1.0f; }
			bool has_been_deactivated() const { return get_faded_active() < 0.05f && activeTarget == 0.0f; }
			void deactivate();
			void keep_placement() { keepPlacement = true; }
			
			void set_cant_be_deactivated_by_system() { cantBeDeactivatedBySystem = true; }
			bool get_cant_be_deactivated_by_system() const { return cantBeDeactivatedBySystem; }

			void auto_deactivate(float _time) { autoDeactivateTimeLeft = _time; }
			float get_auto_deactivate_time_left() const { return autoDeactivateTimeLeft; }

			Transform const& get_placement() const { return finalPlacement; } // with radius

			Element* with_id(Name const& _id) { id = _id; return this; }
			Element* with_usage(Usage::Flags _usage) { usage = _usage; return this; }
			Element* with_render_layer(RenderLayer _renderLayer) { renderLayer = _renderLayer; return this; }
			Element* with_location(Relativity _locationRelativity, Vector3 const& _location = Vector3::zero) { locationRelativity = _locationRelativity; placement.set_translation(_location); update_placement = nullptr; return this; }
			Element* with_rotation(Relativity _rotationRelativity, Rotator3 const& _rotation = Rotator3::zero) { rotationRelativity = _rotationRelativity; initialRotation.clear(); placement.set_orientation(_rotation.to_quat()); update_placement = nullptr; return this; }
			Element* with_placement(Relativity _relativity, Transform const& _placement) { locationRelativity = _relativity; rotationRelativity = _relativity; initialRotation.clear(); placement = _placement; update_placement = nullptr; return this; }
			Element* with_distance(float _dist) { dist = _dist; return this; }
			Element* with_custom_placement(Relativity _relativity, std::function<Transform(System const* _system, Element const* _element)> _update_placement) { with_placement(RelativeToVRAnchor, Transform::identity)->with_distance(0.0f); update_placement = _update_placement; return this; }

			Element* with_vr_placement(Transform const& _placement) { return with_placement(RelativeToVRAnchor, _placement)->with_distance(0.0f); }
			Element* with_custom_vr_placement(std::function<Transform(System const* _system, Element const * _element)> _update_placement) { return with_custom_placement(RelativeToVRAnchor, _update_placement); }

			Element* apply_vr_anchor_displacement(bool _applyVRAnchorDisplacement = true) { applyVRAnchorDisplacement = _applyVRAnchorDisplacement; return this; }

			Element* with_colour(Colour const& _colour) { colour = _colour; return this; }
			Element* with_pulse(float _pulsePeriod = 1.0f, float _pulseStrength = 0.3f) { pulsePeriod = _pulsePeriod; pulseStrength = _pulseStrength; return this; }

			Element* with_follow_camera_yaw(Optional<float> const& _followCameraYaw) { followCameraYaw = _followCameraYaw; return this; }
			Element* with_follow_camera_pitch(Optional<float> const& _followCameraPitch) { followCameraPitch = _followCameraPitch; return this; }
			Element* with_follow_camera_location(Optional<float> const& _followCameraLocation) { followCameraLocation = _followCameraLocation; return this; }
			Element* with_follow_camera_location_xy(Optional<float> const& _followCameraLocationXY) { followCameraLocationXY = _followCameraLocationXY; return this; }
			Element* with_follow_camera_location_z(Optional<float> const& _followCameraLocationZ) { followCameraLocationZ = _followCameraLocationZ; return this; }

			Usage::Flags get_usage() const { return usage; }

		public:
			virtual void advance(System const* _system, float _deltaTime);
			virtual void request_render_layers(OUT_ ArrayStatic<int, 8>& _renderLayers) const { _renderLayers.push_back_unique(renderLayer); }
			virtual void render(System const* _system, int _renderLayer) const = 0;

			void apply_vr_displacement(Transform const& _vrAnchorDisplacement, bool _force = false); // force to ignoreapplyVRAnchorDisplacement

		protected:
			Name id;
			float autoDeactivateTimeLeft = 0.0f;
			bool cantBeDeactivatedBySystem = false; // if true, cannot be deactivated with deactivate all, has to be deactivated by a direct all

			RenderLayer renderLayer = RenderLayer::Default;

			// everything is in vr space
			Transform placement = Transform::identity;
			float dist = 5.0f;

			bool keepPlacement = false; // if deactivating, we should keep (final) placement in some cases

			Optional<Rotator3> initialRotation; // to get yaw offset

			Optional<float> followCameraYaw;
			Optional<float> followCameraPitch;
			Optional<float> followCameraLocation;
			Optional<float> followCameraLocationXY;
			Optional<float> followCameraLocationZ;

			Optional<Transform> startingCameraPlacement;
			Optional<Transform> startingCameraPlacementFlat;

			Usage::Flags usage = Usage::All;

			Relativity rotationRelativity = Relativity::Normal;
			Relativity locationRelativity = Relativity::RelativeToCameraFlat;
			bool applyVRAnchorDisplacement = false;

			AUTO_ Transform finalPlacement = Transform::identity; // with radius
			std::function<Transform(System const* _system, Element const* _element)> update_placement = nullptr;

			float active = 0.0f;
			float activeTarget = 1.0f;
			float fadedIn = 0.0f;

			Colour colour = Colour::white;
			float pulsePeriod = 0.0f;
			float pulseStrength = 0.0f;
			float pulse = 1.0f; // current

			static float calculate_additional_scale(float _distance, Optional<float> const& _useAdditionalScale = NP);

		private:
			Rotator3 calculate_rotation(System const* _system) const;
			Vector3 calculate_location(System const* _system) const; // without radius
			Transform calculate_placement(System const* _system) const; // with radius
		};
	};
};
