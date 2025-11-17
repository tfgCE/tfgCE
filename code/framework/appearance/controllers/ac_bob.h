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
		class BobData;
		class Walker;

		/**
		 *	Bobs body basing on bones from previous frame
		 */
		class Bob
		: public AppearanceController
		{
			FAST_CAST_DECLARE(Bob);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			Bob(BobData const * _data);
			virtual ~Bob();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("bob"); }

		private:
			float bobZ = 0.0f;
			float bobBlendTime = 0.05f;
			float bobAmount = 1.0f;

			struct ApplyToBone
			{
				Meshes::BoneID bone;
				float amount = 1.0f;
			};
			Array<ApplyToBone> applyToBones;

			struct Leg
			{
				Meshes::BoneID bone;
				Meshes::BoneID relToBone;
			};
			Array<Leg> legs;
		};

		class BobData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(BobData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			BobData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_transform(Matrix44 const & _transform);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<float> bobBlendTime;
			MeshGenParam<float> bobAmount;

			struct ApplyToBone
			{
				MeshGenParam<Name> bone;
				MeshGenParam<float> amount;
			};
			Array<ApplyToBone> applyToBones;

			struct Leg
			{
				MeshGenParam<Name> bone;
				MeshGenParam<Name> relToBone;
			};
			Array<Leg> legs;

			static AppearanceControllerData* create_data();
				
			friend class Bob;
		};
	};
};
