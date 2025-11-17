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
		class OffsetData;

		/**
		 *	Lerps between two sockets
		 */
		class Offset
		: public AppearanceController
		{
			FAST_CAST_DECLARE(Offset);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			Offset(OffsetData const * _data);
			virtual ~Offset();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("offset"); }

		private:
			OffsetData const * offsetData;
			Meshes::BoneID bone;

			SimpleVariableInfo offsetPlacementBSVar;
			Transform offsetPlacementBS = Transform::identity;
			Vector3 offsetLocationBS = Vector3::zero;
			Rotator3 offsetRotationBS = Rotator3::zero;
		};

		class OffsetData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(OffsetData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			OffsetData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_transform(Matrix44 const & _transform);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> bone;

			MeshGenParam<Name> offsetPlacementBSVar;
			MeshGenParam<Transform> offsetPlacementBS = Transform::identity;
			MeshGenParam<Vector3> offsetLocationBS = Vector3::zero;
			MeshGenParam<Rotator3> offsetRotationBS = Rotator3::zero;

			static AppearanceControllerData* create_data();
				
			friend class Offset;
		};
	};
};
