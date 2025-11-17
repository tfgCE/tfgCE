#include "objectLayout.h"

#include "roomLayout.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\library\usedLibraryStoredOrTagged.inl"

#include "..\..\..\core\collision\model.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(ObjectLayout);
LIBRARY_STORED_DEFINE_TYPE(ObjectLayout, objectLayout);

ObjectLayout::ObjectLayout(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

ObjectLayout::~ObjectLayout()
{
}

bool ObjectLayout::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear/reset first
	//
	elements.clear();
	sockets.clear();

	// load
	//
	for_every(containerNode, _node->children_named(TXT("elements")))
	{
		for_every(node, containerNode->all_children())
		{
			if (node->get_name() == TXT("mesh") ||
				node->get_name() == TXT("include"))
			{
				elements.push_back();
				if (elements.get_last().load_from_xml(node, _lc))
				{
					// ok
				}
				else
				{
					elements.pop_back();
				}
			}
		}
	}
	for_every(containerNode, _node->children_named(TXT("sockets")))
	{
		for_every(node, containerNode->children_named(TXT("copyFrom")))
		{
			copySocketsFrom.push_back();
			if (copySocketsFrom.get_last().load_from_xml(node, TXT("mesh"), _lc))
			{
				// ok
			}
			else
			{
				copySocketsFrom.pop_back();
			}
		}
		for_every(node, containerNode->children_named(TXT("socket")))
		{
			sockets.push_back();
			if (sockets.get_last().load_from_xml(node))
			{
				// ok
			}
			else
			{
				sockets.pop_back();
			}
		}
	}

	return result;
}

bool ObjectLayout::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(element, elements)
	{
		result &= element->prepare_for_game(_library, _pfgContext);
	}

	for_every(csf, copySocketsFrom)
	{
		result &= csf->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::CopySockets)
	{
		for_every_ref(csf, copySocketsFrom)
		{
			sockets.make_space_for_additional(csf->get_sockets().get_sockets().get_size());
			for_every(srcSocket, csf->get_sockets().get_sockets())
			{
				if (srcSocket->get_placement_MS().is_set())
				{
					sockets.push_back();
					auto& destSocket = sockets.get_last();
					destSocket.name = srcSocket->get_name();
					destSocket.placement = srcSocket->get_placement_MS().get();
				}
				else
				{
					error(TXT("copied sockets have to be defined as having MS placement, not relative"));
					result = false;
				}
			}
		}
	}

	return result;
}

void ObjectLayout::async_prepare_layout(RoomLayoutContext const& _context, Room* _room, Transform const & _inRoomPlacement, REF_ Random::Generator& _rg, OUT_ ObjectLayoutPrepared& _objectLayoutPrepared) const
{
	async_prepare_layout_add_elements(_context, _inRoomPlacement, REF_ _rg, _objectLayoutPrepared.meshes);
}

void ObjectLayout::async_prepare_layout_add_elements(RoomLayoutContext const& _context, Transform const& _onObjectPlacement, REF_ Random::Generator& _rg, OUT_ Array<LayoutElementMeshInstance>& _toMeshes) const
{
	Random::Generator rg = _rg.spawn();
	for_every(e, elements)
	{
		Random::Generator erg = rg.spawn();
		if (e->chance < 1.0f &&
			!erg.get_chance(e->chance))
		{
			continue;
		}
		Optional<Transform> placement = e->prepare_placement(_onObjectPlacement, _toMeshes, REF_ erg);
		if (placement.is_set())
		{
			if (auto* mesh = UsedLibraryStoredOrTagged<Mesh>::choose_random(e->mesh, REF_ rg, _context.roomContentTagged, true))
			{
				LayoutElementMeshInstance lemi;
				lemi.copy_ids_from(*e);
				lemi.mesh = mesh;
				Optional<Transform> atPlacement = e->alter_placement_with_place_using_socket(placement.get(), mesh);
				if (atPlacement.is_set())
				{
					lemi.placement = atPlacement.get();
					if (auto* mc = mesh->get_movement_collision_model())
					{
						if (auto* cm = mc->get_model())
						{
							lemi.spaceOccupied = cm->calculate_bounding_box(lemi.placement, nullptr, false);
						}
					}
					_toMeshes.push_back(lemi);
				}
			}
			if (auto* objectLayout = UsedLibraryStoredOrTagged<ObjectLayout>::choose_random(e->includeObjectLayout, REF_ rg, _context.roomContentTagged, true))
			{
				Optional<Transform> atPlacement = e->alter_placement_with_place_using_socket(placement.get(), objectLayout);
				if (atPlacement.is_set())
				{
					objectLayout->async_prepare_layout_add_elements(_context, atPlacement.get(), rg, OUT_ _toMeshes);
				}
			}
		}
	}
}

Optional<Transform> ObjectLayout::find_socket_placement(Name const& _socketName) const
{
	for_every(s, sockets)
	{
		if (s->name == _socketName)
		{
			return s->placement;
		}
	}
	return NP;
}

//--

bool ObjectLayoutElement::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	if (_node->get_name() == TXT("include"))
	{
		includeObjectLayout.load_from_xml(_node, TXT("objectLayout"), TXT("objectLayoutTagged"), _lc);
	}

	return result;
}

bool ObjectLayoutElement::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= includeObjectLayout.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	return result;
}
