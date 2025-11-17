#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\types\optional.h"
#include "..\nav\navPath.h"

struct Vector3;
struct Transform;

namespace Framework
{
	interface_class IModulesOwner;
	class Room;

	namespace Nav
	{
		struct PathRequestInfo;
		struct PlacementAtNode;
		class Task;
	};

	namespace AI
	{
		class MindInstance;

		class Navigation
		{
		public:
			Navigation(MindInstance* _mind);
			~Navigation();

			bool is_at_transport_route() const;

			Nav::PlacementAtNode find_nav_location() const;
			Nav::PlacementAtNode find_nav_location(Room* _room, Transform const & _placement, Optional<int> const & _navMeshKeepGroup = NP) const;
			Nav::PlacementAtNode find_nav_location(IModulesOwner* _of, Optional<int> const & _navMeshKeepGroup = NP) const;
			Nav::PlacementAtNode find_nav_location(IModulesOwner* _of, Vector3 const & _relativeLocOS, Optional<int> const & _navMeshKeepGroup = NP) const;

			Nav::Task* find_path_to(IModulesOwner* _to, Nav::PathRequestInfo const & _requestInfo) const;
			Nav::Task* find_path_to(Room* _room, Vector3 const & _where, Nav::PathRequestInfo const & _requestInfo) const;
			Nav::Task* find_path_to(Nav::PlacementAtNode const & _to, Nav::PathRequestInfo const & _requestInfo) const;
			Nav::Task* find_path_through(DoorInRoom* _dir, Optional<float> const & _distanceIn, Nav::PathRequestInfo const & _requestInfo) const;

			Nav::Task* get_random_location(float _distance) const;
			Nav::Task* get_random_path(float _distance, Optional<Vector3> const & _startInDir, Nav::PathRequestInfo const & _requestInfo) const;

		private:
			MindInstance* mind;

			void fill(Nav::PathRequestInfo & _requestInfo) const;
		};
	};
};
