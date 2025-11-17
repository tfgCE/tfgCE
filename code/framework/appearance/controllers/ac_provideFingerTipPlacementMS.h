#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"
#include "..\socketID.h"

#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\types\hand.h"
#include "..\..\..\core\vr\iVR.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class ProvideFingerTipPlacementMSData;

		/**
		 *	
		 */
		class ProvideFingerTipPlacementMS
		: public AppearanceController
		{
			FAST_CAST_DECLARE(ProvideFingerTipPlacementMS);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			ProvideFingerTipPlacementMS(ProvideFingerTipPlacementMSData const * _data);
			virtual ~ProvideFingerTipPlacementMS();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("provide placement for finger tip ms"); }

		private:
			ProvideFingerTipPlacementMSData const * provideFingerTipPlacementMSData;

			SimpleVariableInfo resultPlacementMSVar;

			Hand::Type hand;
			VR::VRFinger::Type finger;
			VR::VRHandBoneIndex::Type fingerTipVRHandBone;
			VR::VRHandBoneIndex::Type fingerBaseVRHandBone;

			Meshes::BoneID baseBone;
			Meshes::BoneID tipBone;
			float meshBaseToTipLength = 0.0f;
			float vrRefBaseToTipLength = 0.0f;
			Vector3 vrRefBaseToTip = Vector3::zero;

			Vector3 offsetLocationOS;

			void update_ref();
		};

		class ProvideFingerTipPlacementMSData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(ProvideFingerTipPlacementMSData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			ProvideFingerTipPlacementMSData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> resultPlacementMSVar;

			MeshGenParam<Name> hand;
			MeshGenParam<Name> finger;

			MeshGenParam<Name> baseBone;
			MeshGenParam<Name> tipBone;

			MeshGenParam<Vector3> offsetLocationOS = Vector3::zero;

			static AppearanceControllerData* create_data();
				
			friend class ProvideFingerTipPlacementMS;
		};
	};
};
