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
		class PilgrimsFakePointingData;

		/**
		 *	rotates down to make it easier to point at stuff
		 */
		class PilgrimsFakePointing
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(PilgrimsFakePointing);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			PilgrimsFakePointing(PilgrimsFakePointingData const * _data);
			virtual ~PilgrimsFakePointing();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("fake pointing"); }

		private:
			PilgrimsFakePointingData const * pilgrimsFakePointingData;
			Meshes::BoneID bone;

			float pointingActive = 0.0f; // instead of normal active

#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
			Hand::Type hand = Hand::Left;
#endif
#endif
		};

		class PilgrimsFakePointingData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(PilgrimsFakePointingData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			PilgrimsFakePointingData();

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

			float pointingTime = 0.1f;
			Vector3 pointingOffset = Vector3::zero;
			Rotator3 pointingRotation = Rotator3::zero;

			static Framework::AppearanceControllerData* create_data();

			friend class PilgrimsFakePointing;
		};
	};
};
