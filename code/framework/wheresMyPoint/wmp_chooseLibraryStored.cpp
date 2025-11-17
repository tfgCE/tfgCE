#include "wmp_chooseLibraryStored.h"

#include "..\framework.h"
#include "..\debug\previewGame.h"
#include "..\library\library.h"
#include "..\world\region.h"
#include "..\world\room.h"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\random\randomUtils.h"

using namespace Framework;

bool ChooseLibraryStored::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	type = _node->get_name_attribute(TXT("type"), type);
	error_loading_xml_on_assert(type.is_valid(), result, _node, TXT("\"type\" not provided for choose library stored"));

	for_every(node, _node->children_named(TXT("choose")))
	{
		Entry e;
		if (e.load_from_xml(node))
		{
			entries.push_back(e);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool ChooseLibraryStored::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	Random::Generator & rg = _context.access_random_generator();

	ARRAY_STACK(Entry const *, entriesLeft, entries.get_size());
	for_every(entry, entries)
	{
		entriesLeft.push_back(entry);
	}

	// default return value
	_value.set<LibraryName>(LibraryName::invalid());

	if (entriesLeft.is_empty())
	{
		error(TXT("no candidates to use!"));
		return false;
	}

	bool result = false;
	while (!entriesLeft.is_empty())
	{
		int idx = RandomUtils::ChooseFromContainer<ArrayStack<Entry const *>, Entry const *>::choose(rg, entriesLeft, [](Entry const * const & _entry) { return _entry->probabilityCoef; });
		if (entriesLeft.is_index_valid(idx))
		{
			auto const & entry = *entriesLeft[idx];
			if (entry.name.is_valid())
			{
				_value.set<LibraryName>(entry.name);
				break;
			}
			else if (! entry.worldSettingCondition.is_empty())
			{
				Array<LibraryStored*> stored;
				Room* inRoom = nullptr;
				Region* inRegion = nullptr;
				{
					WheresMyPoint::IOwner* wmpOwner = _context.get_owner();
					while (wmpOwner)
					{
						if ((inRoom = fast_cast<Room>(wmpOwner)))
						{
							break;
						}
						if ((inRegion = fast_cast<Region>(wmpOwner)))
						{
							break;
						}
						wmpOwner = wmpOwner->get_wmp_owner();
					}
				}
				WorldSettingConditionContext wscc(inRoom);
				if (! inRoom && inRegion) wscc = WorldSettingConditionContext(inRegion);
				wscc.get(Library::get_current(), type, stored, entry.worldSettingCondition);
				if (!inRoom && !inRegion && !is_preview_game())
				{
					warn_processing_wmp(TXT("no room or region for choose library stored, this may fail"));
				}
				if (!stored.is_empty())
				{
					int storedIdx = RandomUtils::ChooseFromContainer<Array<LibraryStored*>, LibraryStored*>::choose(rg, stored, LibraryStored::get_prob_coef_from_tags<LibraryStored>);
					if (stored.is_index_valid(storedIdx))
					{
						_value.set<LibraryName>(stored[storedIdx]->get_name());
						result = true;
						break;
					}
					else
					{
						entriesLeft.remove_fast_at(idx);
					}
				}
				else
				{
					entriesLeft.remove_fast_at(idx);
				}
			}
			else
			{
				entriesLeft.remove_fast_at(idx);
			}
		}
	}

	return result;
}

//

bool ChooseLibraryStored::Entry::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	result &= name.load_from_xml(_node, TXT("name"));
	result &= worldSettingCondition.load_from_xml(_node);
	error_loading_xml_on_assert(name.is_valid() || ! worldSettingCondition.is_empty(), result, _node, TXT("nothing to choose provided"));
	probabilityCoef = _node->get_float_attribute_or_from_child(TXT("probCoef"), probabilityCoef);
	probabilityCoef = _node->get_float_attribute_or_from_child(TXT("probabilityCoef"), probabilityCoef);
	return result;
}
