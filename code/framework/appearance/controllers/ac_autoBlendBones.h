#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\sound\soundSource.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class AutoBlendBonesData;

		/**
		 *	Blends two poses over time
		 *	- stores pose every frame
		 *	- monitors one bone to decide whether to blend
		 *	- if detected, blends last valid pose with the current one (and monitors new pose)
		 */
		class AutoBlendBones
		: public AppearanceController
		{
			FAST_CAST_DECLARE(AutoBlendBones);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			AutoBlendBones(AutoBlendBonesData const * _data);
			virtual ~AutoBlendBones();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("blend bones"); }

		private:
			Meshes::BoneID bone;
			Meshes::BoneID monitorBone;
			float linearSpeed = 2.0f;
			float rotationSpeed = 120.0f;
			SimpleVariableInfo linearSpeedVar;
			SimpleVariableInfo rotationSpeedVar;
			SimpleVariableInfo blendTimeVar;

			SimpleVariableInfo outAtTargetVar;

			struct AutoBlendBone
			{
				bool inUse = false;
				Transform blendStartPoseLS;
				Transform poseLS;
			};
			Array<AutoBlendBone> bones;

			Optional<Transform> lastMonitorBonePoseMS;
			float blendTimeLeft = 0.0f;
			float currentBlendTime = 0.0f; // to know the whole duration to calculate pt correctly

			Name blendCurve;

			Name sound;
			SoundSourcePtr playedSound;
		};

		class AutoBlendBonesData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(AutoBlendBonesData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			AutoBlendBonesData();

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
			MeshGenParam<Name> monitorBone;

			MeshGenParam<Name> outAtTargetVar;

			MeshGenParam<float> linearSpeed = 2.0f;
			MeshGenParam<float> rotationSpeed = 120.0f;
			MeshGenParam<Name> linearSpeedVar;
			MeshGenParam<Name> rotationSpeedVar;
			MeshGenParam<Name> blendTimeVar;

			MeshGenParam<Name> blendCurve;

			MeshGenParam<Name> sound;

			static AppearanceControllerData* create_data();
				
			friend class AutoBlendBones;
		};
	};
};
