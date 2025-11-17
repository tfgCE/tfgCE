#include "roomPartInstance.h"

#include "room.h"
#include "world.h"

#include "..\appearance\mesh.h"
#include "..\game\game.h"
#include "..\library\usedLibraryStored.inl"
#include "..\video\texture.h"

#include "..\..\core\mesh\mesh3d.h"

using namespace Framework;

RoomPartInstance::RoomPartInstance(RoomPart const * _roomPart)
: insideRoom(nullptr)
, roomPart(_roomPart)
, placement(Transform::identity)
{
}

RoomPartInstance::~RoomPartInstance()
{
	if (insideRoom)
	{
		for_every(element, elements)
		{
			element->remove_from(insideRoom);
		}
	}
}

void RoomPartInstance::place(Room * _insideRoom, Transform const & _placement)
{
	an_assert(elements.is_empty());
	placement = _placement;
	insideRoom = _insideRoom;
	elements.make_space_for(roomPart->elements.get_size()); // we shouldn't be resizing!
	for_every(roomPartElement, roomPart->elements)
	{
		elements.push_back(Element());
		elements.get_last().place(insideRoom, _placement, *roomPartElement);
	}
	pois.make_space_for(roomPart->pois.get_size()); // we shouldn't be resizing!
	Random::Generator rg = _insideRoom->get_individual_random_generator();
	rg.advance_seed(elements.get_size(), pois.get_size());
	for_every(poi, roomPart->pois)
	{
		pois.push_back(PointOfInterestInstancePtr(Game::get()->get_customisation().create_point_of_interest_instance()));
		pois.get_last()->access_tags().set_tags_from(_insideRoom->get_mark_POIs_inside());
		pois.get_last()->create_instance(rg.spawn().temp_ref(), poi->get(), _insideRoom, nullptr, NONE, placement, nullptr, nullptr);
	}
}

//

RoomPartInstance::Element::Element()
: mesh(nullptr)
, appearance(nullptr)
, movementCollisionId(NONE)
, preciseCollisionId(NONE)
{
}

RoomPartInstance::Element::~Element()
{
	an_assert(mesh == nullptr);
	an_assert(appearance == nullptr);
	an_assert(movementCollisionId == NONE);
	an_assert(preciseCollisionId == NONE);
}

void RoomPartInstance::Element::remove_from(Room * _room)
{
	if (appearance)
	{
		_room->access_appearance().remove(appearance);
		appearance = nullptr;
	}
	if (movementCollisionId != NONE)
	{
		_room->access_movement_collision().remove_by_id(movementCollisionId);
		_room->mark_requires_update_bounding_box();
		movementCollisionId = NONE;
	}
	if (preciseCollisionId != NONE)
	{
		_room->access_movement_collision().remove_by_id(preciseCollisionId);
		_room->mark_requires_update_bounding_box();
		preciseCollisionId = NONE;
	}
	if (mesh)
	{
		mesh = nullptr;
	}
}

void RoomPartInstance::Element::place(Room * _insideRoom, Transform const & _placement, RoomPart::Element const & _element)
{
	mesh = _element.mesh.get();
	Transform placement = _placement.to_world(_element.placement);
	if (mesh)
	{
		_insideRoom->add_mesh(mesh, placement, OUT_ &appearance, OUT_ &movementCollisionId, OUT_ &preciseCollisionId);
	}
	Random::Generator rg = _insideRoom->get_individual_random_generator();
	rg.advance_seed(pois.get_size(), pois.get_size());
	pois.make_space_for(_element.pois.get_size()); // we shouldn't be resizing!
	for_every(poi, _element.pois)
	{
		pois.push_back(PointOfInterestInstancePtr(Game::get()->get_customisation().create_point_of_interest_instance()));
		pois.get_last()->access_tags().set_tags_from(_insideRoom->get_mark_POIs_inside());
		pois.get_last()->create_instance(rg.spawn().temp_ref(), poi->get(), _insideRoom, nullptr, NONE, placement, _element.mesh.get(), appearance);
	}
}

void RoomPartInstance::for_every_point_of_interest(Optional<Name> const & _poiName, OnFoundPointOfInterest _fpoi, Room const * _room, IsPointOfInterestValid _is_valid) const
{
	ASSERT_NOT_ASYNC_OR(_room->get_in_world()->multithread_check__reading_world_is_safe());
	for_every_const(element, elements)
	{
		for_every_const(poi, element->pois)
		{
			if (!poi->get()->is_cancelled() &&
				(!_poiName.is_set() || poi->get()->get_name() == _poiName.get()) &&
				(!_is_valid || _is_valid(poi->get())))
			{
				_fpoi(poi->get());
			}
		}
	}
	for_every_const(poi, pois)
	{
		if (!poi->get()->is_cancelled() &&
			(!_poiName.is_set() || poi->get()->get_name() == _poiName.get()) &&
			(!_is_valid || _is_valid(poi->get())))
		{
			_fpoi(poi->get());
		}
	}
}

void RoomPartInstance::debug_draw_pois(bool _setDebugContext)
{
	for_every(element, elements)
	{
		for_every_ref(poi, element->pois)
		{
			if (!poi->is_cancelled())
			{
				poi->debug_draw(_setDebugContext);
			}
		}
	}
	for_every_ref(poi, pois)
	{
		if (!poi->is_cancelled())
		{
			if (!poi->is_cancelled())
			{
				poi->debug_draw(_setDebugContext);
			}
		}
	}
}

void RoomPartInstance::debug_draw_mesh_nodes()
{
	for_every(element, elements)
	{
		Transform p = placement;
		if (element->appearance)
		{
			p = p.to_world(element->appearance->get_placement());
		}
		for_every_ref(mn, element->mesh->get_mesh_nodes())
		{
			mn->debug_draw(p);
		}
	}
}
