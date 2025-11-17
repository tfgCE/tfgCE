#include "wallLayout.h"

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

REGISTER_FOR_FAST_CAST(WallLayout);
LIBRARY_STORED_DEFINE_TYPE(WallLayout, wallLayout);

WallLayout::WallLayout(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

WallLayout::~WallLayout()
{
}

bool WallLayout::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear/reset first
	//
	elements.clear();
	disallowedDoorPlacements.clear();
	fixedDoorPlacements.clear();

	// load
	//
	for_every(containerNode, _node->children_named(TXT("elements")))
	{
		for_every(node, containerNode->all_children())
		{
			if (node->get_name() == TXT("mesh") ||
				node->get_name() == TXT("include"))
			{
				elements.push_back(WallLayoutElement());
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

	for_every(node, _node->children_named(TXT("disallowDoorPlacement")))
	{
		disallowedDoorPlacements.push_back();
		disallowedDoorPlacements.get_last().range = Range(-1000.0f, 1000.0f);
		if (disallowedDoorPlacements.get_last().load_from_xml(node, _lc))
		{
			// ok
		}
		else
		{
			disallowedDoorPlacements.pop_back();
		}
	}

	for_every(node, _node->children_named(TXT("fixedDoorPlacement")))
	{
		fixedDoorPlacements.push_back();
		if (fixedDoorPlacements.get_last().load_from_xml(node, _lc))
		{
			// ok
		}
		else
		{
			fixedDoorPlacements.pop_back();
		}
	}

	return result;
}

bool WallLayout::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(element, elements)
	{
		result &= element->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

void WallLayout::async_prepare_layout(RoomLayoutContext const& _context, Room* _room, Transform const& _wallPlacement, float _wallLength, REF_ Random::Generator& _rg, OUT_ WallLayoutPrepared& _wallLayoutPrepared) const
{
	_wallLayoutPrepared.wallPlacement = _wallPlacement;
	_wallLayoutPrepared.wallRange.min = -_wallLength * 0.5f;
	_wallLayoutPrepared.wallRange.max = _wallLength * 0.5f;

	// add two disallows on sides
	{
		{
			_wallLayoutPrepared.disallowedDoorPlacements.push_back();
			auto& l = _wallLayoutPrepared.disallowedDoorPlacements.get_last();
			l.range = Range(-1000.0f, -_wallLength * 0.5f);
		}
		{
			_wallLayoutPrepared.disallowedDoorPlacements.push_back();
			auto& l = _wallLayoutPrepared.disallowedDoorPlacements.get_last();
			l.range = Range(_wallLength * 0.5f, 1000.0f);
		}
	}

	async_prepare_layout_add_elements(_context, Transform::identity, _wallLength, REF_ _rg, _wallLayoutPrepared);

	// simplify disallowed door ranges
	_wallLayoutPrepared.unify_disallowed_door_placements();
}

void WallLayout::async_prepare_layout_add_elements(RoomLayoutContext const& _context, Transform const& _onWallPlacement, float _wallLength, REF_ Random::Generator& _rg, OUT_ WallLayoutPrepared& _wallLayoutPrepared) const
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
		Optional<Transform> placement = e->prepare_placement(_onWallPlacement, _wallLayoutPrepared.meshes, REF_ erg);
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
					if (auto* mc = mesh->get_precise_collision_model()) // use precise here as for some objects we may ignore movement collision when they're on wall
					{
						if (auto* cm = mc->get_model())
						{
							lemi.spaceOccupied = cm->calculate_bounding_box(lemi.placement, nullptr, false);
						}
					}
					_wallLayoutPrepared.meshes.push_back(lemi);
				}
			}
			if (auto* wallLayout = UsedLibraryStoredOrTagged<WallLayout>::choose_random(e->includeWallLayout, REF_ rg, _context.roomContentTagged, true))
			{
				Transform wlPlacement = placement.get();
				wallLayout->async_prepare_layout_add_elements(_context, wlPlacement, _wallLength, rg, OUT_ _wallLayoutPrepared);
			}
			if (auto* objectLayout = UsedLibraryStoredOrTagged<ObjectLayout>::choose_random(e->includeObjectLayout, REF_ rg, _context.roomContentTagged, true))
			{
				Optional<Transform> atPlacement = e->alter_placement_with_place_using_socket(placement.get(), objectLayout);
				if (atPlacement.is_set())
				{
					objectLayout->async_prepare_layout_add_elements(_context, atPlacement.get(), rg, OUT_ _wallLayoutPrepared.meshes);
				}
			}
		}
	}

	for_every(ddr, disallowedDoorPlacements)
	{
		_wallLayoutPrepared.disallowedDoorPlacements.push_back(*ddr);
		_wallLayoutPrepared.disallowedDoorPlacements.get_last().offset_by(_onWallPlacement);
	}

	for_every(fdp, fixedDoorPlacements)
	{
		_wallLayoutPrepared.fixedDoorPlacements.push_back(*fdp);
		auto& l = _wallLayoutPrepared.fixedDoorPlacements.get_last();
		l.offset_by(_onWallPlacement);
	}

	_wallLayoutPrepared.usesFixedDoorPlacements = ! _wallLayoutPrepared.fixedDoorPlacements.is_empty();
}

//--

bool WallLayoutFixedDoorPlacement::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	at = _node->get_float_attribute(TXT("at"), at);

	return result;
}

bool WallLayoutFixedDoorPlacement::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

void WallLayoutFixedDoorPlacement::offset_by(Transform const& _onWallPlacement)
{
	float by = _onWallPlacement.get_translation().x;
	at += by;
}

//--

bool WallLayoutDisallowedDoorPlacement::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	range.load_from_attribute_or_from_child(_node, TXT("range"));

	return result;
}

bool WallLayoutDisallowedDoorPlacement::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

void WallLayoutDisallowedDoorPlacement::offset_by(Transform const& _onWallPlacement)
{
	if (!range.is_empty())
	{
		float by = _onWallPlacement.get_translation().x;
		range.min += by;
		range.max += by;
	}
}

//--

bool WallLayoutElement::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);


	if (_node->get_name() == TXT("include"))
	{
		includeWallLayout.load_from_xml(_node, TXT("wallLayout"), TXT("wallLayoutTagged"), _lc);
		includeObjectLayout.load_from_xml(_node, TXT("objectLayout"), TXT("objectLayoutTagged"), _lc);
	}

	return result;
}

bool WallLayoutElement::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= includeWallLayout.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	result &= includeObjectLayout.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	return result;
}

//--

void WallLayoutPrepared::unify_disallowed_door_placements()
{
	bool somethingToDo = true;
	while (somethingToDo)
	{
		somethingToDo = false;
		for (int i = 0; i < disallowedDoorPlacements.get_size(); ++i)
		{
			auto& ddri = disallowedDoorPlacements[i];
			for (int j = i + 1; j < disallowedDoorPlacements.get_size(); ++j)
			{
				auto& ddrj = disallowedDoorPlacements[j];
				if (ddrj.range.min <= ddri.range.max && ddrj.range.max >= ddri.range.min)
				{
					ddri.range.include(ddrj.range);
					disallowedDoorPlacements.remove_fast_at(j);
					--j;
					somethingToDo = true;
				}
			}
		}
	}
}
