#pragma once

#include "moduleMovementInteractivePart.h"

namespace Framework
{
	class ModuleMovementSwitchData;

	struct ModuleMovementSwitchSetup
	{
		struct Placement
		{
			Optional<float> output;
			Optional<Transform> placement;
			Optional<Transform> relativePlacement;
			Name parentSocket; // more important than socket
			Name socket; // own, relative to initial placement
			Name soundIn;
			Name soundOut;
			bool allowRest = true;
			bool load_from_xml(IO::XML::Node const * _node);
		};
		Array<Placement> placements;
		Axis::Type placementCurveAlongAxis = Axis::None;
		Name parentRadialCentreSocket;
		Axis::Type radialToCentreAxis = Axis::None;
		Axis::Type radialRotationAxis = Axis::None;
		float radialRadius = 0.0f;
		float blendPlacementPtTime = 0.1f;
		float allowRestDistPt = 0.3f; // distance from state that pulls towards state
		bool canStopAnywhere = false; // if false, goes back to last valid rest, if true may stop anywhere
		float activeDist = 0.1f; // distance from state considered active
		float inertia = 0.0f;
	};

	class ModuleMovementSwitch
	: public ModuleMovementInteractivePart
	{
		FAST_CAST_DECLARE(ModuleMovementSwitch);
		FAST_CAST_BASE(ModuleMovementInteractivePart);
		FAST_CAST_END();

		typedef ModuleMovementInteractivePart base;
	public:
		static RegisteredModule<ModuleMovement> & register_itself();

		ModuleMovementSwitch(IModulesOwner* _owner);

		void clear_placements();
		void add_placement(Transform const & _placement, bool _allowRest = true);
		void set_allow_rest(int _idx, bool _allowRest);

		void go_to(int _idx, float _tryFor = 1.5f);
		void rest_at(int _idx);
		void lock_at(int _idx);
		void resume(); // cancel rest and lock

		int get_closest(Optional<float> const & _pt = NP) const;
		float get_at() const { return placementPt; }
		float get_output() const;
		bool is_active_at(int _idx) const;
		bool has_been_actived_at(int _idx) const;
		bool has_been_deactived_at(int _idx) const;

	protected:	// Module
		override_ void setup_with(ModuleData const * _moduleData);
		override_ void activate();

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);

	protected:	// ModuleMovementInteractivePart
		virtual void mark_requires_movement(bool _bigChange) { updateFrames = 1; if (_bigChange) update_placements(true); }

	private:
		ModuleMovementSwitchData const * moduleMovementSwitchData = nullptr;

		float timeSinceNonZeroDisplacement = 0.0f;
		float lastChangePt = 0.0f;

		ModuleMovementSwitchSetup setup; // copied from data and modified as required

		struct Placement
		{
			Name parentSocket; // more important than socket
			Name socket; // own, relative to initial placement, this means that we store initial placement (may be relative to base) and calculate where we should be using socket
			Name soundIn;
			Name soundOut;
			Optional<float> output;
			Transform placement; // relative to "attached to object" (based on, attached to), or to initial placement or WS, this is using placementType
			bool allowRest = true; // if false, will go back to closest location that allows resting
			BezierCurve<Vector3> curve; // this to next
		};
		Array<Placement> placements; // actual placements, based on setup
		bool placementsUpdateRequired = true;
		bool placementsUpdateRequiredForced = true;

		float prevPlacementPt = 0.0f;
		float placementPt = 0.0f;
		float lastRestPt = 0.0;
		Optional<float> restAt;
		Optional<float> lockedAt;
		Optional<float> goTo;
		float goToFor = 0.0f;

		float lastMoveDueToForces = 0.0f;
		float lastMoveDueToForcesTimeLeft = 0.0f;

		int updateFrames = 0; // to allow to be at right place

		void update_placements_if_required();
		void update_placements(bool _forced = false);
		void update_curves();

		Transform calculate_relative_placement(float _addPt = 0.0f) const;

		void break_pt(float _pt, OUT_ int & _i, OUT_ float & _t) const;
	};

	class ModuleMovementSwitchData
	: public ModuleMovementInteractivePartData
	{
		FAST_CAST_DECLARE(ModuleMovementSwitchData);
		FAST_CAST_BASE(ModuleMovementInteractivePartData);
		FAST_CAST_END();
		typedef ModuleMovementInteractivePartData base;
	public:
		ModuleMovementSwitchData(LibraryStored* _inLibraryStored);

	public: // ModuleData
		override_ bool read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
		override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	protected:
		ModuleMovementSwitchSetup setup;

		friend class ModuleMovementSwitch;
	};
};

