#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\sound\soundSource.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\remapValue.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class AtPlacementData;

		namespace AtPlacementType
		{
			enum Type
			{
				BS,
				MS,
				WS
			};

			inline Type parse(String const & _val, Type _default = BS)
			{
				if (String::compare_icase(_val, TXT("bs"))) { return BS; }
				if (String::compare_icase(_val, TXT("ms"))) { return MS; }
				if (String::compare_icase(_val, TXT("ws"))) { return WS; }
				return _default;
			}
		};

		/**
		 *	Lerps between two placements (with given type)
		 */
		class AtPlacement
		: public AppearanceController
		{
			FAST_CAST_DECLARE(AtPlacement);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			AtPlacement(AtPlacementData const * _data);
			virtual ~AtPlacement();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("at placement"); }

		private:
			AtPlacementData const * atPlacementData;
			Meshes::BoneID bone;
			SimpleVariableInfo variable; // appearance controller
			SimpleVariableInfo outputVariable;
			AtPlacementType::Type placementType = AtPlacementType::BS;
			float allowedDistance = 0.0f;
			Optional<Transform> placement0;
			Optional<Transform> placement1;
			SimpleVariableInfo placement0Var;
			SimpleVariableInfo placement1Var;
			float value0 = 0.0f;
			float value1 = 1.0f;
			float blendTime = 0.0f;
			float variableValue = 0.0f;
			bool locationOnly = false;
			bool orientationOnly = false;

			Name movementSound;
			float movementSoundEarlyStop = 0.0f;
			SoundSourcePtr playedMovementSound;
		};

		class AtPlacementData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(AtPlacementData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			AtPlacementData();

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
			MeshGenParam<Name> outputVariable; // write back
			AtPlacementType::Type placementType = AtPlacementType::BS;
			MeshGenParam<float> allowedDistance = 0.0f;
			MeshGenParam<Transform> placement0;
			MeshGenParam<Transform> placement1;
			MeshGenParam<Name> placement0Var;
			MeshGenParam<Name> placement1Var;
			MeshGenParam<float> value0 = 0.0f;
			MeshGenParam<float> value1 = 1.0f;
			MeshGenParam<float> blendTime = 0.0f;
			MeshGenParam<bool> locationOnly = false;
			MeshGenParam<bool> orientationOnly = false;

			RemapValue<float, float> remapVariable;

			MeshGenParam<Name> movementSound;
			MeshGenParam<float> movementSoundEarlyStop;

			Optional<BezierCurve<BezierCurveTBasedPoint<float>>> curve;
			Optional<float> curveAccuracy;

			static AppearanceControllerData* create_data();
				
			friend class AtPlacement;
		};
	};
};
