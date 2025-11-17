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
		class LagPlacementMSData;

		/**
		 *	Adds lag effect to placement MS variable
		 */
		class LagPlacementMS
		: public AppearanceController
		{
			FAST_CAST_DECLARE(LagPlacementMS);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			LagPlacementMS(LagPlacementMSData const * _data);
			virtual ~LagPlacementMS();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("lag placement ms"); }

		private:
			LagPlacementMSData const * lagPlacementMSData;

			SimpleVariableInfo placementMSVar;

			SocketID refSocket;

			float useMovement;
			float movementBlendTime;
			float lagEffect;
			Vector3 maxDistance; // if zero, not in use, translates distance to maxDistance * (1.0 - 1.0 / (1.0 + (distance / maxDistance) * maxDistanceCoef))
			Vector3 maxDistanceCoef;

			Transform lastPlacementMS;
		};

		class LagPlacementMSData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(LagPlacementMSData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			LagPlacementMSData();

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
			MeshGenParam<float> movementBlendTime; // if 0, only uses lag effect
			MeshGenParam<float> lagEffect;
			MeshGenParam<Vector3> maxDistance;
			MeshGenParam<Vector3> maxDistanceCoef;

			static AppearanceControllerData* create_data();
				
			friend class LagPlacementMS;
		};
	};
};
