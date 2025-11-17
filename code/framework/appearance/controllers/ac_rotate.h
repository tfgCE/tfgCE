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
		class RotateData;

		/**
		 *	Just rotate
		 */
		class Rotate
		: public AppearanceController
		{
			FAST_CAST_DECLARE(Rotate);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			Rotate(RotateData const * _data);
			virtual ~Rotate();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("rotate"); }

		private:
			RotateData const * rotateData;
			bool relativeToDefault = false;
			bool ignoreOwnerRotation = false;
			Meshes::BoneID bone;
			float speed;
			Optional<Rotator3> rotationDir;
			Optional<float> angleLimit;
			SimpleVariableInfo rotatorVariable;
			SimpleVariableInfo quatVariable;

			Quat currentRotation = Quat::identity;
		};

		class RotateData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(RotateData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			RotateData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			bool relativeToDefault = false;
			bool ignoreOwnerRotation = false; // only if rotationDir is active

			MeshGenParam<Name> bone;
			MeshGenParam<Rotator3> rotationDir;
			SimpleVariableInfo rotatorVariable;
			SimpleVariableInfo quatVariable;

			MeshGenParam<float> speed = 0.0f;
			MeshGenParam<float> angleLimit;

			static AppearanceControllerData* create_data();
				
			friend class Rotate;
		};
	};
};
