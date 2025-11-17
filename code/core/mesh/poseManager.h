#pragma once

#include "..\containers\arrayStatic.h"
#include "..\concurrency\spinLock.h"

namespace Meshes
{
	struct Pose;
	class Skeleton;

	class PoseManager
	{
	public:
		static void initialise_static();
		static void close_static();

		static Pose* get_pose(Skeleton const * _skeleton);
		static void release_pose(Pose* _pose);

	private:
		PoseManager();
		~PoseManager();

	private:
		static PoseManager* s_poseManager;
		static int const MAX_BONES = 256;

		// by numBones
		// todo - if needed, extend to have inner array for tracks?
		ArrayStatic<Pose*, MAX_BONES> usedPoses;
		ArrayStatic<Pose*, MAX_BONES> freePoses;

		Concurrency::SpinLock poseManagementLock = Concurrency::SpinLock(TXT("Meshes.PoseManager.poseManagementLock"));

		void prune();
	};

};
