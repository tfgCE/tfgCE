#include "pose.h"

#include "boneID.h"
#include "poseManager.h"
#include "skeleton.h"

using namespace Meshes;

Pose* Pose::get_one(Skeleton const * _skeleton)
{
	return PoseManager::get_pose(_skeleton);
}

void Pose::release()
{
	PoseManager::release_pose(this);
}

Pose::Pose(int _numBones)
:
#ifdef AN_ASSERT
numBones(_numBones),
#endif
  prevPose(nullptr)
, nextPose(nullptr)
{
	bones.set_size(_numBones);
}

Pose::~Pose()
{
	an_assert(validate_num_bones());
	an_assert(prevPose == nullptr);
	an_assert(nextPose == nullptr);
}

void Pose::blend_in(float _blendInWeight, Pose const* _blendInPose)
{
	an_assert(get_skeleton() == _blendInPose->get_skeleton());

	auto* dest = bones.get_data();
	auto* blendInBone = _blendInPose->bones.get_data();
	for_count(int, boneIdx, bones.get_size())
	{
		*dest = lerp(_blendInWeight, *dest, *blendInBone);
		++blendInBone;
		++dest;
	}
}

void Pose::fill_with(Pose* _pose)
{
	if (_pose)
	{
		fill_with(_pose->get_bones());
	}
}

void Pose::fill_with(Array<Transform> const & _bones)
{
	an_assert(bones.get_size() == _bones.get_size());
	memory_copy(bones.get_data(), _bones.get_data(), _bones.get_data_size());
#ifndef AN_CLANG
	an_assert(is_ok());
#endif
}

void Pose::disconnect()
{
	if (prevPose)
	{
		prevPose->nextPose = nextPose;
	}
	if (nextPose)
	{
		nextPose->prevPose = prevPose;
	}
	prevPose = nullptr;
	nextPose = nullptr;
}

void Pose::connect_next(Pose* _pose)
{
	an_assert(nextPose == nullptr);
	an_assert(_pose == nullptr || _pose->prevPose == nullptr);
	nextPose = _pose;
	if (nextPose)
	{
		nextPose->prevPose = this;
	}
}

void Pose::ls_to_ms_from(Pose const * _source, Skeleton const * _skeleton)
{
#ifndef AN_CLANG
	an_assert(_source->is_ok());
#endif
	_skeleton->change_bones_ls_to_ms(_source->bones, bones);
#ifndef AN_CLANG
	an_assert(is_ok());
#endif
}

Transform Pose::get_bone_ms_from_ls(int _boneIndex) const
{
	an_assert(bones.is_index_valid(_boneIndex));
	Transform result = Transform::identity;
	int boneIdx = _boneIndex;
	while (boneIdx != NONE)
	{
#ifndef AN_CLANG
		an_assert(bones[boneIdx].get_orientation().is_normalised(), TXT("not normalised: %.6f (diff:%.6f) : %S"), bones[boneIdx].get_orientation().length(), abs(bones[boneIdx].get_orientation().length() - 1.0f), bones[boneIdx].get_orientation().to_string().to_char());
		an_assert(result.get_orientation().is_normalised(), TXT("not normalised: %.6f (diff:%.6f) : %S"), result.get_orientation().length(), abs(result.get_orientation().length() - 1.0f), result.get_orientation().to_string().to_char());
#endif
		result = bones[boneIdx].to_world(result);
		result.set_orientation(result.get_orientation().normal()); // make it sane
		boneIdx = skeleton->get_parent_of(boneIdx);
	}
	return result;
}

Transform const & Pose::get_bone(int _boneIndex) const
{
	return bones.is_index_valid(_boneIndex)? bones[_boneIndex] : Transform::identity;
}

void Pose::set_bone(int _boneIndex, Transform const & _bone)
{
	if (bones.is_index_valid(_boneIndex))
	{
		bones[_boneIndex] = _bone;
#ifndef AN_CLANG
		an_assert(_bone.get_orientation().is_normalised(), TXT("not normalised: %.6f : %S"), _bone.get_orientation().length(), _bone.get_orientation().to_string().to_char());
#endif
	}
}

void Pose::set_bone_ms(int _boneIndex, Transform const & _boneMS)
{
	if (bones.is_index_valid(_boneIndex))
	{
		Transform parentMS = Transform::identity;
		int parentIdx = skeleton->get_parent_of(_boneIndex);
		{
			if (parentIdx != NONE)
			{
				parentMS = get_bone_ms_from_ls(parentIdx);
			}
		}

		bones[_boneIndex] = parentMS.to_local(_boneMS);
#ifndef AN_CLANG
		an_assert(_boneMS.get_orientation().is_normalised(), TXT("not normalised: %.6f : %S"), _boneMS.get_orientation().length(), _boneMS.get_orientation().to_string().to_char());
		an_assert(bones[_boneIndex].get_orientation().is_normalised(), TXT("not normalised: %.6f : %S"), bones[_boneIndex].get_orientation().length(), bones[_boneIndex].get_orientation().to_string().to_char());
#endif
	}
}

void Pose::to_matrix_array(OUT_ Array<Matrix44> & _bones) const
{
	_bones.set_size(bones.get_size());
	auto outBone = _bones.begin();
	for_every(bone, bones)
	{
		*outBone = bone->to_matrix();
		++outBone;
	}
}

bool Pose::is_ok() const
{
	for_every(bone, bones)
	{
		if (!bone->get_orientation().is_normalised())
		{
			return false;
		}
	}

	return true;
}
