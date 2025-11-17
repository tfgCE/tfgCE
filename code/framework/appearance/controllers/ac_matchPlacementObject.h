#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"
#include "..\socketID.h"

#include "..\..\sound\soundSource.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\remapValue.h"
#include "..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\core\tags\tagCondition.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class MatchPlacementObjectData;

		/**
		 *	Matches location within given bounds (locally in own bone's space) to location of a different object (may be provided via tags)
		 *	This is useful for scripting, where we provide only tags for object we match placement.
		 */
		class MatchPlacementObject
		: public AppearanceController
		{
			FAST_CAST_DECLARE(MatchPlacementObject);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			MatchPlacementObject(MatchPlacementObjectData const * _data);
			virtual ~MatchPlacementObject();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("at placement"); }

		private:
			MatchPlacementObjectData const * matchPlacementObjectData;
			Meshes::BoneID bone;
			// uses one of these, if none provided, matches full location
			Optional<Vector3> alongBoneAxis;
			Optional<Plane> onBonePlane;
			bool toInitialPlacement;
			TagCondition toObjectTagged;
			SafePtr<IModulesOwner> toObject;
			Meshes::BoneID toBone;
			SocketID toSocket;
			Vector3 toOffset = Vector3::zero;
			float applyRotation = 1.0f;
			Optional<Transform> initialPlacementWS;
		};

		class MatchPlacementObjectData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(MatchPlacementObjectData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			MatchPlacementObjectData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> bone;
			MeshGenParam<Vector3> alongBoneAxis;
			MeshGenParam<Plane> onBonePlane;
			MeshGenParam<bool> toInitialPlacement; // instead of an object
			MeshGenParam<TagCondition> toObjectTagged;
			MeshGenParam<Name> toBone;
			MeshGenParam<Vector3> toOffset;
			MeshGenParam<Name> toSocket;
			MeshGenParam<float> applyRotation;

			static AppearanceControllerData* create_data();
				
			friend class MatchPlacementObject;
		};
	};
};
