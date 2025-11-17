#include "worldSetting.h"

#include "doorType.h"
#include "regionType.h"
#include "roomType.h"

#include "..\game\game.h"

#include "..\library\library.h"
#include "..\library\libraryPrepareContext.h"
#include "..\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

WorldSetting::WorldSetting()
{
}

WorldSetting::~WorldSetting()
{
}

bool WorldSetting::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	Name typeName = _node->get_name_attribute(TXT("settingType"));
	String optionId = _node->get_string_attribute(TXT("option"));
	TagCondition forGameTags;
	forGameTags.load_from_xml_attribute_or_child_node(_node, TXT("forGameTags"));
	if (!forGameTags.is_empty() && optionId.is_empty())
	{
		optionId = forGameTags.to_string();
	}

	Type * readInto = nullptr;

	for_every(type, types)
	{
		if (type->type == typeName &&
			type->optionId == optionId)
		{
			readInto = type;
			break;
		}
	}
	if (! readInto)
	{
		types.push_back(Type());
		readInto = &types.get_last();
		readInto->type = typeName;
		readInto->optionId = optionId;
		readInto->forGameTags = forGameTags;
	}

	result &= readInto->load_from_xml(_node, _lc);

	return result;
}

bool WorldSetting::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(type, types)
	{
		result &= type->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

WorldSetting::Type const * WorldSetting::get_type(Name const & _type) const
{
	auto* game = Game::get();
	an_assert(game);
	for_every(type, types)
	{
		if (type->type == _type)
		{
			if (!type->forGameTags.is_empty())
			{
				if (!type->forGameTags.check(game->get_game_tags()))
				{
					// skip this one
					continue;
				}
			}
			return type;
		}
	}
	return nullptr;
}

//

bool WorldSetting::Type::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	forGameTags.load_from_xml_attribute_or_child_node(_node, TXT("forGameTags"));

	error_loading_xml_on_assert(forGameTags.is_empty() || ! optionId.is_empty(), result, _node, TXT("if using \"forGameTags\" remember to set \"option\""));

	if (auto* attr = _node->get_attribute(TXT("parent")))
	{
		checkParent = parse_other(attr->get_as_string());
	}
	if (auto* attr = _node->get_attribute(TXT("main")))
	{
		checkMain = parse_other(attr->get_as_string());
	}

	for_every(node, _node->children_named(TXT("include")))
	{
		if (node->has_attribute(TXT("tagged")))
		{
			TagCondition* t = new TagCondition();
			if (t->load_from_xml_attribute(node, TXT("tagged")))
			{
				tagged.push_back(RefCountObjectPtr<TagCondition>(t));
			}
			else
			{
				error_loading_xml(node, TXT("problem loading tag condition"));
				result = false;
				delete t;
			}
		}
		if (node->has_attribute(TXT("taggedFallback")))
		{
			TagCondition* t = new TagCondition();
			if (t->load_from_xml_attribute(node, TXT("taggedFallback")))
			{
				taggedFallback.push_back(RefCountObjectPtr<TagCondition>(t));
			}
			else
			{
				error_loading_xml(node, TXT("problem loading tag condition"));
				result = false;
				delete t;
			}
		}
		if (auto* attr = node->get_attribute(TXT("doorType")))
		{
			UsedLibraryStored<DoorType> uls;
			if (uls.load_from_xml(attr, _lc))
			{
				doorTypes.push_back(uls);
			}
			else
			{
				result = false;
			}
		}
		if (auto* attr = node->get_attribute(TXT("roomType")))
		{
			UsedLibraryStored<RoomType> uls;
			if (uls.load_from_xml(attr, _lc))
			{
				roomTypes.push_back(uls);
			}
			else
			{
				result = false;
			}
		}
		if (auto* attr = node->get_attribute(TXT("regionType")))
		{
			UsedLibraryStored<RegionType> uls;
			if (uls.load_from_xml(attr, _lc))
			{
				regionTypes.push_back(uls);
			}
			else
			{
				result = false;
			}
		}
	}

	return result;
}

bool WorldSetting::Type::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(doorType, doorTypes)
	{
		result &= doorType->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	for_every(roomType, roomTypes)
	{
		result &= roomType->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	for_every(regionType, regionTypes)
	{
		result &= regionType->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}

bool WorldSetting::Type::is_ok(LibraryStored const * _stored, bool _fallback) const
{
	// for fallback, process all as usual, as we may have just few fallback specific conditions but other are fine (eg. "main")
	for_every(uls, doorTypes)
	{
		if (_stored == uls->get())
		{
			return true;
		}
	}
	for_every(uls, roomTypes)
	{
		if (_stored == uls->get())
		{
			return true;
		}
	}
	for_every(uls, regionTypes)
	{
		if (_stored == uls->get())
		{
			return true;
		}
	}
	for_every_ref(tc, tagged)
	{
		if (tc->check(_stored->get_tags()))
		{
			return true;
		}
	}
	if (_fallback)
	{
		for_every_ref(tc, taggedFallback)
		{
			if (tc->check(_stored->get_tags()))
			{
				return true;
			}
		}
	}
	return false;
}

//

bool CopyWorldSettings::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("copySettings")))
	{
		Source f;
		f.from.load_from_xml(node, TXT("from"), _lc);
		f.fromType = node->get_name_attribute(TXT("fromType"), f.fromType);
		if (f.from.is_valid())
		{
			from.push_back(f);
		}
		else
		{
			error_loading_xml(node, TXT("couldn't load copySettings"));
			result = false;
		}
	}

	return result;
}

bool CopyWorldSettings::apply_to(WorldSettingPtr& _ws) const
{
	bool result = true;
	for_every(f, from)
	{
		WorldSetting const * src = nullptr;

		if (auto* lib = Library::get_current())
		{
			if (!f->fromType.is_valid() || f->fromType == lib->get_room_types().get_type_name())
			{
				if (auto* rt = lib->get_room_types().find_may_fail(f->from))
				{
					src = rt->get_setting();
				}
			}
			if (!f->fromType.is_valid() || f->fromType == lib->get_region_types().get_type_name())
			{
				if (auto* rt = lib->get_region_types().find_may_fail(f->from))
				{
					src = rt->get_setting();
				}
			}
		}

		if (!src)
		{
			error(TXT("could not find \"%S\" of type \"%S\""), f->from.to_string().to_char(), f->fromType.to_char());
			result = false;
		}
		if (src && ! src->types.is_empty())
		{
			if (!_ws.get())
			{
				_ws = new WorldSetting();
			}
			for_every(t, src->types)
			{
				bool addOne = true;
				for_every(d, _ws->types)
				{
					if (d->type == t->type &&
						d->optionId == t->optionId)
					{
						addOne = false; // don't override
						break;
					}
				}
				if (addOne)
				{
					_ws->types.push_back(*t);
				}
			}
		}
	}
	return result;
}
