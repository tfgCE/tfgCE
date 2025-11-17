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
		class AlignmentRollData;

		/**
		 *	We roll to match orientation of other bone, we can choose which other axis we prefer X or Z
		 *
		 *	But we can also use yaw or pitch for Y axis
		 *
		 */
		class AlignmentRoll
		: public AppearanceController
		{
			FAST_CAST_DECLARE(AlignmentRoll);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			AlignmentRoll(AlignmentRollData const * _data);
			virtual ~AlignmentRoll();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("alignment roll"); }

		private:
			AlignmentRollData const * alignmentRollData;
			Meshes::BoneID bone;
			Meshes::BoneID alignToBone;
			Meshes::BoneID alignToBoneB;
			float alignToBoneBWeight;
			Meshes::BoneID boneParent;

			Vector3 lastAlignedAxisMS = Vector3::zero;
			Optional<Transform> prevAlignBoneMS;
			float smoothAlignBoneTime;
		};

		class AlignmentRollData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(AlignmentRollData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			AlignmentRollData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> bone;
			MeshGenParam<Name> alignToBone;
			MeshGenParam<Name> alignToBoneB;
			MeshGenParam<float> alignToBoneBWeight;
			MeshGenParam<float> smoothAlignBoneTime;

			Axis::Type usingAxis = Axis::X;
			Axis::Type forYAxisKeep = Axis::Z;
			Axis::Type alignToAxis = Axis::X;
			float alignToAxisSign = 1.0f;

			Optional<Axis::Type> secondaryAlignToAxis;
			float secondaryAlignToAxisSign = 1.0f;

			bool maintainLastAlignedAxisSide = false; // will store last axis aligned and try to face it

#ifdef AN_DEVELOPMENT
			bool debugDraw = false;
#endif

			static AppearanceControllerData* create_data();
				
			friend class AlignmentRoll;
		};
	};
};
