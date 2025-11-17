#pragma once

#include "..\containers\arrayStack.h"
#include "..\mesh\pose.h"

#include "collisionFlags.h"

#include "iCollidableObject.h"

namespace Collision
{

	struct CheckCollisionContext
	{
	public:
		typedef std::function<bool(ICollidableObject const * _object)> CheckObject;

	public:
		inline CheckCollisionContext();

		inline bool should_check(ICollidableObject const * _object) const;
		inline void avoid(ICollidableObject const * _object);
		inline void use_check_object(CheckObject _checkObject) { an_assert(!checkObject); checkObject = _checkObject; }

		inline bool is_collision_info_needed() const { return collisionInfoNeeded; }
		inline void collision_info_needed() { collisionInfoNeeded = true; }
		inline void collision_info_not_needed() { collisionInfoNeeded = false; }

		inline Flags const & get_collision_flags() const { return flags; }
		inline void use_collision_flags(Flags const & _flags) { flags = _flags; }

		inline Meshes::PoseMatBonesRef const & get_pose_mat_bones_ref() const { return poseMatBonesRef; }
		inline void use_pose_mat_bones_ref(Meshes::PoseMatBonesRef const & _poseMatBonesRef) { an_assert(!poseMatBonesRef.is_set()); poseMatBonesRef = _poseMatBonesRef; }
		inline void clear_pose_mat_bones_ref() { poseMatBonesRef = Meshes::PoseMatBonesRef(); }

		inline bool should_ignore_reversed_normal() const { return ignoreReversedNormal; }
		inline void ignore_reversed_normal(bool _ignoreReversedNormal = true) { ignoreReversedNormal = _ignoreReversedNormal; }

	private:
		Flags flags = Flags::all();
		ArrayStatic<ICollidableObject const *, 16> avoids;
		bool collisionInfoNeeded = false; // don't provide info if it's not requested!
		CheckObject checkObject = nullptr;

		bool ignoreReversedNormal = false; // if hit into something with reversed normal, will be ignored (might be used for shields)

		Meshes::PoseMatBonesRef poseMatBonesRef;
	};

};

#include "checkCollisionContext.inl"
