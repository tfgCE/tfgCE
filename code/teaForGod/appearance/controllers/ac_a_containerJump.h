#pragma once

#include "..\..\..\framework\appearance\appearanceController.h"
#include "..\..\..\framework\appearance\appearanceControllerData.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace TeaForGodEmperor
{
	namespace AppearanceControllersLib
	{
		class A_ContainerJumpData;

		class A_ContainerJump
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(A_ContainerJump);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			A_ContainerJump(A_ContainerJumpData const * _data);
			virtual ~A_ContainerJump();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("a_containerJump"); }

		private:
			A_ContainerJumpData const * a_containerJumpData;
			
			Meshes::BoneID bone;
			
			SimpleVariableInfo jumpVar;

			Range interval = Range(0.0f);
			float maxDistance = 0.0f;
			float maxYaw = 0.0f;
			Range jumpTime = Range(0.0f);

			Name makeSound;

			float timeLeftToJump = 0.0f;

			Transform jumpDest = Transform::identity;
			Transform jumpAt = Transform::identity;
			float jumpTimeLeft = 0.0f;
		};

		class A_ContainerJumpData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(A_ContainerJumpData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			A_ContainerJumpData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);

			override_ Framework::AppearanceControllerData* create_copy() const;
			override_ Framework::AppearanceController* create_controller();

		private:
			Framework::MeshGenParam<Name> bone;

			Framework::MeshGenParam<Name> jumpVar; // if var is set, jumps

			Framework::MeshGenParam<Range> interval = Range(0.5f, 2.0f);
			Framework::MeshGenParam<float> maxDistance = 0.2f;
			Framework::MeshGenParam<float> maxYaw = 2.0f;
			Framework::MeshGenParam<Range> jumpTime = Range(0.15f, 0.25f);

			Framework::MeshGenParam<Name> makeSound;

			static AppearanceControllerData* create_data();
				
			friend class A_ContainerJump;
		};
	};
};
