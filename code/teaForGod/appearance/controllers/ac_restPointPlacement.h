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
		class RestPointPlacementData;

		/**
		 *	calculates placement between socket in us and in other object
		 */
		class RestPointPlacement
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(RestPointPlacement);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			RestPointPlacement(RestPointPlacementData const * _data);
			virtual ~RestPointPlacement();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("rest point placement"); }

		private:
			RestPointPlacementData const * restPointPlacementData;
			
			Framework::SocketID handHoldPointSocket;
			Framework::SocketID restHoldPointSocket;
			Framework::SocketID restPointSocket;
			Framework::SocketID restPointIdleSocket;
			SimpleVariableInfo handObjectVar;
			SimpleVariableInfo equipmentObjectVar;
			Framework::SocketID equipmentHoldPointSocket;
			Framework::SocketID equipmentRestPointSocket;
			SimpleVariableInfo restPointPlacementMSVar;
			SimpleVariableInfo restTVar;
			SimpleVariableInfo restEmptyVar;

			float atPortRestT; // rest
			float atPortHoldT; // hold
			Vector3 atPortRestOffset; // rest
			Vector3 atPortHoldOffset; // hold
			Vector3 atPortHoldEntryDir; // hold

			Transform holdToRestPointInEquipment = Transform::identity; // to keep it if weapon is lost

			// just for reference
			Framework::IModulesOwner* lastHand = nullptr;
			Framework::Mesh const * lastHandMesh = nullptr;
			Framework::IModulesOwner* lastEquipment = nullptr;
			Framework::Mesh const * lastEquipmentMesh = nullptr;
		};

		class RestPointPlacementData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(RestPointPlacementData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			RestPointPlacementData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_transform(Matrix44 const & _transform);
			override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);

			override_ Framework::AppearanceControllerData* create_copy() const;
			override_ Framework::AppearanceController* create_controller();

		private:
			Framework::MeshGenParam<Name> handHoldPointSocket; // hand hold point in "hand"
			Framework::MeshGenParam<Name> restHoldPointSocket; // where in use should we place a gun
			Framework::MeshGenParam<Name> restPointSocket; // what is our rest point - our connector to a gun
			Framework::MeshGenParam<Name> restPointIdleSocket; // where rest point should be when idling
			Framework::MeshGenParam<Name> handObjectVar; // pointer at "hand" object - to get current placement of unfolded port
			Framework::MeshGenParam<Name> equipmentObjectVar; // pointer at "main equipment" object - to get rest point to port
			Framework::MeshGenParam<Name> equipmentHoldPointSocket; // hold point socket in equipment
			Framework::MeshGenParam<Name> equipmentRestPointSocket; // hold point socket in equipment
			Framework::MeshGenParam<Name> restPointPlacementMSVar; // output - where we store actual rest point placement
			Framework::MeshGenParam<Name> restTVar; // input - where between rest and hand hold points are we
			Framework::MeshGenParam<Name> restEmptyVar; // input - is rest point empty?

			Framework::MeshGenParam<float> atPortRestT; // rest
			Framework::MeshGenParam<float> atPortHoldT; // hold
			Framework::MeshGenParam<Vector3> atPortRestOffset; // rest
			Framework::MeshGenParam<Vector3> atPortHoldOffset; // hold
			Framework::MeshGenParam<Vector3> atPortHoldEntryDir; // hold

			static Framework::AppearanceControllerData* create_data();
				
			friend class RestPointPlacement;
		};
	};
};
