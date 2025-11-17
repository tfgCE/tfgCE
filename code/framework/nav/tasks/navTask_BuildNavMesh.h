#pragma once

#include "..\navFlags.h"
#include "..\navTask.h"
#include "..\..\..\core\memory\safeObject.h"
#include "..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\core\types\name.h"

struct Transform;

namespace Framework
{
	interface_class IModulesOwner;
	class DoorInRoom;
	class Mesh;
	class Room;
	struct MeshNode;

	namespace Nav
	{
		class Mesh;
		class Node;

		namespace Tasks
		{
			/**
			 *	Uses mesh nodes:
			 *		name - groups together mesh nodes of same name - to create edges, convex polygons etc
			 *		tags - to determine type - types cannot be mixed within same name
			 *			nav - obligatory tag
			 *			line - single point to be added to a line
			 *			convexPolygon - to create convex polygon
			 */
			class BuildNavMesh
			: public Nav::Task
			{
				FAST_CAST_DECLARE(BuildNavMesh);
				FAST_CAST_BASE(Nav::Task);
				FAST_CAST_END();

				typedef Nav::Task base;
			public:
				static Nav::Task* new_task(Room* _room, Name const & _navMeshId = Name::invalid());

			protected:
				implement_ void perform();

				implement_ void cancel_if_related_to(Room* _room);

				implement_ tchar const* get_log_name() const { return TXT("build nav mesh"); }

			private:
				BuildNavMesh(Room* _room, Name const & _navMeshId);

				void add_mesh_nodes(Nav::Mesh* _navMesh, Framework::Mesh const * _mesh, Transform const & _placement, IModulesOwner* _owner);

			private:
				Room* room;
				Name navMeshId;

				Array<SafePtr<Framework::IModulesOwner>> roomObjects;
				Array<SafePtr<Framework::DoorInRoom>> doorsInRoom;

				Flags roomNavFlags;

				void setup_nav_node(MeshNode* _meshNode, Nav::Node* _navNode);

				void generate_polygons(OUT_ Array<Nav::Node*>& _navNodes, IModulesOwner* _owner, Optional<Vector3> const& _probeLocWS, Array<Vector3> const& _pointsClockwiseWS, float _voxelSize, float _gridTileSize, float _pushInwards, float _height, float _heightLow);

				void gather_objects_and_doors();
			};
		};
	};
};
