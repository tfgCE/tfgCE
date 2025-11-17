#pragma once

#include "..\..\..\framework\appearance\appearanceController.h"
#include "..\..\..\framework\appearance\appearanceControllerData.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace TeaForGodEmperor
{
	namespace AppearanceControllersLib
	{
		class CentipedeBackPlateData;

		class CentipedeBackPlate
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(CentipedeBackPlate);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			CentipedeBackPlate(CentipedeBackPlateData const * _data);
			virtual ~CentipedeBackPlate();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("centipedeBackPlate"); }

		private:
			CentipedeBackPlateData const * centipedeBackPlateData;
			
			Meshes::BoneID bone;
			
			SafePtr<Framework::IModulesOwner> towardsIMO;
			Framework::SocketID towardsSocket;

			Vector3 targetOS = Vector3(0.0f, -10.0f, 0.0f);
		};

		class CentipedeBackPlateData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(CentipedeBackPlateData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			CentipedeBackPlateData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);

			override_ Framework::AppearanceControllerData* create_copy() const;
			override_ Framework::AppearanceController* create_controller();

		private:
			Framework::MeshGenParam<Name> bone;
			
			Framework::MeshGenParam<Name> towardsSocket;

			static AppearanceControllerData* create_data();
				
			friend class CentipedeBackPlate;
		};
	};
};
