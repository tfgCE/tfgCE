#pragma once

#include "..\..\..\framework\appearance\appearanceController.h"
#include "..\..\..\framework\appearance\appearanceControllerData.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\sound\soundSource.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace TeaForGodEmperor
{
	namespace AppearanceControllersLib
	{
		class PilgrimageEnvironmentRotationData;

		/**
		 *	provides pilgrimage instances' environment rotation
		 */
		class PilgrimageEnvironmentRotation
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(PilgrimageEnvironmentRotation);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			PilgrimageEnvironmentRotation(PilgrimageEnvironmentRotationData const * _data);
			virtual ~PilgrimageEnvironmentRotation();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("rest point placement"); }

		private:
			PilgrimageEnvironmentRotationData const * pilgrimageEnvironmentRotationData;
			
			SimpleVariableInfo asRotatorVar;
			bool negate = false;
		};

		class PilgrimageEnvironmentRotationData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(PilgrimageEnvironmentRotationData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			PilgrimageEnvironmentRotationData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);

			override_ Framework::AppearanceControllerData* create_copy() const;
			override_ Framework::AppearanceController* create_controller();

		private:
			Framework::MeshGenParam<Name> asRotatorVar;
			bool negate = false;

			static Framework::AppearanceControllerData* create_data();
				
			friend class PilgrimageEnvironmentRotation;
		};
	};
};
