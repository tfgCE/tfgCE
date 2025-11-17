#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\remapValue.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class AdjustWalkerPlacementsData;

		/**
		 *	This is done via bones. Actual walker placement sockets should be placed exactly at these bones.
		 */
		class AdjustWalkerPlacements
		: public AppearanceController
		{
			FAST_CAST_DECLARE(AdjustWalkerPlacements);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			AdjustWalkerPlacements(AdjustWalkerPlacementsData const * _data);
			virtual ~AdjustWalkerPlacements();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("adjust walker placements"); }

		private:
			struct Adjust
			{
				struct Input
				{
					Meshes::BoneID useBoneMSLocZPt; // input value is bone's MS location Z percentage
				} input;
				struct WalkerPlacement
				{
					Meshes::BoneID bone;
					Meshes::BoneID parentBone;
					RemapValue<float, Vector3> offsetLoc;
					RemapValue<float, Rotator3> rotate;
				};
				Array<WalkerPlacement> walkerPlacements;
				struct ProvideDir
				{
					SimpleVariableInfo dirVar;
					RemapValue<float, Vector3> dir;
				};
				Array<ProvideDir> provideDirs;
			};
			Array<Adjust> adjust;

			struct WalkerPlacement
			{
				Meshes::BoneID bone;
				Meshes::BoneID parentBone;
			};
			Array<WalkerPlacement> allWalkerPlacements;

			struct OffsetCentre
			{
				Meshes::BoneID bone;
				float weight = 1.0f;
			};
			Array<OffsetCentre> offsetCentre;
			Optional<Vector3> defaultCentreMS;
		};

		class AdjustWalkerPlacementsData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(AdjustWalkerPlacementsData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			AdjustWalkerPlacementsData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			struct Adjust
			{
				struct Input
				{
					MeshGenParam<Name> useBoneMSLocZPt; // input value is bone's MS location Z percentage
				} input;
				struct WalkerPlacement
				{
					MeshGenParam<Name> bone;
					RemapValue<float, Vector3> offsetLoc;
					RemapValue<float, Rotator3> rotate;
				};
				Array<WalkerPlacement> walkerPlacements;
				struct ProvideDir
				{
					MeshGenParam<Name> dirVar;
					RemapValue<float, Vector3> dir;
				};
				Array<ProvideDir> provideDirs;
			};
			Array<Adjust> adjust;

			struct OffsetCentre
			{
				MeshGenParam<Name> bone;
				MeshGenParam<float> weight = 1.0f;
			};
			Array<OffsetCentre> offsetCentre;

			static AppearanceControllerData* create_data();
				
			friend class AdjustWalkerPlacements;
		};
	};
};
