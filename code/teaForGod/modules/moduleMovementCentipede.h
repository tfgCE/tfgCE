#pragma once

#include "..\..\framework\appearance\socketID.h"
#include "..\..\framework\module\moduleMovement.h"

namespace TeaForGodEmperor
{
	class ModuleMovementCentipede
	: public Framework::ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementCentipede);
		FAST_CAST_BASE(Framework::ModuleMovement);
		FAST_CAST_END();

		typedef Framework::ModuleMovement base;
	public:
		static Framework::RegisteredModule<Framework::ModuleMovement> & register_itself();

		ModuleMovementCentipede(Framework::IModulesOwner* _owner);
	
		void set_head(Framework::IModulesOwner* imo);
		void set_tail(Framework::IModulesOwner* imo);

		void set_stationary(bool _stationary) { stationary = _stationary; }

		Framework::IModulesOwner* get_tail() const;

	public: // Module
		override_ void setup_with(Framework::ModuleData const* _moduleData); // read parameters from moduleData

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);
		override_ Vector3 calculate_requested_velocity(float _deltaTime, bool _adjustForStoppingEtc, PrepareMoveContext& _context) const;

	private:
		Random::Generator rg;

		struct OtherSegment
		{
			SafePtr<Framework::IModulesOwner> imo;
			Transform placementOS = Transform::identity; // for head it is head's back, for tail, it is tail's front
			float weight = 0.0f;
			Framework::SocketID connectorHere;
			Framework::SocketID connectorThere;
		};
		OtherSegment tail;
		OtherSegment head;

		bool stationary = false;

		Framework::SocketID pivot;

		struct MovementTrace
		{
			Vector3 locOS; // where do we start trace (on ground, we need to move along up axis)
			Vector3 hitOS;
		};

		enum Trace
		{
			LF,
			RF,
			LB,
			RB,

			NUM
		};
		ArrayStatic<MovementTrace, Trace::NUM> traces;

		Collision::Flags traceCollideWithFlags;
		Vector3 traceCollisionsDoneAt = Vector3::zero;
		Vector3 traceCollisionsDoneForUp = Vector3::zero;
		float traceCollisionsDistance = 0.2f;

		float matchZBlendTime = 0.3f;
		float reorientateUpBlendTime = 0.3f;
		
		float followAngle = 45.0f;

		Vector3 currentMove = Vector3::zero;
		Rotator3 currentTurn = Rotator3::zero;

		void update_connectors(OtherSegment& _segment);
	};
};

