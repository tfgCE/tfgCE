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
		class IKSolver3Data;

		/**
		 *	
		 */
		class IKSolver3
		: public AppearanceController
		{
			FAST_CAST_DECLARE(IKSolver3);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			IKSolver3(IKSolver3Data const * _data);
			virtual ~IKSolver3();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("ik solver 3"); }

		private:
			enum BoneIndex
			{
				BoneParent,
				BoneFirstSegment,
				BoneSecondSegment,
				BoneThirdSegment,
				BoneTip,
				//
				BoneCount
			};
			IKSolver3Data const * ikSolver3Data;
			bool isValid;
			Meshes::BoneID bones[BoneCount];

			Vector3 lastRightMS = Vector3::zero;

			SimpleVariableInfo targetPlacementMSVar;
			SimpleVariableInfo straightSegmentVar; // 0 (first two segments 0&1 straight) to 1 (two last segments (1&2) straight)
		};

		class IKSolver3Data
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(IKSolver3Data);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			IKSolver3Data();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> tipBone;
			MeshGenParam<float> bendFirstJoint; // positive or negative
			MeshGenParam<float> bendSecondJoint;
			bool restrictTargetPlacementToBendPlane; // without restriction we may turn leg
			bool allowStretching = false;

			MeshGenParam<Name> targetPlacementMSVar;
			MeshGenParam<Name> straightSegmentVar;

			static AppearanceControllerData* create_data();
				
			friend class IKSolver3;
		};
	};
};
