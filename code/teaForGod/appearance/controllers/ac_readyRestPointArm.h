#pragma once

#include "..\..\..\framework\appearance\appearanceController.h"
#include "..\..\..\framework\appearance\appearanceControllerData.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\sound\soundSource.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace TeaForGodEmperor
{
	namespace AppearanceControllersLib
	{
		class ReadyRestPointArmData;

		/**
		 *	prepares stuff for rest point arm's ik
		 */
		class ReadyRestPointArm
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(ReadyRestPointArm);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			ReadyRestPointArm(ReadyRestPointArmData const * _data);
			virtual ~ReadyRestPointArm();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("ready rest point arm"); }

		private:
			ReadyRestPointArmData const * readyRestPointArmData;
			
			Meshes::BoneID tipBone;
			AUTO_ Meshes::BoneID forearmBone;
			AUTO_ Meshes::BoneID armBone;
			Vector3 normalUpDirMS = Vector3::zAxis;
			Vector3 belowUpDirMS = Vector3::zAxis;
			SimpleVariableInfo targetPlacementMSVar;
			SimpleVariableInfo outUpDirMSVar;
			Range zRangeBelowToNormal;

			SimpleVariableInfo outStretchArmVar;
			SimpleVariableInfo outStretchForearmVar;
			float maxStretchedLengthPt;
			float startStretchingAtPt;

			float emptyArmStretchCoef;
			float emptyForearmStretchCoef;

			float stretchArm = 0.0f;
			float stretchForearm = 0.0f;

			SimpleVariableInfo restEmptyVar;
		};

		class ReadyRestPointArmData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(ReadyRestPointArmData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			ReadyRestPointArmData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_transform(Matrix44 const & _transform);
			override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);

			override_ Framework::AppearanceControllerData* create_copy() const;
			override_ Framework::AppearanceController* create_controller();

		private:
			Framework::MeshGenParam<Name> tipBone;
			Framework::MeshGenParam<Name> targetPlacementMSVar;
			Framework::MeshGenParam<Vector3> normalUpDirMS;
			Framework::MeshGenParam<Vector3> belowUpDirMS;
			Framework::MeshGenParam<Name> outUpDirMSVar;
			Framework::MeshGenParam<Range> zRangeBelowToNormal;

			Framework::MeshGenParam<Name> outStretchArmVar;
			Framework::MeshGenParam<Name> outStretchForearmVar;
			Framework::MeshGenParam<float> maxStretchedLengthPt;
			Framework::MeshGenParam<float> startStretchingAtPt;

			Framework::MeshGenParam<Name> restEmptyVar; // input - is rest point empty?

			Framework::MeshGenParam<float> emptyArmStretchCoef;
			Framework::MeshGenParam<float> emptyForearmStretchCoef;

			static Framework::AppearanceControllerData* create_data();
				
			friend class ReadyRestPointArm;
		};
	};
};
