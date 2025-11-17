#include "presenceLinkProxy.h"

#include "..\..\core\globalSettings.h"
#include "..\..\core\profilePerformanceSettings.h"
#include "..\..\core\mesh\mesh3dRenderingBuffer.h"
#include "..\..\core\system\input.h"
#ifdef BUILD_NON_RELEASE
#include "..\..\core\system\video\renderTargetSave.h"
#endif
#include "..\..\core\vr\iVR.h"

#include "renderContext.h"
#include "renderScene.h"
#include "renderSystem.h"
#include "sceneBuildContext.h"

#include "..\framework.h"
#include "..\debug\previewGame.h"
#include "..\display\display.h"
#include "..\game\gameConfig.h"
#include "..\module\moduleAppearance.h"
#include "..\module\moduleCollision.h"
#include "..\module\modulePresence.h"
#include "..\module\custom\mc_display.h"
#include "..\module\custom\mc_lightSources.h"
#include "..\modulesOwner\modulesOwner.h"
#include "..\modulesOwner\modulesOwnerImpl.inl"
#include "..\object\object.h"
#include "..\object\temporaryObject.h"
#include "..\world\doorInRoom.h"
#include "..\world\presenceLink.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef AN_DEVELOPMENT
#include "..\..\core\system\input.h"
#endif

//#define DISABLE_CLIP_PLANES

//

using namespace Framework;
using namespace Framework::Render;

//

// appearance name
DEFINE_STATIC_NAME_STR(presenceIndicator, TXT("presence indicator"));

//

void PresenceLinkProxy::add_to_list(REF_ PresenceLinkProxy * & _list, PresenceLinkProxy * _toAdd)
{
	if (!_toAdd)
	{
		// nothing to add
		return;
	}
	// just add
	auto* lastToAdd = _toAdd;
	while (lastToAdd->next)
	{
		lastToAdd = lastToAdd->next;
	}
	lastToAdd->next = _list;
	_list = _toAdd;
}

PresenceLinkProxy* PresenceLinkProxy::build_all_in_room(SceneBuildContext & _context, RoomProxy* _forRoomProxy, PresenceLink const * _link, DoorInRoom const * _throughDoorFromOuter, PresenceLinkProxy* _addToPresenceLinks)
{
	PresenceLinkProxy * resultLinks = _addToPresenceLinks;
	PresenceLink const * link = _link;
	DoorInRoom const * throughDoor = _throughDoorFromOuter ? _throughDoorFromOuter->get_door_on_other_side() : nullptr;
	Vector3 cameraLocWS = _context.get_view_matrix().location_to_local(Vector3::zero);
	while (link)
	{
		if (link->is_actual_presence())
		{
#ifdef DONT_ADD_TEMPORARY_OBJECTS_TO_RENDER_SCENE
			if (fast_cast<Framework::TemporaryObject>(link->get_modules_owner()) == nullptr)
#endif
			if (!_forRoomProxy || ! _forRoomProxy->get_predefined_occlusion_culling().is_occluded(cameraLocWS, link->get_placement_in_room().get_translation()))
			{
				bool addAsNormalLink = true;
				// check if we should add camera's owner as something different (using tpp model etc)
				if (_context.get_camera().get_owner() == link->get_modules_owner())
				{
					// view matrix -> to world, as it is inverted matrix
					// can be done just for one eye, we calculate how far away we are
					Vector3 const linkPlacementInMVS = _context.get_view_matrix().location_to_world(link->get_placement_in_room().get_translation());
					Vector3 const ownerPlacementInMVS = _context.get_camera().get_owner_relative_placement().get_translation();
					addAsNormalLink = Vector3::distance_squared(linkPlacementInMVS, ownerPlacementInMVS) > sqr(0.1f); magic_number // use something else than 0.1? this may get problematic when scale kicks in
				}
#ifdef RENDER_SCENE_WITH_ON_SHOULD_ADD_TO_RENDER_SCENE
				if (! _context.get_scene()->get_on_should_add_to_render_scene() ||
					_context.get_scene()->get_on_should_add_to_render_scene()(link, _context))
#endif				
				{
					PresenceLinkProxy* addToList = nullptr;
					if (addAsNormalLink)
					{
						addToList = build(_context, link, throughDoor, _context.get_appearance_name_for_non_owner_to_build());
					}
					else
					{
						addToList = build(_context, link, throughDoor, _context.get_appearance_name_for_owner_to_build());
					}
					if (addToList)
					{
						add_to_list(REF_ resultLinks, addToList);
					}
#ifdef WITH_PRESENCE_INDICATOR
					if (ModulePresence::showPresenceIndicator)
					{
						PresenceLinkProxy* addToList = nullptr;
						addToList = build(_context, link, throughDoor, NAME(presenceIndicator), true);
						if (addToList)
						{
							add_to_list(REF_ resultLinks, addToList);
						}
					}
#endif
				}
			}
		}
		if (link->is_valid_for_light_sources())
		{
			if (_forRoomProxy)
			{
				if (auto* imo = link->get_modules_owner())
				{
					if (auto* ls = imo->get_custom<CustomModules::LightSources>())
					{
						_forRoomProxy->access_lights().add(ls, link->get_placement_in_room());
					}
				}
			}
		}
		link = link->get_next_in_room();
	}
	return resultLinks;
}

static inline bool is_in_front(Plane const & _p, Range3 const & _range)
{
	return _p.get_in_front(Vector3(_range.x.min, _range.y.min, _range.z.min)) > 0.0f ||
		   _p.get_in_front(Vector3(_range.x.max, _range.y.min, _range.z.min)) > 0.0f ||
		   _p.get_in_front(Vector3(_range.x.min, _range.y.max, _range.z.min)) > 0.0f ||
		   _p.get_in_front(Vector3(_range.x.max, _range.y.max, _range.z.min)) > 0.0f ||
		   _p.get_in_front(Vector3(_range.x.min, _range.y.min, _range.z.max)) > 0.0f ||
		   _p.get_in_front(Vector3(_range.x.max, _range.y.min, _range.z.max)) > 0.0f ||
		   _p.get_in_front(Vector3(_range.x.min, _range.y.max, _range.z.max)) > 0.0f ||
		   _p.get_in_front(Vector3(_range.x.max, _range.y.max, _range.z.max)) > 0.0f;
}

PresenceLinkProxy* PresenceLinkProxy::build(SceneBuildContext & _context, PresenceLink const * _link, DoorInRoom const * _throughDoor, Name const & _preferredAppearanceName, bool _onlyIfNameMatches)
{
	ModuleAppearance * appearance = nullptr;
	auto * linkOwner = _link->get_modules_owner();
	if (!linkOwner || !linkOwner->should_be_rendered())
	{
		return nullptr;
	}
#ifdef BUILD_NON_RELEASE
	if (auto* onlyObject = ::System::RenderTargetSaveSpecialOptions::get().onlyObject)
	{
		bool renderThisOne = false;
		{
			auto* o = linkOwner;
			while (o)
			{
				if (o == onlyObject)
				{
					renderThisOne = true;
					break;
				}
				o = o->get_presence()->get_attached_to();
			}
		}
		if (!renderThisOne)
		{
			return nullptr;
		}
	}
#endif
	scoped_call_stack_ptr(linkOwner);
	if (_preferredAppearanceName.is_valid())
	{
		appearance = linkOwner->get_appearance_named(_preferredAppearanceName);
	}
	if (!appearance && _context.get_appearance_name_to_build().is_valid())
	{
		// try preferred to build
		appearance = linkOwner->get_appearance(_context.get_appearance_name_to_build());
	}
	if (!appearance)
	{
		// just get first
		appearance = linkOwner->get_visible_appearance();
	}
	if (appearance && appearance->is_visible())
	{
		if (_onlyIfNameMatches && appearance->get_name() != _preferredAppearanceName)
		{
			return nullptr;
		}
		bool isVisible = false;
		bool doDoorCheck = _throughDoor != nullptr;
		if (appearance->get_ignore_rendering_beyond_distance().is_set() &&
			_context.get_view_matrix().location_to_world(_link->get_placement_in_room_for_rendering_matrix().get_translation()).length() > appearance->get_ignore_rendering_beyond_distance().get())
		{
			isVisible = false;
		}
		else
		{
			/**
			 *	NOTE!
			 *	we do door checks using placement-in-room but visibility checks using placement-in-room-for-rendering
			 *	this is due to for-rendering being more update but that ignores going through door
			 */
			bool checked = false;
			if (!checked && appearance->should_be_visible_to_owner_only())
			{
				bool ownerIsOk = false;
				if (_context.get_camera().get_owner())
				{
					IModulesOwner const * check = appearance->get_owner();
					while (check)
					{
						if (_context.get_camera().get_owner() == check)
						{
							ownerIsOk = true;
							break;
						}
						if (auto * p = check->get_presence())
						{
							check = p->get_attached_to();
						}
						else
						{
							check = nullptr;
						}
					}
				}
				if (!ownerIsOk)
				{
					checked = true; // stop checking, it should remain not visible
					an_assert(!isVisible);
				}
			}
			if (!checked && appearance->does_use_precise_collision_bounding_box_for_frustum_check())
			{
				int id = appearance->get_precise_collision_id();
				if (id != NONE)
				{
					if (auto const * collision = linkOwner->get_collision())
					{
						if (auto const * instance = collision->get_precise_collision().get_instance_by_id(id))
						{
							checked = true;
							auto checkPlacement = _context.get_view_matrix().to_world(_link->get_placement_in_room_for_rendering_matrix());
							if (_context.get_frustum()->is_visible(checkPlacement, instance->get_bounding_box()) &&
								_context.get_planes()->is_visible(checkPlacement, instance->get_bounding_box()))
							{
								isVisible = true;
							}
							// do door check 
							if (isVisible && doDoorCheck)
							{
								if (!is_in_front(_link->get_placement_in_room().to_local(_throughDoor->get_plane()), instance->get_bounding_box()))
								{
									isVisible = false;
								}
								doDoorCheck = false;
							}
						}
					}
				}
			}
			if (!checked && (appearance->does_use_movement_collision_bounding_box_for_frustum_check() ||
							 appearance->does_use_precise_collision_bounding_box_for_frustum_check())) // fallback to movement for precise
			{
				int id = appearance->get_movement_collision_id();
				if (id != NONE)
				{
					if (auto const * collision = linkOwner->get_collision())
					{
						if (auto const * instance = collision->get_movement_collision().get_instance_by_id(id))
						{
							checked = true;
							auto checkPlacement = _context.get_view_matrix().to_world(_link->get_placement_in_room_for_rendering_matrix());
							if (_context.get_frustum()->is_visible(checkPlacement, instance->get_bounding_box()) &&
								_context.get_planes()->is_visible(checkPlacement, instance->get_bounding_box()))
							{
								isVisible = true;
							}
							// do door check 
							if (isVisible && doDoorCheck)
							{
								if (! is_in_front(_link->get_placement_in_room().to_local(_throughDoor->get_plane()), instance->get_bounding_box()))
								{
									isVisible = false;
								}
								doDoorCheck = false;
							}
						}
					}
				}
			}
			if (!checked && appearance->does_use_bones_bounding_box_for_frustum_check())
			{
				checked = true;
				auto checkPlacement = _context.get_view_matrix().to_world(_link->get_placement_in_room_for_rendering_matrix());
				if (_context.get_frustum()->is_visible(checkPlacement, appearance->get_bones_bounding_box()) &&
					_context.get_planes()->is_visible(checkPlacement, appearance->get_bones_bounding_box()))
				{
					isVisible = true;
				}
				// do door check 
				if (isVisible && doDoorCheck)
				{
					if (!is_in_front(_link->get_placement_in_room().to_local(_throughDoor->get_plane()), appearance->get_bones_bounding_box()))
					{
						isVisible = false;
					}
					doDoorCheck = false;
				}
			}
			if (!checked)
			{
				auto checkPlacement = _context.get_view_matrix().location_to_world(_link->get_placement_in_room_for_presence_primitive().get_translation());
				auto checkCentreDistance = _context.get_view_matrix().vector_to_world(_link->get_placement_in_room_for_presence_primitive().vector_to_world(_link->presenceCentreDistance));
				if (_context.get_frustum()->is_visible(checkPlacement, _link->presenceRadius, checkCentreDistance) &&
					_context.get_planes()->is_visible(checkPlacement, _link->presenceRadius, checkCentreDistance))
				{
					isVisible = true;
				}
			}
		}
		if (isVisible && doDoorCheck)
		{
			// check if isn't behind door we entered through
			Vector3 at = _link->get_placement_in_room_for_presence_primitive().get_translation();
			if (_link->presenceCentreDistance.is_zero())
			{
				if (_throughDoor->get_plane().get_in_front(at) < -_link->presenceRadius)
				{
					isVisible = false;
				}
			}
			else
			{
				Vector3 const halfCentreDistance = _link->presenceCentreDistance * 0.5f;
				Vector3 const atA = at - halfCentreDistance;
				Vector3 const atB = at + halfCentreDistance;
				if (_throughDoor->get_plane().get_in_front(atA) < -_link->presenceRadius &&
					_throughDoor->get_plane().get_in_front(atB) < -_link->presenceRadius)
				{
					isVisible = false;
				}
			}
		}
		if (isVisible)
		{
			PresenceLinkProxy* link = PresenceLinkProxy::get_one();

			int lodLevel = 0;
#ifdef AN_DEVELOPMENT_OR_PROFILER
			bool usedLOD = false;
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (GameConfig::does_allow_lod())
#endif
			if (auto* fm = appearance->get_mesh())
			{
				if (auto* m = fm->get_mesh())
				{
					if (fm->get_lod_count() > 1)
					{
						Vector3 placementMVS = _context.get_view_matrix().location_to_world(_link->get_placement_in_room_for_rendering_matrix().get_translation());
						float dist = placementMVS.length();
						if (VR::IVR::can_be_used())
						{
							float threshold = 5.0f;
							if (dist > threshold)
							{
								dist += max(0.0f, (placementMVS * Vector3(1.0f, 0.0f, 1.0f)).length() - threshold);
							}
						}
#ifdef AN_DEVELOPMENT_OR_PROFILER
						float actualDist = dist;
#endif
						float minLODSize = GameConfig::get_min_lod_size();
						float size = max(minLODSize, m->get_size_for_lod());
						float fovSize = min(_context.get_fov_size().x, _context.get_fov_size().y);
						dist *= max(0.001f, fovSize);
						float aggressiveLOD = appearance->get_aggressive_lod();
						float aggressiveLODStart = appearance->get_aggressive_lod_start();
						float aggressiveLODCoef = pow(2.0f, aggressiveLOD);
						dist += size * aggressiveLODStart;
						dist *= aggressiveLODCoef;
#ifdef AN_ALLOW_BULLSHOTS
						if (! BullshotSystem::is_active())
#endif
						if (dist > 0.0f)
						{
							float lodThreshold = GameConfig::get_lod_selector_start();
							float const lodThresholdStep = GameConfig::get_lod_selector_step();
							float const ds = (size * 0.5f) / dist; // this is tan of half angle 
							int const lodCount = fm->get_lod_count();
							while (ds <= lodThreshold && lodLevel < lodCount)
							{
								lodThreshold *= lodThresholdStep;
								++lodLevel;
							}
							/*
							if (appearance->get_owner()->ai_get_name().does_contain(TXT("automap")))
							{
								output(TXT("!@# %S [%i] %.3f (%.3f * 0.5 / %0.3f) -> %i"), appearance->get_owner()->ai_get_name().to_char(), lodLevel, ds, size, dist, fm->get_lod(lodLevel)->get_mesh()->get_part(0)->get_number_of_tris());
							}
							*/
						}
#ifdef AN_DEVELOPMENT_OR_PROFILER
						usedLOD = true;
						if (is_preview_game())
						{
							if (auto* pg = Game::get_as<PreviewGame>())
							{
								pg->store_lod_info(_link->owner->ai_get_name(), lodLevel, size, actualDist, aggressiveLOD, aggressiveLODStart, appearance->get_mesh_instance_for_lod(lodLevel).calculate_triangles());
							}
						}
#endif
					}
				}
			}
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (is_preview_game() && !usedLOD)
			{
				if (auto* pg = Game::get_as<PreviewGame>())
				{
					pg->store_lod_info(_link->owner->ai_get_name(), lodLevel, 0.0f, 0.0f, 0.0f, 0.0f, appearance->get_mesh_instance_for_lod(lodLevel).calculate_triangles());
				}
			}
#endif

			link->appearanceCopy.hard_copy_from(appearance->get_mesh_instance_for_lod(lodLevel));
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
			if (_context.get_scene()->get_on_prepare_to_render())
			{
				_context.get_scene()->get_on_prepare_to_render()(REF_ link->appearanceCopy, _link->get_in_room(), _link->get_placement_in_room());
			}
#endif
			link->appearanceCopy.prepare_to_render();
#ifdef ALLOW_SELECTIVE_RENDERING
			link->appearanceCopy.shouldRender = true;
#ifdef DONT_RENDER_TEMPORARY_OBJECTS
			if (fast_cast<Framework::TemporaryObject>(_link->get_modules_owner()))
			{
				link->appearanceCopy.shouldRender = false;
			}
#endif
#endif
			link->placementInRoomMat = _link->placementInRoomForRenderingMat;
			link->clipPlanes = _link->clipPlanes;
			if (_throughDoor)
			{
				// always add clipping plane for door through which we're looking (as we might be fully beyond that plane)
				link->clipPlanes.add(_throughDoor->get_plane());
			}
			for_every(cp, link->clipPlanes.planes)
			{
				*cp = cp->get_adjusted_along_normal(-0.001f); // render it a bit into the doors to prevent holes/discontinuities between parts of the mesh
			}
			link->next = nullptr;

			_context.for_override_material_render_of(_link->get_in_room(), linkOwner, [link](::System::MaterialInstance const* _materialOverride, int _parts)
			{
				link->appearanceCopy.hard_copy_materials_at(_parts, _materialOverride);
				link->appearanceCopy.prepare_to_render();
			});

			get_display_for(_context, link, linkOwner);

			PresenceLinkProxy* lastLink = link;

#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
			if (::System::Input::get()->has_key_been_pressed(::System::Key::I))
			{
				output(TXT("    [%S] %S (%i)"), appearance->get_name().to_char(), appearance->get_owner()->ai_get_name().to_char(), ++_context.presenceLinkProxyCount);
			}
#endif
#endif

			_context.for_additional_render_of(_link->get_in_room(), linkOwner, [&lastLink, link, appearance, lodLevel
#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
				, &_context
#endif
#else
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
				, & _context
#endif
#endif
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
				, _link
#endif
				](::System::MaterialInstance const* _materialOverride)
			{
				PresenceLinkProxy* addLink = PresenceLinkProxy::get_one();

				addLink->appearanceCopy.hard_copy_from(appearance->get_mesh_instance_for_lod(lodLevel), _materialOverride);
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
				if (_context.get_scene()->get_on_prepare_to_render())
				{
					_context.get_scene()->get_on_prepare_to_render()(REF_ addLink->appearanceCopy, _link->get_in_room(), _link->get_placement_in_room());
				}
#endif
				addLink->appearanceCopy.prepare_to_render();
				addLink->placementInRoomMat = link->placementInRoomMat;
				addLink->clipPlanes = link->clipPlanes;
				lastLink->next = addLink;
				addLink->next = nullptr;
				lastLink = addLink;
#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
				if (::System::Input::get()->has_key_been_pressed(::System::Key::I))
				{
					output(TXT("    [%S] %S (%i) (additional)"), appearance->get_name().to_char(), appearance->get_owner()->ai_get_name().to_char(), ++_context.presenceLinkProxyCount);
				}
#endif
#endif
			});

			return link;
		}
#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->has_key_been_pressed(::System::Key::I) &&
			::System::Input::get()->is_key_pressed(::System::Key::RightShift))
		{
			output_colour(0, 0, 0, 1);
			output(TXT("    [%S] %S"), appearance->get_name().to_char(), appearance->get_owner()->ai_get_name().to_char());
			output_colour();
		}
#endif
#endif
	}
	return nullptr;
}

void PresenceLinkProxy::build_owner_for_foreground(SceneBuildContext & _context, Name const & _appearanceTag, std::function<void(PresenceLinkProxy*)> _do_for_every_link)
{
	build_owner_for_foreground_using_placement_in_room(_context, _appearanceTag,
		Matrix44::identity,
		_do_for_every_link,
		TXT("fg-room"));
}

void PresenceLinkProxy::build_owner_for_foreground_owner_local(SceneBuildContext & _context, Name const & _appearanceTag, std::function<void(PresenceLinkProxy*)> _do_for_every_link)
{
	build_owner_for_foreground_using_placement_in_room(_context, _appearanceTag,
		(_context.get_camera().get_placement().to_world(_context.get_camera().get_owner_relative_placement())).to_matrix(),
		_do_for_every_link,
		TXT("fg-owner-local"));
}

void PresenceLinkProxy::build_owner_for_foreground_camera_local(SceneBuildContext & _context, Name const & _appearanceTag, std::function<void(PresenceLinkProxy*)> _do_for_every_link)
{
	build_owner_for_foreground_using_placement_in_room(_context, _appearanceTag,
		Matrix44::identity,
		_do_for_every_link,
		TXT("fg-camera-local"));
}

void PresenceLinkProxy::build_owner_for_foreground_camera_local_owner_orientation(SceneBuildContext & _context, Name const & _appearanceTag, std::function<void(PresenceLinkProxy*)> _do_for_every_link)
{
	build_owner_for_foreground_using_placement_in_room(_context, _appearanceTag,
		(_context.get_camera().get_placement().to_world(_context.get_camera().get_owner_relative_placement())).get_orientation().to_matrix(),
		_do_for_every_link,
		TXT("fg-camera-local-owner-orientation"));
}

void PresenceLinkProxy::build_owner_for_foreground_using_placement_in_room(SceneBuildContext & _context, Name const & _appearanceTag, Matrix44 const & _placementInRoom, std::function<void(PresenceLinkProxy*)> _do_for_every_link, tchar const * _debugInfo)
{
	if (!_appearanceTag.is_valid())
	{
		return;
	}
	if (IModulesOwner const * owner = _context.get_camera().get_owner())
	{
		if (!owner->should_be_rendered())
		{
			return;
		}
		for_every_ptr(appearance, owner->get_appearances())
		{
			if (!appearance->get_tags().get_tag(_appearanceTag))
			{
				continue;
			}
			if (appearance && appearance->is_visible())
			{
				PresenceLinkProxy* link = PresenceLinkProxy::get_one();

				link->appearanceCopy.hard_copy_from(appearance->meshInstance);
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
				if (_context.get_scene()->get_on_prepare_to_render())
				{
					_context.get_scene()->get_on_prepare_to_render()(REF_ link->appearanceCopy, owner->get_presence()->get_in_room(), _placementInRoom.to_transform());
				}
#endif
				link->appearanceCopy.prepare_to_render();
				link->placementInRoomMat = _placementInRoom;
				link->clipPlanes.clear();
				link->next = nullptr;

				get_display_for(_context, link, owner);

#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
				if (::System::Input::get()->has_key_been_pressed(::System::Key::I))
				{
					output(TXT("    %S [%S] %S (%i)"), _debugInfo, appearance->get_name().to_char(), appearance->get_owner()->ai_get_name().to_char(), ++_context.presenceLinkProxyCount);
				}
#endif
#endif

				_context.for_override_material_render_of(owner->get_presence()->get_in_room(), owner, [link](::System::MaterialInstance const * _materialOverride, int _parts)
				{
					link->appearanceCopy.hard_copy_materials_at(_parts, _materialOverride);
					link->appearanceCopy.prepare_to_render();
				});

				PresenceLinkProxy* lastLink = link;

				_context.for_additional_render_of(owner->get_presence()->get_in_room(), owner, [
#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
					_debugInfo, & _context,
#endif
#else
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
					& _context,
#endif
#endif
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
					owner,
#endif
					&lastLink, link, appearance](::System::MaterialInstance const* _materialOverride)
				{
					PresenceLinkProxy* addLink = PresenceLinkProxy::get_one();

					addLink->appearanceCopy.hard_copy_from(appearance->meshInstance, _materialOverride);
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
					if (_context.get_scene()->get_on_prepare_to_render())
					{
						_context.get_scene()->get_on_prepare_to_render()(REF_ link->appearanceCopy, owner->get_presence()->get_in_room(), link->placementInRoomMat.to_transform());
					}
#endif
					addLink->appearanceCopy.prepare_to_render();
					addLink->placementInRoomMat = link->placementInRoomMat;
					addLink->clipPlanes = link->clipPlanes;
					lastLink->next = addLink;
					addLink->next = nullptr;
					lastLink = addLink;
#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
					if (::System::Input::get()->has_key_been_pressed(::System::Key::I))
					{
						output(TXT("    %S [%S] %S (%i) (additional)"), _debugInfo, appearance->get_name().to_char(), appearance->get_owner()->ai_get_name().to_char(), ++_context.presenceLinkProxyCount);
					}
#endif
#endif
				});

				_do_for_every_link(link);
			}
		}
	}
}

void PresenceLinkProxy::on_release()
{
#ifndef LIGHTS_PER_ROOM_ONLY
	lights.clear();
#endif
	if (next)
	{
		next->release();
		next = nullptr;
	}
}

void PresenceLinkProxy::add_all_to(SceneBuildContext & _context, REF_ Meshes::Mesh3DRenderingBuffer & _renderingBuffer, Matrix44 const & _placement, bool _placementOverride) const
{
	PresenceLinkProxy const * presenceLink = this;
	while (presenceLink)
	{
#ifdef AN_RENDERER_STATS
		Stats::get().renderedPresenceLinkProxies.increase();
		_context.access_stats().renderedPresenceLinkProxies.increase();
#endif

		Matrix44 placement = _placement;

		// add
		if (
#ifndef LIGHTS_PER_ROOM_ONLY
			true ||
#endif
			Render::System::get()->setup_shader_program_on_bound_for_presence_link)
		{
			_renderingBuffer.add(&presenceLink->appearanceCopy, _placementOverride, _placement.to_world(presenceLink->placementInRoomMat),
				[placement, presenceLink](Matrix44 const & _renderingBufferModelViewMatrixForRendering, ::Meshes::Mesh3DRenderingContext & _renderingContext)
				{
					an_assert(!_renderingContext.setup_shader_program_on_bound);

					Matrix44 renderingBufferModelViewMatrixForRendering = _renderingBufferModelViewMatrixForRendering;

					_renderingContext.setup_shader_program_on_bound = [renderingBufferModelViewMatrixForRendering, placement, presenceLink](::System::ShaderProgram * _shaderProgram)
					{
						if (Render::System::get()->setup_shader_program_on_bound_for_presence_link)
						{
							Render::System::get()->setup_shader_program_on_bound_for_presence_link(_shaderProgram, renderingBufferModelViewMatrixForRendering, placement, presenceLink);
						}
						// currently we do lights per room
#ifndef LIGHTS_PER_ROOM_ONLY
						presenceLink->get_lights().setup_shader_program(_shaderProgram, renderingBufferModelViewMatrixForRendering);
#endif
					};
				},
				presenceLink->clipPlanes);
		}
		else
		{
			_renderingBuffer.add(&presenceLink->appearanceCopy, _placementOverride, _placement.to_world(presenceLink->placementInRoomMat),
				nullptr, 
				presenceLink->clipPlanes);
		}

		// next
		presenceLink = presenceLink->next;
	}
}

void PresenceLinkProxy::add_all_to(SceneBuildContext & _context, REF_ Meshes::Mesh3DRenderingBufferSet & _renderingBuffers, Matrix44 const & _placement, bool _placementOverride) const
{
	PresenceLinkProxy const * presenceLink = this;
	while (presenceLink)
	{
#ifdef AN_RENDERER_STATS
		Stats::get().renderedPresenceLinkProxies.increase();
		_context.access_stats().renderedPresenceLinkProxies.increase();
#endif

		Matrix44 placement = _placement;

		todo_note(TXT("implement_ lights in other places too!"));
		// add
		if (
#ifndef LIGHTS_PER_ROOM_ONLY
			true ||
#endif
			Render::System::get()->setup_shader_program_on_bound_for_presence_link)
		{
			_renderingBuffers.add(&presenceLink->appearanceCopy, _placementOverride, _placement.to_world(presenceLink->placementInRoomMat),
				[placement, presenceLink](Matrix44 const& _renderingBufferModelViewMatrixForRendering, ::Meshes::Mesh3DRenderingContext& _renderingContext)
				{
					an_assert(!_renderingContext.setup_shader_program_on_bound);

					Matrix44 renderingBufferModelViewMatrixForRendering = _renderingBufferModelViewMatrixForRendering;

					_renderingContext.setup_shader_program_on_bound = [renderingBufferModelViewMatrixForRendering, placement, presenceLink](::System::ShaderProgram* _shaderProgram)
					{
						if (Render::System::get()->setup_shader_program_on_bound_for_presence_link)
						{
							Render::System::get()->setup_shader_program_on_bound_for_presence_link(_shaderProgram, renderingBufferModelViewMatrixForRendering, placement, presenceLink);
						}
						// currently we do lights per room
#ifndef LIGHTS_PER_ROOM_ONLY
						presenceLink->get_lights().setup_shader_program(_shaderProgram, renderingBufferModelViewMatrixForRendering);
#endif
					};
				},
				presenceLink->clipPlanes);
		}
		else
		{
			_renderingBuffers.add(&presenceLink->appearanceCopy, _placementOverride, _placement.to_world(presenceLink->placementInRoomMat),
				nullptr,
				presenceLink->clipPlanes);
		}

		// next
		presenceLink = presenceLink->next;
	}
}

void PresenceLinkProxy::get_display_for(SceneBuildContext & _context, PresenceLinkProxy* _link, IModulesOwner const * _owner)
{
	if (auto* display = _owner->get_custom<CustomModules::Display>())
	{
		if (display->get_display()->is_visible())
		{
			_context.get_scene()->add_display(display->get_display());
		}

		for_count(int, i, _link->appearanceCopy.get_material_instance_count())
		{
			if (auto * material = _link->appearanceCopy.access_material_instance(i))
			{
				display->update_display_in(*material);
			}
		}
	}
}
