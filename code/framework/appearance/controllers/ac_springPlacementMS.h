#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"
#include "..\socketID.h"

#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class SpringPlacementMSData;

		/**
		 *	Adds spring effect to placement MS variable
		 */
		class SpringPlacementMS
		: public AppearanceController
		{
			FAST_CAST_DECLARE(SpringPlacementMS);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			SpringPlacementMS(SpringPlacementMSData const * _data);
			virtual ~SpringPlacementMS();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("spring placement ms"); }

		private:
			SpringPlacementMSData const * springPlacementMSData;

			SimpleVariableInfo placementMSVar;

			SocketID refSocket;

			float useMovement;
			float movementBlendTime;
			float dampEffect;

			// these are taken from movement and blended
			Vector3 velocityLinear;
			Rotator3 velocityOrientation;

			Transform lastPlacementMS;
		};

		class SpringPlacementMSData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(SpringPlacementMSData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			SpringPlacementMSData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
			override_ void apply_transform(Matrix44 const & _transform);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> placementMSVar;

			MeshGenParam<Name> refSocket;

			MeshGenParam<float> useMovement;
			MeshGenParam<float> movementBlendTime; // if 0, not used
			MeshGenParam<float> dampEffect;

			static AppearanceControllerData* create_data();
				
			friend class SpringPlacementMS;
		};
	};
};
