#pragma once

#include "navTask_PathTask.h"

namespace Framework
{
	namespace Nav
	{
		class Mesh;

		namespace Tasks
		{
			/**
			 */
			class FindPath
			: public PathTask
			{
				FAST_CAST_DECLARE(FindPath);
				FAST_CAST_BASE(PathTask);
				FAST_CAST_END();

				typedef PathTask base;
			public:
				static FindPath* new_task(PlacementAtNode const & _start, PlacementAtNode const & _end, PathRequestInfo const & _requestInfo);

#ifdef AN_LOG_NAV_TASKS
			public: // Nav::Task
				override_ void log_internal(LogInfoContext& _log) const;
				override_ bool should_log() const { return true; }
#endif
			protected:
				implement_ void perform();

				implement_ void cancel_if_related_to(Room* _room);

				implement_ tchar const* get_log_name() const { return TXT("find path"); }

			private:
				FindPath(PlacementAtNode const & _start, PlacementAtNode const & _end, PathRequestInfo const & _requestInfo);

			private:
				PathRequestInfo requestInfo;

				PlacementAtNode start;
				PlacementAtNode end;

			private:
				struct PathNodeWorker
				{
					Node* node;
					Node::ConnectionSharedData* connectionData = nullptr;
					float dist;
					PathNodeWorker() {}
					explicit PathNodeWorker(Node* _node, Node::ConnectionSharedData* _connectionData = nullptr, float _dist = 0.0f) : node(_node), connectionData(_connectionData), dist(_dist) {}
				};
				bool add_path(PlacementAtNode const & _start, PlacementAtNode const & _end);
				void add_path_worker(ArrayStack<PathNodeWorker> & bestWay, float & bestDist, ArrayStack<PathNodeWorker> & currentWay, float & currentDist, PlacementAtNode const & _end) const;

			private:
				struct ThroughDoorWorker
				{
					DoorInRoom* throughDoor;
					float dist;
					ThroughDoorWorker() {}
					explicit ThroughDoorWorker(DoorInRoom* _throughDoor, float _dist = 0.0f) : throughDoor(_throughDoor), dist(_dist) {}
				};
				bool add_through_door(Room* _start, int _startNavGroup, Room* _end, int _endNavGroup, ArrayStack<DoorInRoom*> & throughDoors);
				void add_through_door_worker(Room* _current, int _currentNavGroup, Room* _end, Optional<int> const& _endNavGroup, Array<ThroughDoorWorker> & bestWay, float & bestDist, Array<ThroughDoorWorker> & currentWay, float & currentDist) const;
			};
		};
	};
};
