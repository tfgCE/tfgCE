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
		class AtBoneData;

		namespace AtBoneType
		{
			enum Type
			{
				BS,
				MS
			};

			inline Type parse(String const & _val, Type _default = BS)
			{
				if (String::compare_icase(_val, TXT("bs"))) { return BS; }
				if (String::compare_icase(_val, TXT("ms"))) { return MS; }
				return _default;
			}
		};

		/**
		 *	Lerps between two placements (with given type)
		 */
		class AtBone
		: public AppearanceController
		{
			FAST_CAST_DECLARE(AtBone);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			AtBone(AtBoneData const * _data);
			virtual ~AtBone();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("at bone"); }

		private:
			AtBoneData const * atBoneData;
			Meshes::BoneID bone;
			Meshes::BoneID atBone;
			bool locationOnly = false;
		};

		class AtBoneData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(AtBoneData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			AtBoneData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> bone;
			MeshGenParam<Name> atBone;
			MeshGenParam<bool> locationOnly = false;

			static AppearanceControllerData* create_data();
				
			friend class AtBone;
		};
	};
};
