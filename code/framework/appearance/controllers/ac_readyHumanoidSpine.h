#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\remapValue.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class ReadyHumanoidSpineData;

		/**
		 *	
		 */
		class ReadyHumanoidSpine
		: public AppearanceController
		{
			FAST_CAST_DECLARE(ReadyHumanoidSpine);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			ReadyHumanoidSpine(ReadyHumanoidSpineData const * _data);
			virtual ~ReadyHumanoidSpine();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("ready humanoid spine"); }

		private:
			ReadyHumanoidSpineData const * readyHumanoidSpineData;

			Meshes::BoneID pelvisBone;
			Meshes::BoneID headBone;
			Meshes::BoneID rightHandBone;
			Meshes::BoneID leftHandBone;
			AUTO_ Meshes::BoneID rightArmBone;
			AUTO_ Meshes::BoneID leftArmBone;
			Meshes::BoneID rightHandControlBone;
			Meshes::BoneID leftHandControlBone;

			Vector3 offsetHandRight;

			SimpleVariableInfo outPelvisMSVar;
			SimpleVariableInfo outTopSpineMSVar;

			SimpleVariableInfo rightArmRequestedLengthXYPtVar;
			SimpleVariableInfo leftArmRequestedLengthXYPtVar;

			Transform spineToHeadOffset;
			SimpleVariableInfo spineToHeadOffsetVar;

			float useDirFromHead = 0.5f;
			float useDirFromPelvis = 1.5f;
			float useDirFromHands = 2.2f;
			float minRequestedLengthXYPtToUseDirFromHands = 0.9f;
			float maxRequestedLengthXYPtToUseDirFromPelvis = 0.4f;

			RUNTIME_ float actualUseDirFromHead = 0.0f; // will blend
			RUNTIME_ float actualUseDirFromPelvis = 0.0f; // will blend
			RUNTIME_ float actualUseDirFromHands = 0.0f; // will blend
		};

		class ReadyHumanoidSpineData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(ReadyHumanoidSpineData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			ReadyHumanoidSpineData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
			override_ void apply_transform(Matrix44 const & _transform);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> pelvisBone;
			MeshGenParam<Name> headBone;
			MeshGenParam<Name> rightHandBone;
			MeshGenParam<Name> leftHandBone;
			MeshGenParam<Name> rightHandControlBone;
			MeshGenParam<Name> leftHandControlBone;

			MeshGenParam<Name> outPelvisMSVar;
			MeshGenParam<Name> outTopSpineMSVar;

			MeshGenParam<Name> rightArmRequestedLengthXYPtVar;
			MeshGenParam<Name> leftArmRequestedLengthXYPtVar;

			MeshGenParam<Vector3> offsetHandRight = Vector3(2.0f, 0.0f, 0.0f);

			// one of those should be used
			MeshGenParam<Transform> spineToHeadOffset = Transform::identity;
			MeshGenParam<Name> spineToHeadOffsetVar;

			MeshGenParam<float> useDirFromHead = 0.5f;
			MeshGenParam<float> useDirFromPelvis = 1.5f;
			MeshGenParam<float> useDirFromHands = 2.2f;
			MeshGenParam<float> minRequestedLengthXYPtToUseDirFromHands = 0.9f;
			MeshGenParam<float> maxRequestedLengthXYPtToUseDirFromPelvis = 0.4f;

			RemapValue<float, Vector3> pelvisLocFromHeadLocZPt; // z is relative to head pt, y is absolute
			RemapValue<float, Vector3> pelvisLocOffsetFromHeadAxisYZ; // offset
			RemapValue<float, Rotator3> pelvisRotFromHeadLocZPt;
			
			// will keep distance
			RemapValue<float, Vector3> spineLocOffsetFromHeadLocZPt; // offset
			RemapValue<float, Vector3> spineLocOffsetFromHeadAxisYZ; // offset

			static AppearanceControllerData* create_data();
				
			friend class ReadyHumanoidSpine;
		};
	};
};
