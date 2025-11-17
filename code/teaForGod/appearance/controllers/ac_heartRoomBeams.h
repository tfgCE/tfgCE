#pragma once

#include "..\..\..\framework\appearance\appearanceController.h"
#include "..\..\..\framework\appearance\appearanceControllerData.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\memory\safeObject.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\core\types\hand.h"

#include "..\..\..\framework\modulesOwner\modulesOwner.h"

#include "..\..\ai\logics\aiLogic_heartRoomBeams.h"

namespace TeaForGodEmperor
{
	namespace AppearanceControllersLib
	{
		class HeartRoomBeamsData;

		/**
		 *	Works tightly with heart room beams logic, grabs bone info from there and just sets it here
		 */
		class HeartRoomBeams
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(HeartRoomBeams);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			HeartRoomBeams(HeartRoomBeamsData const * _data);
			virtual ~HeartRoomBeams();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("heart room beams"); }

		private:
			Meshes::BoneID rotatingBone;

			Array<AI::Logics::HeartRoomBeams::BeamInfo> beams;
		};

		class HeartRoomBeamsData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(HeartRoomBeamsData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			HeartRoomBeamsData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);

			override_ Framework::AppearanceControllerData* create_copy() const;
			override_ Framework::AppearanceController* create_controller();

		private:
			Framework::MeshGenParam<Name> rotatingBone;

			static Framework::AppearanceControllerData* create_data();

			friend class HeartRoomBeams;
		};
	};
};
