#include "navTask_BuildNavMesh.h"

#include "..\navMesh.h"
#include "..\nodes\navNode_ConvexPolygon.h"
#include "..\nodes\navNode_Point.h"

#include "..\..\game\game.h"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleCollision.h"
#include "..\..\module\moduleNavElement.h"
#include "..\..\object\object.h"
#include "..\..\world\doorInRoom.h"
#include "..\..\world\room.h"
#include "..\..\world\roomRegionVariables.inl"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\debug\debugVisualiser.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
//#define AN_VISUALISE_GENERATE_POLYGONS
#endif

//

using namespace Framework;
using namespace Nav;
using namespace Tasks;

//

DEFINE_STATIC_NAME(convexPolygon);
DEFINE_STATIC_NAME(generatePolygonsWithinConvexPolygon);
DEFINE_STATIC_NAME(convexIn);
DEFINE_STATIC_NAME(pushInwards);
DEFINE_STATIC_NAME(voxelSize);
DEFINE_STATIC_NAME(gridTileSize);
DEFINE_STATIC_NAME(height);
DEFINE_STATIC_NAME(heightLow);
DEFINE_STATIC_NAME(info);
DEFINE_STATIC_NAME(probe);
DEFINE_STATIC_NAME(line);
DEFINE_STATIC_NAME(nav);
DEFINE_STATIC_NAME(navFlags);
DEFINE_STATIC_NAME(irrelevantOnSingleConnection);

//

REGISTER_FOR_FAST_CAST(BuildNavMesh);

Nav::Task* BuildNavMesh::new_task(Room* _room, Name const & _navMeshId)
{
	return new BuildNavMesh(_room, _navMeshId);
}

BuildNavMesh::BuildNavMesh(Room* _room, Name const & _navMeshId)
: base(true)
, room(_room)
, navMeshId(_navMeshId)
{
}

void BuildNavMesh::perform()
{
	scoped_call_stack_info(TXT("build nav mesh"));
#ifdef DEBUG_WORLD_LIFETIME
	scoped_call_stack_info_str_printf(TXT("r%p"), room);
#endif
	CANCEL_NAV_TASK_ON_REQUEST();

#ifdef AN_OUTPUT_NAV_GENERATION
	output_colour_nav();
	output(TXT("build nav mesh for r%p"), room);
	output_colour();
	::System::TimeStamp ts;
#endif

	todo_note(TXT("lock room appearance?"));

	// collect flags
	roomNavFlags.clear();
	if (room)
	{
		room->on_each_variable_down<String>(NAME(navFlags), [this](String const & _stringFlags)
		{
			roomNavFlags.apply(_stringFlags);
		});
	}

	// for time being it is ok to just replace nav meshes and create from scrap - in future we may want to modify existing ones
	Nav::MeshPtr navMesh(new Nav::Mesh(room, navMeshId));

	gather_objects_and_doors();
	
	if (is_cancel_requested())
	{
		scoped_call_stack_info(TXT("cancel -> clear"));

		roomObjects.clear();
		doorsInRoom.clear();
		
		navMesh.clear();
		end_cancelled();
		return;
	}

	if (doorsInRoom.get_size() == 2 &&
		(doorsInRoom[0]->get_placement().get_translation() - doorsInRoom[1]->get_placement().get_translation()).length() < 0.1f)
	{
		if (doorsInRoom[0].get() && doorsInRoom[1].get())
		{
			scoped_call_stack_info(TXT("small room hack"));

			todo_hack(TXT("it is sort of hack, well, special case, to avoid problem with very small spaces"));
			for_every_ref(dir, doorsInRoom)
			{
				dir->add_to(navMesh.get());
			}

			doorsInRoom[0]->nav_connect_to(doorsInRoom[1].get());
		}
	}
	else
	{
		scoped_call_stack_info(TXT("add nodes"));

		{
			scoped_call_stack_info(TXT("add nodes from room objects"));

			for_every_ref(object, roomObjects)
			{
				if (auto* appearance = object->get_appearance())
				{
					if (auto* mesh = appearance->get_mesh())
					{
						add_mesh_nodes(navMesh.get(), mesh, Transform::identity, object);
					}
				}
				// add ModuleNavElement first!
				if (auto * nav = object->get_nav_element())
				{
					nav->add_to(navMesh.get());
				}
			}
		}

		{
			scoped_call_stack_info(TXT("add nodes from doors"));

			for_every_ref(dir, doorsInRoom)
			{
				dir->add_to(navMesh.get());
			}
		}

		{
			scoped_call_stack_info(TXT("add nodes from room parts"));

			for_every_ptr(meshInstance, room->get_appearance().get_instances())
			{
				if (auto * usedMesh = room->find_used_mesh_for(meshInstance->get_mesh()))
				{
					add_mesh_nodes(navMesh.get(), usedMesh, meshInstance->get_placement(), nullptr);
				}
			}
		}

		// unify some nodes, reconnect them to a single node
		navMesh->unify_open_nodes();

		// connect all open now (this should be last step to connect all existing nodes)
		navMesh->connect_open_nodes();
	}

	// remove nodes that mark themselves as irrelevant for any reason
	navMesh->outdate_irrelevant();

	// when connecting open nodes, we could outdate few others, drop them now
	navMesh->remove_outdated();

	// assign groups, so we will know how many groups do we have
	navMesh->assign_groups();

	if (is_cancel_requested())
	{
		navMesh.clear();
	}
	else
	{
		Game::get()->perform_sync_world_job(TXT("use nav mesh"),
			[this, navMesh]()
			{
				room->use_nav_mesh(navMesh.get());
			}
		);
	}

#ifdef AN_OUTPUT_NAV_GENERATION
	output_colour_nav();
	output(TXT("built nav mesh for \"%S\" in %.2fs"), room->get_name().to_char(), ts.get_time_since());
	output_colour();
#endif

	roomObjects.clear();
	doorsInRoom.clear();

	end_success();
}

void BuildNavMesh::gather_objects_and_doors()
{
	roomObjects.clear();
	doorsInRoom.clear();
	Game::get()->perform_sync_world_job(TXT("get objects for nav mesh"),
		[this]()
		{
			if (is_cancel_requested())
			{
				return;
			}
			for_every_ptr(object, room->get_objects())
			{
				roomObjects.push_back(SafePtr<Framework::IModulesOwner>(object));
			}
			for_every_ptr(dir, room->get_all_doors())
			{
				if (!dir->can_move_through() || !dir->is_relevant_for_movement() || ! dir->is_visible()) continue;
				doorsInRoom.push_back(SafePtr<DoorInRoom>(dir));
			}
		}
	);
}


void BuildNavMesh::add_mesh_nodes(Nav::Mesh* _navMesh, Framework::Mesh const * _mesh, Transform const & _placement, IModulesOwner* _owner)
{
	Array<Name> namesHandled; // to add by each name once

	Array<MeshNode*> meshNodes;

	for_every_ref(meshNode, _mesh->get_mesh_nodes())
	{
		if (meshNode->is_dropped())
		{
			continue;
		}
		if (namesHandled.does_contain(meshNode->name))
		{
			// already handled
			continue;
		}

		namesHandled.push_back(meshNode->name);

		meshNodes.clear();
		for_every_ref(gather, _mesh->get_mesh_nodes())
		{
			if (! gather->is_dropped() &&
				gather->name == meshNode->name)
			{
				meshNodes.push_back(gather);
			}
		}

		if (meshNode->tags.get_tag(NAME(nav)))
		{
			if (meshNode->tags.get_tag(NAME(line)))
			{
				// build nav mesh nodes into a line
				Nav::Nodes::Point* firstNavNodePoint = nullptr;
				Nav::Nodes::Point* lastNavNodePoint = nullptr;
				for_every_ptr(meshNode, meshNodes)
				{
					if (!meshNode->tags.get_tag(NAME(line)))
					{
						error(TXT("mixing nav nodes: line v non line"));
						continue;
					}
					Nav::Nodes::Point* navNode = Nav::Nodes::Point::get_one();
					setup_nav_node(meshNode, navNode);
					_navMesh->add(navNode);
					navNode->place_LS(_placement.to_world(meshNode->placement), _owner);
					if (lastNavNodePoint)
					{
						navNode->connect(lastNavNodePoint);
					}
					lastNavNodePoint = navNode;
					if (!firstNavNodePoint) firstNavNodePoint = navNode;
				}

				if (firstNavNodePoint && lastNavNodePoint)
				{
					firstNavNodePoint->be_open_node();
					lastNavNodePoint->be_open_node();
				}
			}
			else if (meshNode->tags.get_tag(NAME(convexPolygon)))
			{
				// build nav mesh nodes into a line
				Nav::Nodes::ConvexPolygon* navNodePolygon = Nav::Nodes::ConvexPolygon::get_one();
				IModulesOwner* owner = _owner;
				navNodePolygon->place_LS(Transform::identity, owner);
				float in = 0.0f;
				for_every_ptr(meshNode, meshNodes)
				{
					if (!meshNode->tags.get_tag(NAME(convexPolygon)))
					{
						error(TXT("mixing nav nodes: convexPolygon v non convexPolygon"));
						continue;
					}
					if (meshNode->tags.get_tag(NAME(info)))
					{
						in = max(in, meshNode->variables.get_value(NAME(convexIn), 0.0f));
						in = max(in, meshNode->variables.get_value(NAME(pushInwards), 0.0f));
						if (auto* navFlagsString = meshNode->variables.get_existing<String>(NAME(navFlags)))
						{
							roomNavFlags.clear();
							roomNavFlags.apply(*navFlagsString);
						}
					}
					else
					{
						navNodePolygon->add_clockwise(_placement.to_world(meshNode->placement).get_translation());
					}
				}
				setup_nav_node(meshNode, navNodePolygon);
				navNodePolygon->build(in);
				_navMesh->add(navNodePolygon);
				navNodePolygon->be_open_node();
			}
			else if (meshNode->tags.get_tag(NAME(generatePolygonsWithinConvexPolygon)))
			{
				Array<Vector3> pointsClockwise;
				float pushInwards = 0.0f;
				float voxelSize = 0.1f;
				float gridTileSize = 0.0f;
				float height = 1.8f;
				float heightLow = 1.0f;
				Optional<Vector3> probeLocWS;
				Transform placementWS = _placement;
				if (_owner)
				{
					placementWS = _owner->get_presence()->get_placement().to_world(placementWS);;
				}
				for_every_ptr(meshNode, meshNodes)
				{
					if (!meshNode->tags.get_tag(NAME(generatePolygonsWithinConvexPolygon)))
					{
						error(TXT("mixing nav nodes: generatePolygonsWithinConvexPolygon v non generatePolygonsWithinConvexPolygon (%S)"), _navMesh->get_room()? _navMesh->get_room()->get_name().to_char() : TXT("--"));
						continue;
					}
					if (meshNode->tags.get_tag(NAME(probe))) // might be shared with info
					{
						probeLocWS = placementWS.location_to_world(meshNode->placement.get_translation());
					}
					if (meshNode->tags.get_tag(NAME(info)))
					{
						pushInwards = max(pushInwards, meshNode->variables.get_value(NAME(convexIn), 0.0f));
						pushInwards = max(pushInwards, meshNode->variables.get_value(NAME(pushInwards), 0.0f));
						voxelSize = meshNode->variables.get_value(NAME(voxelSize), voxelSize);
						gridTileSize = meshNode->variables.get_value(NAME(gridTileSize), gridTileSize);
						height = meshNode->variables.get_value(NAME(height), height);
						heightLow = meshNode->variables.get_value(NAME(heightLow), heightLow);
						if (auto* navFlagsString = meshNode->variables.get_existing<String>(NAME(navFlags)))
						{
							roomNavFlags.clear();
							roomNavFlags.apply(*navFlagsString);
						}
					}
					else if (! meshNode->tags.get_tag(NAME(probe)))
					{
						pointsClockwise.push_back(placementWS.location_to_world(meshNode->placement.get_translation()));
					}
				}
				Array<Nav::Node*> navNodes;
				generate_polygons(OUT_ navNodes, _owner, probeLocWS, pointsClockwise, voxelSize, gridTileSize, pushInwards, height, heightLow);

				for_every_ptr(node, navNodes)
				{
					setup_nav_node(meshNode, node);
					_navMesh->add(node);
					node->be_open_node();
				}
			}
			else
			{
				todo_important(TXT("implement_"));
			}
		}
	}
}

void BuildNavMesh::setup_nav_node(MeshNode* _meshNode, Nav::Node* _navNode)
{
#ifdef AN_DEVELOPMENT
	_navNode->set_name(_meshNode->name);
#endif

	_navNode->access_flags() = roomNavFlags;

	if (_meshNode->variables.get_value<bool>(NAME(irrelevantOnSingleConnection), false))
	{
		_navNode->be_irrelevant_on_single_connection();
	}
	if (auto* navFlagsString = _meshNode->variables.get_existing<String>(NAME(navFlags)))
	{
		_navNode->access_flags().apply(*navFlagsString);
	}
	if (_navNode->get_flags().is_empty())
	{
		warn(TXT("no nav flags provided for node in \"%S\""), room? room->get_name().to_char() : TXT("??"));
	}
}

void BuildNavMesh::cancel_if_related_to(Room* _room)
{
	if (room == _room)
	{
		cancel();
	}
}

struct BuildNavMeshHelper
{
	Transform placement;
	IModulesOwner* owner;
	float voxelSize;
	float gridTileSize;
	float pushInwards;
	float heightLow = 1.0f;
	float height = 1.8f;

	enum VoxelFlag
	{
		Floor = 1,
		Low = 2,
		High = 4,

		All = Floor | Low | High
	};
	Array<int> voxels; // 0 no voxel at all, 1 is floor, 2 is obstruction above
	Array<int> distance;
	Array<int> border; // 0 - no border, 1 - border available to start,
	RangeInt2 voxelRange;
	RangeInt2 validRange; // voxelRange but smaller

	struct BorderLine
	{
		bool used = false;
		// on left empty, on right region
		VectorInt2 from, to;
		BorderLine() {}
		BorderLine(VectorInt2 const & _from, VectorInt2 const& _to): from(_from), to(_to){}
	};
	Array<BorderLine> borderLinesRaw; // short ones, created from cells
	Array<BorderLine> borderLinesLong; // long ones, unified raws

	struct NavEdge
	{
		Optional<bool> clockwiseChain = false;
		bool used = false;
		int connectsFrom = NONE;
		int connectsTo = NONE;
		// on the right is navmesh
		Vector2 from, to;
		Vector2 in; // is not updated automatically! used for continuous pushing inwards using same dir
		Segment2 seg;
		Range2 range;
		NavEdge() {}
		NavEdge(Vector2 const& _from, Vector2 const& _to, Optional<bool> _clockwiseChain) : clockwiseChain(_clockwiseChain), from(_from), to(_to) { update_auto(); in = (to - from).rotated_right().normal(); }
		void update_auto() { seg = Segment2(from, to); range = Range2::empty; range.include(from); range.include(to); }
	};

	struct BorderLineChain
	{
		// on the right is navmesh
		bool valid = false;
		bool clockwise = false; // if clockwise, the navmesh is inside, if anti-clockwise, the navmesh is outside
		Array<BorderLine> linesLong;
		Array<BorderLine> linesSimple;
		Array<NavEdge> navEdges;
		Vector2 centre;
	};
	List<BorderLineChain> borderChains; // all should be looped!

#ifdef AN_DEVELOPMENT
	Array<NavEdge> allNavEdges;
	Array<PolygonUtils::Triangle> triangulised;
	Array<ConvexPolygon> convexPolygons;
#endif

	VectorInt2 size;

	int get_idx(int _x, int _y) { an_assert(voxelRange.x.does_contain(_x) && voxelRange.y.does_contain(_y)); return (_x - voxelRange.x.min) + size.x * (_y - voxelRange.y.min); }
	int& access_voxel(int _x, int _y) { return voxels[get_idx(_x, _y)]; }
	int& access_distance(int _x, int _y) { return distance[get_idx(_x, _y)]; }
	int& access_distance(VectorInt2 const & _v) { return distance[get_idx(_v.x, _v.y)]; }
	int& access_border(int _x, int _y) { return border[get_idx(_x, _y)]; }
	int& access_border(VectorInt2 const& _v) { return border[get_idx(_v.x, _v.y)]; }

	float get_min_z() const { return -min(heightLow * 0.5f, max(voxelSize * 2.0f, height * 0.1f)); }
	float get_low_z() const { return heightLow; }
	float get_max_z() const { return height; }

	// these are templates, they have missing x and y
	Range3 get_floor_voxel() const { return Range3(Range(0.0f), Range(0.0f), Range(get_min_z(), -get_min_z())); }
	Range3 get_low_voxel() const { return Range3(Range(0.0f), Range(0.0f), Range(-get_min_z(), get_low_z())); }
	Range3 get_high_voxel() const { return Range3(Range(0.0f), Range(0.0f), Range(get_low_z(), get_max_z())); }

	Vector3 get_voxel_floor_centre(VectorInt2 const& _v) const
	{
		return Vector3((float)_v.x * voxelSize,
					   (float)_v.y * voxelSize, min(0.01f, voxelSize * 0.5f));
	}

	void update_voxels_for_xy(REF_ Range3& _floorVoxel, REF_ Range3& _lowVoxel, REF_ Range3& _highVoxel, int _x, int _y) const
	{
		_floorVoxel.x.min = (float)_x * voxelSize - voxelSize * 0.5f;
		_floorVoxel.x.max = _floorVoxel.x.min + voxelSize;
		_floorVoxel.y.min = (float)_y * voxelSize - voxelSize * 0.5f;
		_floorVoxel.y.max = _floorVoxel.y.min + voxelSize;
		_lowVoxel.x = _floorVoxel.x;
		_lowVoxel.y = _floorVoxel.y;
		_highVoxel.x = _floorVoxel.x;
		_highVoxel.y = _floorVoxel.y;
	}

	VectorInt2 to_voxel_coord(Vector3 const& _locLS) const
	{
		VectorInt2 coord;
		coord.x = TypeConversions::Normal::f_i_cells(_locLS.x / voxelSize);
		coord.y = TypeConversions::Normal::f_i_cells(_locLS.y / voxelSize);
		return coord;
	}

	RangeInt2 to_voxel_range(Range2 const& _space) const
	{
		RangeInt2 vr;
		vr.x.min = TypeConversions::Normal::f_i_cells(_space.x.min / voxelSize) - 1;
		vr.y.min = TypeConversions::Normal::f_i_cells(_space.y.min / voxelSize) - 1;
		vr.x.max = TypeConversions::Normal::f_i_cells(_space.x.max / voxelSize) + 1;
		vr.y.max = TypeConversions::Normal::f_i_cells(_space.y.max / voxelSize) + 1;
		return vr;
	}

	BuildNavMeshHelper(Transform const& _placement, IModulesOwner* _owner, Range2 const& _space, float _voxelSize, float _gridTileSize, float _pushInwards, float _height, float _heightLow)
	{
		placement = _placement;
		owner = _owner;
		voxelSize = _voxelSize;
		gridTileSize = _gridTileSize;
		pushInwards = _pushInwards;
		height = _height;
		heightLow = _heightLow;

		voxelRange = to_voxel_range(_space);

		size.x = voxelRange.x.length();
		size.y = voxelRange.y.length();

		voxels.set_size(size.x * size.y);
		memory_zero(voxels.get_data(), sizeof(int) * size.x * size.y);
	}

	void add_collision(::Collision::ModelInstance const& _cmi, Transform const& _placement, Name const & _topBone = Name::invalid())
	{
		Transform placement = _placement.to_world(_cmi.get_placement());
		if (auto* m = _cmi.get_model())
		{
			for_every(shape, m->get_boxes())
			{
				add<::Collision::Box, ::Box>(*shape, placement, _topBone);
			}
			for_every(shape, m->get_hulls())
			{
				add_by_is_inside(*shape, placement, _topBone);
				add_by_triangle(*shape, placement, _topBone);
			}
			for_every(shape, m->get_spheres())
			{
				add<::Collision::Sphere, ::Sphere>(*shape, placement, _topBone);
			}
			for_every(shape, m->get_capsules())
			{
				add<::Collision::Capsule, ::Capsule>(*shape, placement, _topBone);
			}
			for_every(shape, m->get_meshes())
			{
				add_by_triangle(*shape, placement, _topBone);
			}
			if (!m->get_skinned_meshes().is_empty())
			{
				error(TXT("skinned meshes not supported for nav mesh generation!"));
			}
		}
	}

	template<typename ShapeClass>
	bool get_vox_space_and_transform(ShapeClass const& _shape, Transform const& _placement, OUT_ RangeInt2& voxSpace, OUT_ Transform& transform)
	{
		transform = placement.inverted().to_world(_placement);

		Range3 shapeSpace3D = _shape.calculate_bounding_box(transform, nullptr, false);

		if (shapeSpace3D.z.max < get_min_z() || shapeSpace3D.z.min > get_max_z())
		{
			// nothing to do here
			return false;
		}

		Range2 shapeSpace(shapeSpace3D.x, shapeSpace3D.y);

		voxSpace = to_voxel_range(shapeSpace);

		voxSpace.intersect_with(voxelRange);
		if (voxSpace.is_empty())
		{
			return false;
		}

		return true;
	}

	template<typename ShapeClass>
	void update_voxels(RangeInt2 const& voxSpace, ShapeClass const& _shapeLS)
	{
		Range3 floorVoxel = get_floor_voxel();
		Range3 lowVoxel = get_low_voxel();
		Range3 highVoxel = get_high_voxel();

		for_range(int, y, voxSpace.y.min, voxSpace.y.max)
		{
			for_range(int, x, voxSpace.x.min, voxSpace.x.max)
			{
				int& voxel = access_voxel(x, y);

				if (voxel != All)
				{
					update_voxels_for_xy(floorVoxel, lowVoxel, highVoxel, x, y);
					if (!is_flag_set(voxel, Floor) && _shapeLS.does_overlap(floorVoxel))
					{
						voxel |= Floor;
					}
					if (!is_flag_set(voxel, Low) && _shapeLS.does_overlap(lowVoxel))
					{
						voxel |= Low;
					}
					if (!is_flag_set(voxel, High) && _shapeLS.does_overlap(highVoxel))
					{
						voxel |= High;
					}
				}
			}
		}
	}

	template<typename CollisionShapeClass, typename PrimitiveShapeClass>
	void add(CollisionShapeClass const& _shape, Transform const& _placement, Name const & _topBone)
	{
		if (_shape.get_bone().is_valid() &&
			_shape.get_bone() != _topBone)
		{
			return;
		}
		Transform transform;
		RangeInt2 voxSpace;
		if (get_vox_space_and_transform(_shape, _placement, OUT_ voxSpace, OUT_ transform))
		{
			PrimitiveShapeClass localShape(_shape);

			localShape.apply_transform(transform.to_matrix());

			update_voxels(voxSpace, localShape);
		}
	}
	
	template<typename CollisionShapeClass>
	void add_by_is_inside(CollisionShapeClass const& _shape, Transform const& _placement, Name const& _topBone)
	{
		if (_shape.get_bone().is_valid() &&
			_shape.get_bone() != _topBone)
		{
			return;
		}

		// this is only for those that centre or corners are inside, we're good with triangles on faces
		Transform transform;
		RangeInt2 voxSpace;
		if (get_vox_space_and_transform(_shape, _placement, OUT_ voxSpace, OUT_ transform))
		{
			Range3 floorVoxel = get_floor_voxel();
			Range3 lowVoxel = get_low_voxel();
			Range3 highVoxel = get_high_voxel();

			for_range(int, y, voxSpace.y.min, voxSpace.y.max)
			{
				for_range(int, x, voxSpace.x.min, voxSpace.x.max)
				{
					int& voxel = access_voxel(x, y);

					if (voxel != All)
					{
						update_voxels_for_xy(floorVoxel, lowVoxel, highVoxel, x, y);

						Range3* r[] = { &floorVoxel, &lowVoxel, &highVoxel };
						int f[] = { Floor, Low, High };
						for_count(int, l, 3)
						{
							if (_shape.does_contain_inside(r[l]->get_at(Vector3(0.5f, 0.5f, 0.5f))) ||
								_shape.does_contain_inside(r[l]->get_at(Vector3(0.0f, 0.0f, 0.0f))) ||
								_shape.does_contain_inside(r[l]->get_at(Vector3(1.0f, 0.0f, 0.0f))) ||
								_shape.does_contain_inside(r[l]->get_at(Vector3(0.0f, 1.0f, 0.0f))) ||
								_shape.does_contain_inside(r[l]->get_at(Vector3(1.0f, 1.0f, 0.0f))) ||
								_shape.does_contain_inside(r[l]->get_at(Vector3(0.0f, 0.0f, 1.0f))) ||
								_shape.does_contain_inside(r[l]->get_at(Vector3(1.0f, 0.0f, 1.0f))) ||
								_shape.does_contain_inside(r[l]->get_at(Vector3(0.0f, 1.0f, 1.0f))) ||
								_shape.does_contain_inside(r[l]->get_at(Vector3(1.0f, 1.0f, 1.0f))))
							{
								voxel |= f[l];
							}
						}
					}
				}
			}
		}
	}
	
	template<typename CollisionShapeClass>
	void add_by_triangle(CollisionShapeClass const& _shape, Transform const& _placement, Name const& _topBone)
	{
		if (_shape.get_bone().is_valid() &&
			_shape.get_bone() != _topBone)
		{
			return;
		}

		Transform transform;
		RangeInt2 voxSpace;
		if (get_vox_space_and_transform(_shape, _placement, OUT_ voxSpace, OUT_ transform))
		{
			for_count(int, triIdx, _shape.get_triangle_count())
			{
				Vector3 a, b, c;
				if (_shape.get_triangle(triIdx, OUT_ a, OUT_ b, OUT_ c))
				{
					a = transform.location_to_world(a);
					b = transform.location_to_world(b);
					c = transform.location_to_world(c);

					add_triangle_ls(a, b, c);
				}
			}
		}
	}

	void add_triangle_ls(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)
	{
		float minZ = get_min_z();
		float maxZ = get_max_z();
		if (_a.z < minZ && _b.z < minZ && _c.z < minZ)
		{
			return;
		}
		if (_a.z > maxZ && _b.z > maxZ && _c.z > maxZ)
		{
			return;
		}
		Range2 triSpace = Range2::empty;
		triSpace.include(_a.to_vector2());
		triSpace.include(_b.to_vector2());
		triSpace.include(_c.to_vector2());
		RangeInt2 voxSpace = to_voxel_range(triSpace);
		if (voxSpace.x.max < voxelRange.x.min ||
			voxSpace.x.min > voxelRange.x.max ||
			voxSpace.y.max < voxelRange.y.min ||
			voxSpace.y.min > voxelRange.y.max)
		{
			return;
		}
		voxSpace.x.min = voxelRange.x.clamp(voxSpace.x.min);
		voxSpace.x.max = voxelRange.x.clamp(voxSpace.x.max);
		voxSpace.y.min = voxelRange.y.clamp(voxSpace.y.min);
		voxSpace.y.max = voxelRange.y.clamp(voxSpace.y.max);

		Range3 floorVoxel = get_floor_voxel();
		Range3 lowVoxel = get_low_voxel();
		Range3 highVoxel = get_high_voxel();

		struct Triangle
		{
			Vector3 a, b, c;
			Plane plane;
			Plane ab, bc, ca; // point towards centre
			Triangle(Vector3 const& _a, Vector3 const& _b, Vector3 const& _c) :a(_a), b(_b), c(_c)
			{
				plane = Plane(a, b, c);
				//Vector3 centre = (a + b + c) / 3.0f;

				ab = Plane(-Vector3::cross(plane.get_normal(), b - a).normal(), a);
				bc = Plane(-Vector3::cross(plane.get_normal(), c - b).normal(), b);
				ca = Plane(-Vector3::cross(plane.get_normal(), a - c).normal(), c);
			}

			bool does_overlap(Range3 const& _range)
			{
				if (a.x < _range.x.min &&
					b.x < _range.x.min &&
					c.x < _range.x.min)
				{
					return false;
				}
				if (a.x > _range.x.max &&
					b.x > _range.x.max &&
					c.x > _range.x.max)
				{
					return false;
				}
				if (a.y < _range.y.min &&
					b.y < _range.y.min &&
					c.y < _range.y.min)
				{
					return false;
				}
				if (a.y > _range.y.max &&
					b.y > _range.y.max &&
					c.y > _range.y.max)
				{
					return false;
				}
				if (a.z < _range.z.min &&
					b.z < _range.z.min &&
					c.z < _range.z.min)
				{
					return false;
				}
				if (a.z > _range.z.max &&
					b.z > _range.z.max &&
					c.z > _range.z.max)
				{
					return false;
				}

				Vector3 centre = _range.centre();
				Vector3 xAxis = Vector3::xAxis;
				Vector3 yAxis = Vector3::yAxis;
				Vector3 zAxis = Vector3::zAxis;
				Vector3 halfSize = _range.length() * 0.5f;

				Vector3 ppp = centre + xAxis * halfSize.x + yAxis * halfSize.y + zAxis * halfSize.z;
				Vector3 ppn = centre + xAxis * halfSize.x + yAxis * halfSize.y - zAxis * halfSize.z;
				Vector3 pnp = centre + xAxis * halfSize.x - yAxis * halfSize.y + zAxis * halfSize.z;
				Vector3 pnn = centre + xAxis * halfSize.x - yAxis * halfSize.y - zAxis * halfSize.z;
				Vector3 npp = centre - xAxis * halfSize.x + yAxis * halfSize.y + zAxis * halfSize.z;
				Vector3 npn = centre - xAxis * halfSize.x + yAxis * halfSize.y - zAxis * halfSize.z;
				Vector3 nnp = centre - xAxis * halfSize.x - yAxis * halfSize.y + zAxis * halfSize.z;
				Vector3 nnn = centre - xAxis * halfSize.x - yAxis * halfSize.y - zAxis * halfSize.z;

				Vector3 points[] = { ppp, ppn, pnp, pnn, npp, npn, nnp, nnn };

				//

				int plus = 0;
				int minus = 0;
				for_count(int, i, 8)
				{
					float inf = plane.get_in_front(points[i]);
					if (inf > EPSILON) ++plus;
					if (inf < -EPSILON) ++minus;
				}
				if (plus == 8 || minus == 8)
				{
					return false;
				}
				for_count(int, outIdx, 3)
				{
					Plane const& edge = (outIdx == 0 ? ab : (outIdx == 1 ? bc : ca));
					int outside = 0;
					for_count(int, i, 8)
					{
						float inf = edge.get_in_front(points[i]);
						if (inf < -EPSILON) ++ outside;
					}
					if (outside == 8)
					{
						return false;
					}
				}

				return true;
			}
		};
		Triangle triangleLS(_a, _b, _c);
		
		for_range(int, y, voxSpace.y.min, voxSpace.y.max)
		{
			for_range(int, x, voxSpace.x.min, voxSpace.x.max)
			{
				int& voxel = access_voxel(x, y);

				if (voxel != All)
				{
					update_voxels_for_xy(floorVoxel, lowVoxel, highVoxel, x, y);
					if (!is_flag_set(voxel, Floor) && triangleLS.does_overlap(floorVoxel))
					{
						voxel |= Floor;
					}
					if (!is_flag_set(voxel, Low) && triangleLS.does_overlap(lowVoxel))
					{
						voxel |= Low;
					}
					if (!is_flag_set(voxel, High) && triangleLS.does_overlap(highVoxel))
					{
						voxel |= High;
					}
				}
			}
		}
	}

	static VectorInt2 dir_to_vector(int _dir)
	{
		if (_dir == 0) return VectorInt2(0, 1);
		if (_dir == 1) return VectorInt2(1, 1);
		if (_dir == 2) return VectorInt2(1, 0);
		if (_dir == 3) return VectorInt2(1, -1);
		if (_dir == 4) return VectorInt2(0, -1);
		if (_dir == 5) return VectorInt2(-1, -1);
		if (_dir == 6) return VectorInt2(-1, 0);
		if (_dir == 7) return VectorInt2(-1, 1);
		an_assert(false, TXT("invalid dir"));
		return VectorInt2(0, 0);
	}

	void block_voxels_using(Array<Vector2> const& pointsToBlockWith)
	{
		ConvexPolygon cp;
		for_every(p, pointsToBlockWith)
		{
			cp.add(*p);
		}
		cp.build();
		for_range(int, y, voxelRange.y.min, voxelRange.y.max)
		{
			for_range(int, x, voxelRange.x.min, voxelRange.x.max)
			{
				Vector2 voxelAt = get_voxel_floor_centre(VectorInt2(x, y)).to_vector2();
				if (!cp.is_inside(voxelAt))
				{
					int& v = access_voxel(x, y);
					v = All;
				}
			}
		}
	}

	int calculate_distance_and_borders(OUT_ Array<Nav::Node*>& _navNodes, Optional<Vector3> const & _probeLocWS = NP)
	{
#ifdef AN_OUTPUT_NAV_GENERATION
		::System::TimeStamp ts;
#endif

		validRange = voxelRange;
		++validRange.x.min;
		++validRange.y.min;
		--validRange.x.max;
		--validRange.y.max;

		int maxd = NONE;

		maxd = build_distances(_probeLocWS);

		create_border_cells();

		create_border_lines();

		create_border_chains();

		push_inwards();

		create_grid_and_nodes(_navNodes);

#ifdef AN_OUTPUT_NAV_GENERATION
		output_colour_nav();
		output(TXT("build convex polygons basing on voxels %.2fs"), ts.get_time_since());
		output_colour();
#endif

		return maxd;
	}

private:
	int build_distances(Optional<Vector3> const& _probeLocWS = NP)
	{
		int maxd = NONE;

		// distances
		{
			distance.set_size(voxels.get_size());

			for_every(dist, distance)
			{
				*dist = NONE;
			}

			// mark interior
			{	// mark internal parts
				for_range(int, y, validRange.y.min, validRange.y.max)
				{
					for_range(int, x, validRange.x.min, validRange.x.max)
					{
						int& v = access_voxel(x, y);
						if (is_flag_set(v, Floor) &&
							!is_flag_set(v, Low) &&
							!is_flag_set(v, High))
						{
							access_distance(x, y) = 0;
						}
					}
				}
			}

			// keep probe
			if (_probeLocWS.is_set())
			{
				Vector3 probeLocLS = placement.location_to_local(_probeLocWS.get());

				VectorInt2 probeLoc = to_voxel_coord(probeLocLS);

				if (access_distance(probeLoc) == 0)
				{
					access_distance(probeLoc) = 1;
					// fill from probe
					{
						// 0 - not done
						// 1 - filled
						bool keepDoing = true;
						while (keepDoing)
						{
							keepDoing = false;
							for_range(int, y, validRange.y.min, validRange.y.max)
							{
								for_range(int, x, validRange.x.min, validRange.x.max)
								{
									if (access_distance(x, y) == 0 &&
										(access_distance(x + 1, y) == 1 ||
										 access_distance(x - 1, y) == 1 ||
										 access_distance(x, y + 1) == 1 ||
										 access_distance(x, y - 1) == 1))
									{
										for (int vd = -1; vd <= 1; vd += 1) // vertical dir
										{
											for (int vo = (vd < 0 ? 0 : 1);; vo += vd) // vertical offset
											{
												if (!validRange.does_contain(VectorInt2(x, y + vo)))
												{
													break;
												}
												if (access_distance(x, y + vo) == 0)
												{
													keepDoing = true;
													access_distance(x, y + vo) = 1;
													for (int hd = -1; hd <= 1; hd += 1) // horizontal dir
													{
														for (int ho = (hd < 0 ? 0 : 1);; ho += hd) // horizontal offset
														{
															if (!validRange.does_contain(VectorInt2(x + ho, y + vo)))
															{
																break;
															}
															auto& d = access_distance(x + ho, y + vo);
															if (d == 0)
															{
																d = 1;
															}
															else
															{
																break;
															}
														}
													}
												}
												else
												{
													break;
												}
											}
										}
									}
								}
							}
						}
					}

					// remove parts not marked as probed
					{
						// change 0 to NONE, 1 to 0
						for_range(int, y, validRange.y.min, validRange.y.max)
						{
							for_range(int, x, validRange.x.min, validRange.x.max)
							{
								auto& d = access_distance(x, y);
								if (d == 0) d = NONE; else
									if (d > 0) d = 0;
							}
						}
					}
				}
				else
				{
					warn(TXT("probe doesn't lay in navigable space, not used"));
				}
			}

			// remove orphan cells
			{
				for_range(int, y, validRange.y.min, validRange.y.max)
				{
					for_range(int, x, validRange.x.min, validRange.x.max)
					{
						if (access_distance(x, y) == 0 &&
							access_distance(x + 1, y) == NONE &&
							access_distance(x - 1, y) == NONE &&
							access_distance(x, y + 1) == NONE &&
							access_distance(x, y - 1) == NONE)
						{
							access_distance(x, y) = NONE;
						}
					}
				}
			}

			// remove dead ends
			{
				bool keepDoing = true;
				while (keepDoing)
				{
					keepDoing = false;
					for_range(int, y, validRange.y.min, validRange.y.max)
					{
						for_range(int, x, validRange.x.min, validRange.x.max)
						{
							if (access_distance(x, y) == 0)
							{
								int emptyNeighbour = 0;
								if (access_distance(x - 1, y) == NONE) ++emptyNeighbour;
								if (access_distance(x + 1, y) == NONE) ++emptyNeighbour;
								if (access_distance(x, y - 1) == NONE) ++emptyNeighbour;
								if (access_distance(x, y + 1) == NONE) ++emptyNeighbour;
								if (access_distance(x - 1, y - 1) == NONE) ++emptyNeighbour;
								if (access_distance(x + 1, y - 1) == NONE) ++emptyNeighbour;
								if (access_distance(x - 1, y + 1) == NONE) ++emptyNeighbour;
								if (access_distance(x + 1, y + 1) == NONE) ++emptyNeighbour;
								if (emptyNeighbour == 7)
								{
									access_distance(x, y) = NONE;
									keepDoing = true;
								}
							}
						}
					}
				}
			}

			// iterate distance
			{
				bool keepDoing = true;
				while (keepDoing)
				{
					keepDoing = false;
					for_range(int, y, validRange.y.min, validRange.y.max)
					{
						for_range(int, x, validRange.x.min, validRange.x.max)
						{
							int& d = access_distance(x, y);
							if (d != NONE)
							{
								int dxp = access_distance(x + 1, y);
								int dxm = access_distance(x - 1, y);
								int dyp = access_distance(x, y + 1);
								int dym = access_distance(x, y - 1);
								int nd = min(min(dxp + 1, dxm + 1), min(dyp + 1, dym + 1));
								keepDoing |= (d != nd);
								d = nd;
								maxd = max(maxd, d);
							}
						}
					}
				}
			}
		}

		return maxd;
	}

	void create_border_cells()
	{
		// border (cells)
		{
			border.set_size(voxels.get_size());
			memory_zero(border.get_data(), border.get_data_size());

			{	// mark borders
				for_range(int, y, validRange.y.min, validRange.y.max)
				{
					for_range(int, x, validRange.x.min, validRange.x.max)
					{
						int& d = access_distance(x, y);
						int& b = access_border(x, y);
						if (d == 0)
						{
							int emptyNeighbour = 0;
							if (access_distance(x - 1, y) > 0) ++emptyNeighbour; else
							if (access_distance(x + 1, y) > 0) ++emptyNeighbour; else
							if (access_distance(x, y - 1) > 0) ++emptyNeighbour; else
							if (access_distance(x, y + 1) > 0) ++emptyNeighbour; else
							if (access_distance(x - 1, y - 1) > 0) ++emptyNeighbour; else
							if (access_distance(x + 1, y - 1) > 0) ++emptyNeighbour; else
							if (access_distance(x - 1, y + 1) > 0) ++emptyNeighbour; else
							if (access_distance(x + 1, y + 1) > 0) ++emptyNeighbour;
							if (emptyNeighbour > 0)
							{
								b = 1;
							}
							else
							{
								// non borders that should be borders are changed to NONE
								d = NONE;
							}
						}
					}
				}
			}
		}
	}

	void create_border_lines()
	{
		// create short border (lines)
		{
			borderLinesRaw.clear();
			for_range(int, y, validRange.y.min, validRange.y.max)
			{
				for_range(int, x, validRange.x.min, validRange.x.max)
				{
					VectorInt2 at(x, y);
					int& b = access_border(x, y);
					if (b == 1)
					{
						for (int dir = 0; dir < 8; dir += 2)
						{
							// dir	|
							//		*
							bool added = false;
							if (! added && access_distance(at + dir_to_vector(dir)) > 0)
							{
								if (!added)
								{
									//  .b				bdir1	 /		bdir2
									//	*b   *-					*		to		*-
									int bdir1 = mod(dir + 1, 8);
									int bdir2 = mod(dir + 2, 8);
									VectorInt2 to = at + dir_to_vector(bdir2);
									if (access_border(at + dir_to_vector(bdir1)) != 0 &&
										access_border(to) != 0)
									{
										++b;
										borderLinesRaw.push_back(BorderLine(to, at));
										added = true;
									}
								}
								if (!added)
								{
									//  .b    /			bdir1	 /
									//	*#   *			to		*
									int bdir1 = mod(dir + 1, 8);
									VectorInt2 to = at + dir_to_vector(bdir1);
									if (access_border(to) != 0)
									{
										++b;
										borderLinesRaw.push_back(BorderLine(to, at));
										added = true;
									}
								}
								if (!added)
								{
									//  ..				bdir1	 /		bdir2
									//	*b   *-					*		to		*-
									int bdir1 = mod(dir + 1, 8);
									int bdir2 = mod(dir + 2, 8);
									VectorInt2 to = at + dir_to_vector(bdir2);
									if (access_distance(at + dir_to_vector(bdir1)) > 0 &&
										access_border(to) != 0)
									{
										++b;
										borderLinesRaw.push_back(BorderLine(to, at));
										added = true;
									}
								}
							}
							if (!added && access_border(at + dir_to_vector(dir)) > 0)
							{
								if (!added)
								{
									//  b.				bdir1	 /		bdir2
									//	*b   *-					*		to		*-
									int bdir1 = mod(dir + 1, 8);
									int bdir2 = mod(dir + 2, 8);
									VectorInt2 to = at + dir_to_vector(bdir2);
									if (access_distance(at + dir_to_vector(bdir1)) > 0 &&
										access_border(to) != 0)
									{
										++b;
										borderLinesRaw.push_back(BorderLine(to, at));
										added = true;
									}
								}
							}
						}
					}
				}
			}
		}

		// simplify border lines - straight
		{
			borderLinesLong.clear();
			for_every(bl, borderLinesRaw)
			{
				if (!bl->used)
				{
					VectorInt2 dir = bl->to - bl->from;
					bool first = true;
					// check if it is the first in possible new border line
					for_every(obl, borderLinesRaw)
					{
						if (obl != bl && ! obl->used)
						{
							if (obl->to == bl->from &&
								obl->to - obl->from == dir)
							{
								first = false;
								break;
							}
						}
					}
					if (!first)
					{
						// it will be a part of something bigger eventually
						continue;
					}

					bl->used = true;
					BorderLine nbl(bl->from, bl->to);
					bool keepAdding = true;
					while (keepAdding)
					{
						keepAdding = false;
						for_every(obl, borderLinesRaw)
						{
							if (obl != bl && !obl->used)
							{
								if (obl->from == nbl.to &&
									obl->to - obl->from == dir)
								{
									keepAdding = true;
									nbl.to = obl->to;
									obl->used = true;
								}
							}
						}
					}

					borderLinesLong.push_back(nbl);
				}
			}
		}

#ifndef AN_DEVELOPMENT
		borderLinesRaw.clear();
#endif
	}

	void create_border_chains()
	{
		// combine long border lines into chain
		{
			borderChains.clear();
			for_every(bl, borderLinesLong)
			{
				if (!bl->used)
				{
					bl->used = true;
					borderChains.push_back(BorderLineChain());
					auto& bc = borderChains.get_last();
					bc.linesLong.push_back(*bl);

					bool keepAdding = true;
					VectorInt2 last = bl->to;
					while (keepAdding)
					{
						keepAdding = false;
						for_every(bl, borderLinesLong)
						{
							if (!bl->used &&
								bl->from == last)
							{
								bl->used = true;
								last = bl->to;
								bc.linesLong.push_back(*bl);
								keepAdding = true;
							}
						}
					}

					if (bc.linesLong.get_last().to != bc.linesLong.get_first().from)
					{
						bc.valid = false;
						error(TXT("border lines do not construct a chain!"));
					}
					else
					{
						bc.valid = true;

						// clockwise or not?
						// get the sum of angles in which we turn, in the end it should be either 360 or -360

						Vector2 centre = Vector2::zero;
						for_every(bl, bc.linesLong)
						{
							centre += bl->from.to_vector2();
						}
						centre /= (float)bc.linesLong.get_size();

						float angle = 0.0f;

						for_every(bl, bc.linesLong)
						{
							float startYaw = Rotator3::get_yaw(bl->from.to_vector2() - centre);
							float endYaw = Rotator3::get_yaw(bl->to.to_vector2() - centre);
							float yawAdd = Rotator3::normalise_axis(endYaw - startYaw);
							angle += yawAdd;
						}

						bc.centre = centre;

						if (abs(angle) < 90.0f)
						{
							// this is possible if centre is not inside the chain
							// get the furthest point and check in which direction does it go - take both from and to into account
							Optional<int> furthestIdx;
							float furthestDist = 0.0f;
							for_every(bl, bc.linesLong)
							{
								Vector2 f = bl->from.to_vector2();
								Vector2 t = bl->to.to_vector2();
								float dist = (f - centre).length() + (t - centre).length();
								if (!furthestIdx.is_set() || furthestDist < dist)
								{
									furthestIdx = for_everys_index(bl);
									furthestDist = dist;
								}
							}

							an_assert(furthestIdx.is_set());

							{
								auto& f = bc.linesLong[furthestIdx.get()];
								float startYaw = Rotator3::get_yaw(f.from.to_vector2() - centre);
								float endYaw = Rotator3::get_yaw(f.to.to_vector2() - centre);
								angle = Rotator3::normalise_axis(endYaw - startYaw);
							}
						}

						bc.clockwise = angle > 0.0f;
					}
				}
			}
		}

		// simplify border lines - to get rid of jaggies
		{
			for_every(bc, borderChains)
			{
				// get the furthest point
				Optional<int> furthestIdx;
				float furthestDist = 0.0f;
				for_every(bl, bc->linesLong)
				{
					Vector2 f = bl->from.to_vector2();
					float dist = (f - bc->centre).length();
					if (!furthestIdx.is_set() || furthestDist < dist)
					{
						furthestIdx = for_everys_index(bl);
						furthestDist = dist;
					}
				}

				an_assert(furthestIdx.is_set());

				{
					todo_note(TXT("make it a little bit better than doing it one by one"));
					int moveForward = furthestIdx.get();
					while (moveForward > 0)
					{
						BorderLine bl = bc->linesLong.get_first();
						bc->linesLong.pop_front();
						bc->linesLong.push_back(bl);
						--moveForward;
					}
				}

				// now the furthest point is at the start

				float maxAcceptableOff = 0.9f; // this is in voxels

				bool keepDoing = true;
				int triesLeft = 100;
				bool degeneratedLineError = false;
				while (triesLeft && keepDoing)
				{
					--triesLeft;
					keepDoing = false;
					// go through lines and see if we can simplify consecutive lines
					bc->linesSimple.clear();

					ARRAY_STACK(Vector2, points, bc->linesLong.get_size());
					for (int i = 0; i < bc->linesLong.get_size(); ++i)
					{
						points.clear();
						Vector2 start = bc->linesLong[i].from.to_vector2();
						Vector2 to = bc->linesLong[i].to.to_vector2();
						points.push_back(to);
						Optional<int> lastI;
						for (int j = i + 1; j < bc->linesLong.get_size(); ++j)
						{
							Vector2 newEnd = bc->linesLong[j].to.to_vector2();
							Vector2 newDir = (newEnd - start).normal();
							Vector2 newRight = newDir.rotated_right();
							float maxOff = 0.0f;
							for_every(point, points)
							{
								float off = Vector2::dot(*point - start, newRight);
								maxOff = max(maxOff, abs(off));
							}
							if (maxOff > maxAcceptableOff)
							{
								lastI = j - 1;
								break;
							}
							points.push_back(newEnd);
						}
						if (!lastI.is_set())
						{
							lastI = bc->linesLong.get_size() - 1;
						}

						BorderLine bl;
						bl.from = bc->linesLong[i].from;
						bl.to = bc->linesLong[lastI.get()].to;

						// don't add null lines
						if (bl.from == bl.to)
						{
							if (!degeneratedLineError)
							{
								degeneratedLineError = true;
								warn(TXT("degenerated line simple"));
							}
						}
						else
						{
							bc->linesSimple.push_back(bl);
						}

						i = lastI.get();
					}

					if (bc->linesSimple.get_size() < 3)
					{
						// degenerated, try again
						keepDoing = true;
						maxAcceptableOff *= 0.75f;
					}
				}

				bc->navEdges.clear();
				if (bc->linesSimple.get_size() >= 3)
				{
					for_every(bl, bc->linesSimple)
					{
						bc->navEdges.push_back(NavEdge(get_voxel_floor_centre(bl->from).to_vector2(), get_voxel_floor_centre(bl->to).to_vector2(), bc->clockwise));
					}
				}
				else
				{
					warn(TXT("couldn't generate nav edges"));
				}

#ifndef AN_DEVELOPMENT
				bc->linesSimple.clear();
				bc->linesLong.clear();
#endif
			}
		}

#ifndef AN_DEVELOPMENT
		borderLinesLong.clear();
#endif
	}

	bool are_border_chains_ok()
	{
		// check if they cross

		for_every(bc, borderChains)
		{
			for_every(ne, bc->navEdges)
			{
				for_every(obc, borderChains)
				{
					for_every(one, obc->navEdges)
					{
						if (ne != one)
						{
							if (ne->range.overlaps(one->range) &&
								ne->from != one->from &&
								ne->to != one->to &&
								ne->from != one->to &&
								ne->to != one->from)
							{
								if (ne->seg.does_intersect_with(one->seg))
								{
									return false;
								}
							}
						}
					}
				}
			}
		}

		return true;
	}

	void push_inwards()
	{
		if (pushInwards == 0.0f)
		{
			return;
		}

		int stepCount = 5;
		float pushInwardsStep = pushInwards / (float)stepCount;

		if (!are_border_chains_ok())
		{
			return;
		}

		for_count(int, stepIdx, stepCount)
		{
			for_every(bc, borderChains)
			{
				int edgeCount = bc->navEdges.get_size();
				for_count(int, eIdx, edgeCount)
				{
					auto& ce = bc->navEdges[eIdx];
					auto& ne = bc->navEdges[mod(eIdx + 1, edgeCount)];
					Vector2 moveIn = (ce.in + ne.in).normal() * pushInwardsStep;
					Vector2 storedCTo = ce.to;
					Vector2 storedNFrom = ne.from;
					ce.to += moveIn;
					ne.from += moveIn;
					ce.update_auto();
					ne.update_auto();
					if (!are_border_chains_ok())
					{
						ce.to = storedCTo;
						ne.from = storedNFrom;
						ce.update_auto();
						ne.update_auto();
					}
				}
			}
		}
	}

	static void propagate_used(Array<NavEdge> & tileNavEdges)
	{
		bool keepDoing = true;
		while (keepDoing)
		{
			keepDoing = false;
			for_every(ne, tileNavEdges)
			{
				if (ne->used)
				{
					if (ne->connectsTo != NONE &&
						!tileNavEdges[ne->connectsTo].used)
					{
						tileNavEdges[ne->connectsTo].used = true;
						keepDoing = true;
					}
					if (ne->connectsFrom != NONE &&
						!tileNavEdges[ne->connectsFrom].used)
					{
						tileNavEdges[ne->connectsFrom].used = true;
						keepDoing = true;
					}
				}
			}
		}
	}

	void create_grid_and_nodes(OUT_ Array<Nav::Node*>& _navNodes)
	{
		// create a grid
		Range2 space(Range(voxelSize* (float)voxelRange.x.min - voxelSize * 0.5f,
						   voxelSize* (float)voxelRange.x.max + voxelSize * 0.5f),
					 Range(voxelSize* (float)voxelRange.y.min - voxelSize * 0.5f,
						   voxelSize* (float)voxelRange.y.max + voxelSize * 0.5f));
		Vector2 spaceSize = space.length();

		float const requestedTileSize = gridTileSize == 0.0f? max(spaceSize.x * 1.2f, spaceSize.y * 1.2f) : gridTileSize;
		float const tileSize = max(voxelSize, round(requestedTileSize / voxelSize) * voxelSize);

		VectorInt2 gridSize = VectorInt2::one;
		if (gridTileSize != 0.0f)
		{
			while ((float)gridSize.x * tileSize < spaceSize.x) ++gridSize.x;
			while ((float)gridSize.y * tileSize < spaceSize.y) ++gridSize.y;
		}

		bool singleGridTile = gridSize.x == 1 && gridSize.y == 1;

		// align with voxels, this should save lots of issues, where a point would land on the edge of a grid tile
		Vector2 gridStartsAt = space.bottom_left() - (gridSize.to_vector2() * tileSize - spaceSize) * 0.5f;
		gridStartsAt.x = round_down_to(gridStartsAt.x, voxelSize) - voxelSize * 0.5f;
		gridStartsAt.y = round_down_to(gridStartsAt.y, voxelSize) - voxelSize * 0.5f;

		// make sure we cover the whole thing
		if (gridTileSize != 0.0f)
		{
			while ((float)gridSize.x * tileSize + gridStartsAt.x < space.x.max) ++gridSize.x;
			while ((float)gridSize.y * tileSize + gridStartsAt.y < space.y.max) ++gridSize.y;
		}

		for_count(int, y, gridSize.y)
		{
			for_count(int, x, gridSize.x)
			{
				Vector2 bottomLeft = gridStartsAt + VectorInt2(x, y).to_vector2() * tileSize;
				Vector2 topRight = bottomLeft + Vector2::one * tileSize;
				Range2 gridTile = Range2::empty;
				gridTile.include(bottomLeft);
				gridTile.include(topRight);

				Array<NavEdge> tileNavEdges;

				// go through border chains to cut them into edges
				for_every(bc, borderChains)
				{
					for_every(ne, bc->navEdges)
					{
						if (ne->range.x.max <= gridTile.x.min) continue;
						if (ne->range.x.min >= gridTile.x.max) continue;
						if (ne->range.y.max <= gridTile.y.min) continue;
						if (ne->range.y.min >= gridTile.y.max) continue;

						if (!(ne->to - ne->from).is_almost_zero())
						{
							if (gridTile.does_contain(ne->from) &&
								gridTile.does_contain(ne->to))
							{
								tileNavEdges.push_back(*ne);
							}
							else
							{
								Range2 gridTileExpanded = gridTile.expanded_by(Vector2::one * EPSILON);
								float t = 0.0f;
								Range tInside = Range::empty;
								while (t < 1.0f)
								{
									Vector2 at = ne->from * (1.0f - t) + t * ne->to;
									float nextt = 1.0f;
									Vector2 diff = ne->to - ne->from;
									if (diff.x != 0.0f)
									{
										if ((diff.x > 0.0f && at.x < gridTile.x.min) ||
											(diff.x < 0.0f && at.x > gridTile.x.min))
										{
											float ct = (gridTile.x.min - ne->from.x) / diff.x;
											if (ct > t) nextt = min(nextt, ct);
										}
										if ((diff.x > 0.0f && at.x < gridTile.x.max) ||
											(diff.x < 0.0f && at.x > gridTile.x.max))
										{
											float ct = (gridTile.x.max - ne->from.x) / diff.x;
											if (ct > t) nextt = min(nextt, ct);
										}
									}
									if (diff.y != 0.0f)
									{
										if ((diff.y > 0.0f && at.y < gridTile.y.min) ||
											(diff.y < 0.0f && at.y > gridTile.y.min))
										{
											float ct = (gridTile.y.min - ne->from.y) / diff.y;
											if (ct > t) nextt = min(nextt, ct);
										}
										if ((diff.y > 0.0f && at.y < gridTile.y.max) ||
											(diff.y < 0.0f && at.y > gridTile.y.max))
										{
											float ct = (gridTile.y.max - ne->from.y) / diff.y;
											if (ct > t) nextt = min(nextt, ct);
										}
									}

									Vector2 nextat = ne->from * (1.0f - nextt) + nextt * ne->to;
									if (gridTileExpanded.does_contain(at) &&
										gridTileExpanded.does_contain(nextat))
									{
										tInside.include(t);
										tInside.include(nextt);
									}
									t = nextt;
								}
								if (!tInside.is_empty())
								{
									Vector2 at0 = ne->from * (1.0f - tInside.min) + tInside.min * ne->to;
									Vector2 at1 = ne->from * (1.0f - tInside.max) + tInside.max * ne->to;

									// if aligned to voxels, we may still be able to hit a corner, ignore these
									if (!(at0 - at1).is_almost_zero())
									{
										tileNavEdges.push_back(NavEdge(at0, at1, NP));
									}
								}
							}
						}
					}
				}

				if (tileNavEdges.is_empty() && (!singleGridTile))
				{
					// check if we're not inside of something else
					int cuts = 0;
					Vector2 at = bottomLeft;
					for_every(bc, borderChains)
					{
						for_every(ne, bc->navEdges)
						{
							if (ne->range.y.min > at.y) continue;

							if (at.x > ne->range.x.min &&
								at.x < ne->range.x.max)
							{
								float t = (at.x - ne->from.x) / (ne->to.x - ne->from.x);

								float cutAtY = ne->from.y + (ne->to.y - ne->from.y) * t;
								if (cutAtY < at.y)
								{
									++cuts;
								}
							}
							else if (ne->range.x.min != ne->range.x.max) // ignore parallel edges
							{
								// if we just continue movement in one direction, there should be one cut
								// if we switch the direction, two cuts
								//
								//	to1 <- from1 = to0 <= from0   <- continue
								//
								//			from1 => to1
								//			 =
								//			to0 <= from0   <- switch direction
								//
								if ((at.x == ne->from.x && ne->to.x < ne->from.x && ne->from.y < at.y) ||
									(at.x == ne->to.x && ne->to.x > ne->from.x && ne->to.y < at.y))
								{
									++cuts;
								}
							}
						}
					}
					if ((cuts % 2) == 1)
					{
						// add a square
						int firstEdge = tileNavEdges.get_size();
						tileNavEdges.push_back(NavEdge(bottomLeft, Vector2(bottomLeft.x, topRight.y), true));
						tileNavEdges.push_back(NavEdge(Vector2(bottomLeft.x, topRight.y), topRight, true));
						tileNavEdges.push_back(NavEdge(topRight, Vector2(topRight.x, bottomLeft.y), true));
						tileNavEdges.push_back(NavEdge(Vector2(topRight.x, bottomLeft.y), bottomLeft, true));
						// connect them together
						for_count(int, e, 4)
						{
							tileNavEdges[firstEdge + e].connectsTo = firstEdge + mod(e + 1, 4);
							tileNavEdges[firstEdge + e].connectsFrom = firstEdge + mod(e - 1, 4);
						}
					}
				}
				else
				{
					bool outerEdges = false;

					// fill the edges
					bool keepDoing = true;
					while (keepDoing)
					{
						keepDoing = false;
						for (int i = 0; i < tileNavEdges.get_size(); ++i)
						{
							auto& ne = tileNavEdges[i];
							if (ne.connectsTo != NONE) continue;
							Vector2 to = tileNavEdges[i].to;
							for_every(one, tileNavEdges)
							{
								if (one->connectsFrom == NONE && one->from == to && for_everys_index(one) != i)
								{
									ne.connectsTo = for_everys_index(one);
									one->connectsFrom = i;
									break;
								}
							}

							if (ne.connectsTo == NONE)
							{
								//Vector2 at = ne.to;
								Vector2 dir = Vector2::zero;
								// corners first, edges next; we're going clockwise
								if ((ne.to - Vector2(gridTile.x.min, gridTile.y.max)).is_almost_zero()) dir = Vector2(1.0f, 0.0f); else
								if ((ne.to - Vector2(gridTile.x.max, gridTile.y.max)).is_almost_zero()) dir = Vector2(0.0f, -1.0f); else
								if ((ne.to - Vector2(gridTile.x.max, gridTile.y.min)).is_almost_zero()) dir = Vector2(-1.0f, 0.0f); else
								if ((ne.to - Vector2(gridTile.x.min, gridTile.y.min)).is_almost_zero()) dir = Vector2(0.0f, 1.0f); else
								if (abs(ne.to.x - gridTile.x.min) < EPSILON) dir = Vector2(0.0f, 1.0f); else
								if (abs(ne.to.x - gridTile.x.max) < EPSILON) dir = Vector2(0.0f, -1.0f); else
								if (abs(ne.to.y - gridTile.y.min) < EPSILON) dir = Vector2(-1.0f, 0.0f); else
								if (abs(ne.to.y - gridTile.y.max) < EPSILON) dir = Vector2(1.0f, 0.0f);

								if (dir.is_zero())
								{
									error(TXT("it is inside a tile, it should have something to connect to"));
								}
								else
								{
									outerEdges = true;

									int bestIdx = NONE;
									float bestDist = 0.0f;
									for_every(one, tileNavEdges)
									{
										int oneIdx = for_everys_index(one);
										if (one->connectsFrom == NONE && oneIdx != i)
										{
											Vector2 diff = one->from - ne.to;
											if ((dir.y == 0.0f && abs(diff.y) < EPSILON) ||
												(dir.x == 0.0f && abs(diff.x) < EPSILON))
											{
												// it is in line
												float dist = Vector2::dot(diff, dir);
												if (dist > 0.0f)
												{
													if (bestIdx == NONE || dist < bestDist)
													{
														bestIdx = oneIdx;
														bestDist = dist;
													}
												}
											}
										}
									}

									Optional<Vector2> goTo;
									if (bestIdx == NONE)
									{
										// get to the corner
										if (dir.x > 0.0f) goTo = gridTile.get_at(Vector2(1.0f, 1.0f));
										if (dir.x < 0.0f) goTo = gridTile.get_at(Vector2(0.0f, 0.0f));
										if (dir.y > 0.0f) goTo = gridTile.get_at(Vector2(0.0f, 1.0f));
										if (dir.y < 0.0f) goTo = gridTile.get_at(Vector2(1.0f, 0.0f));
									}
									else
									{
										goTo = tileNavEdges[bestIdx].from;
									}

									an_assert(goTo.is_set());

									if (goTo.is_set())
									{
										// we will soon add an edge, we should modify ne before we do that
										ne.connectsTo = tileNavEdges.get_size(); // new one

										NavEdge nne(ne.to, goTo.get(), NP);
										nne.connectsFrom = i;
										if (bestIdx != NONE)
										{
											// connect to the one we already found, so we don't go through the joys of connecting that again
											nne.connectsTo = bestIdx;
											tileNavEdges[bestIdx].connectsFrom = tileNavEdges.get_size(); // new one
										}
										tileNavEdges.push_back(nne);

										keepDoing = true;
									}
									else
									{
										error(TXT("couldn't build nav mesh"));
										return;
									}
								}
							}
						}
					}

					if (! singleGridTile)
					{
						// we may need outer edges to connect between tiles

						// check if we have outer edges, anything on them
						if (!outerEdges)
						{
							for_every(ne, tileNavEdges)
							{
								if ((abs(ne->to.x - gridTile.x.min) < EPSILON) ||
									(abs(ne->to.x - gridTile.x.max) < EPSILON) ||
									(abs(ne->to.y - gridTile.y.min) < EPSILON) ||
									(abs(ne->to.y - gridTile.y.max) < EPSILON))
								{
									outerEdges = true;
									break;
								}
							}
						}

						// still no outer edges? add a square
						if (!outerEdges)
						{
							int firstEdge = tileNavEdges.get_size();
							tileNavEdges.push_back(NavEdge(bottomLeft, Vector2(bottomLeft.x, topRight.y), true));
							tileNavEdges.push_back(NavEdge(Vector2(bottomLeft.x, topRight.y), topRight, true));
							tileNavEdges.push_back(NavEdge(topRight, Vector2(topRight.x, bottomLeft.y), true));
							tileNavEdges.push_back(NavEdge(Vector2(topRight.x, bottomLeft.y), bottomLeft, true));
							// connect them together
							for_count(int, e, 4)
							{
								tileNavEdges[firstEdge + e].connectsTo = firstEdge + mod(e + 1, 4);
								tileNavEdges[firstEdge + e].connectsFrom = firstEdge + mod(e - 1, 4);
							}
						}
					}

					// look for islands
					// afterthought: we're not looking for islands but for pieces that are holes, hence all clockwise are marked as main
					{
						for_every(ne, tileNavEdges)
						{
							ne->used = false;
						}

						// check for those that touch borders
						for_every(ne, tileNavEdges)
						{
							if (ne->clockwiseChain.get(false))
							{
								ne->used = true;
								continue;
							}
							if ((abs(ne->to.x - gridTile.x.min) < EPSILON) ||
								(abs(ne->to.x - gridTile.x.max) < EPSILON) ||
								(abs(ne->to.y - gridTile.y.min) < EPSILON) ||
								(abs(ne->to.y - gridTile.y.max) < EPSILON))
							{
								ne->used = true;
								continue;
							}
							if ((abs(ne->from.x - gridTile.x.min) < EPSILON) ||
								(abs(ne->from.x - gridTile.x.max) < EPSILON) ||
								(abs(ne->from.y - gridTile.y.min) < EPSILON) ||
								(abs(ne->from.y - gridTile.y.max) < EPSILON))
							{
								ne->used = true;
								continue;
							}
						}

						// look for islands, if present, try to connect it to non-island
						{
							bool island = true;
							while (island)
							{
								// propagate islands
								propagate_used(tileNavEdges);

								island = false;
								// we want to connect at TO, we will be creating two edges that go both directions
								//	-one->*----->	main
								//
								//
								//	  <---*<-ne		island
								//
								//	after connection:
								//
								//	----->*----->	main
								//       | ^
								//       v |
								//	  <---*<---		island
								Optional<int> connectIsland;
								Optional<int> connectMain;
								float bestDist = 0.0f;
								for_every(ne, tileNavEdges)
								{
									if (!ne->used)
									{
										island = true;
										for_every(one, tileNavEdges)
										{
											if (one->used)
											{
												Vector2 islandAt = ne->to;
												Vector2 mainAt = one->to;

												float dist = (mainAt - islandAt).length();

												// this might be an improvement
												if (!connectIsland.is_set() || dist < bestDist)
												{
													// check if we intersect with anything else
													Segment2 islandToMainSeg(islandAt, mainAt);
													bool noIntersection = true;
													for_every(ane, tileNavEdges)
													{
														if (ane != ne && ane != one)
														{
															Vector2 aneDir = (ane->to - ane->from).normal() * 0.001f; // better to not go too close
															Segment2 aneSeg(ane->from - aneDir, ane->to + aneDir);
															if (islandToMainSeg.does_intersect_with(aneSeg))
															{
																noIntersection = false;
																break;
															}
														}
													}
													if (noIntersection)
													{
														connectIsland = for_everys_index(ne);
														connectMain = for_everys_index(one);
														bestDist = dist;
													}
												}
											}
										}
									}
								}
								if (island)
								{
									if (connectIsland.is_set())
									{
										//	--me->*-nme->	main
										//       | ^
										//       v |
										//	  <nie*<-ie		island

										int islandIdx = connectIsland.get();
										int mainIdx = connectMain.get();
										auto& islandEdge = tileNavEdges[islandIdx];
										auto& mainEdge = tileNavEdges[mainIdx];
										if (mainEdge.connectsTo == NONE ||
											islandEdge.connectsTo == NONE)
										{
											error(TXT("main and island have to create a chain"));
											return;
										}
										int nextIslandIdx = islandEdge.connectsTo;
										int nextMainIdx = mainEdge.connectsTo;
										auto& nextIslandEdge = tileNavEdges[nextIslandIdx];
										auto& nextMainEdge = tileNavEdges[nextMainIdx];

										int mainToIslandIdx = tileNavEdges.get_size();
										int islandToMainIdx = mainToIslandIdx + 1;

										// island
										nextIslandEdge.connectsFrom = mainToIslandIdx;
										islandEdge.connectsTo = islandToMainIdx;

										// main
										nextMainEdge.connectsFrom = islandToMainIdx;
										mainEdge.connectsTo = mainToIslandIdx;

										NavEdge mainToIslandEdge(mainEdge.to, islandEdge.to, NP);
										mainToIslandEdge.connectsFrom = mainIdx;
										mainToIslandEdge.connectsTo = nextIslandIdx;

										NavEdge islandToMainEdge(islandEdge.to, mainEdge.to, NP);
										islandToMainEdge.connectsFrom = islandIdx;
										islandToMainEdge.connectsTo = nextMainIdx;

										// add, finally
										tileNavEdges.push_back(mainToIslandEdge);
										tileNavEdges.push_back(islandToMainEdge);
									}
									else
									{
										error(TXT("can't connect an island"));
										return;
									}
								}
							}
						}
					}
				}

#ifdef AN_DEVELOPMENT
				for_every(ne, tileNavEdges)
				{
					allNavEdges.push_back(*ne);
				}
#endif
				// convert into triangles
				{
					for_every(ne, tileNavEdges)
					{
						ne->used = false;
					}

					Array<Vector2> polygon;
					Array<PolygonUtils::Triangle> triangles;

					bool keepDoing = true;
					while (keepDoing)
					{
						keepDoing = false;
						for_every(ne, tileNavEdges)
						{
							if (! ne->used)
							{
								ne->used = true;
								polygon.clear();
								polygon.push_back(ne->from);

								int neIdx = for_everys_index(ne);
								int idx = ne->connectsTo;
								while (idx != NONE && idx != neIdx)
								{
									auto & one = tileNavEdges[idx];
									one.used = true;
									polygon.push_back(one.from);
									idx = one.connectsTo;
								}

								PolygonUtils::triangulate(polygon, triangles);
							}
						}
					}

#ifdef AN_DEVELOPMENT
					for_every(tri, triangles)
					{
						triangulised.push_back(*tri);
					}
#endif
					
					{
						while (! triangles.is_empty())
						{
							auto const t = triangles.get_last();
							triangles.pop_back();
							ConvexPolygon cp;
							cp.add(t.a);
							cp.add(t.b);
							cp.add(t.c);
							cp.clear_keep_points();
							cp.build();

							// try to add a triangle that shares an edge - try to keep it convex
							bool keepTrying = true; // we may need multiple passes as some triangles may become feasable only if we add others
							while (keepTrying)
							{
								keepTrying = false;
								for (int triIdx = 0; triIdx < triangles.get_size(); ++triIdx)
								{
									auto const t = triangles[triIdx];
									Vector2 toAdd;
									if (cp.does_share_edge_with_triangle(t.a, t.b, t.c, &toAdd))
									{
										if (cp.can_add_to_remain_convex(toAdd))
										{
											cp.clear_keep_points();
											cp.add(toAdd);
											cp.build();
											triangles.remove_fast_at(triIdx);
											--triIdx;
											keepTrying = true;
										}
									}
								}
							}

							Nav::Nodes::ConvexPolygon* navNodePolygon = Nav::Nodes::ConvexPolygon::get_one();
							if (owner)
							{
								Transform ownersPlacement = owner->get_presence()->get_placement();
								navNodePolygon->place_LS(ownersPlacement.to_local(placement), owner);
								for_count(int, i, cp.get_edge_count())
								{
									navNodePolygon->add_clockwise(cp.get_edge(i)->a.to_vector3());
								}
							}
							else
							{
								navNodePolygon->place_WS(placement, nullptr);
								for_count(int, i, cp.get_edge_count())
								{
									navNodePolygon->add_clockwise(cp.get_edge(i)->a.to_vector3());
								}
							}
							navNodePolygon->build(0.0f);
							navNodePolygon->be_open_node(true);

							_navNodes.push_back(navNodePolygon);
#ifdef AN_DEVELOPMENT
							convexPolygons.push_back(cp);
#endif
						}
					}
				}
			}
		}
	}

public:
	void debug_draw_voxels()
	{
#ifdef AN_DEBUG_RENDERER
		debug_push_transform(placement);

		Range3 floorVoxel = get_floor_voxel();
		Range3 lowVoxel = get_low_voxel();
		Range3 highVoxel = get_high_voxel();

		for_range(int, y, voxelRange.y.min + 1, voxelRange.y.max - 1)
		{
			for_range(int, x, voxelRange.x.min + 1, voxelRange.x.max - 1)
			{
				int& voxel = access_voxel(x, y);
				int& voxelxp = access_voxel(x + 1, y);
				int& voxelxm = access_voxel(x - 1, y);
				int& voxelyp = access_voxel(x, y + 1);
				int& voxelym = access_voxel(x, y - 1);

				if (voxel != 0)
				{
					update_voxels_for_xy(floorVoxel, lowVoxel, highVoxel, x, y);

					int l[] = { is_flag_set(voxel, Floor) , is_flag_set(voxel, Low), is_flag_set(voxel, High) };
					int lxp[] = { is_flag_set(voxelxp, Floor) , is_flag_set(voxelxp, Low), is_flag_set(voxelxp, High) };
					int lxm[] = { is_flag_set(voxelxm, Floor) , is_flag_set(voxelxm, Low), is_flag_set(voxelxm, High) };
					int lyp[] = { is_flag_set(voxelyp, Floor) , is_flag_set(voxelyp, Low), is_flag_set(voxelyp, High) };
					int lym[] = { is_flag_set(voxelym, Floor) , is_flag_set(voxelym, Low), is_flag_set(voxelym, High) };
					Range3 r[] = { floorVoxel, lowVoxel, highVoxel };

					for_count(int, li, 3)
					{
						if (li == 0 && mod(x, 2) == mod(y, 2) && (abs(x) > 2 || abs(y) > 2))
						{
							continue;
						}
						Range3 const& rng = r[li];
						Vector3 nnn = rng.get_at(Vector3(0.0f, 0.0f, 0.0f));
						Vector3 npn = rng.get_at(Vector3(0.0f, 1.0f, 0.0f));
						Vector3 ppn = rng.get_at(Vector3(1.0f, 1.0f, 0.0f));
						Vector3 pnn = rng.get_at(Vector3(1.0f, 0.0f, 0.0f));
						Vector3 nnp = rng.get_at(Vector3(0.0f, 0.0f, 1.0f));
						Vector3 npp = rng.get_at(Vector3(0.0f, 1.0f, 1.0f));
						Vector3 ppp = rng.get_at(Vector3(1.0f, 1.0f, 1.0f));
						Vector3 pnp = rng.get_at(Vector3(1.0f, 0.0f, 1.0f));
						Colour c = Colour::red;
						if (li == 1)
						{
							c = Colour::orange;
						}
						if (li == 2)
						{
							c = Colour::yellow;
						}
						if (x == 0 && y == 0)
						{
							c = Colour::purple;
						}
						if (x == 1 && y == 0)
						{
							c = Colour::blue;
						}
						if (x == 0 && y == 1)
						{
							c = Colour::green;
						}
						c = c.with_alpha(0.3f);
						if (li == 0 && l[li])
						{
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, Colour::magenta, nnp, npp, ppp));
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, Colour::magenta, nnp, ppp, pnp));
						}
						if (li < 2 && l[li] && !l[li + 1])
						{
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, nnp, npp, ppp));
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, nnp, ppp, pnp));
						}
						if (li > 0 && l[li] && !l[li - 1])
						{
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, nnn, pnn, ppn));
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, nnn, ppn, npn));
						}
						if (l[li] && ! lxp[li])
						{
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, pnn, pnp, ppp));
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, pnn, ppp, ppn));
						}
						if (l[li] && ! lxm[li])
						{
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, npn, npp, nnp));
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, npn, nnp, nnn));
						}
						if (l[li] && ! lyp[li])
						{
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, ppn, npp, npn));
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, ppn, ppp, npp));
						}
						if (l[li] && ! lym[li])
						{
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, nnn, nnp, pnp));
							debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, nnn, pnp, pnn));
						}
					}
				}
			}
		}

		debug_pop_transform();
#endif
	}
	
	void debug_draw_distance_border(int _maxDist, bool _border)
	{
#ifdef AN_DEBUG_RENDERER
		debug_push_transform(placement);

		Range3 floorVoxel = get_floor_voxel();
		Range3 lowVoxel = get_low_voxel();
		Range3 highVoxel = get_high_voxel();

		int maxd = NONE;
		for_range(int, y, voxelRange.y.min + 1, voxelRange.y.max - 1)
		{
			for_range(int, x, voxelRange.x.min + 1, voxelRange.x.max - 1)
			{
				maxd = max(maxd, access_distance(x, y));
			}
		}
		if (maxd > 0)
		{
			for_range(int, y, voxelRange.y.min + 1, voxelRange.y.max - 1)
			{
				for_range(int, x, voxelRange.x.min + 1, voxelRange.x.max - 1)
				{
					int& dist = access_distance(x, y);
					int& bord = access_border(x, y);

					if ((dist != NONE && dist <= _maxDist) ||
						(bord > 0 && _border))
					{
						update_voxels_for_xy(floorVoxel, lowVoxel, highVoxel, x, y);

						if (mod(x, 2) == mod(y, 2) && _maxDist > 5)
						{
							continue;
						}
						Box floorBox; floorBox.setup(floorVoxel);
						auto& box = floorBox;
						Vector3 nn = box.get_centre() - box.get_x_axis() * box.get_half_size().x - box.get_y_axis() * box.get_half_size().y;
						Vector3 np = box.get_centre() - box.get_x_axis() * box.get_half_size().x + box.get_y_axis() * box.get_half_size().y;
						Vector3 pp = box.get_centre() + box.get_x_axis() * box.get_half_size().x + box.get_y_axis() * box.get_half_size().y;
						Vector3 pn = box.get_centre() + box.get_x_axis() * box.get_half_size().x - box.get_y_axis() * box.get_half_size().y;
						Colour c = Colour::none;
						if (_maxDist >= 0)
						{
							float w = (float)dist / (float)(maxd);
							c = Colour::lerp(w, Colour::blue, Colour::red);
						}
						if (_border && bord > 0)
						{
							c = Colour::purple;
							if (bord >= 2)
							{
								c = Colour::white;
							}
						}
						c = c.with_alpha(0.4f);
						debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, nn, np, pp));
						debug_draw_time_based(1000000.0f, debug_draw_triangle(true, c, nn, pp, pn));
					}
				}
			}
		}

		debug_pop_transform();
#endif
	}

	void debug_draw_border_lines(int _bits = 0xffffffff)
	{
#ifdef AN_DEBUG_RENDERER
		debug_push_transform(placement);

		float offset = 0.0f;
		if (is_flag_set(_bits, bit(1)))
		{
			for_every(borderLine, borderLinesRaw)
			{
				Vector3 a = get_voxel_floor_centre(borderLine->from) + Vector3::zAxis * offset;
				Vector3 b = get_voxel_floor_centre(borderLine->to) + Vector3::zAxis * offset;
				Colour c = borderLine->used ? Colour::orange : Colour::yellow;
				debug_draw_time_based(1000000.0f, debug_draw_arrow(true, c, a, b));
			}
			offset += 0.01f;
		}

		if (is_flag_set(_bits, bit(2)))
		{
			for_every(borderLine, borderLinesLong)
			{
				Vector3 a = get_voxel_floor_centre(borderLine->from) + Vector3::zAxis * offset;
				Vector3 b = get_voxel_floor_centre(borderLine->to) + Vector3::zAxis * offset;
				Colour c = Colour::cyan;
				debug_draw_time_based(1000000.0f, debug_draw_arrow(true, c, a, b));
			}
			offset += 0.01f;
		}

		if (is_flag_set(_bits, bit(3)))
		{
			for_every(bc, borderChains)
			{
				VectorInt2 last = bc->linesLong.get_last().to;
				for_every(borderLine, bc->linesLong)
				{
					Vector3 a = get_voxel_floor_centre(last) + Vector3::zAxis * 0.02f;
					Vector3 b = get_voxel_floor_centre(borderLine->to) + Vector3::zAxis * offset;
					Colour c = Colour::purple;
					debug_draw_time_based(1000000.0f, debug_draw_arrow(true, c, a, b));
					last = borderLine->to;
				}
			}
			offset += 0.01f;
		}
		if (is_flag_set(_bits, bit(4)))
		{
			for_every(bc, borderChains)
			{
				if (!bc->linesSimple.is_empty())
				{
					VectorInt2 last = bc->linesSimple.get_last().to;
					for_every(borderLine, bc->linesSimple)
					{
						Vector3 a = get_voxel_floor_centre(last) + Vector3::zAxis * 0.02f;
						Vector3 b = get_voxel_floor_centre(borderLine->to) + Vector3::zAxis * offset;
						Colour c = Colour::white;
						debug_draw_time_based(1000000.0f, debug_draw_arrow(true, c, a, b));
						last = borderLine->to;
					}
				}
			}
			offset += 0.01f;
		}

		debug_pop_transform();
#endif
	}

	void debug_draw_all_nav_edges()
	{
#ifdef AN_DEVELOPMENT
		debug_push_transform(placement);

		for_every(ne, allNavEdges)
		{
			Vector2 neDir = ne->to - ne->from;
			Vector2 neRight = neDir.rotated_right().normal() * 0.001f;

			float offset = 0.05f;
			Vector3 from = (ne->from + neRight).to_vector3() + Vector3::zAxis * offset;
			Vector3 to = (ne->to + neRight).to_vector3() + Vector3::zAxis * offset;
			Colour c = Colour::c64LightRed;
			debug_draw_time_based(1000000.0f, debug_draw_arrow(true, c, from, to));
		}

		debug_pop_transform();
#endif
	}

	void debug_draw_triangulised()
	{
#ifdef AN_DEVELOPMENT
		debug_push_transform(placement);

		for_every(tri, triangulised)
		{
			float offset = 0.06f;
			Vector3 a = tri->a.to_vector3() + Vector3::zAxis * offset;
			Vector3 b = tri->b.to_vector3() + Vector3::zAxis * offset;
			Vector3 c = tri->c.to_vector3() + Vector3::zAxis * offset;
			Vector3 centre = (a + b + c) / 3.0f;
			a = centre + (a - centre) * 0.99f;
			b = centre + (b - centre) * 0.99f;
			c = centre + (c - centre) * 0.99f;
			Colour colour = Colour::c64LightGreen;
			debug_draw_time_based(1000000.0f, debug_draw_arrow(true, colour, a, b));
			debug_draw_time_based(1000000.0f, debug_draw_arrow(true, colour, b, c));
			debug_draw_time_based(1000000.0f, debug_draw_arrow(true, colour, c, a));
		}

		debug_pop_transform();
#endif
	}

	void debug_draw_convex_polygons()
	{
#ifdef AN_DEVELOPMENT
		debug_push_transform(placement);

		for_every(cp, convexPolygons)
		{
			Colour c = Colour::c64LightBlue;
			for_count(int, i, cp->get_edge_count())
			{
				Vector2 a = cp->get_edge(i)->a;
				Vector2 b = cp->get_edge(i)->b;
				Vector2 neRight = (b - a).rotated_right().normal() * 0.001f;

				float offset = 0.05f;
				Vector3 from = (a + neRight).to_vector3() + Vector3::zAxis * offset;
				Vector3 to = (b + neRight).to_vector3() + Vector3::zAxis * offset;
				debug_draw_time_based(1000000.0f, debug_draw_arrow(true, c, from, to));
			}
		}

		debug_pop_transform();
#endif
	}

};

void BuildNavMesh::generate_polygons(OUT_ Array<Nav::Node*>& _navNodes, IModulesOwner* _owner, Optional<Vector3> const & _probeLocWS, Array<Vector3> const& _pointsClockwiseWS, float _voxelSize, float _gridTileSize, float _pushInwards, float _height, float _heightLow)
{
	if (_pointsClockwiseWS.get_size() < 3)
	{
		error(TXT("too few points to generate polygons"));
		return;
	}
	Vector3 centre = Vector3::zero;
	{
		for_every(p, _pointsClockwiseWS)
		{
			centre += *p;
		}
		centre /= (float)_pointsClockwiseWS.get_size();
	}

	Vector3 forward = (_pointsClockwiseWS[0] - centre).normal();
	Vector3 right = Vector3::zero;
	float bestDot = 1.0f;
	for (int i = 1; i < _pointsClockwiseWS.get_size(); ++i)
	{
		Vector3 r = (_pointsClockwiseWS[i] - centre).normal();
		float dot = abs(Vector3::dot(forward, r));
		if (dot < bestDot && dot <= 0.9999f)
		{
			bestDot = dot;
			right = r;
			break;
		}
	}

	Vector3 up;
	if (right.is_zero())
	{
		warn_dev_investigate(TXT("we didn't get the right angle, hope for the best, with flat nav mesh it should always work"));
		up = Vector3::zAxis;
	}
	else
	{
		up = Vector3::cross(right, forward).normal();
	}
	if ((up - Vector3::zAxis).length() < 0.05f ||
		(up + Vector3::zAxis).length() < 0.05f)
	{
		// normalise
		forward = Vector3::yAxis;
	}
	right = Vector3::cross(forward, up).normal();

	Transform placement = look_matrix(centre, forward, up).to_transform();

	Array<Vector2> pointsToBlockWith;
	Range2 spaceTaken = Range2::empty;

	for_every(p, _pointsClockwiseWS)
	{
		Vector2 local = placement.location_to_local(*p).to_vector2();
		pointsToBlockWith.push_back(local);
		spaceTaken.include(local);
	}

	BuildNavMeshHelper bnmHelper(placement, _owner, spaceTaken, _voxelSize, _gridTileSize, _pushInwards, _height, _heightLow);

	bnmHelper.block_voxels_using(pointsToBlockWith);

	{
		for_every_ref(object, roomObjects)
		{
			if (auto* collision = object->get_collision())
			{
				if (collision->is_nav_obstructor())
				{
					// we require top bone so we only consider the main part, not the other parts that might be created not in place
					Name topBone;
					if (auto* a = object->get_appearance())
					{
						if (auto* fs = a->get_skeleton())
						{
							if (auto* s = fs->get_skeleton())
							{
								if (s->get_num_bones() > 0)
								{
									topBone = s->get_bones()[0].get_name();
								}
							}
						}
					}
					auto& cmis = collision->get_movement_collision();
					for_every(ci, cmis.get_instances())
					{
						bnmHelper.add_collision(*ci, object->get_presence()->get_placement(), topBone);
					}
				}
			}
		}

		for_every_ref(dir, doorsInRoom)
		{
			auto& cmis = dir->get_movement_collision();
			for_every(ci, cmis.get_instances())
			{
				bnmHelper.add_collision(*ci, dir->get_placement());
			}
		}

		todo_note(TXT("add doors here?"))

		{
			auto& cmis = room->get_movement_collision();
			for_every(ci, cmis.get_instances())
			{
				bnmHelper.add_collision(*ci, Transform::identity);
			}
		}
	}

	bnmHelper.calculate_distance_and_borders(OUT_ _navNodes, _probeLocWS);

#ifdef AN_VISUALISE_GENERATE_POLYGONS
	debug_context(room);
	//bnmHelper.debug_draw_voxels();
	//bnmHelper.debug_draw_distance_border(2, true);
	//bnmHelper.debug_draw_border_lines(bit(4));
	bnmHelper.debug_draw_all_nav_edges();
	//bnmHelper.debug_draw_triangulised();
	//bnmHelper.debug_draw_convex_polygons();
	debug_no_context();
#endif
}
