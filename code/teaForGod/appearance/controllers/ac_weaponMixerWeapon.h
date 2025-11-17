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
		class WeaponMixerWeaponData;

		/**
		 *	item hovers in the bay
		 *	doesn't have to be weapon mixer but does extra stuff when working as its part
		 */
		class WeaponMixerWeapon
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(WeaponMixerWeapon);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			WeaponMixerWeapon(WeaponMixerWeaponData const * _data);
			virtual ~WeaponMixerWeapon();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("fake pointing"); }

		private:
			WeaponMixerWeaponData const * weaponMixerWeaponData;
			Meshes::BoneID bone;

			float swappingPartsChangeTimeLeft = 0.0f;
			float randomImpulseTimeLeft = 0.0f;
			
			Vector3 velocityLinear = Vector3::zero;
			Rotator3 velocityRotation = Rotator3::zero;
			
			Vector3 impulseLinear = Vector3::zero;
			Rotator3 impulseRotation = Rotator3::zero;

			Transform offset = Transform::identity;
		};

		class WeaponMixerWeaponData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(WeaponMixerWeaponData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			WeaponMixerWeaponData();

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

			Range swappingPartsChangeTime = Range(0.05f, 0.5f);
			Range randomImpulseTime = Range(1.0f, 6.0f);

			float backImpulsePower = 0.8f;
			float randomImpulsePower = 0.3f;

			float applyImpulsePower = 0.3f;

			float maxVelocityLinear = 0.2f;
			float maxVelocityRotation = 5.0f;

			float stopAccelerationLinear = 0.02f;
			float stopAccelerationRotation = 0.2f;

			float maxOffLinear = 0.03f;
			float maxOffRotation = 5.0f;

			static Framework::AppearanceControllerData* create_data();

			friend class WeaponMixerWeapon;
		};
	};
};
