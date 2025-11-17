#pragma once

#include "..\meshGenElementBone.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace Bones
		{
			void add_bones();

			/** params:
			 *		placement			Transform	placement of the bone
			 */
			bool bone_start(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementBoneData const * _data, int _boneIdx);
			bool bone_end(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementBoneData const * _data, int _boneIdx);
			ElementBoneData* create_bone_data();

		};
	};
};
