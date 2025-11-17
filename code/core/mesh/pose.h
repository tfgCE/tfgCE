#pragma once

#include "..\containers\array.h"
#include "..\math\math.h"

namespace Meshes
{
	class PoseManager;
	class Skeleton;

	struct BoneID;

	struct Pose
	{
	public:
		static Pose* get_one(Skeleton const * _skeleton);
		void release();

		Skeleton const * get_skeleton() const { return skeleton; }

		void ls_to_ms_from(Pose const * _source, Skeleton const * _skeleton);

		Transform get_bone_ms_from_ls(int _boneIndex) const;
		Transform const & get_bone(int _boneIndex) const;
		void set_bone(int _boneIndex, Transform const & _bone);
		void set_bone_ms(int _boneIndex, Transform const & _boneMS);

		Array<Transform> & access_bones() { an_assert(validate_num_bones()); return bones; }
		Array<Transform> const & get_bones() const { an_assert(validate_num_bones()); return bones; }

		void blend_in(float _blendInWeight, Pose const* _blendInPose);

		void fill_with(Pose* _pose);
		void fill_with(Array<Transform> const & _bones);

		inline int get_num_bones() const { return bones.get_size(); }

		void to_matrix_array(OUT_ Array<Matrix44> & _bones) const;

		bool operator==(Pose const & _other) const { return skeleton == _other.skeleton && bones == _other.bones; }
		bool operator!=(Pose const & _other) const { return skeleton != _other.skeleton || bones != _other.bones; }
		Pose& operator =(Pose const& _other) { an_assert(skeleton == _other.skeleton); bones = _other.bones; return *this; }

		bool is_ok() const;

	private:
#ifdef AN_ASSERT
		int numBones; // to check
#endif
		Array<Transform> bones;

#ifdef AN_ASSERT
		inline bool validate_num_bones() const { return bones.get_size() == numBones; }
#else
		inline bool validate_num_bones() const { return true; }
#endif

	private: friend class PoseManager;
		Skeleton const * skeleton;
		Pose* prevPose;
		Pose* nextPose;

		Pose(int _numBones);
		~Pose();

		void disconnect();
		void connect_next(Pose* _pose);

	private:
		Pose(Pose const & _other); // do not implement_
	};

	struct PoseMatBonesRef
	{
		PoseMatBonesRef(): skeleton(nullptr), bones(nullptr) {}
		PoseMatBonesRef(PoseMatBonesRef const & _source) : skeleton(_source.skeleton), bones(_source.bones) {}
		PoseMatBonesRef(Skeleton* _skeleton, Array<Matrix44> const & _bones) : skeleton(_skeleton), bones(&_bones) {}
		void operator=(PoseMatBonesRef const & _source) { skeleton = _source.skeleton; bones = _source.bones; }

		bool is_set() const { return skeleton != nullptr && bones != nullptr; }

		Skeleton* const get_skeleton() const { return skeleton; }
		Array<Matrix44> const & get_bones() const { return *bones; }

	private:
		Skeleton* skeleton = nullptr;
		Array<Matrix44> const * bones = nullptr;
	};
};
