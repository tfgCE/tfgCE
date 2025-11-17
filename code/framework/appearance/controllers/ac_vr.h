#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"
#include "..\socketID.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class VRData;

		/**
		 *	
		 */
		class VR
		: public AppearanceController
		{
			FAST_CAST_DECLARE(VR);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			VR(VRData const * _data);
			virtual ~VR();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("vr"); }

		private:
			VRData const * vrData;
			bool isValid;
			int activeForFrames = 0;
			Meshes::BoneID headBone;
			SocketID headBoneLocationFromSocket;
			Transform headVRViewOffset = Transform::identity;
			Meshes::BoneID handLeftBone;
			Meshes::BoneID handRightBone;
			Meshes::BoneID headParentBone;
			Meshes::BoneID handLeftParentBone;
			Meshes::BoneID handRightParentBone;
			// only used by non-vr player
			Transform handLeftNonVRPlayerOffset = Transform::identity;
			Transform handRightNonVRPlayerOffset = Transform::identity;
			Transform additionalHandLeftNonVRPlayerOffset = Transform::identity;
			Transform additionalHandRightNonVRPlayerOffset = Transform::identity;

			Transform lastHeadBoneOS = Transform::identity;
			Transform lastHandLeftBoneOS = Transform::identity;
			Transform lastHandRightBoneOS = Transform::identity;

			Transform relativeToHeadBoneOS = Transform::identity; // flattened (pitch+roll)
			SimpleVariableInfo relativeForwardDirYawOSVar;
			SimpleVariableInfo relativeForwardDirRollOSVar; // to allow upside down

			Rotator3 offsetProvidedDir = Rotator3::zero;
			Rotator3 offsetProvidedDirForVR = Rotator3::zero;
			SimpleVariableInfo provideForwardDirMSVar;
			SimpleVariableInfo provideTargetPointMSVar;
		};

		class VRData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(VRData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			VRData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> headBone;
			MeshGenParam<Transform> headVRViewOffset = Transform::identity; // head in view space
			MeshGenParam<Name> handLeftBone;
			MeshGenParam<Name> handRightBone;
			// only used by non-vr player
			MeshGenParam<Transform> handLeftNonVRPlayerOffset = Transform::identity;
			MeshGenParam<Transform> handRightNonVRPlayerOffset = Transform::identity;

			MeshGenParam<Name> headBoneLocationFromSocket; // if set, won't do translation by design

			bool doTranslation = true;
			bool doOrientation = true;

			// if we want to use controller to provide direction only
			bool skipSettingBones = false;
			
			// otherwise we will be using rotation as it is stored - this is useful when we want to point at the right rotation always (using relative look) or when doing controller twice
			// it is irrelevant for vr
			bool useAbsoluteRotation = false;

			// the below are useful if we switch to an object and we want to face in a specific direction straight away)
			bool relativeInOSToInitialHeadPlacementOnActivation = false; // with this on, all placement is relative in object space to initial head placement on activation, don't use with translation
			MeshGenParam<Name> relativeForwardDirYawOSVar; // will use var as a relative dir (if we want the main placement to be backward)
			MeshGenParam<Name> relativeForwardDirRollOSVar; // for upside down
			 
			MeshGenParam<Rotator3> offsetProvidedDir;
			MeshGenParam<Rotator3> offsetProvidedDirForVR;
			MeshGenParam<Name> provideForwardDirMSVar;
			MeshGenParam<Name> provideTargetPointMSVar;

			static AppearanceControllerData* create_data();
				
			friend class VR;
		};
	};
};
