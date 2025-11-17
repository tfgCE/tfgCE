#pragma once

#include "walkers.h"

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class Walker;
		class WalkerData;

		struct WalkerLegData
		{
			MeshGenParam<Name> targetPlacementMSVar;
			MeshGenParam<Name> disableVar; // set this var to true to disable leg (to operate it manually or to not operate it at all)
			MeshGenParam<Name> trajectoryMSVar; // this is fed to other systems, this is the trajectory along which the foot will move (check food adjustor).

			Walkers::LegSetup legSetup;

			WalkerLegData();

			bool load_from_xml(IO::XML::Node const * _node, IO::XML::Node const* _mainNode, LibraryLoadingContext & _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
			bool bake(Walkers::Lib const & _lib);
			void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
		};

		struct WalkerLeg
		{
			Walker * walker;
			WalkerLegData const * data;
			SimpleVariableInfo targetPlacementMSVar;
			SimpleVariableInfo disableVar;
			SimpleVariableInfo trajectoryMSVar;

			void initialise(WalkerLegData const * _data, Walker* _walker);
			void update_vars(Walkers::LegInstance const & _legInstance);
		};

		/**
		 *	
		 */
		class Walker
		: public AppearanceController
		{
			FAST_CAST_DECLARE(Walker);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			Walker(WalkerData const * _data);
			virtual ~Walker();

			Array<WalkerLeg> const & get_legs() const { return legs; }
			Walkers::Instance const & get_walker_instance() const { return walkerInstance; }

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("walker"); }

		private:
			WalkerData const * walkerData;
			Array<WalkerLeg> legs;

			SimpleVariableInfo shouldFallVar;

			Walkers::Instance walkerInstance;
			Walkers::InstanceContext walkerInstanceContext;

			struct VRMovement
			{
				bool active = false;
				float timeToUpdate = 0.0f; // we update on intervals
				float accumulatedInterval = 0.0f;
				// placements are in vr space
				// using them we calculate relative velocity and turn speed
				Optional<Transform> vrPlacementPrev; 
				Optional<Transform> vrPlacementCurr;
				// output
				Optional<Vector3> relativeVelocityLinear;
				Optional<float> relativeMovementDirection;
				Optional<Rotator3> velocityOrientation;
			} vrMovement;

			void update_leg_variables_input();

			void update_leg_variables(bool _recalculateLegPlacements);

			void update_gait_vars(Name const & _gaitName);
		};

		class WalkerData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(WalkerData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			WalkerData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			Array<WalkerLegData> legs;
			Walkers::Lib lib;
			Walkers::InstanceSetup walkerSetup;
			Walkers::ChooseGait chooseGait;
			Walkers::ProvideAllowedGait provideAllowedGait;
			Array<Walkers::LockGait> lockGaits;
			Array<Walkers::GaitVar> gaitVars;
			MeshGenParam<Name> shouldFallVar;

			static AppearanceControllerData* create_data();
				
			friend class Walker;
		};
	};
};
