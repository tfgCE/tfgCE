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
		class SwayData;
		class Walker;

		namespace SwayTo
		{
			enum Type
			{
				Walker, // legs from walker
				DefaultPlacement
			};
		};

		/**
		 *	Sways body basing on walker controller state
		 */
		class Sway
		: public AppearanceController
		{
			FAST_CAST_DECLARE(Sway);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			Sway(SwayData const * _data);
			virtual ~Sway();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("sway"); }

		private:
			SwayData const * swayData;
			Meshes::BoneID bone;

			SwayTo::Type swayTo = SwayTo::Walker;

			RefCountObjectPtr<Walker> walker;

			float legSwayAngle = 10.0f;
			float legSwayDistance = 0.0f;
			float legSwayVerticalDistance = 0.0f;

			float angleBlendTime = 0.2f;
			float distanceBlendTime = 0.2f;

			Rotator3 swayRotationBS = Rotator3::zero;
			Vector3 swayLocationBS = Vector3::zero;
		};

		class SwayData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(SwayData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			SwayData();

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

			SwayTo::Type swayTo = SwayTo::Walker;

			MeshGenParam<float> legSwayAngle = 10.0f; // how big is impact from single leg (positive will turn up)
			MeshGenParam<float> legSwayDistance = 0.0f; // how big is impact from single leg in plane perpendicular to vertical (positive will pull towards leg)
			MeshGenParam<float> legSwayVerticalDistance = 0.0f; // this is vertical only (postive is up)

			MeshGenParam<float> angleBlendTime = 0.2f;
			MeshGenParam<float> distanceBlendTime = 0.2f;

			static AppearanceControllerData* create_data();
				
			friend class Sway;
		};
	};
};
