#pragma once

#include "..\..\..\framework\appearance\appearanceController.h"
#include "..\..\..\framework\appearance\appearanceControllerData.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\memory\safeObject.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\core\types\hand.h"

#include "..\..\..\framework\modulesOwner\modulesOwner.h"

namespace TeaForGodEmperor
{
	namespace AppearanceControllersLib
	{
		class PilgrimsGrabbingHandData;

		/**
		 */
		class PilgrimsGrabbingHand
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(PilgrimsGrabbingHand);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			PilgrimsGrabbingHand(PilgrimsGrabbingHandData const * _data);
			virtual ~PilgrimsGrabbingHand();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("grabbing hand"); }

		private:
			PilgrimsGrabbingHandData const * pilgrimsGrabbingHandData;
			Meshes::BoneID bone;
			Meshes::BoneID parent;

			SafePtr<Framework::IModulesOwner> grabbedObject;
			Optional<Transform> handRelativeToGrabbedPlacement; // relative to grabbed

			float grabbingActive = 0.0f; // instead of normal active
			Transform grabbedPlacementOS = Transform::identity;

			Vector3 lastGrabPointRequestedGrabDirOS;

#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
			Hand::Type hand = Hand::Left;
#endif
#endif
		};

		class PilgrimsGrabbingHandData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(PilgrimsGrabbingHandData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			PilgrimsGrabbingHandData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_transform(Matrix44 const & _transform);
			override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);

			override_ Framework::AppearanceControllerData* create_copy() const;
			override_ Framework::AppearanceController* create_controller();

		private:
			Framework::MeshGenParam<Name> bone;
			Hand::Type hand = Hand::Left;

			float grabbingTime = 0.1f;

			static Framework::AppearanceControllerData* create_data();

			friend class PilgrimsGrabbingHand;
		};
	};
};
