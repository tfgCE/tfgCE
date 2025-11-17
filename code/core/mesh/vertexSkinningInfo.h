#pragma once

#include "..\containers\arrayStatic.h"

namespace Meshes
{
	struct VertexSkinningInfo
	{
		static int const MAX_BONE = 4;
		struct Bone
		{
			int bone = NONE;
			float weight = 0.0f;

			Bone() {}
			Bone(int _bone, float _weight) : bone(_bone), weight(_weight) {}
		};
		ArrayStatic<Bone, MAX_BONE> bones;

		VertexSkinningInfo() { SET_EXTRA_DEBUG_INFO(bones, TXT("VertexSkinningInfo.bones")); }

		bool is_empty() const { return bones.is_empty(); }

		void add(Bone const & _bone) { if (_bone.weight > 0.0f && _bone.bone != NONE) { bones.push_back(_bone); } }
		void normalise();

		bool operator==(VertexSkinningInfo const & _other) const;

		static VertexSkinningInfo blend(VertexSkinningInfo const & _a, VertexSkinningInfo const & _b, float _weight); // _weight a (0.0) to b (1.0)
	};
};
