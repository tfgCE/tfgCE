#pragma once

#include "..\..\..\framework\appearance\appearanceController.h"
#include "..\..\..\framework\appearance\appearanceControllerData.h"

#include "..\..\..\core\collision\collisionFlags.h"
#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace TeaForGodEmperor
{
	namespace AppearanceControllersLib
	{
		class ElevatorCartAttacherArmData;

		/**
		 *	rotates randomly around given axis
		 */
		class ElevatorCartAttacherArm
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(ElevatorCartAttacherArm);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			ElevatorCartAttacherArm(ElevatorCartAttacherArmData const * _data);
			virtual ~ElevatorCartAttacherArm();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("elevator cart rotating core"); }

		private:
			ElevatorCartAttacherArmData const * elevatorCartAttacherArmData;
			Meshes::BoneID attacherBone;
			Meshes::BoneID armBone;
			SimpleVariableInfo placementVar;

			Random::Generator rg;

			Optional<Transform> placementWS; // no moving attachments

			Range3 attachDirRange = Range3(Range(-1.0f, 1.0f), Range(-1.0f, 1.0f), Range(0.2f, 1.0f));
		};

		class ElevatorCartAttacherArmData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(ElevatorCartAttacherArmData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			ElevatorCartAttacherArmData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_transform(Matrix44 const & _transform);
			override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);

			override_ Framework::AppearanceControllerData* create_copy() const;
			override_ Framework::AppearanceController* create_controller();

		private:
			Framework::MeshGenParam<Name> attacherBone;
			Framework::MeshGenParam<Name> armBone;

			Framework::MeshGenParam<Name> placementVar;

			Framework::MeshGenParam<Range3> attachDirRange;

			Collision::Flags attachToFlags = Collision::Flags::none();

			static Framework::AppearanceControllerData* create_data();
				
			friend class ElevatorCartAttacherArm;
		};
	};
};
