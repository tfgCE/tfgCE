#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\math\math.h"

namespace Meshes
{
	struct Pose;
};

namespace Framework
{
	struct AppearanceControllerPoseContext
	{
	public:
		AppearanceControllerPoseContext();

		AppearanceControllerPoseContext & with_delta_time(float _deltaTime) { deltaTime = _deltaTime; return *this; }
		AppearanceControllerPoseContext & with_pose_LS(Meshes::Pose* _poseLS) { poseLS = _poseLS; return *this; }
		AppearanceControllerPoseContext & with_prev_pose_LS(Meshes::Pose* _poseLS) { prevPoseLS = _poseLS; return *this; }

	public:
		float get_delta_time() const { return deltaTime; }

		Meshes::Pose* access_pose_LS() const { an_assert(poseLS); return poseLS; }
		Meshes::Pose* access_prev_pose_LS() const { an_assert(prevPoseLS); return prevPoseLS; }

	public: // processing level management
		void reset_processing();

		int get_processing_order() const { return processingOrder; }
		void require_processing_order(int _processingOrder) { if (_processingOrder > processingOrder) { nextProcessingOrder = (nextProcessingOrder == NONE) ? _processingOrder : min(_processingOrder, nextProcessingOrder); } }

		bool is_more_processing_required() const { return nextProcessingOrder > processingOrder; }
		void start_next_processing() { processingOrder = nextProcessingOrder; nextProcessingOrder = NONE; }

	private:
		float deltaTime = 0.0f;
		Meshes::Pose* poseLS = nullptr;
		Meshes::Pose* prevPoseLS = nullptr;
		int processingOrder;
		int nextProcessingOrder;
	};
};
