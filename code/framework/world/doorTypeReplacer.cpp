#include "doorTypeReplacer.h"

#include "doorInRoom.h"
#include "doorType.h"
#include "room.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
	#ifdef AN_ALLOW_EXTENSIVE_LOGS
		#define AN_INSPECT_DOOR_TYPE_REPLACER
	#endif
#endif

//

using namespace Framework;

//

bool DoorTypeReplacer::Operation::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= ifTagged.load_from_xml_attribute(_node, TXT("ifTagged"));
	result &= ifActualDoorTagged.load_from_xml_attribute(_node, TXT("ifActualDoorTagged"));
	isWorldSeparator.load_from_xml(_node, TXT("isWorldSeparator"));

	type = Disallow;
	if (_node->get_name() == TXT("disallow")) type = Disallow; else
	if (_node->get_name() == TXT("allow")) type = Allow; else
	if (_node->get_name() == TXT("changeIfAllowed")) type = ChangeIfAllowed; else
	if (_node->get_name() == TXT("changeIfBothAgree")) type = ChangeIfBothAgree; else
	if (_node->get_name() == TXT("forceChange")) type = ForceChange; else
	{
		error_loading_xml(_node, TXT("not recognised door type replacer operation \"%S\""), _node->get_name().to_char());
		result = false;
	}

	if (!doorType.load_from_xml(_node, TXT("doorType"), _lc))
	{
		error_loading_xml(_node, TXT("could not load doorType"));
		result = false;
	}

	if (type == ChangeIfAllowed ||
		type == ChangeIfBothAgree ||
		type == ForceChange)
	{
		if (!doorType.is_name_valid())
		{
			error_loading_xml(_node, TXT("no doorType given for door type replacer operation"));
			result = false;
		}
	}

	return result;
}

bool DoorTypeReplacer::Operation::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (doorType.is_name_valid())
	{
		result &= doorType.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}

bool DoorTypeReplacer::Operation::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	if (doorType.is_name_valid())
	{
		result &= _renamer.apply_to(doorType, _library);
	}

	return true;
}

//

bool DoorTypeReplacer::For::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (!doorType.load_from_xml(_node, TXT("doorType"), _lc))
	{
		error_loading_xml(_node, TXT("could not load doorType"));
		result = false;
	}

	result &= tagged.load_from_xml_attribute(_node, TXT("tagged"));
	result &= actualDoorTagged.load_from_xml_attribute(_node, TXT("actualDoorTagged"));

	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("for"))
		{
			continue;
		}
		Operation op;
		if (op.load_from_xml(node, _lc))
		{
			operations.push_back(op);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool DoorTypeReplacer::For::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (doorType.is_name_valid())
	{
		result &= doorType.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	for_every(operation, operations)
	{
		result &= operation->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool DoorTypeReplacer::For::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	if (doorType.is_name_valid())
	{
		result &= _renamer.apply_to(doorType, _library);
	}

	for_every(operation, operations)
	{
		result &= operation->apply_renamer(_renamer, _library);
	}

	return true;
}

bool DoorTypeReplacer::For::process(REF_ DoorType * & _doorType, DoorTypeReplacerContext const& _context, DoorInRoom const* _thisDIR, For const * _other) const
{
	if (_thisDIR && _thisDIR->get_door() && !_thisDIR->get_door()->is_replacable_by_door_type_replacer())
	{
#ifdef AN_INSPECT_DOOR_TYPE_REPLACER
		output(TXT(" --> not replacable by door type replacer (door itself or type \"%S\")"), _doorType->get_name().to_string().to_char());
#endif
		// we could switch to an irreplacable door type
		return true;
	}
	for_every(operation, operations)
	{
		if (operation->isWorldSeparator.is_set() &&
			operation->isWorldSeparator.get() != _context.isWorldSeparator)
		{
			continue;
		}
		if (! operation->ifTagged.check(_doorType->get_tags()))
		{
			// skip
			continue;
		}
		if (! operation->ifActualDoorTagged.is_empty() && _thisDIR)
		{
			Tags tags;
			tags.set_tags_from(_thisDIR->get_tags());
			if (auto* d = _thisDIR->get_door_on_other_side())
			{
				tags.set_tags_from(d->get_tags());
			}
			tags.set_tags_from(_thisDIR->get_door()->get_tags());
			if (!operation->ifActualDoorTagged.check(tags))
			{
				continue;
			}
		}
		// allow ignored, because why? it is only a response
		if (_other) // these require other's responses
		{
			if (operation->type == Operation::ChangeIfAllowed)
			{
				for_every(otherOperation, _other->operations)
				{
					if (! otherOperation->ifTagged.check(_doorType->get_tags()))
					{
						// skip
						continue;
					}
					if (otherOperation->type == Operation::Allow &&
						(! otherOperation->doorType.is_name_valid() ||
						 otherOperation->doorType.get() == operation->doorType.get())
						)
					{
						_doorType = operation->doorType.get();
#ifdef AN_INSPECT_DOOR_TYPE_REPLACER
						output(TXT(" --> \"%S\""), _doorType ? _doorType->get_name().to_string().to_char() : TXT("--"));
#endif
						return true;
					}
					if (otherOperation->type == Operation::Disallow &&
						otherOperation->doorType.get() == operation->doorType.get())
					{
						// keep looking but skip this one
						break;
					}
				}
			}
			if (operation->type == Operation::ChangeIfBothAgree)
			{
				for_every(otherOperation, _other->operations)
				{
					if (otherOperation->type == Operation::ChangeIfBothAgree &&
						otherOperation->doorType.get() == operation->doorType.get())
					{
						_doorType = operation->doorType.get();
#ifdef AN_INSPECT_DOOR_TYPE_REPLACER
						output(TXT(" --> \"%S\""), _doorType ? _doorType->get_name().to_string().to_char() : TXT("--"));
#endif
						return true;
					}
					if (otherOperation->type == Operation::Disallow &&
						otherOperation->doorType.get() == operation->doorType.get())
					{
						// keep looking but skip this one
						break;
					}
				}
			}
		}
		if (operation->type == Operation::ForceChange ||
			(! _other && operation->type == Operation::ChangeIfAllowed)) // there is no other to disallow, both agree requires actual door type replacer to explicitly agree
		{
			_doorType = operation->doorType.get();
#ifdef AN_INSPECT_DOOR_TYPE_REPLACER
			output(TXT(" --> \"%S\""), _doorType ? _doorType->get_name().to_string().to_char() : TXT("--"));
#endif
			return true;
		}
		/*	
			for:
				operation->type == Operation::Allow
				operation->type == Operation::Disallow
			keep looking but skip this one
			is just an information what to allow, what to disallow
		*/
	}
	return false;
}

//

REGISTER_FOR_FAST_CAST(DoorTypeReplacer);
LIBRARY_STORED_DEFINE_TYPE(DoorTypeReplacer, doorTypeReplacer);

DoorTypeReplacer::DoorTypeReplacer()
: LibraryStored(nullptr, LibraryName::invalid())
{
}

DoorTypeReplacer::DoorTypeReplacer(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
{
}

void DoorTypeReplacer::set(For const & _fdt)
{
	for_every(forDoorType, forDoorTypes)
	{
		if (forDoorType->doorType == _fdt.doorType)
		{
			*forDoorType = _fdt;
			return;
		}
	}
	forDoorTypes.push_back(_fdt);
}

bool DoorTypeReplacer::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	priority = _node->get_int_attribute(TXT("priority"), priority);

	for_every(node, _node->children_named(TXT("for")))
	{
		For fdt;
		if (fdt.load_from_xml(node, _lc))
		{
			set(fdt);
		}
		else
		{
			result = false;
		}
	}

	For fdt;
	if (fdt.load_from_xml(_node, _lc))
	{
		set(fdt);
	}
	else
	{
		result = false;
	}

	return result;
}

bool DoorTypeReplacer::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(forDoorType, forDoorTypes)
	{
		result &= forDoorType->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool DoorTypeReplacer::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	for_every(forDoorType, forDoorTypes)
	{
		result &= forDoorType->apply_renamer(_renamer, _library);
	}

	return true;
}

void DoorTypeReplacer::prepare_to_unload()
{
	forDoorTypes.clear();

	base::prepare_to_unload();
}

DoorType* DoorTypeReplacer::process(DoorType * _doorType, DoorTypeReplacerContext const& _context, DoorInRoom const* _aDIR, DoorTypeReplacer const* _a, DoorInRoom const* _bDIR, DoorTypeReplacer const* _b)
{
	DoorType* result = _doorType;

#ifdef AN_INSPECT_DOOR_TYPE_REPLACER
	output(TXT("[DTR]: process for \"%S\""), _doorType->get_name().to_string().to_char());
	output(TXT("[DTR]:   A dr'%p in r%p origr r%p"), _aDIR, _aDIR->get_in_room(), _aDIR->get_in_room()? _aDIR->get_in_room()->get_origin_room() : nullptr);
	output(TXT("[DTR]:   B dr'%p in r%p origr r%p"), _bDIR, _bDIR->get_in_room(), _bDIR->get_in_room()? _bDIR->get_in_room()->get_origin_room() : nullptr);
	{
		Tags tags;
		tags.set_tags_from(_aDIR->get_tags());
		tags.set_tags_from(_aDIR->get_door()->get_tags());
		output(TXT("[DTR]:   dtrA:\"%S\" priority:%i tags:\"%S\""), _a ? _a->get_name().to_string().to_char() : TXT("--"), _a ? _a->priority : 0, tags.to_string().to_char());
	}
	{
		Tags tags;
		tags.set_tags_from(_bDIR->get_tags());
		tags.set_tags_from(_bDIR->get_door()->get_tags());
		output(TXT("[DTR]:   dtrB:\"%S\" priority:%i tags:\"%S\""), _b ? _b->get_name().to_string().to_char() : TXT("--"), _b ? _b->priority : 0, tags.to_string().to_char());
	}
	if (_aDIR->get_door() && _aDIR->get_door()->is_world_separator_door())
	{
		output(TXT("[DTR]:   A is world separator door"));
	}
	if (_bDIR->get_door() && _bDIR->get_door()->is_world_separator_door())
	{
		output(TXT("[DTR]:   B is world separator door"));
	}
#endif

	if (!_doorType->is_replacable_by_door_type_replacer())
	{
#ifdef AN_INSPECT_DOOR_TYPE_REPLACER
		output(TXT("[DTR]: result (not replacable) \"%S\""), _doorType->get_name().to_string().to_char());
#endif
		return _doorType;
	}

	if (!_a && _b)
	{
		swap(_a, _b);
		swap(_aDIR, _bDIR);
	}

	if (_a && _b && _a->priority < _b->priority)
	{
		swap(_a, _b);
		swap(_aDIR, _bDIR);
	}

#ifdef AN_INSPECT_DOOR_TYPE_REPLACER
	output(TXT("[DTR]: pass priority swap"));
	{
		Tags tags;
		tags.set_tags_from(_aDIR->get_tags());
		tags.set_tags_from(_aDIR->get_door()->get_tags());
		output(TXT("[DTR]:   dtrA:\"%S\" priority:%i tags:\"%S\""), _a ? _a->get_name().to_string().to_char() : TXT("--"), _a ? _a->priority : 0, tags.to_string().to_char());
	}
	{
		Tags tags;
		tags.set_tags_from(_bDIR->get_tags());
		tags.set_tags_from(_bDIR->get_door()->get_tags());
		output(TXT("[DTR]:   dtrB:\"%S\" priority:%i tags:\"%S\""), _b ? _b->get_name().to_string().to_char() : TXT("--"), _b ? _b->priority : 0, tags.to_string().to_char());
	}
	{
		Tags tags;
		tags.set_tags_from(_aDIR->get_tags());
		tags.set_tags_from(_bDIR->get_tags());
		tags.set_tags_from(_aDIR->get_door()->get_tags());
		tags.set_tags_from(_bDIR->get_door()->get_tags());
		output(TXT("[DTR]: tags for actual door:\"%S\""), tags.to_string().to_char());
	}
#endif

	while(true)
	{
		For const * fdtA = _a ? _a->find_for(_doorType, _aDIR) : nullptr;
		For const * fdtDefaultA = _a? _a->find_default(_aDIR) : nullptr;
		For const * fdtB = _b ? _b->find_for(_doorType, _bDIR) : nullptr;
		For const * fdtDefaultB = _b ? _b->find_default(_bDIR) : nullptr;

#ifdef AN_INSPECT_DOOR_TYPE_REPLACER
		output(TXT("[DTR]: fdtA \"%S\""), fdtA && fdtA->doorType.get() ? fdtA->doorType->get_name().to_string().to_char() : TXT("--"));
		output(TXT("[DTR]: fdtDefaultA \"%S\""), fdtDefaultA && fdtDefaultA->doorType.get() ? fdtDefaultA->doorType->get_name().to_string().to_char() : TXT("--"));
		output(TXT("[DTR]: fdtB \"%S\""), fdtB && fdtB->doorType.get() ? fdtB->doorType->get_name().to_string().to_char() : TXT("--"));
		output(TXT("[DTR]: fdtDefaultB \"%S\""), fdtDefaultB && fdtDefaultB->doorType.get() ? fdtDefaultB->doorType->get_name().to_string().to_char() : TXT("--"));
#endif

		// fdtA -> fdtB
		// fdtB -> fdtA
		// fdtA -> fdtDefaultB
		// fdtB -> fdtDefaultA
		// fdtDefaultA -> fdtB
		// fdtDefaultB -> fdtA
		// fdtDefaultA -> fdtDefaultB
		// fdtDefaultB -> fdtDefaultA
		// fdtA -> ...
		// fdtB -> ... 
		// fdtDefaultA -> ...
		// fdtDefaultB -> ... 
		if (fdtA && fdtB && fdtA->process(REF_ result, _context, _aDIR, fdtB)) break;
		if (fdtB && fdtA && fdtB->process(REF_ result, _context, _bDIR, fdtA)) break;
		if (fdtA && fdtDefaultB && fdtA->process(REF_ result, _context, _aDIR, fdtDefaultB)) break;
		if (fdtB && fdtDefaultA && fdtB->process(REF_ result, _context, _bDIR, fdtDefaultA)) break;
		if (fdtDefaultA && fdtB && fdtDefaultA->process(REF_ result, _context, _aDIR, fdtB)) break;
		if (fdtDefaultB && fdtA && fdtDefaultB->process(REF_ result, _context, _bDIR, fdtA)) break;
		if (fdtDefaultA && fdtDefaultB && fdtDefaultA->process(REF_ result, _context, _aDIR, fdtDefaultB)) break;
		if (fdtDefaultB && fdtDefaultA && fdtDefaultB->process(REF_ result, _context, _bDIR, fdtDefaultA)) break;

		// in this cases only forced is allowed
		if (fdtA && fdtA->process(REF_ result, _context, _aDIR, nullptr)) break;
		if (fdtB && fdtB->process(REF_ result, _context, _bDIR, nullptr)) break;
		if (fdtDefaultA && fdtDefaultA->process(REF_ result, _context, _aDIR, nullptr)) break;
		if (fdtDefaultB && fdtDefaultB->process(REF_ result, _context, _bDIR, nullptr)) break;

		// in other cases, assume default behaviour, which is disallow
		break;
	}

#ifdef AN_INSPECT_DOOR_TYPE_REPLACER
	output(TXT("[DTR]: result \"%S\""),
		result? result->get_name().to_string().to_char() : TXT("--")
	);
#endif

	return result;
}

DoorTypeReplacer::For const * DoorTypeReplacer::find_for(DoorType* _doorType, DoorInRoom const* _thisDIR) const
{
	for_every(fdt, forDoorTypes)
	{
		if (!fdt->actualDoorTagged.is_empty() && _thisDIR)
		{
			Tags tags;
			tags.set_tags_from(_thisDIR->get_tags());
			if (auto* d = _thisDIR->get_door_on_other_side())
			{
				tags.set_tags_from(d->get_tags());
			}
			tags.set_tags_from(_thisDIR->get_door()->get_tags());
			if (!fdt->actualDoorTagged.check(tags))
			{
				continue;
			}
		}
		bool doorTypeMatches = fdt->doorType.get() == _doorType;
		bool tagsMatch = fdt->tagged.check(_doorType->get_tags()); // works if empty
		if (tagsMatch && doorTypeMatches)
		{
			return fdt;
		}
		if (tagsMatch && ! fdt->doorType.is_set() && ! fdt->tagged.is_empty()) // if we just added tagged but haven't provided door type, should work as well
		{
			return fdt;
		}
	}
	return nullptr;
}

DoorTypeReplacer::For const * DoorTypeReplacer::find_default(DoorInRoom const* _thisDIR) const
{
	for_every(fdt, forDoorTypes)
	{
		if (!fdt->actualDoorTagged.is_empty() && _thisDIR)
		{
			Tags tags;
			tags.set_tags_from(_thisDIR->get_tags());
			if (auto* d = _thisDIR->get_door_on_other_side())
			{
				tags.set_tags_from(d->get_tags());
			}
			tags.set_tags_from(_thisDIR->get_door()->get_tags());
			if (!fdt->actualDoorTagged.check(tags))
			{
				continue;
			}
		}
		if (!fdt->doorType.is_name_valid() &&
			fdt->tagged.is_empty())
		{
			return fdt;
		}
	}
	return nullptr;
}
