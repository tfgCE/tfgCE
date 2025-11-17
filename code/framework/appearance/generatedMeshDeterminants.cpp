#include "generatedMeshDeterminants.h"

#include "..\world\doorInRoom.h"
#include "..\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

void GeneratedMeshDeterminants::clear()
{
	determinants.clear();
}

bool GeneratedMeshDeterminants::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	for_every(node, _node->all_children())
	{
		TypeId readingType = RegisteredType::get_type_id_by_name(node->get_name().to_char());
		if (RegisteredType::is_plain_data(readingType) &&
			RegisteredType::get_size_of(readingType) <= sizeof(DeterminantValue))
		{
			Determinant d;
			d.name = node->get_name_attribute(TXT("name"), d.name);
			d.type = readingType;
			error_loading_xml_on_assert(d.name.is_valid(), result, node, TXT("\"name\" required"));
			if (!RegisteredType::load_from_xml(node, readingType, &d.value))
			{
				error_loading_xml(node, TXT("problem loading value for determinant"));
				result = false;
			}

			bool present = false;
			for_every(dt, determinants)
			{
				if (dt->name == d.name &&
					dt->value == d.value)
				{
					dt->value = d.value;
					present = true;
					break;
				}
			}
			if (!present)
			{
				determinants.push_back(d);
			}
		}
		else
		{
			error_dev_ignore(TXT("\"%S\" %S"), RegisteredType::get_name_of(readingType), RegisteredType::is_plain_data(readingType)? TXT("is plain data") : TXT("IS NOT PLAIN DATA"));
			error_dev_ignore(TXT("size %i should be less or equal %i"), RegisteredType::get_size_of(readingType), sizeof(DeterminantValue));
			error_loading_xml(node, TXT("only limited number of types supported"));
			todo_implement;
			result = false;
		}
	}

	return result;
}

bool GeneratedMeshDeterminants::load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName)
{
	bool result = true;

	for_every(node, _node->children_named(_childName))
	{
		result &= load_from_xml(node);
	}

	return result;
}

void GeneratedMeshDeterminants::apply_to(SimpleVariableStorage& _variables) const
{
	for_every(d, determinants)
	{
		RegisteredType::copy(d->type, _variables.access(d->name, d->type), &d->value);
	}
}

void GeneratedMeshDeterminants::gather_for(IModulesOwner const* _imo, GeneratedMeshDeterminants const& _determinantsToGet)
{
	gather_for(_imo, nullptr, _determinantsToGet);
}

void GeneratedMeshDeterminants::gather_for(IModulesOwner const* _imo, SimpleVariableStorage const* _variables, GeneratedMeshDeterminants const& _determinantsToGet)
{
	for_every(d, _determinantsToGet.determinants)
	{
		if (_variables)
		{
			if (auto* v = _variables->get_raw(d->name, d->type))
			{
				set(d->name, d->type, v);
				continue;
			}
		}
		if (auto* v = _imo->get_variables().get_raw(d->name, d->type))
		{
			set(d->name, d->type, v);
			continue;
		}
		// missing altogether
		{
			set(d->name, d->type, &d->value);
			continue;
		}
	}
}

void GeneratedMeshDeterminants::gather_for(DoorInRoom const* _doorInRoom, GeneratedMeshDeterminants const& _determinantsToGet)
{
	for_every(d, _determinantsToGet.determinants)
	{
		if (auto* v = _doorInRoom->get_door()->get_variables().get_raw(d->name, d->type))
		{
			set(d->name, d->type, v);
		}
		else
		{
			DeterminantValue value; // used as void*
			if (_doorInRoom->get_in_room()->restore_value(d->name, d->type, &value))
			{
				set(d->name, d->type, &value);
			}
			else
			{
				set(d->name, d->type, &d->value);
			}
		}
	}
}

void GeneratedMeshDeterminants::set(Name const& _name, TypeId _type, void const * _value)
{
	an_assert(RegisteredType::get_size_of(_type) <= sizeof(DeterminantValue), TXT("\"%S\"'s size is %ib, exceeds allowed %i bytes"), RegisteredType::get_name_of(_type), RegisteredType::get_size_of(_type), sizeof(DeterminantValue::value));

	for_every(d, determinants)
	{
		if (d->name == _name &&
			d->type == _type)
		{
			an_assert(RegisteredType::get_size_of(_type) <= sizeof(d->value));
			RegisteredType::copy(d->type, &d->value, _value);
			return;
		}
	}

	determinants.grow_size(1);
	auto& l = determinants.get_last();
	l.name = _name;
	l.type = _type;
	an_assert(RegisteredType::get_size_of(_type) <= sizeof(l.value));
	RegisteredType::copy(l.type, &l.value, _value);
}

bool GeneratedMeshDeterminants::operator==(GeneratedMeshDeterminants const& _other) const
{
	if (determinants.is_empty() && _other.determinants.is_empty())
	{
		return true;
	}
	bool anyMatches = false;
	bool anyMismatches = false;
	for_every(d, determinants)
	{
		bool matches = false;
		// find if any of _other matches this one
		for_every(od, _other.determinants)
		{
			if (d->name == od->name &&
				d->type == od->type)
			{
				matches = memory_compare(&d->value, &od->value, RegisteredType::get_size_of(d->type));
				break;
			}
		}
		anyMatches |= matches;
		anyMismatches |= !matches;
		if (anyMismatches)
		{
			return false;
		}
	}
	return anyMatches && !anyMismatches; // all determinants should be present but in any case they're not, better to make sure this is fulfilled
}
