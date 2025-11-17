#pragma once

#include "appearanceControllerPtr.h"

#include "..\..\core\fastCast.h"
#include "..\..\core\globalDefinitions.h"
#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\other\simpleVariableStorage.h"

template <typename Element> class ArrayStack;

namespace Meshes
{
	struct Pose;
};

namespace Framework
{
	class AppearanceControllerData;
	class ModuleAppearance;

	struct AppearanceControllerPoseContext;

	/**
	 *	Appearance controllers can be added in any order, what's more important is their "process at level" number.
	 *	ACs with lower PAT will be processed sooner, higher number means later processing.
	 *	This also means that ACs that may influence other ACs should have lower PAT, example
	 *		PAT  10	full body rotation
	 *		PAT  50	logic for legs (as long as it considers body rotation, if it works with default MS values, this doesn't matter)
	 *		PAT  75	adjusting torso up&down
	 *		PAT 100	IK for legs
	 */
	class AppearanceController
	: public RefCountObject
	{
		FAST_CAST_DECLARE(AppearanceController);
		FAST_CAST_END();
	public:
		AppearanceController(AppearanceControllerData const * _data);
		virtual ~AppearanceController();

		ModuleAppearance* get_owner() const { an_assert(owner);  return owner; }
		AppearanceControllerData* get_data() const { return data.get(); }

		bool is_initialised() const { return initialised; }

		bool should_hold() const { return shouldHold; }

		float get_active() const;
		float get_active_target() const { return activeTarget; }
		bool has_just_activated() const { return justActivated; }
		bool has_just_deactivated() const { return justDeactivated; }

		bool is_deactivating() const { return prevActive > active; }

		int get_process_at_level() const { return processAtLevel; }

	public:
		void log(LogInfoContext & _log) const; // just header and basic info
	protected:
		virtual tchar const * get_name_for_log() const = 0;
		virtual void internal_log(LogInfoContext & _log) const;

	public: // 
		virtual void initialise(ModuleAppearance* _owner);
		virtual void activate();

		virtual void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);

		virtual void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);

		virtual void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
			OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

	protected:
		void set_processing_order(int _processingOrder) { processingOrder = _processingOrder; }

	public:
		int get_processing_order() const { return processingOrder; }
		bool check_processing_order(REF_ AppearanceControllerPoseContext & _context); // returns true if should process now

	private:
		ModuleAppearance* owner = nullptr;
		RefCountObjectPtr<AppearanceControllerData> data;

		Array<SimpleVariableInfo> activeVars;
		Array<SimpleVariableInfo> blockActiveVars;
		Array<SimpleVariableInfo> holdVars;
		Name activeBlendCurve;
		float prevActive;
		float active;
		CACHED_ float activeTarget;
		SimpleVariableInfo outActiveVar;
		float activeBlendInTime;
		float activeBlendOutTime;
		SimpleVariableInfo activeBlendTimeVar;
		float activeMask;
		bool justActivated = false;
		bool justDeactivated = false;
		bool shouldHold = false;

		bool firstUpdate = false;
		bool initialised = false;

		int processAtLevel; // similar to order, but defined by user to separate certain controllers
		int processingOrder; // (auto calculated) order in which controllers are processed, lower the number, sooner it will be processed, starts at 0

		friend class ModuleAppearance;
	};
};
