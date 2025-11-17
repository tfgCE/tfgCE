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
		class CombineVariablesData;

		/**
		 *	gets one or two variables to combine them into one result
		 *	may do multiple steps/combinations
		 */
		class CombineVariables
		: public AppearanceController
		{
			FAST_CAST_DECLARE(CombineVariables);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			enum Operation
			{
				And,
				Or,
				Not
			};
		public:
			CombineVariables(CombineVariablesData const * _data);
			virtual ~CombineVariables();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("combine variables"); }

		private:
			CombineVariablesData const * combineVariablesData;

			struct Combine
			{
				SimpleVariableInfo aVar;
				SimpleVariableInfo bVar;
				SimpleVariableInfo outVar;
				CombineVariables::Operation operation = CombineVariables::Operation::And;
			};
			Array<Combine> combine;
		};

		class CombineVariablesData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(CombineVariablesData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			CombineVariablesData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_transform(Matrix44 const & _transform);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			struct Combine
			{
				MeshGenParam<Name> aVar;
				MeshGenParam<Name> bVar;
				MeshGenParam<Name> outVar;
				CombineVariables::Operation operation = CombineVariables::Operation::And;
			};
			Array<Combine> combine;

			static AppearanceControllerData* create_data();
				
			friend class CombineVariables;
		};
	};
};
