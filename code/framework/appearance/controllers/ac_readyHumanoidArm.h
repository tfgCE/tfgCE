#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class ReadyHumanoidArmData;

		/**
		 *	
		 */
		class ReadyHumanoidArm
		: public AppearanceController
		{
			FAST_CAST_DECLARE(ReadyHumanoidArm);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			ReadyHumanoidArm(ReadyHumanoidArmData const * _data);
			virtual ~ReadyHumanoidArm();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("ready humanoid arm"); }

		private:
			ReadyHumanoidArmData const * readyHumanoidArmData;

			Meshes::BoneID armBone;
			Meshes::BoneID handBone;
			AUTO_ Meshes::BoneID forearmBone;
			Meshes::BoneID handControlBone;

			SimpleVariableInfo outUpDirMSVar;
			SimpleVariableInfo allowStretchVar;
			SimpleVariableInfo disallowStretchVar;
			SimpleVariableInfo outStretchArmVar;
			SimpleVariableInfo outStretchForearmVar;
			SimpleVariableInfo outRequestedLengthXYPtVar;

			float maxStretchedLengthPt;
			float startStretchingAtPt;

			bool right = true;
		};

		class ReadyHumanoidArmData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(ReadyHumanoidArmData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			ReadyHumanoidArmData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
			override_ void apply_transform(Matrix44 const & _transform);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> armBone;
			MeshGenParam<Name> handBone;
			MeshGenParam<Name> handControlBone;
			MeshGenParam<Name> outUpDirMSVar;
			MeshGenParam<Name> allowStretchVar;
			MeshGenParam<Name> disallowStretchVar;
			MeshGenParam<Name> outStretchArmVar;
			MeshGenParam<Name> outStretchForearmVar;
			MeshGenParam<Name> outRequestedLengthXYPtVar;
			MeshGenParam<float> maxStretchedLengthPt;
			MeshGenParam<float> startStretchingAtPt;

			String rightPrefix;
			String leftPrefix;

			static AppearanceControllerData* create_data();
				
			friend class ReadyHumanoidArm;
		};
	};
};
