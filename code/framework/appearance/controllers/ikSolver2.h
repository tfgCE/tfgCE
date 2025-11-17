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
		class IKSolver2Data;

		/**
		 *	
		 */
		class IKSolver2
		: public AppearanceController
		{
			FAST_CAST_DECLARE(IKSolver2);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			IKSolver2(IKSolver2Data const * _data);
			virtual ~IKSolver2();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("ik solver 2"); }

		private:
			enum BoneIndex
			{
				BoneParent,
				BoneFirstSegment,
				BoneSecondSegment,
				BoneTip,
				//
				BoneCount
			};
			IKSolver2Data const * ikSolver2Data;
			bool isValid;
			Meshes::BoneID bones[BoneCount];

			Vector3 lastRightMS = Vector3::zero;

			SimpleVariableInfo targetPlacementMSVar;
			SimpleVariableInfo upDirMSVar;
			Vector3 upDirMS;
			bool forcePreferRightDirMS;
			SimpleVariableInfo preferRightDirMSVar;
			Vector3 preferRightDirMS;

			Transform remapBone0;
			Transform remapBone1;
			Transform remapBone2;
			Transform remapBone0Inv;
			Transform remapBone1Inv;
			Transform remapBone2Inv;
		};

		class IKSolver2Data
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(IKSolver2Data);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			IKSolver2Data();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
			override_ void apply_transform(Matrix44 const & _transform);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> tipBone;
			MeshGenParam<float> bendJoint; // positive or negative
			bool restrictTargetPlacementToBendPlane; // without restriction we may turn leg

			MeshGenParam<Name> targetPlacementMSVar;

			MeshGenParam<Name> upDirMSVar;
			MeshGenParam<Vector3> upDirMS;

			MeshGenParam<bool> forcePreferRightDirMS;
			MeshGenParam<Name> preferRightDirMSVar;
			MeshGenParam<Vector3> preferRightDirMS;

			MeshGenParam<Transform> remapBone0;
			MeshGenParam<Transform> remapBone1;
			MeshGenParam<Transform> remapBone2;

			static AppearanceControllerData* create_data();
				
			friend class IKSolver2;
		};
	};
};
