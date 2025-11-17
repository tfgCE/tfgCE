#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\meshGen\meshGenParam.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class FootAdjusterData;

		/**
		 *	we do not alter any bone, we just adjust foot related variables
		 */
		class FootAdjuster
		: public AppearanceController
		{
			FAST_CAST_DECLARE(FootAdjuster);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			FootAdjuster(FootAdjusterData const * _data);
			virtual ~FootAdjuster();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("foot adjuster"); }

		private:
			FootAdjusterData const * footAdjusterData;
			bool isValid;
			Meshes::BoneID footBone;

			Transform footToBaseOffset = Transform::identity; // used to offset target placement (foot relative to base)
			Range2 footSize = Range2::zero; // in base socket space in XY plane
			Range2 rotationLimits = Range2::zero;
			Vector2 zeroLengthRotation = Vector2::zero;
			float fullRotationLimitsTrajectoryLength = 0.0f;

			bool lastRotationNonZero = false;
			Vector2 footMovementTrajectory = Vector2::zero; // if last rotation was non zero, we have to blend, otherwise we may immediately change dir

			SimpleVariableInfo plantStateVar;
			SimpleVariableInfo trajectoryMSVar;
			SimpleVariableInfo targetPlacementMSVar; // here, it points at the base of foot
			SimpleVariableInfo adjustedTargetPlacementMSVar; // this is where foot is, we rotate base properly and then we offset it by footToBaseOffet
		};

		class FootAdjusterData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(FootAdjusterData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			FootAdjusterData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_transform(Matrix44 const & _transform);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
			
			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> footBone;
			MeshGenParam<Name> footBaseSocket; // to learn how much we need to adjust target placement - this should not change during run time!
			Array<MeshGenParam<Name>> footTipSockets; // tips define size of a foot - this should not change during run time!

			MeshGenParam<Name> plantStateVar; // negative going to be placed, positive after being placed and going up, 0 when planted on the ground
			MeshGenParam<Name> trajectoryMSVar; // trajectory along which foot is moving to know how to rotate foot
			MeshGenParam<Name> targetPlacementMSVar;
			MeshGenParam<Name> adjustedTargetPlacementMSVar;

			MeshGenParam<Range2> footSize;
			MeshGenParam<Range2> rotationLimits = Range2::zero; // x -> roll, y -> pitch
			MeshGenParam<Vector2> zeroLengthRotation = Vector2::zero; // as above, length is smaller than fullRotationLimitsTrajectoryLength
			MeshGenParam<float> fullRotationLimitsTrajectoryLength = 0.0f; // at this distance rotation limits are fully incorporated

			static AppearanceControllerData* create_data();
				
			friend class FootAdjuster;
		};
	};
};
