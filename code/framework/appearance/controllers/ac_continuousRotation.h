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
		class ContinuousRotationData;

		class ContinuousRotation
		: public AppearanceController
		{
			FAST_CAST_DECLARE(ContinuousRotation);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			ContinuousRotation(ContinuousRotationData const * _data);
			virtual ~ContinuousRotation();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("continuous rotation"); }

		private:
			ContinuousRotationData const * continuousRotationData;
			Meshes::BoneID bone;
			SimpleVariableInfo rotationSpeedVariable;
			Rotator3 rotationSpeed;
			Rotator3 oscilationPeriod;
			Rotator3 oscilationAt;

			Name sound;
			SoundSourcePtr playedSound;

			Quat currentRotation = Quat::identity;
		};

		class ContinuousRotationData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(ContinuousRotationData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			ContinuousRotationData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> bone;
			MeshGenParam<Name> rotationSpeedVariable;

			MeshGenParam<Range> speed = Range(0.0f);
			MeshGenParam<Rotator3> rotationSpeed = Rotator3::zero;
			MeshGenParam<Range> oscilationPeriod = Range::zero;

			MeshGenParam<Name> sound;

			static AppearanceControllerData* create_data();
				
			friend class ContinuousRotation;
		};
	};
};
