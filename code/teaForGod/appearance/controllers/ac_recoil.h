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
		class RecoilData;

		/**
		 */
		class Recoil
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(Recoil);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			Recoil(RecoilData const * _data);
			virtual ~Recoil();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("recoil"); }

		private:
			RecoilData const * recoilData;
			Meshes::BoneID bone;
			Meshes::BoneID parent;

			SafePtr<Framework::IModulesOwner> pilgrimOwner;
			SafePtr<Framework::IModulesOwner> currentEquipment;
			float timeSinceLastShot = 0.0f;
			
			Optional<int> recoilDelay;
			float accumulatedRecoil = 0.0f;

			float recoilTimeLeft = 0.0f;
			float recoilAttackTimeLeft = 0.0f;

			float maxRecoilTime = 1.0f;
			float maxAccumulatedRecoilTime = 1.0f;
			float recoilAttackTimePt = 0.3f;
			float maxRecoilAttackTime = 0.1f;

			Range locationRecoil = Range(0.0f);
			Range orientationRecoil = Range(0.0f);

			Rotator3 orientationRecoilMask = Rotator3(1.0f, 1.0f, 1.0f);

			Range applyRecoilStrength = Range(0.0f);

			Transform currentRecoilTargetLS = Transform::identity; // blend into it with attack
			Transform currentRecoilLS = Transform::identity; // active recoil
		};

		class RecoilData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(RecoilData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			RecoilData();

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

			Framework::MeshGenParam<float> maxRecoilTime;
			Framework::MeshGenParam<float> maxAccumulatedRecoilTime;
			Framework::MeshGenParam<float> recoilAttackTimePt;
			Framework::MeshGenParam<float> maxRecoilAttackTime;

			Framework::MeshGenParam<Range> locationRecoil;
			Framework::MeshGenParam<Range> orientationRecoil;

			Framework::MeshGenParam<Rotator3> orientationRecoilMask;

			Framework::MeshGenParam<Range> applyRecoilStrength;

			static Framework::AppearanceControllerData* create_data();

			friend class Recoil;
		};
	};
};
