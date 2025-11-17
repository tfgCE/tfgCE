#pragma once

#include "..\..\appearanceController.h"
#include "..\..\appearanceControllerData.h"

#include "..\..\..\..\core\mesh\boneID.h"

namespace Framework
{
	class ModuleAppearance;

	namespace AppearanceControllersLib
	{
		namespace Components
		{
			struct KeepChildrenWhereTheyWereData;

			struct KeepChildrenWhereTheyWere
			{
				KeepChildrenWhereTheyWere(KeepChildrenWhereTheyWereData const & _data);

				void initialise(ModuleAppearance* _owner, Meshes::BoneID const & _bone);
				
				void preform_pre_bone_changes(REF_ AppearanceControllerPoseContext & _context);
				void preform_post_bone_changes(REF_ AppearanceControllerPoseContext & _context);

			private:
				bool keepChildrenWhereTheyWere = false;
				Meshes::BoneID bone;
				Transform preChangesBoneMS;
				Array<Meshes::BoneID> childrenBones;
			};

			struct KeepChildrenWhereTheyWereData
			{
				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			private:
				MeshGenParam<bool> keepChildrenWhereTheyWere;

				friend struct KeepChildrenWhereTheyWere;
			};
		};
	};
};
