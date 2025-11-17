#pragma once

#include "moduleMovementInteractivePart.h"

namespace Framework
{
	class ModuleMovementDialData;

	/**
	 *	Requires Grabable module
	 */
	class ModuleMovementDial
	: public ModuleMovementInteractivePart
	{
		FAST_CAST_DECLARE(ModuleMovementDial);
		FAST_CAST_BASE(ModuleMovementInteractivePart);
		FAST_CAST_END();

		typedef ModuleMovementInteractivePart base;
	public:
		static RegisteredModule<ModuleMovement> & register_itself();

		ModuleMovementDial(IModulesOwner* _owner);

		int get_dials(OUT_ int& _startedAt, OUT_ int& _currentlyAt) const;
		int get_dial() const; // since started
		int get_absolute_dial() const;

	protected:	// Module
		override_ void setup_with(ModuleData const * _moduleData);
		override_ void activate();

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);

	private:
		ModuleMovementDialData const * moduleMovementDialData = nullptr;

		SafePtr<IModulesOwner> baseObject;

		float step = 22.5f; // should sum up to 360'

		float turn = 0.0f;
		float useTurnForDial = 0.0f;
		int lastDial = 0;
		int absoluteDialOffset = 0;
		
		float keepDeadZonedAt = 0.0f; // if slightly out of this range, will come back to it
		float deadZonePct = 0.2f; // percentage

		Optional<float> turnStartedAt;

#ifdef AN_DEVELOPMENT
		Optional<Transform> initialPlacementForDebug;
#endif

		Transform calculate_relative_placement() const;
	};

	class ModuleMovementDialData
	: public ModuleMovementInteractivePartData
	{
		FAST_CAST_DECLARE(ModuleMovementDialData);
		FAST_CAST_BASE(ModuleMovementInteractivePartData);
		FAST_CAST_END();
		typedef ModuleMovementInteractivePartData base;
	public:
		ModuleMovementDialData(LibraryStored* _inLibraryStored);

	public: // ModuleData
		override_ bool read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
		override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	protected:
		Name sound;

		friend class ModuleMovementDial;
	};
};

