#include "aiManagerData_spawnSet.h"

#include "..\..\roomGenerators\roomGeneratorBase.h"
#include "..\..\regionGenerators\regionGeneratorBase.h"

#include "..\..\..\core\random\randomUtils.h"
#include "..\..\..\core\types\name.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\world\region.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace ManagerDatasLib;

//

DEFINE_STATIC_NAME(spawnSetProbCoef);

//

REGISTER_FOR_FAST_CAST(SpawnSet);

void SpawnSet::register_itself()
{
	ManagerDatas::register_data(Name(TXT("spawn set")), []() { return new SpawnSet(); });
}

bool SpawnSet::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	allowGoingUp = _node->get_bool_attribute_or_from_child_presence(TXT("allowGoingUp"), allowGoingUp);

	// allow easy to use "limitAny"
	Optional<int> limit;
	limit.load_from_xml(_node, TXT("limit"));

	for_every(node, _node->children_named(TXT("type")))
	{
		Element el;
		el.name.load_from_xml(node, TXT("name"), _lc);
		el.tagged.load_from_xml_attribute_or_child_node(node, TXT("tagged"));
		el.probCoef = node->get_float_attribute(TXT("probCoef"), el.probCoef);
		el.limit.load_from_xml(node, TXT("limit"));
		el.limitAny = limit;
		el.limitAny.load_from_xml(node, TXT("limitAny"));
		{
			el.spawnBatch.load_from_xml(node, TXT("spawnBatch"));
			el.spawnBatchOffset.load_from_xml_child_node(node, TXT("spawnBatchOffset"));
			for_every(n, node->children_named(TXT("spawnBatch")))
			{
				el.spawnBatchOffset.load_from_xml_child_node(n, TXT("offset"));
			}
		}
		elements.push_back(el);
		error_loading_xml_on_assert(el.name.is_valid() || ! el.tagged.is_empty(), result, node, TXT("no type name nor tagged info provided"));
	}

	return result;
}

bool SpawnSet::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		for_every(el, elements)
		{
			Framework::ObjectType* objectType = nullptr;
			if (el->name.is_valid())
			{
				if (!objectType) objectType = _library->get_actor_types().find_may_fail(el->name);
				if (!objectType) objectType = _library->get_scenery_types().find_may_fail(el->name);
				if (!objectType) objectType = _library->get_item_types().find_may_fail(el->name);
			}
			if (objectType)
			{
				el->objectTypes.push_back(Element::ObjectType(objectType));
			}
			else if (!objectType && el->name.is_valid())
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (!Framework::Library::may_ignore_missing_references())
#endif
				{
					error(TXT("couldn't find objectType \"%S\""), el->name.to_string().to_char());
					result = false;
				}
			}
			if (!el->tagged.is_empty())
			{
				for_every_ptr(ot, _library->get_actor_types().get_tagged(el->tagged))
				{
					el->objectTypes.push_back(Element::ObjectType(ot));
				}
				for_every_ptr(ot, _library->get_scenery_types().get_tagged(el->tagged))
				{
					el->objectTypes.push_back(Element::ObjectType(ot));
				}
				for_every_ptr(ot, _library->get_item_types().get_tagged(el->tagged))
				{
					el->objectTypes.push_back(Element::ObjectType(ot));
				}
			}
		}
	}

	return result;
}

void SpawnSet::log(LogInfoContext& _log) const
{
	base::log(_log);
	LOG_INDENT(_log);
	for_every(element, elements)
	{
		_log.set_colour(Colour::greyLight);
		String info;
		info += element->name.to_string();

		if (element->probCoef < 1.0f)
		{
			info += String::printf(TXT("  (%.1f)"), element->probCoef);
		}
		if (element->limit.is_set())
		{
			info += String::printf(TXT("  limit=%i"), element->limit.get());
		}
		if (element->limitAny.is_set())
		{
			info += String::printf(TXT("  limit-any=%i"), element->limitAny.get());
		}
		_log.log(TXT("%S"), info.to_char());
		_log.set_colour();
	}
}

Framework::ObjectType* SpawnSet::choose_from(ArrayStatic<Element, MAX_CHOOSE_FROM_ELEMENTS>& _from, Random::Generator & _rg, OUT_ SpawnSetChosenExtraInfo* _extraInfo, std::function<int(Framework::ObjectType* _ofType, int _any)> _how_many_already_spawned)
{
	if (_from.is_empty())
	{
		return nullptr;
	}

	int randomSubIdx = _rg.get_int();

	int tryIdx = RandomUtils::ChooseFromContainer<ArrayStatic<Element, MAX_CHOOSE_FROM_ELEMENTS>, Element>::choose(
		_rg, _from,
		[](Element const& _el) { return _el.probCoef; },
		[_how_many_already_spawned, randomSubIdx](Element const& _el)
		{
			if (_el.objectTypes.is_empty())
			{
				return false;
			}
			int useIdx = mod(randomSubIdx, _el.objectTypes.get_size());
			if (_el.limit.is_set() && _how_many_already_spawned(_el.objectTypes[useIdx].objectType.get(), false) >= _el.limit.get())
			{
				return false;
			}
			if (_el.limitAny.is_set() && _how_many_already_spawned(_el.objectTypes[useIdx].objectType.get(), true) >= _el.limitAny.get())
			{
				return false;
			}
			return true;
		});

	auto from = _from[tryIdx];

	if (!from.objectTypes.is_empty())
	{
		int useIdx = RandomUtils::ChooseFromContainer<Array<Element::ObjectType>, Element::ObjectType>::choose(
			_rg, from.objectTypes,
			[](Element::ObjectType const& _el) { return _el.probCoef; });
		if (_extraInfo)
		{
			_extraInfo->spawnBatch = 1;
			if (!from.spawnBatch.is_empty())
			{
				_extraInfo->spawnBatch = from.spawnBatch.get(_rg);
			}
			_extraInfo->spawnBatchOffset = from.spawnBatchOffset;
		}
		return from.objectTypes[useIdx].objectType.get();
	}
	else
	{
		return nullptr;
	}
}

Framework::ObjectType* SpawnSet::choose(Name const & _spawnSet, Framework::Region* _region, Framework::Room* _room, Random::Generator & _rg, OUT_ SpawnSetChosenExtraInfo* _extraInfo, std::function<int(Framework::ObjectType* _ofType, int _any)> _how_many_already_spawned)
{
	if (_spawnSet.is_valid())
	{
		ArrayStatic<Element, MAX_CHOOSE_FROM_ELEMENTS> chooseFrom; SET_EXTRA_DEBUG_INFO(chooseFrom, TXT("SpawnSet::choose.chooseFrom"));
		bool keepGoing = true;
		if (auto* r = _room)
		{
			_region = r->get_in_region();
			if (auto* rb = fast_cast<RoomGenerators::Base>(r->get_room_generator_info()))
			{
				if (auto* ss = rb->get_ai_managers().get_data<AI::ManagerDatasLib::SpawnSet>(_spawnSet))
				{
					if (! ss->allowGoingUp)
					{
						keepGoing = false;
					}
					chooseFrom.add_from(ss->elements);
				}
			}
		}
		while (_region && keepGoing)
		{
			if (auto* rb = fast_cast<RegionGenerators::Base>(_region->get_region_generator_info()))
			{
				if (auto* ss = rb->get_ai_managers().get_data<AI::ManagerDatasLib::SpawnSet>(_spawnSet))
				{
					if (! ss->allowGoingUp)
					{
						keepGoing = false;
					}
					chooseFrom.add_from(ss->elements);
				}
			}
			_region = _region->get_in_region();
		}
		return choose_from(chooseFrom, _rg, OUT_ _extraInfo, _how_many_already_spawned);
	}
	return nullptr;
}

bool SpawnSet::has_anything(Name const& _spawnSet, Framework::Region* _region, Framework::Room* _room)
{
	if (_spawnSet.is_valid())
	{
		bool keepGoing = true;
		if (auto* r = _room)
		{
			_region = r->get_in_region();
			if (auto* rb = fast_cast<RoomGenerators::Base>(r->get_room_generator_info()))
			{
				if (auto* ss = rb->get_ai_managers().get_data<AI::ManagerDatasLib::SpawnSet>(_spawnSet))
				{
					if (!ss->elements.is_empty())
					{
						return true;
					}
					if (! ss->allowGoingUp)
					{
						keepGoing = false;
					}
				}
			}
		}
		while (_region && keepGoing)
		{
			if (auto* rb = fast_cast<RegionGenerators::Base>(_region->get_region_generator_info()))
			{
				if (auto* ss = rb->get_ai_managers().get_data<AI::ManagerDatasLib::SpawnSet>(_spawnSet))
				{
					if (!ss->elements.is_empty())
					{
						return true;
					}
					if (! ss->allowGoingUp)
					{
						keepGoing = false;
					}
				}
			}
			_region = _region->get_in_region();
		}
	}
	return false;
}

//

SpawnSet::Element::ObjectType::ObjectType(Framework::ObjectType* _ot)
{
	objectType = _ot;
	probCoef = _ot->get_tags().get_tag(NAME(spawnSetProbCoef), 1.0f);
}
