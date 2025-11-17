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
		class RotateTowardsObjectData;

		/**
		 *	Rotates towards a specific placement.
		 *	This is useful for scripting, where we provide only tags for object we match placement.
		 */
		class RotateTowardsObject
		: public AppearanceController
		{
			FAST_CAST_DECLARE(RotateTowardsObject);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			RotateTowardsObject(RotateTowardsObjectData const * _data);
			virtual ~RotateTowardsObject();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("at placement"); }

		private:
			RotateTowardsObjectData const * rotateTowardsObjectData;
			Meshes::BoneID bone;
			// uses one of these, if none provided, matches full location
			TagCondition toObjectTagged;
			SafePtr<IModulesOwner> toObject;
			Meshes::BoneID toBone;
			SocketID toSocket;
			Vector3 toOffset = Vector3::zero;
			Vector3 toDir = Vector3::zero;
		};

		class RotateTowardsObjectData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(RotateTowardsObjectData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			RotateTowardsObjectData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> bone;
			Axis::Type dirAxis = Axis::Forward;
			Axis::Type keepAxis = Axis::Right;
			MeshGenParam<TagCondition> toObjectTagged;
			MeshGenParam<Name> toBone;
			MeshGenParam<Vector3> toOffset;
			MeshGenParam<Vector3> toDir;
			MeshGenParam<Name> toSocket;

			static AppearanceControllerData* create_data();
				
			friend class RotateTowardsObject;
		};
	};
};
