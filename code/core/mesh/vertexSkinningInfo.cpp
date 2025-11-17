#include "vertexSkinningInfo.h"

#include "..\utils.h"

using namespace Meshes;

bool VertexSkinningInfo::operator==(VertexSkinningInfo const & _other) const
{
	if (bones.get_size() != _other.bones.get_size())
	{
		return false;
	}
	for_every(bone, bones)
	{
		bool exists = false;
		for_every(otherBone, _other.bones)
		{
			if (bone->bone == otherBone->bone &&
				bone->weight == otherBone->weight)
			{
				exists = true;
				break;
			}
		}
		if (!exists)
		{
			return false;
		}
	}
	return true;
}

VertexSkinningInfo VertexSkinningInfo::blend(VertexSkinningInfo const & _a, VertexSkinningInfo const & _b, float _weight)
{
	VertexSkinningInfo result = _a;
	float invWeight = 1.0f - _weight;
	for_every(bone, result.bones)
	{
		an_assert(bone->bone != NONE);
		bone->weight *= invWeight;
	}
	for_every(boneB, _b.bones)
	{
		an_assert(boneB->bone != NONE);
		bool found = false;
		for_every(boneResult, result.bones)
		{
			if (boneB->bone == boneResult->bone)
			{
				boneResult->weight += boneB->weight * _weight;
				found = true;
				break;
			}
		}
		if (!found)
		{
			if (result.bones.get_size() < result.bones.get_max_size())
			{
				result.add(Meshes::VertexSkinningInfo::Bone(boneB->bone, boneB->weight * _weight));
			}
			else
			{
				error(TXT("can't contain more bones!"));
			}
		}
	}

	result.normalise();

	return result;
}

void VertexSkinningInfo::normalise()
{
	if (bones.is_empty())
	{
		return;
	}
	float weightSum = 0.0f;
	for_every(bone, bones)
	{
		weightSum += bone->weight;
	}
	if (weightSum != 0.0f)
	{
		float const weightMultiplier = 1.0f / weightSum;
		for_every(bone, bones)
		{
			bone->weight *= weightMultiplier;
		}
	}
	else
	{
		float const weights = 1.0f / (float)bones.get_size();
		for_every(bone, bones)
		{
			bone->weight = weights;
		}
	}
}
