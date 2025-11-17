#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class DragBoneUpData;

		/**
		 *	Drags to bone's previous location, useful for tails
		 *	Has to drag towards parent!
		 */
		class DragBoneUp
		: public AppearanceController
		{
			FAST_CAST_DECLARE(DragBoneUp);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			DragBoneUp(DragBoneUpData const * _data);
			virtual ~DragBoneUp();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("drag bone"); }

		private:
			DragBoneUpData const * dragBoneUpData;
			Meshes::BoneID bone;
			Meshes::BoneID toBone;
			Meshes::BoneID parentBone;

			Optional<Vector3> upDirBS;
			Optional<Vector3> rightDirBS;
			Vector3 forwardDirBS;

			Optional<float> angleLimitDot;
		};

		class DragBoneUpData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(DragBoneUpData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			DragBoneUpData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> bone;
			MeshGenParam<Name> toBone;

			MeshGenParam<Vector3> upDirBS;
			MeshGenParam<Vector3> rightDirBS;
			MeshGenParam<Vector3> forwardDirBS = Vector3::yAxis;

			MeshGenParam<float> angleLimit;

			static AppearanceControllerData* create_data();
				
			friend class DragBoneUp;
		};
	};
};
