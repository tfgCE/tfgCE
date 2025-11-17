#include "poseManager.h"
#include "pose.h"
#include "skeleton.h"

//

using namespace Meshes;

//

PoseManager* PoseManager::s_poseManager = nullptr;

void PoseManager::initialise_static()
{
	an_assert(s_poseManager == nullptr);
	s_poseManager = new PoseManager();
}

void PoseManager::close_static()
{
	an_assert(s_poseManager);
	delete s_poseManager;
	s_poseManager = nullptr;
}

PoseManager::PoseManager()
{
	SET_EXTRA_DEBUG_INFO(usedPoses, TXT("PoseManager.usedPoses"));
	SET_EXTRA_DEBUG_INFO(freePoses, TXT("PoseManager.freePoses"));

	while (usedPoses.has_place_left())
	{
		usedPoses.push_back(nullptr);
	}
	while (freePoses.has_place_left())
	{
		freePoses.push_back(nullptr);
	}
}

PoseManager::~PoseManager()
{
	for_every_ptr(usedPose, usedPoses)
	{
		an_assert(usedPose == nullptr, TXT("this pose is stil in use!"));
	}
	prune();
}

void PoseManager::prune()
{
	for_every(freePose, freePoses)
	{
		Pose* pose = *freePose;
		*freePose = nullptr;
		while (pose)
		{
			Pose* nextPose = pose->nextPose;
			pose->disconnect();
			delete pose;
			pose = nextPose;
		}
	}
}

Pose* PoseManager::get_pose(Skeleton const * _skeleton)
{
	an_assert(s_poseManager);
	s_poseManager->poseManagementLock.acquire(TXT("get pose"));

	Pose* newPose = nullptr;

	int numBones = _skeleton->get_num_bones();

	// get pose from free poses or create new one
	if (s_poseManager->freePoses.get_size() > numBones &&
		s_poseManager->freePoses[numBones])
	{
		Pose*& freePose = s_poseManager->freePoses[numBones];
		newPose = freePose;
		freePose = freePose->nextPose;
		newPose->disconnect();
	}
	else
	{
		s_poseManager->poseManagementLock.release();
		newPose = new Pose(numBones);
		s_poseManager->poseManagementLock.acquire(TXT("get pose (new)"));
	}

	an_assert(s_poseManager->usedPoses.is_index_valid(numBones));

	Pose*& usedPose = s_poseManager->usedPoses[numBones];
	newPose->connect_next(usedPose);
	usedPose = newPose;

	s_poseManager->poseManagementLock.release();
	
	newPose->skeleton = _skeleton;

	return newPose;
}

void PoseManager::release_pose(Pose* _pose)
{
	an_assert(s_poseManager);
	s_poseManager->poseManagementLock.acquire(TXT("release"));

	int numBones = _pose->get_num_bones();

	// remove from usedPoses
	if (! _pose->prevPose)
	{
		an_assert(s_poseManager->usedPoses[numBones] == _pose);
		s_poseManager->usedPoses[numBones] = _pose->nextPose;
	}
	_pose->disconnect();

	an_assert(s_poseManager->freePoses.is_index_valid(numBones));

	Pose*& freePose = s_poseManager->freePoses[numBones];
	_pose->connect_next(freePose);
	freePose = _pose;

	s_poseManager->poseManagementLock.release();
}
