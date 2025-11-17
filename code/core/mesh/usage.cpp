#include "usage.h"

#include "..\serialisation\serialiser.h"

BEGIN_SERIALISER_FOR_ENUM(Meshes::Usage::Type, byte)
	ADD_SERIALISER_FOR_ENUM(0, Meshes::Usage::Static)
	ADD_SERIALISER_FOR_ENUM(1, Meshes::Usage::Skinned)
	ADD_SERIALISER_FOR_ENUM(2, Meshes::Usage::SkinnedToSingleBone)
END_SERIALISER_FOR_ENUM(Meshes::Usage::Type)
