#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\meshGen\meshGenParam.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class HandToLegsData;

		class HandToLegs
		: public AppearanceController
		{
			FAST_CAST_DECLARE(HandToLegs);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			HandToLegs(HandToLegsData const* _data);
			virtual ~HandToLegs();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext& _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext& _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int>& dependsOnBones, OUT_ ArrayStack<int>& dependsOnParentBones, OUT_ ArrayStack<int>& usesBones, OUT_ ArrayStack<int>& providesBones,
				OUT_ ArrayStack<Name>& dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const* get_name_for_log() const { return TXT("hand to legs"); }

		private:
			bool isValid = false;

			Vector3 offset = Vector3::zero;

			Meshes::BoneID bone;
			Meshes::BoneID provideBone;
			Meshes::BoneID relToBone;
			float offsetBlendTime = 0.0f;

			float useScaleFromLeg = 0.5f;
			float addLength = 0.0f;

			Axis::Type alongAxis = Axis::Forward; // this is for the hand to point along the bone
			Axis::Type awayAxis = Axis::Up; // this is for the hand to be faced palm towards the body, outer surface outwards
			Vector3 addOffsetToAlong = Vector3::zero;

			Vector3 diffMul = Vector3::one;
			Rotator3 diffRot = Rotator3::zero;
			Vector3 addOffset = Vector3::zero;
			
			Meshes::BoneID pelvisBone;
			Meshes::BoneID spineTopBone;
			float spineRadius = 0.0f;

			Meshes::BoneID legBone;
			Meshes::BoneID legRelToBone;
			Meshes::BoneID otherLegBone;
		};
		
		class HandToLegsData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(HandToLegsData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			HandToLegsData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
			override_ void apply_transform(Matrix44 const & _transform);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> bone; // hand
			MeshGenParam<Name> provideBone; // hand_c
			MeshGenParam<Name> relToBone; // shoulder
			MeshGenParam<float> offsetBlendTime;

			MeshGenParam<float> useScaleFromLeg; // using to scale arm with leg length / def leg length
			MeshGenParam<float> addLength; // add to final length used

			Axis::Type alongAxis = Axis::Forward; // this is for the hand to point along the bone
			Axis::Type awayAxis = Axis::Up; // this is for the hand to be faced palm towards the body, outer surface outwards
			MeshGenParam<Vector3> addOffsetToAlong; // will add to normalised value

			MeshGenParam<Vector3> diffMul; // how differences are exagerated
			MeshGenParam<Rotator3> diffRot; // rotate difference post mul
			MeshGenParam<Vector3> addOffset; // additional offset
			
			MeshGenParam<Name> pelvisBone;
			MeshGenParam<Name> spineTopBone;
			MeshGenParam<float> spineRadius; // to disallow going through spine

			MeshGenParam<Name> legBone; // leg we mirror - foot
			MeshGenParam<Name> legRelToBone; // thigh
			MeshGenParam<Name> otherLegBone; // the leg below us
			
			static AppearanceControllerData* create_data();
				
			friend class HandToLegs;
		};
	};
};
