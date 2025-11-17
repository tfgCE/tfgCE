#pragma once

#include "..\..\framework\appearance\socketID.h"
#include "..\..\framework\module\moduleMovement.h"

namespace TeaForGodEmperor
{
	class ModuleMovementMonorailCart
	: public Framework::ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementMonorailCart);
		FAST_CAST_BASE(Framework::ModuleMovement);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		static Framework::RegisteredModule<Framework::ModuleMovement> & register_itself();

		ModuleMovementMonorailCart(Framework::IModulesOwner* _owner);
	
		float get_speed() const { return speed; }
		void set_speed(float _speed) { speed = _speed; }

		void couple_forward_to(Framework::IModulesOwner* _to, Name const& _throughSocket, Name const& _toSocket);
		void decouple_forward();
		void decouple_back();
		void decouple();

		void use_track(Vector3 const & _startWS, Vector3 const & _endWS);

	protected:	// Module
		override_ void setup_with(Framework::ModuleData const * _moduleData);

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);

	public:
		float speed = 1.0f;

		struct TrackLinear
		{
			Vector3 a;
			Vector3 b;
			Segment track;
		} trackLinear;

		enum TrackType
		{
			NoTrack,
			Linear
		};

		TrackType trackType = TrackType::NoTrack;

		::SafePtr<Framework::IModulesOwner> coupledForward;
		Framework::SocketID connectorForward;
		::SafePtr<Framework::IModulesOwner> coupledBack;
		Framework::SocketID connectorBack;

		static void decouple_internal(Framework::IModulesOwner* _forward, Framework::IModulesOwner* _back);
	};
};

