#include "roomProxy.h"

#include "..\..\core\types\names.h"

#include "renderScene.h"
#include "sceneBuildContext.h"
#include "sceneRenderContext.h"
#include "renderContext.h"
#include "presenceLinkProxy.h"
#include "environmentProxy.h"
#include "doorInRoomProxy.h"

#include "..\game\game.h"

#include "..\world\room.h"
#include "..\world\region.h"
#include "..\world\subWorld.h"
#include "..\world\world.h"
#include "..\world\doorInRoom.h"
#include "..\world\presenceLink.h"
#include "..\world\environment.h"

#include "..\..\core\mainSettings.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\mesh\combinedMesh3dInstanceSet.h"
#include "..\..\core\system\input.h"
#include "..\..\core\system\video\iMaterialInstanceProvider.h"
#include "..\..\core\system\video\renderTargetSave.h"
#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"

#ifdef DEBUG_RENDER_DOOR_HOLES
#include "..\..\core\system\core.h"
#endif

#ifdef AN_ALLOW_BULLSHOTS
#include "..\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#define USE_SCISSORS

//#define DISABLE_DOOR_RENDERING

//#define DISABLE_CLIP_PLANES
 
//#define DEBUG_BUILD_ROOM_PROXY

//#define DEBUG_DOOR_HOLES

using namespace Framework;
using namespace Framework::Render;

//

DEFINE_STATIC_NAME(skyBox);
DEFINE_STATIC_NAME(background);

// shader program for doors
DEFINE_STATIC_NAME(doorOpen);

// shader options
DEFINE_STATIC_NAME(renderSkyBoxFirst);

//

#ifdef DEBUG_COMPLETE_FRONT_PLANE
bool debugCompleteFrontPlane = true;
#endif

#ifdef DEBUG_RENDER_DOOR_HOLES
bool RoomProxy::debugRenderDoorHolesCloseAlwaysDepth = true;
bool RoomProxy::debugRenderDoorHolesDontUseDepthRange = false;
bool RoomProxy::debugRenderDoorHolesPaddedDepthRange = false;
bool RoomProxy::debugRenderDoorHolesPartially = false;
bool RoomProxy::debugRenderDoorHolesPartiallyOnlyNormal = false;
bool RoomProxy::debugRenderDoorHolesPartiallyOnlyNear = false;
bool RoomProxy::debugRenderDoorHolesNoExtend = true;
bool RoomProxy::debugRenderDoorHolesNegativeExtend = false;
bool RoomProxy::debugRenderDoorHolesFrontCapZSubZero = false;
bool RoomProxy::debugRenderDoorHolesFrontCapZZero = false;
bool RoomProxy::debugRenderDoorHolesFrontCapZSmall = false;
bool RoomProxy::debugRenderDoorHolesFrontCapZLarger = false;
bool RoomProxy::debugRenderDoorHolesFrontCapZSmaller = false;
bool RoomProxy::debugRenderDoorHolesSmallerCap = false;
#endif

Range const g_depthRangePadded(0.005f, 0.995f);
#ifdef AN_GLES
Range const g_depthRange(0.002f, 0.9995f); // for gles we have a lower depth buffer resolution and using anything below 0.001f would be the same as 0
#else
Range const g_depthRange(0.0001f, 0.9999f);
#endif

void RoomProxy::set_rendering_depth_range(::System::Video3D* v3d)
{
#ifdef USE_DEPTH_RANGE
#ifdef DEBUG_RENDER_DOOR_HOLES
	if (!debugRenderDoorHolesDontUseDepthRange)
#endif
	{
#ifdef DEBUG_RENDER_DOOR_HOLES
		if (debugRenderDoorHolesPaddedDepthRange)
		{
			v3d->depth_range(g_depthRangePadded.min, g_depthRangePadded.max); // use the rest, allow front cap to be rendered fully as something separate that can't be overwritten
		}
		else
#endif
		{
			v3d->depth_range(g_depthRange.min, g_depthRange.max); // use the rest, allow front cap to be rendered fully as something separate that can't be overwritten
		}
	}
#endif
}

void RoomProxy::set_rendering_depth_range_front(::System::Video3D* v3d)
{
#ifdef USE_DEPTH_RANGE
#ifdef DEBUG_RENDER_DOOR_HOLES
	if (!debugRenderDoorHolesDontUseDepthRange)
#endif
	{
		v3d->depth_range(0.0f, 0.0f);
	}
#endif
}

void RoomProxy::set_rendering_depth_range_back(::System::Video3D* v3d)
{
#ifdef USE_DEPTH_RANGE
#ifdef DEBUG_RENDER_DOOR_HOLES
	if (!debugRenderDoorHolesDontUseDepthRange)
#endif
	{
		v3d->depth_range(1.0f, 1.0f);
	}
#endif
}

void RoomProxy::clear_rendering_depth_range(::System::Video3D* v3d)
{
#ifdef USE_DEPTH_RANGE
#ifdef DEBUG_RENDER_DOOR_HOLES
	if (!debugRenderDoorHolesDontUseDepthRange)
#endif
	{
		v3d->depth_range(0.0f, 1.0f);
	}
#endif
}

RoomProxy::RoomProxy()
{
}

#ifdef AN_DEBUG_RENDERER
static Vector3 match_y(Vector3 _v, float _y)
{
	if (_v.y != 0.0f)
	{
		float adj = _y / _v.y;
		return _v * adj;
	}
	else
	{
		return _v;
	} 
}
#endif

RoomProxy* RoomProxy::build(SceneBuildContext & _context, Room * _room, RoomProxy * _fromRoomProxy, DoorInRoom const * _throughDoorFromOuter)
{
	if (! _room)
	{
		return nullptr;
	}

#ifdef AN_DEBUG_RENDERER
	if (Framework::Render::ContextDebug::use &&
		!_throughDoorFromOuter)
	{
		Framework::Render::ContextDebug::use->start_rendering();
	}
#endif

	Matrix44 const currentViewMatrix = _context.get_view_matrix();
	Matrix44 const currentThroughDoorMatrix = _context.get_through_door_matrix();
	Matrix44 swappedViewMatrix;
	Matrix44 swappedThroughDoorMatrix;

	::System::ViewFrustum* swappedClippedDoorVF = nullptr;
	::System::ViewFrustum clippedDoorVF; // keep it here, as we're going to use it for swapped

	// get new room proxy
	RoomProxy* roomProxy = nullptr;

	// if we're entering through door
	if (_throughDoorFromOuter)
	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE_COLOURED(buildRoomProxyThroughDoor, Colour::zxCyan);
#endif

		bool doorOk = true;

		// check if camera (0,0,0) is in front of door if they are in current model view space
		float frontDist;
		float nearPlane = _context.get_near_plane();
		float maxDist = _context.get_max_door_dist();
		{
			bool anyOk = false;
			{
				// we calculate both in MVS and in door-in-room's room's space
				// to get the higher value and we have a generous threshold as well
				// this is due to the problems with floating point accuracy
				Vector3 placementInDoorsSpace = currentViewMatrix.location_to_local(Vector3::zero);
				Plane doorPlaneInMVS = currentViewMatrix.to_world(_throughDoorFromOuter->get_plane());
				frontDist = doorPlaneInMVS.get_in_front(Vector3::zero);
				float frontDist2 = _throughDoorFromOuter->get_plane().get_in_front(placementInDoorsSpace);
				static float threshold = -0.0001f;
				frontDist = max(frontDist, frontDist2);
				if (frontDist <= threshold)
				{
					doorOk = false;
				}
				else
				{
					anyOk = true;
				}
			}
#ifdef AN_ALLOW_BULLSHOTS
			if (BullshotSystem::is_active() &&
				BullshotSystem::get_near_z().get(0.0f) > 0.5f)
			{
				// for larger near z forced by bullshot system, throw out closer doors
				Vector3 doorCentreInMVS = currentViewMatrix.location_to_world(_throughDoorFromOuter->get_hole_centre_WS());
				if (doorCentreInMVS.y < BullshotSystem::get_near_z().get(0.0f))
				{
					anyOk = false;
				}
			}
#endif
			if (! anyOk)
			{
#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
				if (::System::Input::get()->has_key_been_pressed(::System::Key::I) &&
					::System::Input::get()->is_key_pressed(::System::Key::RightShift))
				{
					output_colour(0, 1, 0, 0);
					output(TXT("  room %i"), _room);
					output_colour();
				}
#endif
#endif
				return nullptr;
			}
		}

		// handle doorhole to check if current doorhole intersects with new one - cut new one to existing and check if it isn't empty
		// (build frustum and check against current one)
		// note: we're ok with creating new view frustums as they are pooled
		::System::ViewFrustum doorVF;
		float doorDistanceToCamera = 0.0f;
		DoorHole const* doorHole = _throughDoorFromOuter->get_hole();
		{
			bool anyOk = false;
			if (doorOk) // only if frustum is not empty, otherwise we are actually not rendering anything here
			{
				doorHole->setup_frustum_view(_throughDoorFromOuter->get_side(), _throughDoorFromOuter->get_hole_scale(), doorVF, currentViewMatrix, _throughDoorFromOuter->get_outbound_matrix());
				{
					Vector3 cameraInDoorSpace = _throughDoorFromOuter->get_outbound_matrix().location_to_local(currentViewMatrix.location_to_local(Vector3::zero));
					doorDistanceToCamera = cameraInDoorSpace.length();
					if (_context.get_frustum()->is_empty())
					{
						clippedDoorVF = doorVF; // use whole door
						anyOk = true;
					}
					else
					{
						if (frontDist <= nearPlane &&
							doorHole->is_point_inside(_throughDoorFromOuter->get_side(), _throughDoorFromOuter->get_hole_scale(), cameraInDoorSpace, maxDist))
						{
							// if too close (actually at the door), just keep frustum?
							clippedDoorVF = *_context.get_frustum();
							anyOk = true;
						}
						else if (clippedDoorVF.clip(&doorVF, _context.get_frustum()))
						{
							anyOk = true;
						}
						else
						{
							doorOk = false;
						}
					}
				}
			}
			if (!anyOk)
			{
#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
				if (::System::Input::get()->has_key_been_pressed(::System::Key::I) &&
					::System::Input::get()->is_key_pressed(::System::Key::RightShift))
				{
					output_colour(0, 1, 0, 0);
					output(TXT("  room %i (view frustum)"), _room);
					output_colour();
				}
#endif
#endif
				return nullptr;
			}
		}

		// check view planes if the door is actually within the view planes
		{
			if (! _context.get_planes()->is_empty())
			{
				Matrix44 doorInCameraSpace = currentViewMatrix.to_world(_throughDoorFromOuter->get_outbound_matrix());
				ArrayStatic<Vector3, 4> doorPoints;
				// setup points from door hole size
				{
					Range2 doorHoleSize = doorHole->calculate_size(_throughDoorFromOuter->get_side(), _throughDoorFromOuter->get_hole_scale());
					doorPoints.set_size(4);
					doorPoints[0] = Vector3(doorHoleSize.x.min, 0.0f, doorHoleSize.y.min);
					doorPoints[1] = Vector3(doorHoleSize.x.min, 0.0f, doorHoleSize.y.max);
					doorPoints[2] = Vector3(doorHoleSize.x.max, 0.0f, doorHoleSize.y.min);
					doorPoints[3] = Vector3(doorHoleSize.x.max, 0.0f, doorHoleSize.y.max);
				}
				// move points to camera space
				for_every(p, doorPoints)
				{
					*p = doorInCameraSpace.location_to_world(*p);
				}
				// check for every plane if points are behind it
				bool behindAnyPlane = false;
				for_every(plane, _context.get_planes()->get_planes())
				{
					bool allBehind = true;
					for_every(p, doorPoints)
					{
						if (plane->get_in_front(*p) >= 0.0f)
						{
							allBehind = false;
							break;
						}
					}
					if (allBehind)
					{
						behindAnyPlane = true;
						break;
					}
				}
				if (behindAnyPlane)
				{
#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
					if (::System::Input::get()->has_key_been_pressed(::System::Key::I) &&
						::System::Input::get()->is_key_pressed(::System::Key::RightShift))
					{
						output_colour(0, 1, 0, 0);
						output(TXT("  room %i (view planes)"), _room);
						output_colour();
					}
#endif
#endif
					return nullptr;
				}
			}
		}

		roomProxy = get_one();
		roomProxy->depth = _context.get_depth();
		roomProxy->room = _room;
		roomProxy->predefinedRoomOcclusionCulling = _room->get_predefined_occlusion_culling();

		// setup
		roomProxy->entered_through_door_from_outer(_throughDoorFromOuter, doorDistanceToCamera);

		// setup context
		swappedClippedDoorVF = &clippedDoorVF;
		_context.swap_frustum(swappedClippedDoorVF);
		swappedViewMatrix = currentViewMatrix.to_world(roomProxy->doorOuter.otherRoomMatrix);
		_context.swap_view_matrix(swappedViewMatrix);
		swappedThroughDoorMatrix = currentThroughDoorMatrix.to_world(roomProxy->doorOuter.otherRoomMatrix);
		_context.swap_through_door_matrix(swappedThroughDoorMatrix);

		// check soft door rendering
		roomProxy->softDoorRendering = false;
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		if (_context.get_scene_view() != SceneView::External)
#endif
		{
			RoomType const * roomType = _room->get_room_type();
			if (roomType && roomType->get_soft_door_rendering() != SoftDoorRendering::None)
			{
				if (_fromRoomProxy)
				{
					if (Room const * fromRoom = _fromRoomProxy->room)
					{
						an_assert(fromRoom->get_room_type() != nullptr);
						if (fromRoom->get_room_type()->get_soft_door_rendering_tag() == roomType->get_soft_door_rendering_tag())
						{
							// this can work as soft door rendering, as both rooms have same soft door rendering tag
							if (roomType->get_soft_door_rendering() == SoftDoorRendering::EveryOdd)
							{
								int depthWithOffset = roomProxy->depth + roomType->get_soft_door_rendering_offset();
								if ((depthWithOffset % 2) == 1)
								{
									roomProxy->softDoorRendering = true;
								}
							}
						}
					}
				}
			}
		}

#ifdef DEBUG_BUILD_ROOM_PROXY
		debug_context(roomProxy);
		debug_filter(buildRoomProxy);
		if (DoorInRoom* doorInRoomProxy = _throughDoorFromOuter->get_door_on_other_side())
		{
			doorInRoomProxy->debug_draw_hole(true, Colour::red.with_alpha(0.5f));
		}
		debug_no_filter();
		debug_no_context();
#endif
	}
	else
	{
		roomProxy = get_one();
		roomProxy->depth = _context.get_depth();
		roomProxy->room = _room;
		roomProxy->predefinedRoomOcclusionCulling = _room->get_predefined_occlusion_culling();
		roomProxy->softDoorRendering = false;

		roomProxy->entered_through_door_from_outer(nullptr, 0.0f);
	}

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	if (auto* f = _context.get_frustum())
	{
		roomProxy->externalViewDoorClipPlaneSetRS.set_with_to_local(_context.get_view_matrix(), *f);
	}
	else
	{
		roomProxy->externalViewDoorClipPlaneSetRS.clear();
	}
#endif

#ifdef AN_DEBUG_RENDERER
	if (Framework::Render::ContextDebug::use)
	{
		Framework::Render::ContextDebug::use->next_room_proxy();
	}
#endif

#ifdef AN_DEBUG_RENDERER
#ifdef DEBUG_DRAW_FRUSTUM
	if (Framework::Render::ContextDebug::use &&
		Framework::Render::ContextDebug::use->should_render_room_alone() &&
		(Framework::Render::ContextDebug::use->should_render_room_proxy() || ! _throughDoorFromOuter))
	{
		debug_context(roomProxy->room);
		debug_push_transform(_context.get_view_matrix().inverted().to_transform());
		float distInterval = 1.0f;
		if (swappedClippedDoorVF)
		{
			// the one we entered through
			Colour colour = Colour::orange;
			auto const & edges = swappedClippedDoorVF->get_edges();
			for_every(edge, edges)
			{
				int i = for_everys_index(edge);
				auto const * prevEdge = &edges[mod(i - 1, edges.get_size())];
				for_count(int, s, 5)
				{
					float d = (float)s;
					// from both sides!
					debug_draw_triangle_border(true, colour, Vector3::zero, match_y(edge->point, distInterval) * d, match_y(prevEdge->point, distInterval) * d);
					debug_draw_triangle_border(true, colour, Vector3::zero, match_y(prevEdge->point, distInterval) * d, match_y(edge->point, distInterval) * d);
				}
			}
		}
		Colour colour = Colour::green;
		if (!_throughDoorFromOuter && !Framework::Render::ContextDebug::use->should_render_room_proxy())
		{
			colour = Colour::blue;
		}
		{
			auto const & edges = _context.get_frustum()->get_edges();
			for_every(edge, edges)
			{
				int i = for_everys_index(edge);
				auto const * prevEdge = &edges[mod(i - 1, edges.get_size())];
				for_count(int, s, 5)
				{
					float d = (float)s;
					// from both sides!
					debug_draw_triangle_border(true, colour, Vector3::zero, match_y(edge->point, distInterval) * d, match_y(prevEdge->point, distInterval) * d);
					debug_draw_triangle_border(true, colour, Vector3::zero, match_y(prevEdge->point, distInterval) * d, match_y(edge->point, distInterval) * d);
				}
			}
		}
		debug_pop_transform();
		debug_no_context();
		if (Framework::Render::ContextDebug::use->should_render_room_proxy())
		{
			debug_context(roomProxy->room);
			for_every_ptr(door, roomProxy->room->allDoors)
			{
				Colour colour = Colour::blue;
				if (roomProxy->doorLeave.doorInRoomRef == door)
				{
					colour = Colour::green;
				}
				if (door->get_side() == DoorSide::A)
				{
					door->debug_draw_hole(true, colour);
					debug_draw_sphere(true, false, colour, 0.2f, Sphere(door->get_placement().get_translation(), 0.1f));
					debug_draw_line(true, colour, door->get_placement().get_translation(), door->get_placement().get_translation() - door->get_plane().get_normal() * 0.3f);
				}
				else
				{
					door->debug_draw_hole(true, colour);
					debug_draw_sphere(true, false, colour, 0.2f, Sphere(door->get_placement().get_translation(), 0.1f));
					debug_draw_line(true, colour, door->get_placement().get_translation(), door->get_placement().get_translation() + door->get_plane().get_normal() * 0.3f);
				}
			}
			debug_no_context();
		}
	}
#endif
#endif

#ifdef AN_RENDERER_STATS
	Stats::get().renderedRoomProxies.increase();
	_context.access_stats().renderedRoomProxies.increase();
#endif

#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
	if (::System::Input::get()->has_key_been_pressed(::System::Key::I))
	{
		output_colour(0, 1, 0, 1);
		output(TXT("  room %i (%i)"), _room, ++_context.roomProxyCount);
		output_colour();
	}
#endif
#endif

	// setup environment
	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE_COLOURED(buildRoomProxyBuildEnvironment, Colour::zxBlue);
#endif
		an_assert(roomProxy->environment == nullptr);
		roomProxy->environment = EnvironmentProxy::build(_context, roomProxy, _fromRoomProxy, _throughDoorFromOuter);
	}

	// make sure we don't have any rooms
	an_assert(roomProxy->rooms == nullptr);
	RoomProxy** lastRoomPtr = &roomProxy->rooms;

	if (! _context.get_build_request().noRoomMeshes)
	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE_COLOURED(buildRoomProxyAppearanceHardCopy, Colour::zxRed);
#endif
		// setup appearance set for room (copy existing) (this will fill shader program instance up to base material)
		roomProxy->appearanceCopy.hard_copy_from(_room->get_appearance_for_rendering());
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
		if (_context.get_scene()->get_on_prepare_to_render())
		{
			for_every(meshInstance, roomProxy->appearanceCopy.access_instances())
			{
				_context.get_scene()->get_on_prepare_to_render()(REF_ *meshInstance, _room, Transform::identity);
			}
		}
#endif
		roomProxy->appearanceCopy.prepare_to_render();
	}
	else
	{
		roomProxy->appearanceCopy.clear();
	}

	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE_COLOURED(buildRoomProxyShaderProgramBindingContextCopy, Colour::zxGreen);
#endif
		// copy shader program binding context
		roomProxy->shaderProgramBindingContextCopy = _room->get_shader_program_binding_context();
		Region* inRegion = _room->get_in_region();
		while (inRegion)
		{
			roomProxy->shaderProgramBindingContextCopy.fill_with(inRegion->get_shader_program_binding_context());
			inRegion = inRegion->get_in_region();
		}
		if (SubWorld* inSubWorld = _room->get_in_sub_world())
		{
			roomProxy->shaderProgramBindingContextCopy.fill_with(inSubWorld->get_shader_program_binding_context());
		}
		if (World* inWorld = _room->get_in_world())
		{
			roomProxy->shaderProgramBindingContextCopy.fill_with(inWorld->get_shader_program_binding_context());
		}
	}

	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE_COLOURED(buildRoomProxyAddObjects, Colour::zxMagenta);
#endif
		// add objects
		an_assert(roomProxy->presenceLinks == nullptr);
		roomProxy->presenceLinks = PresenceLinkProxy::build_all_in_room(_context, roomProxy, _room->presenceLinks, _throughDoorFromOuter);
		if (Room* anotherRoom = _room->get_room_for_rendering_additional_objects())
		{
			roomProxy->presenceLinks = PresenceLinkProxy::build_all_in_room(_context, roomProxy, anotherRoom->presenceLinks, _throughDoorFromOuter, roomProxy->presenceLinks);
		}
	}

	// add rooms through doors
	if (_context.increase_depth())
	{
		// create new room proxies - if doors are open or you can see through them:
		//		are open or can see through them (closed)
		//		and are not nulled (hole scale > 0)
		//		and at least one point is in front of door we came through
		DoorInRoom const * cameThrough = _throughDoorFromOuter ? _throughDoorFromOuter->get_door_on_other_side() : nullptr;
		for_every_ptr(doorInRoom, _room->doors)
		{
			if (!doorInRoom->is_visible())
			{
				continue;
			}
			DoorInRoomProxy::build(_context, doorInRoom, roomProxy->doors, doorInRoom == cameThrough);
			if (doorInRoom->has_world_active_room_on_other_side())
			{
				if (doorInRoom->can_see_through_it_now())
				{
					// skip parallel and hidden beyond current doors we came through
					if (!cameThrough || doorInRoom->is_in_front_of(cameThrough->get_plane(), 0.001f))
					{
						if (RoomProxy* onOtherSide = RoomProxy::build(_context, doorInRoom->get_world_active_room_on_other_side(), roomProxy, doorInRoom))
						{
							// there was something - add to "rooms"
							(*lastRoomPtr) = onOtherSide;
							lastRoomPtr = &onOtherSide->next;
						}
					}
				}
			}
		}
		_context.decrease_depth();
	}

	// prepare light sources
	{
		roomProxy->lights.prepare(); // will read those automatically from settings
	}

	// swap back
	if (_throughDoorFromOuter)
	{
		_context.swap_frustum(swappedClippedDoorVF);
		_context.swap_view_matrix(swappedViewMatrix);
		_context.swap_through_door_matrix(swappedThroughDoorMatrix);
	}

	// as we are done with building our proxies, let's build rendering buffer
	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE_COLOURED(buildRoomProxyRenderingBufferBuild, Colour::zxRed);
#endif
		// gather all the data into rendering buffer

#ifdef BUILD_NON_RELEASE
		if (!::System::RenderTargetSaveSpecialOptions::get().onlyObject)
#endif
		{
			// environment
			if (roomProxy->environment)
			{
				roomProxy->environment->add_to(_context, roomProxy->renderingBuffers.buffers);
			}

			// room appearance
			roomProxy->appearanceCopy.add_to(roomProxy->renderingBuffers.buffers);
		}

		// presence links
		if (roomProxy->presenceLinks)
		{
			roomProxy->presenceLinks->add_all_to(_context, roomProxy->renderingBuffers.buffers);
		}

#ifdef BUILD_NON_RELEASE
		if (!::System::RenderTargetSaveSpecialOptions::get().onlyObject)
#endif
		{
			// doors
			if (roomProxy->doors)
			{
				roomProxy->doors->add_all_to(_context, roomProxy->renderingBuffers.buffers);
			}
		}
	}

	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE_COLOURED(buildRoomProxyRenderingBufferSort, Colour::zxCyan);
#endif
		// finally, sort rendering buffer
		roomProxy->renderingBuffers.sort(_context.get_view_matrix());
	}

	return roomProxy;
}

void RoomProxy::entered_through_door_from_outer(DoorInRoom const * _throughDoorFromOuter, float _distanceToCamera)
{
	if (_throughDoorFromOuter)
	{
		for_count(int, idx, 2)
		{
			auto* useDoor = idx == 0 ? _throughDoorFromOuter : _throughDoorFromOuter->get_door_on_other_side();
			auto & d = idx == 0 ? doorOuter : doorLeave;
			d.side = useDoor->get_side();
			d.hole = useDoor->get_hole();
			d.plane = useDoor->get_plane();
			d.distance = _distanceToCamera;
			d.outboundMatrix = useDoor->get_outbound_matrix();
			d.outboundMatrixWithScale = d.outboundMatrix;
			d.otherRoomMatrix = useDoor->get_other_room_matrix();
			d.otherRoomTransform = useDoor->get_other_room_transform();
			d.holeOutboundMesh = d.hole->get_outbound_mesh(d.side);
			d.holeOutboundMeshScale = useDoor->get_hole_scale();
			d.holeShaderProgramInstance = useDoor->get_door()->get_door_hole_shader_program_instance();
			if (d.holeShaderProgramInstance &&
				useDoor->get_door())
			{
				d.holeShaderProgramInstance->set_uniform(NAME(doorOpen), useDoor->get_door()->get_open_factor());
			}
#ifdef DEBUG_DRAW_FRUSTUM
			d.doorInRoomRef = useDoor;
#endif
			if (! d.holeOutboundMeshScale.is_one())
			{
				d.outboundMatrixWithScale = d.outboundMatrix.to_world(scale_matrix(d.holeOutboundMeshScale.to_vector3_xz(1.0f)));
			}
		}
	}
	else
	{
		doorOuter.side = DoorSide::A;
		doorOuter.hole = nullptr;
		doorOuter.plane = Plane::zero;
		doorOuter.distance = _distanceToCamera;
		doorOuter.outboundMatrix = Matrix44::identity;
		doorOuter.outboundMatrixWithScale = Matrix44::identity;
		doorOuter.otherRoomMatrix = Matrix44::identity;
		doorOuter.otherRoomTransform = Transform::identity;
		doorOuter.holeOutboundMesh = nullptr;
		doorOuter.holeOutboundMeshScale = Vector2::one;
		doorOuter.holeShaderProgramInstance = nullptr;
#ifdef DEBUG_DRAW_FRUSTUM
		doorOuter.doorInRoomRef = nullptr;
#endif
		// just copy
		doorLeave = doorOuter;
	}
}

void RoomProxy::on_release()
{
	predefinedRoomOcclusionCulling.clear();
	lights.on_release();
	renderingBuffers.clear();
	if (next)
	{
		next->release();
		next = nullptr;
	}
	if (environment)
	{
		environment->release();
		environment = nullptr;
	}
	if (rooms)
	{
		rooms->release();
		rooms = nullptr;
	}
	if (presenceLinks)
	{
		presenceLinks->release();
		presenceLinks = nullptr;
	}
	if (doors)
	{
		doors->release();
		doors = nullptr;
	}
}

// based on DoorInRoom::does_go_through_hole
bool RoomProxy::does_enter_through_door(Vector3 const & _currLocation, Vector3 const & _nextLocation, OUT_ Vector3 & _destLocation, float _minInFront) const
{
	float currInFront = doorOuter.plane.get_in_front(_currLocation);
	if (currInFront >= _minInFront)
	{
		float nextInFront = doorOuter.plane.get_in_front(_nextLocation);
		if (nextInFront < currInFront && nextInFront <= 0.0f) // going through
		{
			float newT = currInFront / (-nextInFront + currInFront);
			if (newT <= 1.0f)
			{
				Vector3 const atDoorHole = Vector3::blend(_currLocation, _nextLocation, newT);
				Vector3 const atDoorHoleLS = doorOuter.outboundMatrix.location_to_local(atDoorHole);
				if (doorOuter.hole->is_point_inside(doorOuter.side, doorOuter.holeOutboundMeshScale, atDoorHoleLS))
				{
					_destLocation = atDoorHole;
					return true;
				}
			}
		}
	}
	return false;
}

void RoomProxy::swap_main(REF_ RoomProxy * _room, REF_ RoomProxy * _with)
{
#ifdef AN_DEVELOPMENT
	an_assert(_room->doorOuter.plane.is_zero());
	bool withAmongRooms = false;
	RoomProxy* check = _room->rooms;
	while (check)
	{
		if (check == _with)
		{
			withAmongRooms = true;
			break;
		}
		check = check->next;
	}
	an_assert(withAmongRooms);
#endif
	// remove _with from _room's rooms
	if (_room->rooms == _with)
	{
		_room->rooms = _with->next;
	}
	else
	{
		RoomProxy* room = _room->rooms;
		while (room)
		{
			if (room->next == _with)
			{
				room->next = _with->next;
				break;
			}
			room = room->next;
		}
	}
	_with->next = nullptr;

	// add room to _with's rooms
	an_assert(_room->next == nullptr);
	_room->next = _with->rooms;
	_with->rooms = _room;

	// swap doors
	swap(_room->doorLeave, _with->doorOuter);
	swap(_room->doorOuter, _with->doorLeave);
}

void RoomProxy::adjust_for_camera_offset(REF_ RoomProxy * & _room, REF_ Render::Camera & _renderCamera, Transform const & _offset, OUT_ Transform & _intoRoomTransform)
{
	if (!_room)
	{
		_renderCamera.offset_placement(_offset);
		_intoRoomTransform = Transform::identity;
		return;
	}

	an_assert(_room->room == _renderCamera.get_in_room());

	{
		Transform ownerAbsolutePlacement = _renderCamera.get_placement().to_world(_renderCamera.get_owner_relative_placement());
		Transform nextPlacement = _renderCamera.get_placement();
		Transform intoRoomTransform = Transform::identity;

#ifdef AN_DEVELOPMENT
		float offsetLength = _offset.get_translation().length();
		float offsetLengthT = _renderCamera.get_placement().vector_to_world(_offset.get_translation()).length();
		an_assert(abs(offsetLength - offsetLengthT) < 0.001f);
#endif
		nextPlacement = _renderCamera.get_placement().to_world(_offset);

		{	// based on Room::move_through_doors
			Transform placement = _renderCamera.get_placement();

			int32 triesLeft = 10;
			while ((triesLeft--) > 0)
			{
				RoomProxy* goingToRoom = nullptr;

				Vector3 const currLocation = placement.get_translation();
				Vector3 const nextLocation = nextPlacement.get_translation();
				Vector3 destLocation = nextPlacement.get_translation();

				// find closest door on the way
				RoomProxy* checkRoom = _room->rooms;
				while (checkRoom)
				{
					if (checkRoom->does_enter_through_door(currLocation, nextLocation, OUT_ destLocation))
					{
						goingToRoom = checkRoom;
					}
					checkRoom = checkRoom->next;
				}

				if (goingToRoom)
				{
					// move to that other room
					Transform const & doorTransform = goingToRoom->doorOuter.otherRoomTransform;
					placement.set_translation(destLocation);
					placement = doorTransform.to_local(placement);
					nextPlacement = doorTransform.to_local(nextPlacement);
					ownerAbsolutePlacement = doorTransform.to_local(ownerAbsolutePlacement);
					intoRoomTransform = doorTransform.to_world(intoRoomTransform);

					// swap rooms in the tree
					swap_main(_room, goingToRoom);
					_room = goingToRoom;
				}
				else
				{
					break;
				}
			}
		}

		_intoRoomTransform = intoRoomTransform;
		_renderCamera.set_placement(_room->room, nextPlacement);
		_renderCamera.set_owner_relative_placement(_renderCamera.get_placement().to_local(ownerAbsolutePlacement));
	}
}

void RoomProxy::render(RoomProxy * _room, Context & _context, SceneRenderContext & _sceneRenderContext)
{
	if (!_room)
	{
		return;
	}

	an_assert(_room->room == _sceneRenderContext.get_camera().get_in_room());

	// check if we have moved through
	Matrix44 currentViewMatrix;
	currentViewMatrix = _sceneRenderContext.get_view_matrix();
	set_rendering_depth_range(_context.get_video_3d());
	_room->render_worker(_context, _sceneRenderContext, currentViewMatrix);
	clear_rendering_depth_range(_context.get_video_3d());
}

void RoomProxy::render_worker(Context & _context, SceneRenderContext & _sceneRenderContext, Matrix44 const & _currentViewMatrix) const
{
	// at this point, current view matrix is used to view this room from outside
	Matrix44 currentViewMatrix = _currentViewMatrix;

#ifdef AN_USE_RENDER_CONTEXT_DEBUG
	_context.access_debug().next_room_proxy();
#endif

	MEASURE_PERFORMANCE(renderRoomProxy); // whole room, will be nested

#ifdef DEBUG_COMPLETE_FRONT_PLANE
#ifdef AN_STANDARD_INPUT
	if (::System::Input::get()->is_key_pressed(::System::Key::N1))
	{
		debugCompleteFrontPlane = false;
	}
	else
	{
		debugCompleteFrontPlane = true;
	}
#endif
#endif

	::System::Video3D* v3d = _context.get_video_3d();
	v3d->stencil_mask(0xffffffff);

	// get current model view matrix
	::System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();
	::System::VideoClipPlaneStack& clipPlaneStack = v3d->access_clip_plane_stack();

	bool useFrontPlane = false;
	Vector3 frontPlanePoints[4];
#ifdef DEBUG_COMPLETE_FRONT_PLANE
	Vector3 completeFrontPlanePoints[4];
#endif

	bool clearFarPlane = false;

	Range3 doorOuterFlatRange = Range3::empty; // to limit where we draw it

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	bool renderDoorStencil = true;
	if (_sceneRenderContext.get_scene_view() == SceneView::External)
	{
		renderDoorStencil = false;
	}
#endif
	
#ifdef USE_SCISSORS
	RangeInt2 viewRange = RangeInt2::empty;
	viewRange.include(v3d->get_viewport_left_bottom());
	viewRange.include(v3d->get_viewport_left_bottom() + v3d->get_viewport_size() - VectorInt2::one);
	RangeInt2 scissorRange = RangeInt2::empty;
#endif

	// open door
	if (!doorOuter.plane.is_zero())
	{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		if (renderDoorStencil)
#endif
		if (!softDoorRendering) // no need for soft rendered doors
		{
			Plane doorPlaneInMVS;
			doorPlaneInMVS = currentViewMatrix.to_world(doorOuter.plane);

			if (doorPlaneInMVS.get_normal().y > 0.99f)
			{
				// definitely facing away
				// when camera is right in front of the door, aligned with its dir, sometimes the front cap obscures whole view
				return;
			}

			float frontDist = doorPlaneInMVS.get_in_front(Vector3::zero);
			float maxDist = _sceneRenderContext.get_max_door_dist();
			float nearPlane = _sceneRenderContext.get_near_plane();

			if (frontDist < -0.001f)
			{
				// we're behind this door, no point in rendering it
				// we shouldn't even be here as this case should be handled earlier
				// threshold is generous as for inside hole we want to have cap, the other case won't be rendered due to stencil not having written if seen from back
				return;
			}
			if (frontDist < maxDist &&
				doorOuter.hole->is_point_inside(doorOuter.side, doorOuter.holeOutboundMeshScale, doorOuter.outboundMatrix.location_to_local(currentViewMatrix.location_to_local(Vector3::zero)), maxDist))
			{
				// use near plane cap
				Plane nearPlaneInMVS = Plane(Vector3::yAxis, Vector3::yAxis * nearPlane);
				Vector3 edgeInMVS = Vector3::cross(nearPlaneInMVS.get_normal(), doorPlaneInMVS.get_normal()).normal(); // it lays on near plane
				//Vector3 doorNormalInNP = Vector3(doorPlaneInMVS.get_normal().x, 0.0f, doorPlaneInMVS.get_normal().z).normal();
				Vector3 perpEdgeInMVS = Vector3(edgeInMVS.z, 0.0f, -edgeInMVS.x);
				/*
									\
									 ^ edgeInMVS		
									  \
									   \  _-> perpEdgeInMVS
										t-
									+	 \		this is in front 
										  \		of near plane - clipped out
										   \
					door plane
					a1x + b1y + c1z + d1 = 0
					we are looking at edge on near plane, so y is near plane

					perpendicular to our edge, going through 0,0,0 - we want to find point closest to 0,0,0 on our final edge
					x = px + dx * t
					z = pz + dz * t

					dx, dz are known
					px, py are 0

					a1 dx t + b1 np + c1 dz t + d1 = 0
					t (a1 dx + c1 dz) = -d1 - b1 np
					t = (- d1 - b1 np ) / (a1 dx + c1 dz)
				*/

				float bel = perpEdgeInMVS.x * doorPlaneInMVS.normal.x + perpEdgeInMVS.z * doorPlaneInMVS.normal.z;
				// 1.000001f
				// 1.00001f will make it easier to catch issues with depth range
				float scaleToInclude = 
#ifdef USE_DEPTH_RANGE
					1.000001f; // slightly in front
#else
#ifdef USE_DEPTH_CLAMP
					1.0f; // when using depth clamp, we do not need non zero scale
#else
					1.000001f; // when using depth clamp, we do not need non zero scale
#endif
#endif
#ifdef DEBUG_RENDER_DOOR_HOLES
					if (debugRenderDoorHolesFrontCapZSubZero)
					{
						scaleToInclude = 0.95f;
					}
					if (debugRenderDoorHolesFrontCapZZero)
					{
						scaleToInclude = 1.0f;
					}
					if (debugRenderDoorHolesFrontCapZLarger)
					{
						scaleToInclude = 1.001f;
					}
					if (debugRenderDoorHolesFrontCapZSmall)
					{
						scaleToInclude = 1.0000001f;
					}
					if (debugRenderDoorHolesFrontCapZSmaller)
					{
						scaleToInclude = 1.00000001f;
					}
#endif

				if (abs(bel) > 0.01f)
				{
					// extend - to cover the gap between door in 3d and near plane cap
					float const coverGapExtend =
#ifdef DEBUG_RENDER_DOOR_HOLES
						debugRenderDoorHolesNegativeExtend ? -0.001f :
						!debugRenderDoorHolesNoExtend ? 0.0002f :
#endif
						0.0f;
					float const enlargeCap =
#ifdef DEBUG_RENDER_DOOR_HOLES
						debugRenderDoorHolesSmallerCap ? 1.0f :
#endif
						10.0f; // should be enough
					float const t = (-doorPlaneInMVS.calculate_d() - doorPlaneInMVS.normal.y * nearPlane) / bel;
					Vector3 const edgeAnchorInNP = Vector3(t * perpEdgeInMVS.x, nearPlane, t * perpEdgeInMVS.z);
					// perp is pointing away - covering what is beyond plane, check picture above
					frontPlanePoints[0] = edgeAnchorInNP + edgeInMVS - perpEdgeInMVS * coverGapExtend;
					frontPlanePoints[1] = edgeAnchorInNP + edgeInMVS + perpEdgeInMVS * enlargeCap;
					frontPlanePoints[2] = edgeAnchorInNP - edgeInMVS + perpEdgeInMVS * enlargeCap;
					frontPlanePoints[3] = edgeAnchorInNP - edgeInMVS - perpEdgeInMVS * coverGapExtend;
					// move further from the camera to be always rendered
					frontPlanePoints[0] *= scaleToInclude;
					frontPlanePoints[1] *= scaleToInclude;
					frontPlanePoints[2] *= scaleToInclude;
					frontPlanePoints[3] *= scaleToInclude;
#ifdef DEBUG_COMPLETE_FRONT_PLANE
					completeFrontPlanePoints[0] = edgeAnchorInNP - edgeInMVS - perpEdgeInMVS * coverGapExtend;
					completeFrontPlanePoints[1] = edgeAnchorInNP - edgeInMVS - perpEdgeInMVS * enlargeCap;
					completeFrontPlanePoints[2] = edgeAnchorInNP + edgeInMVS - perpEdgeInMVS * enlargeCap; // almost to infinity - we can't allow rendering just narrow strip
					completeFrontPlanePoints[3] = edgeAnchorInNP + edgeInMVS - perpEdgeInMVS * coverGapExtend;
					// move further from the camera to be always rendered
					completeFrontPlanePoints[0] *= scaleToInclude;
					completeFrontPlanePoints[1] *= scaleToInclude;
					completeFrontPlanePoints[2] *= scaleToInclude;
					completeFrontPlanePoints[3] *= scaleToInclude;
#endif
					useFrontPlane = true;
				}
				else
				{
					// render whole nearplane (NP)
					Vector3 const anchorInNP = Vector3(0.0f, nearPlane, 0.0f);
					// use fov size but still make it bigger
					float const x = nearPlane * _sceneRenderContext.get_fov_size().x * 2.0f;
					float const z = nearPlane * _sceneRenderContext.get_fov_size().y * 2.0f;
					// perp is pointing away - covering what is beyond plane, check picture above
					frontPlanePoints[0] = anchorInNP + Vector3(-x, 0.0f, z);
					frontPlanePoints[1] = anchorInNP + Vector3(x, 0.0f, z);
					frontPlanePoints[2] = anchorInNP + Vector3(x, 0.0f, -z);
					frontPlanePoints[3] = anchorInNP + Vector3(-x, 0.0f, -z);
					// move further from the camera to be always rendered
					frontPlanePoints[0] *= scaleToInclude;
					frontPlanePoints[1] *= scaleToInclude;
					frontPlanePoints[2] *= scaleToInclude;
					frontPlanePoints[3] *= scaleToInclude;
#ifdef DEBUG_COMPLETE_FRONT_PLANE
					completeFrontPlanePoints[0] = anchorInNP + Vector3(-x, 0.0f, z);
					completeFrontPlanePoints[1] = anchorInNP + Vector3(x, 0.0f, z);
					completeFrontPlanePoints[2] = anchorInNP + Vector3(x, 0.0f, -z);
					completeFrontPlanePoints[3] = anchorInNP + Vector3(-x, 0.0f, -z);
					// move further from the camera to be always rendered
					completeFrontPlanePoints[0] *= scaleToInclude;
					completeFrontPlanePoints[1] *= scaleToInclude;
					completeFrontPlanePoints[2] *= scaleToInclude;
					completeFrontPlanePoints[3] *= scaleToInclude;
#endif
					useFrontPlane = true;
				}
			}
			
			Range3 doorFlatRange = doorOuter.hole->calculate_flat_box(doorOuter.side, doorOuter.holeOutboundMeshScale, currentViewMatrix.to_world(doorOuter.outboundMatrix));

			if (doorFlatRange.y.min > 1.0f)
			{
				doorOuterFlatRange = doorFlatRange;
			}

#ifdef USE_SCISSORS
			if (!doorOuterFlatRange.is_empty())
			{
				if (doorOuterFlatRange.y.min > 0.5f)
				{
					ArrayStatic<Vector3, 4> corners; // still in model view space (our, not opengl)
					corners.push_back(Vector3(doorOuterFlatRange.x.min, 1.0f, doorOuterFlatRange.z.min));
					corners.push_back(Vector3(doorOuterFlatRange.x.min, 1.0f, doorOuterFlatRange.z.max));
					corners.push_back(Vector3(doorOuterFlatRange.x.max, 1.0f, doorOuterFlatRange.z.min));
					corners.push_back(Vector3(doorOuterFlatRange.x.max, 1.0f, doorOuterFlatRange.z.max));

					ArrayStatic<Vector3, 4> projectedCorners; // this will be in opengl clip space (xy: -1 to 1)

					Matrix44 projectionMatrix = v3d->get_projection_matrix();
					Matrix44 const& pm = projectionMatrix;

					bool allValid = true;
					for_every(c, corners)
					{
						// we're switching from our model to opengl model, do it before using projection matrix (that is already in opengl's)
						Vector4 cfp;
						cfp.x = c->x;
						cfp.y = c->z;
						cfp.z = -c->y;
						cfp.w = 1.0f;

						Vector4 prcC;
						prcC.x = pm.m00 * cfp.x + pm.m10 * cfp.y + pm.m20 * cfp.z + pm.m30 * cfp.w;
						prcC.y = pm.m01 * cfp.x + pm.m11 * cfp.y + pm.m21 * cfp.z + pm.m31 * cfp.w;
						prcC.z = pm.m02 * cfp.x + pm.m12 * cfp.y + pm.m22 * cfp.z + pm.m32 * cfp.w;
						prcC.w = pm.m03 * cfp.x + pm.m13 * cfp.y + pm.m23 * cfp.z + pm.m33 * cfp.w;

						if (prcC.w <= 0.0f)
						{
							allValid = false;
							break;
						}

						Vector3 projected;
						float invW = 1.0f / prcC.w;
						projected.x = prcC.x * invW;
						projected.y = prcC.y * invW;
						projected.z = prcC.z * invW;

						projectedCorners.push_back(projected);
					}

					if (allValid)
					{
						Vector2 viewportAt = v3d->get_viewport_left_bottom().to_vector2();
						Vector2 viewportSize = v3d->get_viewport_size().to_vector2();
						scissorRange = RangeInt2::empty;
						for_every(c, projectedCorners)
						{
							Vector2 cScreen;
							cScreen.x = viewportAt.x + (c->x * 0.5f + 0.5f) * viewportSize.x;
							cScreen.y = viewportAt.y + (c->y * 0.5f + 0.5f) * viewportSize.y;
							VectorInt2 cScreenInt = TypeConversions::Normal::f_i_closest(cScreen);
							scissorRange.include(cScreenInt);
						}
						scissorRange.expand_by(VectorInt2::one);

						scissorRange.intersect_with(viewRange);
					}
				}
			}
#endif
		}

#ifdef USE_SCISSORS
		if (scissorRange == viewRange)
		{
			// we may as well not use it
			scissorRange = RangeInt2::empty;
		}

		v3d->set_scissors(scissorRange);
#endif

		// open door if needed
		if (!softDoorRendering)
		{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
			if (renderDoorStencil)
#endif
			{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
				MEASURE_PERFORMANCE_FINISH_RENDERING();
				MEASURE_PERFORMANCE_COLOURED(renderRoomProxyOpenDoor, Colour::zxYellowBright);
#endif
				// open door
#ifndef DISABLE_DOOR_RENDERING
#ifdef AN_USE_RENDER_CONTEXT_DEBUG
				if (!_context.access_debug().should_render_room_alone())
#endif
				{
					DRAW_INFO(TXT("[room proxy] open door"));
					// open next stencil's depth (increase depth)
					modelViewStack.push_to_world(doorOuter.outboundMatrixWithScale);
					v3d->stencil_test(::System::Video3DCompareFunc::Equal, _context.get_current_stencil_depth());
					v3d->stencil_op(::System::Video3DOp::Increase);
					v3d->colour_mask(false);
					v3d->depth_mask(false);
					// render if equal too!
					v3d->depth_test(::System::Video3DCompareFunc::LessEqual);
					// render (use normal (whole) doorhole, not the cut one)
#ifdef DEBUG_DOOR_HOLES
					v3d->depth_mask(true);
					v3d->colour_mask(true);
					v3d->set_fallback_colour(Colour(0.0f, 1.0f, 0.0f));
#endif
#ifdef DEBUG_RENDER_DOOR_HOLES
					if (!debugRenderDoorHolesPartially || ::System::Core::get_timer_mod(3.0f) <= 1.0f || ::System::Core::get_timer_mod(3.0f) > 2.0f)
					if (!debugRenderDoorHolesPartiallyOnlyNear)
#endif
					//v3d->mark_enable_polygon_offset(-1.0f, -1.0f);
					doorOuter.holeOutboundMesh->render(v3d, doorOuter.holeShaderProgramInstance?
						const_ptr(::System::FallbackShaderProgramInstanceForMaterialInstanceProvider(doorOuter.holeShaderProgramInstance)) :
						const_ptr(::System::FallbackShaderProgramInstanceForMaterialInstanceProvider(v3d->get_fallback_shader_program_instance(::System::Video3DFallbackShader::For3D | ::System::Video3DFallbackShader::Basic))));
					//v3d->mark_disable_polygon_offset();
#ifdef DEBUG_DOOR_HOLES
					v3d->set_fallback_colour();
#endif
					// (if too close, check where it intersects with near plane and draw filling)
#ifdef DEBUG_RENDER_DOOR_HOLES
					if (! debugRenderDoorHolesPartially || ::System::Core::get_timer_mod(3.0f) <= 2.0f)
					if (!debugRenderDoorHolesPartiallyOnlyNormal)
#endif
					if (useFrontPlane)
					{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
						MEASURE_PERFORMANCE_COLOURED(renderRoomProxyRenderFrontPlane, Colour::zxWhiteBright);
#endif
						// always render near plane cap
						v3d->stencil_test(::System::Video3DCompareFunc::Equal, _context.get_current_stencil_depth());
						v3d->stencil_op(::System::Video3DOp::Increase);
						v3d->depth_test(::System::Video3DCompareFunc::Always);
						modelViewStack.push_set(Matrix44::identity);
						render_front_plane(_context, v3d, useFrontPlane, frontPlanePoints
#ifdef DEBUG_COMPLETE_FRONT_PLANE
							, completeFrontPlanePoints
#endif
							);
						modelViewStack.pop();
					}
					_context.increase_stencil_depth();
					modelViewStack.pop();
				}

				clearFarPlane = true;
#endif
			}
		}

		// update current view matrix, as since now we will be inside room
		if (!doorOuter.plane.is_zero())
		{
			currentViewMatrix = currentViewMatrix.to_world(doorOuter.otherRoomMatrix);
		}

		// move to this room proxy
		{
			// we're going to render next room
			modelViewStack.push_to_world(doorOuter.otherRoomMatrix); // send, we require that for clip planes
#ifdef AN_DEBUG_RENDERER
#ifdef AN_USE_RENDER_CONTEXT_DEBUG
#ifndef DEBUG_DRAW_FRUSTUM // we require all context to be defined for frustum debug
			if (!_context.access_debug().should_render_room_alone() ||
				_context.access_debug().should_render_room_proxy())
#endif
#endif
			{
				// two separate, one for room proxy, other one for room, as sometimes we want just to render for room proxy
				debug_renderer_define_context(this, modelViewStack.get_first().to_local(modelViewStack.get_current()).to_transform()); // get ignoring first matrix that is camera/view
				debug_renderer_define_context(room, modelViewStack.get_first().to_local(modelViewStack.get_current()).to_transform()); // get ignoring first matrix that is camera/view
			}
#endif
			// clip, so only behind door is visible and do it here, at this level of stack
#ifndef DISABLE_CLIP_PLANES
			clipPlaneStack.push();
			Plane plane = doorLeave.plane;
			plane = plane.get_adjusted_along_normal(-0.001f); // a bit to fight discontinuities
			clipPlaneStack.add_to_current(plane);
#endif
		}
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE_FINISH_RENDERING();
#endif
	}
	else
	{
#ifdef AN_DEBUG_RENDERER
#ifdef AN_USE_RENDER_CONTEXT_DEBUG
#ifndef DEBUG_DRAW_FRUSTUM // we require all context to be defined for frustum debug
		if (!_context.access_debug().should_render_room_alone() ||
			_context.access_debug().should_render_room_proxy())
#endif
#endif
		{
			debug_renderer_define_context(this, Transform::identity);
			debug_renderer_define_context(room, Transform::identity);
			if (auto* vr = VR::IVR::get())
			{
				if (Game::get() &&
					!Game::get()->is_world_manager_busy()) // we might be activating something that is processing POIs, adding them and this breaks get_vr_anchor
				{
					debug_renderer_define_context(vr, room->get_vr_anchor());
				}
			}
		}
#endif
#ifndef DISABLE_CLIP_PLANES
		clipPlaneStack.push();
#endif
	}

#ifndef DISABLE_CLIP_PLANES
	// clip against doors with soft rendering
	{	
		RoomProxy* room = rooms;
		while (room)
		{
			if (room->softDoorRendering)
			{
				Plane plane = room->doorOuter.plane;
				plane = plane.get_adjusted_along_normal(-0.001f); // a bit to fight discontinuities
				clipPlaneStack.add_to_current(plane);
			}
			room = room->next;
		}
	}
#endif

	bool skyBoxRendered = false;
	bool renderSkyBoxFirst = MainSettings::global().get_shader_options().get_tag_as_int(NAME(renderSkyBoxFirst)) != 0;

#ifdef AN_USE_BACKGROUND_RENDERING_BUFFER
#ifndef DEBUG_DOOR_HOLES
#ifdef AN_USE_RENDER_CONTEXT_DEBUG
	if (!_context.access_debug().should_render_room_alone())
#endif
#ifdef DEBUG_COMPLETE_FRONT_PLANE
	if (!debugCompleteFrontPlane || (!useFrontPlane))
#endif
	if (!renderingBuffers.background.is_empty())
	{
		DRAW_INFO(TXT("[room proxy] background"));

		_sceneRenderContext.get_camera().switch_background_render(v3d, true);

		render_far_plane(_context, v3d, true); // background

		{	// actual rendering of background
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
			MEASURE_PERFORMANCE_FINISH_RENDERING();
#endif
			EnvironmentProxySetupScope environmentScope(_context, v3d, this);

			// render content of this room as last, as we may have some blending
#ifndef DISABLE_DOOR_RENDERING
#ifdef AN_USE_RENDER_CONTEXT_DEBUG
			if (!_context.access_debug().should_render_room_alone())
#endif
			{
				// render only if stencil is at current depth
				v3d->stencil_test(::System::Video3DCompareFunc::Equal, _context.get_current_stencil_depth());
				v3d->stencil_op(::System::Video3DOp::Keep);
			}
#endif

			// ready for rendering
			v3d->depth_mask(true);
			v3d->depth_test(::System::Video3DCompareFunc::LessEqual);
			v3d->colour_mask(true);

			// render contents of sky box rendering buffer
			if (!skyBoxRendered && renderSkyBoxFirst)
			{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
				MEASURE_PERFORMANCE_COLOURED(renderRoomProxySkyBoxRenderingBuffer, Colour::zxCyan);
#endif
				DRAW_INFO(TXT("[room proxy] sky box"));
#ifdef BUILD_NON_RELEASE
				if (!::System::RenderTargetSaveSpecialOptions::get().transparent)
#endif
				{
					renderingBuffers.skyBox.render(v3d, _context.get_mesh_rendering_context());
				}
				skyBoxRendered = true;
			}

			// check scene rendering for info
			bool requiresBlendingRendering = false;

			{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
				MEASURE_PERFORMANCE_COLOURED(renderRoomProxyBackgroundRenderingBuffer, Colour::zxBlue);
#endif
				// render contents of rendering buffer
				DRAW_INFO(TXT("[room proxy] room background"));
				auto context = _context.get_mesh_rendering_context();
				if (!skyBoxRendered && !renderSkyBoxFirst)
				{
					context.renderBlending = false;
					requiresBlendingRendering = true;
				}
				renderingBuffers.background.render(v3d, context);
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
				MEASURE_PERFORMANCE_FINISH_RENDERING();
#endif
			}

			// render contents of sky box rendering buffer
			if (!skyBoxRendered && ! renderSkyBoxFirst)
			{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
				MEASURE_PERFORMANCE_COLOURED(renderRoomProxySkyBoxRenderingBuffer, Colour::zxCyan);
#endif
				DRAW_INFO(TXT("[room proxy] sky box"));
#ifdef BUILD_NON_RELEASE
				if (!::System::RenderTargetSaveSpecialOptions::get().transparent)
#endif
				{
					renderingBuffers.skyBox.render(v3d, _context.get_mesh_rendering_context());
				}
				skyBoxRendered = true;
			}

			// now render blended if required
			if (requiresBlendingRendering)
			{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
				MEASURE_PERFORMANCE_COLOURED(renderRoomProxyBackgroundRenderingBuffer, Colour::zxBlue);
#endif
				DRAW_INFO(TXT("[room proxy] room background"));
				// render contents of rendering buffer
				auto context = _context.get_mesh_rendering_context();
				context.renderBlending = true;
				renderingBuffers.background.render(v3d, context);
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
				MEASURE_PERFORMANCE_FINISH_RENDERING();
#endif
			}
		}

		_sceneRenderContext.get_camera().switch_background_render(v3d, false);

		clearFarPlane = true;
	}
#endif
#endif

	if (clearFarPlane)
	{
		// clear far plane, if needed - if we're not first room OR we've rendered background
#ifndef DEBUG_DOOR_HOLES
#ifdef AN_USE_RENDER_CONTEXT_DEBUG
		if (!_context.access_debug().should_render_room_alone())
#endif
#ifdef DEBUG_COMPLETE_FRONT_PLANE
		if (!debugCompleteFrontPlane || (!useFrontPlane))
#endif
		{
			DRAW_INFO(TXT("[room proxy] clear far plane"));
			render_far_plane(_context, v3d, false, doorOuterFlatRange);
		}
#endif
	}

	// render other rooms, rooms through doors
#ifdef DEBUG_COMPLETE_FRONT_PLANE
	if (!debugCompleteFrontPlane || (!useFrontPlane))
#endif
	if (rooms)
	{
		DRAW_INFO(TXT("[room proxy] other rooms"));
		RoomProxy* room = rooms;
		while (room)
		{
			room->render_worker(_context, _sceneRenderContext, currentViewMatrix);
			room = room->next;
		}
	}

#ifdef USE_SCISSORS
	v3d->set_scissors(scissorRange);
#endif

	// render content of this room as last, as we may have some blending
#ifdef AN_USE_RENDER_CONTEXT_DEBUG
	if (_context.access_debug().should_render_room_proxy())
#endif
#ifdef DEBUG_COMPLETE_FRONT_PLANE
	if (!debugCompleteFrontPlane || (!useFrontPlane))
#endif
	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE_FINISH_RENDERING();
#endif
		EnvironmentProxySetupScope environmentScope(_context, v3d, this);

		// render content of this room as last, as we may have some blending
#ifndef DISABLE_DOOR_RENDERING
#ifdef AN_USE_RENDER_CONTEXT_DEBUG
		if (!_context.access_debug().should_render_room_alone())
#endif
		{
			// render only if stencil is at current depth
			v3d->stencil_test(::System::Video3DCompareFunc::Equal, _context.get_current_stencil_depth());
			v3d->stencil_op(::System::Video3DOp::Keep);
		}
#endif

		// ready for rendering
		v3d->depth_mask(true);
		v3d->depth_test(::System::Video3DCompareFunc::LessEqual);
		v3d->colour_mask(true);

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		if (_sceneRenderContext.get_scene_view() == SceneView::External)
		{
			ArrayStatic<Plane, 32> externalViewDoorClipPlaneSetCS;
			ArrayStatic<Plane, 256> externalViewDoorClipPlaneExcludeSetCS;
			Matrix44 modelViewMatrix = modelViewStack.get_current();
			for_every(pRS, externalViewDoorClipPlaneSetRS.planes)
			{
				// bring them back to camera space
				Plane p = modelViewMatrix.to_world(*pRS);
				externalViewDoorClipPlaneSetCS.push_back(p);
			}
			RoomProxy* room = rooms;
			while (room)
			{
				if (!room->doorOuter.plane.is_zero())
				{
					//modelViewStack.push_to_world(room->doorOuter.otherRoomMatrix);
					//Matrix44 roomModelViewMatrix = modelViewStack.get_current();
					Matrix44 roomModelViewMatrix = modelViewMatrix;
					roomModelViewMatrix = roomModelViewMatrix.to_world(room->doorOuter.otherRoomMatrix);
					for_every(pRS, room->externalViewDoorClipPlaneSetRS.planes)
					{
						// bring them back to camera space
						Plane p = roomModelViewMatrix.to_world(*pRS);
						externalViewDoorClipPlaneExcludeSetCS.push_back(p);
					}
					// door plane outer, in this room's space
					{
						Plane p = room->doorOuter.plane;
						p = Plane(-p.normal, p.anchor);
						p = modelViewMatrix.to_world(p);
						externalViewDoorClipPlaneExcludeSetCS.push_back(p);
					}
					// break group
					externalViewDoorClipPlaneExcludeSetCS.push_back(Plane::zero);
					//modelViewStack.pop();
				}
				room = room->next;
			}
			Game::get()->set_external_view_clip_planes(externalViewDoorClipPlaneSetCS.get_size(), externalViewDoorClipPlaneSetCS.get_data(), externalViewDoorClipPlaneExcludeSetCS.get_size(), externalViewDoorClipPlaneExcludeSetCS.get_data());
		}
#endif

		// render contents of sky box rendering buffer, at end, as we just want to fill background
		if (!skyBoxRendered && renderSkyBoxFirst)
		{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
			MEASURE_PERFORMANCE_COLOURED(renderRoomProxySkyBoxRenderingBuffer, Colour::zxCyan);
#endif
			DRAW_INFO(TXT("[room proxy] sky box"));
#ifdef BUILD_NON_RELEASE
			if (!::System::RenderTargetSaveSpecialOptions::get().transparent)
#endif
			{
				renderingBuffers.skyBox.render(v3d, _context.get_mesh_rendering_context());
			}
			skyBoxRendered = true;
		}

		// if we render sky first, we simply go with:
		//		render sky
		//		render content (automatically non blend first, blend second)
		// but if we don't render sky first, we may want to break rendering into more passes:
		//		render non blendable content
		//		render sky
		//		render blendable content
		bool requiresBlendingRendering = false;

		{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
			MEASURE_PERFORMANCE_COLOURED(renderRoomProxyRenderingBuffer, Colour::zxCyanBright);
#endif
			DRAW_INFO(TXT("[room proxy] room content"));
			// render contents of rendering buffer
			auto context = _context.get_mesh_rendering_context();
			if (!skyBoxRendered && !renderSkyBoxFirst)
			{
				context.renderBlending = false;
				requiresBlendingRendering = true;
			}
			renderingBuffers.scene.render(v3d, context);
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
			MEASURE_PERFORMANCE_FINISH_RENDERING();
#endif
		}

		// render contents of sky box rendering buffer, at end, as we just want to fill background
		if (!skyBoxRendered && !renderSkyBoxFirst)
		{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
			MEASURE_PERFORMANCE_COLOURED(renderRoomProxySkyBoxRenderingBuffer, Colour::zxCyan);
#endif
			DRAW_INFO(TXT("[room proxy] sky box"));
#ifdef BUILD_NON_RELEASE
			if (!::System::RenderTargetSaveSpecialOptions::get().transparent)
#endif
			{
				renderingBuffers.skyBox.render(v3d, _context.get_mesh_rendering_context());
			}
			skyBoxRendered = true;
		}

		// now render blended if required
		if (requiresBlendingRendering)
		{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
			MEASURE_PERFORMANCE_COLOURED(renderRoomProxyRenderingBuffer, Colour::zxCyanBright);
#endif
			DRAW_INFO(TXT("[room proxy] room content"));
			// render contents of rendering buffer
			auto context = _context.get_mesh_rendering_context();
			context.renderBlending = true;
			renderingBuffers.scene.render(v3d, context);
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
			MEASURE_PERFORMANCE_FINISH_RENDERING();
#endif
		}

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		if (_sceneRenderContext.get_scene_view() == SceneView::External)
		{
			Game::get()->clear_external_view_clip_planes();
		}
#endif
	}

	if (!doorOuter.plane.is_zero())
	{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		if (! renderDoorStencil &&
			!softDoorRendering)
		{
			// before we get back to the previous room, let's draw a set of planes that will block anything to be rendered inside
			//todo_implement(TXT("render invisible clip planes, ie. render to depth buffer only to close the door"));
		}
#endif

#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE_FINISH_RENDERING();
		MEASURE_PERFORMANCE_COLOURED(renderRoomProxyCloseDoor, Colour::zxYellow);
#endif
		// get back from this room proxy
		{
			modelViewStack.pop(); // pop with send for clip plane stack update
			// pop clip plane when we restored model view stack
#ifndef DISABLE_CLIP_PLANES
			clipPlaneStack.pop();
#endif
		}

		// close door if needed
		if (!softDoorRendering)
		{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
			if (renderDoorStencil)
#endif
			{
#ifndef DISABLE_DOOR_RENDERING
				// close door
#ifdef AN_USE_RENDER_CONTEXT_DEBUG
				if (! _context.access_debug().should_render_room_alone())
#endif
				{
					DRAW_INFO(TXT("[room proxy] close door"));
					// close stencil (at current or above level) and working (only working!) depth buffer
					modelViewStack.push_to_world(doorOuter.outboundMatrixWithScale);
					v3d->stencil_test(::System::Video3DCompareFunc::Less, _context.get_current_stencil_depth() - 1);
					v3d->stencil_op(::System::Video3DOp::Replace);
					v3d->colour_mask(false);
					v3d->depth_mask(true);
					// render always as we want to make sure that the door is really closed and at the proper depth
					// if we don't do that, we may get leaks as clip planes might be not so precise when cutting meshes
#ifdef DEBUG_RENDER_DOOR_HOLES
					if (!debugRenderDoorHolesCloseAlwaysDepth)
					{
						v3d->depth_test(::System::Video3DCompareFunc::LessEqual);
					}
					else
#endif
					{
						v3d->depth_test(::System::Video3DCompareFunc::Always);
					}
					// close stencil and working (only working!) depth buffer (and if too close follow previous steps))
#ifdef DEBUG_RENDER_DOOR_HOLES
					if (!debugRenderDoorHolesPartially || ::System::Core::get_timer_mod(3.0f) <= 1.0f || ::System::Core::get_timer_mod(3.0f) > 2.0f)
					if (!debugRenderDoorHolesPartiallyOnlyNear)
#endif
					//v3d->mark_enable_polygon_offset(-1.0f, -1.0f);
					doorOuter.holeOutboundMesh->render(v3d, doorOuter.holeShaderProgramInstance ?
						const_ptr(::System::FallbackShaderProgramInstanceForMaterialInstanceProvider(doorOuter.holeShaderProgramInstance)) :
						const_ptr(::System::FallbackShaderProgramInstanceForMaterialInstanceProvider(v3d->get_fallback_shader_program_instance(::System::Video3DFallbackShader::For3D | ::System::Video3DFallbackShader::Basic))));
					//v3d->mark_disable_polygon_offset();
#ifdef DEBUG_RENDER_DOOR_HOLES
					if (!debugRenderDoorHolesPartially || ::System::Core::get_timer_mod(3.0f) <= 2.0f)
					if (!debugRenderDoorHolesPartiallyOnlyNormal)
#endif
					if (useFrontPlane)
					{
						// always render near plane cap
						v3d->stencil_test(::System::Video3DCompareFunc::Less, _context.get_current_stencil_depth() - 1);
						v3d->stencil_op(::System::Video3DOp::Replace);
						v3d->depth_test(::System::Video3DCompareFunc::Always);
						modelViewStack.push_set(Matrix44::identity);
						render_front_plane(_context, v3d, useFrontPlane, frontPlanePoints
#ifdef DEBUG_COMPLETE_FRONT_PLANE
							, completeFrontPlanePoints
#endif
							);
						modelViewStack.pop();
					}
					_context.decrease_stencil_depth();
					modelViewStack.pop();
				}
#endif
			}
		}
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE_FINISH_RENDERING();
#endif
	}
	else
	{
#ifndef DISABLE_CLIP_PLANES
		clipPlaneStack.pop();
#endif
	}

#ifdef USE_SCISSORS
	v3d->clear_scissors();
#endif
}

void RoomProxy::render_front_plane(Context & _context, ::System::Video3D* _v3d, bool _useFrontPlane, Vector3 const * _frontPlanePoints
#ifdef DEBUG_COMPLETE_FRONT_PLANE
	, Vector3* _completeFrontPlanePoints
#endif
	) const
{
	// do rendering of this on both eyes separately
	if (!_useFrontPlane) return; // skip if not used at all
#ifdef DEBUG_COMPLETE_FRONT_PLANE
	if (debugCompleteFrontPlane)
	{
		_v3d->colour_mask(true);
		_v3d->set_fallback_colour(Colour(0.0f, 1.0f, 0.0f));
		Meshes::Mesh3D::render_data(_v3d, _v3d->get_fallback_shader_program_instance(::System::Video3DFallbackShader::For3D | ::System::Video3DFallbackShader::Basic), nullptr,
			::Meshes::Mesh3DRenderingContext::none(), _completeFrontPlanePoints, ::Meshes::Primitive::TriangleFan, 2, Context::plane_vertex_format());
		_v3d->set_fallback_colour();
	}
#endif
	// if it is not rendered, maybe near & far planes are messed up
	DRAW_OBJECT_NAME(TXT("[RoomProxy] front plane"));
	set_rendering_depth_range_front(_v3d); // force on front plane
#ifdef USE_DEPTH_CLAMP
	_v3d->depth_clamp(true);
#endif
	Meshes::Mesh3D::render_data(_v3d, _v3d->get_fallback_shader_program_instance(::System::Video3DFallbackShader::For3D | ::System::Video3DFallbackShader::Basic), nullptr,
		::Meshes::Mesh3DRenderingContext::none(), _frontPlanePoints, ::Meshes::Primitive::TriangleFan, 2, Context::plane_vertex_format());
	set_rendering_depth_range(_v3d);
#ifdef USE_DEPTH_CLAMP
	_v3d->depth_clamp(false);
#endif
	NO_DRAW_OBJECT_NAME();
}

void RoomProxy::render_far_plane(Context & _context, ::System::Video3D* _v3d, bool _background, Optional<Range3> const& _onlyForFlatRange) const
{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
	MEASURE_PERFORMANCE_COLOURED(renderRoomProxyClearFarPlane, Colour::zxWhite);
#endif

	::System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();
	::System::VideoClipPlaneStack& clipPlaneStack = _v3d->access_clip_plane_stack();
	modelViewStack.push_set(Matrix44::identity);
	clipPlaneStack.push(); // we want to render whole background and existing clipplanes may cause problems with that, we will be using
	_v3d->colour_mask(false);
	_v3d->depth_mask(true);
	_v3d->stencil_test(::System::Video3DCompareFunc::Equal, _context.get_current_stencil_depth());
	_v3d->stencil_op(::System::Video3DOp::Keep);
	_v3d->depth_test(::System::Video3DCompareFunc::Always);
	set_rendering_depth_range_back(_v3d);
#ifdef USE_DEPTH_CLAMP
	_v3d->depth_clamp(true);
#endif
	if (_background)
	{
		_context.render_background_far_plane(_onlyForFlatRange);
	}
	else
	{
		_context.render_far_plane(_onlyForFlatRange);
	}
	// restore some that are not obvious
	set_rendering_depth_range(_v3d);
#ifdef USE_DEPTH_CLAMP
	_v3d->depth_clamp(false);
#endif
	// no sending, we will update that anyway
	modelViewStack.pop();
	clipPlaneStack.pop();
}

void RoomProxy::log(LogInfoContext& _log) const
{
	if (room)
	{
		_log.log(TXT("room \"%S\""), room->get_name().to_char());
	}
	else
	{
		_log.log(TXT("room <unknown>"));
	}
	LOG_INDENT(_log);
	environment->log(_log);

	{
		RoomProxy* room = rooms;
		while (room)
		{
			room->log(_log);
			room = room->next;
		}
	}
}

//

RoomProxy::EnvironmentProxySetupScope::EnvironmentProxySetupScope(Context & _context, ::System::Video3D* _v3d, RoomProxy const * _roomProxy)
: context(_context)
{
	::System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();

	// setup shader program binding context
	::System::ShaderProgramBindingContext prevShaderProgramBindingContext = context.get_shader_program_binding_context();
	::System::ShaderProgramBindingContext currentShaderProgramBindingContext = prevShaderProgramBindingContext;
	currentShaderProgramBindingContext.fill_with(_roomProxy->shaderProgramBindingContextCopy);
	// use copy and override_
	if (_roomProxy->environment)
	{
		_roomProxy->environment->setup_shader_binding_context(&currentShaderProgramBindingContext, modelViewStack);
	}
	else
	{
		EnvironmentProxy::setup_shader_binding_context_no_proxy(&currentShaderProgramBindingContext, modelViewStack);
	}
	_roomProxy->get_lights().setup_shader_binding_context_program_with_prepared(currentShaderProgramBindingContext, modelViewStack);
	context.set_shader_program_binding_context(currentShaderProgramBindingContext);
}

RoomProxy::EnvironmentProxySetupScope::~EnvironmentProxySetupScope()
{
	// restore shader program binding context
	context.set_shader_program_binding_context(prevShaderProgramBindingContext);
}

//

RoomProxyRenderingBuffers::RoomProxyRenderingBuffers()
: scene(buffers.add(Name::invalid()))
, skyBox(buffers.add(NAME(skyBox)))
#ifdef AN_USE_BACKGROUND_RENDERING_BUFFER
, background(buffers.add(NAME(background)))
#endif
{
}