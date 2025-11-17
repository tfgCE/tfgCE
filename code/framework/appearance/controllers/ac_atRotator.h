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
		class AtRotatorData;

		/**
		 *	Lerps between two rotators
		 */
		class AtRotator
		: public AppearanceController
		{
			FAST_CAST_DECLARE(AtRotator);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			AtRotator(AtRotatorData const * _data);
			virtual ~AtRotator();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("at rotator"); }

		private:
			AtRotatorData const * atRotatorData;
			bool relativeToDefault = false;
			Meshes::BoneID bone;
			SimpleVariableInfo variable; // appearance controller
			SimpleVariableInfo outputVariable;
			Rotator3 rotator0;
			Rotator3 rotator1;
			float value0 = 0.0f;
			float value1 = 1.0f;
			float blendTime = 0.0f;
			Name blendCurve;
			float variableValue = 0.0f;

			Name rotationSound;
			SoundSourcePtr playedRotationSound;

			float lastShouldBeAt;
		};

		class AtRotatorData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(AtRotatorData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			AtRotatorData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			bool relativeToDefault = false;
			MeshGenParam<Name> bone;
			MeshGenParam<Name> variable; // appearance controller
			MeshGenParam<Name> outputVariable; // write back
			MeshGenParam<Rotator3> rotator0;
			MeshGenParam<Rotator3> rotator1;
			MeshGenParam<float> value0 = 0.0f;
			MeshGenParam<float> value1 = 1.0f;
			MeshGenParam<float> blendTime = 0.0f;
			MeshGenParam<Name> blendCurve;

			MeshGenParam<Name> rotationSound;

			static AppearanceControllerData* create_data();
				
			friend class AtRotator;
		};
	};
};
