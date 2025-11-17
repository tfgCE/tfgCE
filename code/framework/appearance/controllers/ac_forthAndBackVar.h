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
		class ForthAndBackVarData;

		/**
		 *	changes value from one to another in given time
		 */
		class ForthAndBackVar
		: public AppearanceController
		{
			FAST_CAST_DECLARE(ForthAndBackVar);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			ForthAndBackVar(ForthAndBackVarData const* _data);
			virtual ~ForthAndBackVar();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext& _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext& _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int>& dependsOnBones, OUT_ ArrayStack<int>& dependsOnParentBones, OUT_ ArrayStack<int>& usesBones, OUT_ ArrayStack<int>& providesBones,
				OUT_ ArrayStack<Name>& dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const* get_name_for_log() const { return TXT("forth and back var"); }

		private:
			Random::Generator rg;

			bool firstUpdate = true;

			SimpleVariableInfo var;
			SimpleVariableInfo keepAt0Var;
			SimpleVariableInfo keepAt1Var;

			float value0 = 0.0f;
			float value1 = 1.0f;

			Range to0Time;
			Range to1Time;

			float currentToTime = 0.0f;

			Name playSoundAt0;
			Name playSoundAt1;
			Name playSoundMoving;

			float atPt = 1.0f;
			float targetPt = 1.0f;
			int movingDir = 0;
		};

		class ForthAndBackVarData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(ForthAndBackVarData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			ForthAndBackVarData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_transform(Matrix44 const & _transform);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> var;
			MeshGenParam<Name> keepAt0Var;
			MeshGenParam<Name> keepAt1Var;
			
			MeshGenParam<float> value0 = 0.0f;
			MeshGenParam<float> value1 = 1.0f;

			MeshGenParam<Range> to0Time = Range(1.0f);
			MeshGenParam<Range> to1Time = Range(1.0f);

			MeshGenParam<Name> playSoundAt0;
			MeshGenParam<Name> playSoundAt1;
			MeshGenParam<Name> playSoundMoving;

			static AppearanceControllerData* create_data();
				
			friend class ForthAndBackVar;
		};
	};
};
