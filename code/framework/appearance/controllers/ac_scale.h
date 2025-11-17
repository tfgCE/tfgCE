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
		class ScaleData;

		/**
		 *	Just scales from scale0 to scale1 for variable going from value0 to value1
		 */
		class Scale
		: public AppearanceController
		{
			FAST_CAST_DECLARE(Scale);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			Scale(ScaleData const * _data);
			virtual ~Scale();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("scale"); }

		private:
			ScaleData const * scaleData;
			Meshes::BoneID bone;
			SimpleVariableInfo variable; // appearance controller
			Optional<float> initialValue;
			float value0 = 0.0f;
			float value1 = 1.0f;
			float scale0 = 0.0f;
			float scale1 = 1.0f;
			float blendTime = 0.0f;
			float delayTime = 0.0f;
			float variableValue = 0.0f;

			float delayedShouldBeAt = 0.0f;
			float delayedShouldBeAtTarget = 0.0f;
			float delayedTime = 0.0f;
		};

		class ScaleData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(ScaleData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			ScaleData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> bone;
			MeshGenParam<Name> variable; // appearance controller
			MeshGenParam<float> initialValue; // if not provided, uses "should be at"
			MeshGenParam<float> value0 = 0.0f;
			MeshGenParam<float> value1 = 1.0f;
			MeshGenParam<float> scale0 = 0.0f;
			MeshGenParam<float> scale1 = 1.0f;
			MeshGenParam<float> blendTime = 0.0f;
			MeshGenParam<float> delayTime = 0.0f;

			static AppearanceControllerData* create_data();
				
			friend class Scale;
		};
	};
};
