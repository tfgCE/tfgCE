#include "navFlags.h"

#include "..\..\core\io\xml.h"

#include "..\..\core\containers\list.h"

//

DEFINE_STATIC_NAME(default);
DEFINE_STATIC_NAME(none);
DEFINE_STATIC_NAME(all);

//

using namespace Framework;
using namespace Nav;

Flags Flags::s_none;
Flags Flags::s_default;
Flags Flags::s_all;

Flags Flags::of_index(int _index)
{
	Flags flags;
	flags.flags = 1 << _index;
	return flags;
}

bool Flags::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}

	bool result = true;

	*this = Flags::none();

	result &= apply(_node->get_text());

	return result;
}

bool Flags::load_from_xml(IO::XML::Node const * _node, tchar const * _attrOrChildName)
{
	if (!_node)
	{
		return false;
	}

	bool result = true;

	if (auto* attr = _node->get_attribute(_attrOrChildName))
	{
		*this = Flags::none();

		result &= apply(attr->get_as_string());
	}
	else if (auto * node = _node->first_child_named(_attrOrChildName))
	{
		*this = Flags::none();

		result &= apply(node->get_text());
	}

	return result;
}

String Flags::to_string() const
{
	String result;
	Name maskName = DefinedFlags::get_mask_name(flags);
	if (maskName.is_valid())
	{
		result = result + String::printf(TXT("[%S]"), maskName.to_char());
	}
	uint flag = 1;
	int bitsLeft = 32;
	while (flag <= flags && bitsLeft > 0)
	{
		Name flagName = DefinedFlags::get_flag_name(flag);
		if (flagName.is_valid() &&
			is_flag_set(flags, flag))
		{
			if (!result.is_empty())
			{
				result = result + String::space();
			}
			result += flagName.to_string();
		}
		flag = flag << 1;
		--bitsLeft;
	}
	return result;
}

bool Flags::apply(String const & _string)
{
	bool result = true;

	if (_string.is_empty())
	{
		// nothing to apply, skip it
		return result;
	}

	List<String> tokens;
	_string.split(String::space(), tokens);

	String negativePrefix = String(TXT("-"));
	String positivePrefix = String(TXT("+"));

	bool thereIsOneWithoutPrefix = false;
	for_every(token, tokens)
	{
		bool negativeHas = token->has_prefix(negativePrefix);
		bool positiveHas = token->has_prefix(positivePrefix);
		if (!negativeHas && !positiveHas)
		{
			thereIsOneWithoutPrefix = true;
		}
	}

	if (thereIsOneWithoutPrefix)
	{
		clear();
	}
	
	for_every(token, tokens)
	{
		bool negativeHas = token->has_prefix(negativePrefix);
		bool positiveHas = token->has_prefix(positivePrefix);
		String withoutPrefix = negativeHas || positiveHas ? token->get_sub(1) : *token;
		if (withoutPrefix == TXT("all"))
		{
			if (!negativeHas)
			{
				*this = *this | Flags::all();
			}
			else
			{
				*this = *this & (-Flags::all());
			}
		}
		else if (withoutPrefix == TXT("default"))
		{
			if (!negativeHas)
			{
				*this = *this | Flags::defaults();
			}
			else
			{
				*this = *this & (-Flags::defaults());
			}
		}
		else if (withoutPrefix == TXT("clear") ||
				 withoutPrefix == TXT("none") || 
				 withoutPrefix == TXT("clean"))
		{
			if (negativeHas)
			{
				error(TXT("doesn't make much sense to apply negative clear/none/clean"));
			}
			else
			{
				clear();
			}
		}
		else if (! withoutPrefix.is_empty())
		{
			Flags readFlag = DefinedFlags::get_existing_or_basic(Name(withoutPrefix));
			if (!negativeHas)
			{
				*this = *this | readFlag;
			}
			else
			{
				*this = *this & (-readFlag);
			}
		}
	}

	update_for_nat_vis();

	return result;
}

Flags & Flags::clear()
{
	*this = Flags::none();
	update_for_nat_vis();
	return *this;
}

Flags & Flags::add(Name const & _flag)
{
	*this = *this | DefinedFlags::get_existing_or_basic(_flag);
	update_for_nat_vis();
	return *this;
}

Flags & Flags::remove(Name const & _flag)
{
	*this = *this & (-DefinedFlags::get_existing_or_basic(_flag));
	update_for_nat_vis();
	return *this;
}

#ifdef AN_NAT_VIS
void Flags::update_for_nat_vis()
{
	natVis = to_string();
	if (natVis.is_empty())
	{
		natVis = TXT("--");
	}
}
#endif

//

DefinedFlags* DefinedFlags::s_flags = nullptr;

DefinedFlags::DefinedFlags()
: basicFlagsDefinedCount(0)
{
}

Flags const & DefinedFlags::get_existing_or_basic(Name const & _name)
{
	an_assert(_name != TXT("all"));
	an_assert(_name != TXT("clear"));
	an_assert(_name != TXT("none"));
	an_assert(_name != TXT("clean"));
	an_assert(s_flags != nullptr);
	if (DefinedFlag const * definedFlag = s_flags->flags.get_existing(_name))
	{
		return definedFlag->flags;
	}

	DefinedFlag definedFlag;
	definedFlag.name = _name;
	definedFlag.flags = Flags::of_index(s_flags->basicFlagsDefinedCount); // this here should be changed to unique s_flags->basicFlagsDefinedCount
	s_flags->flags[_name] = definedFlag;
	++s_flags->basicFlagsDefinedCount;
	return s_flags->flags[_name].flags;
}

bool DefinedFlags::load_masks_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("mask")))
	{
		result &= load_mask_from_xml(node);
	}

	// cache
	cache_and_update_built_ins();

	return result;
}

bool DefinedFlags::load_mask_from_xml(IO::XML::Node const * _node)
{
	an_assert(s_flags != nullptr);
	Name name = _node->get_name_attribute(TXT("name"));
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("no name provided for collision mask (defined flags)"));
		return false;
	}

	DefinedFlag * definedFlag = s_flags->flags.get_existing(name);
	if (!definedFlag)
	{
		DefinedFlag newDefinedFlag;
		newDefinedFlag.name = name;
		newDefinedFlag.flags.clear();
		newDefinedFlag.isMask = true;
		s_flags->flags[name] = newDefinedFlag;
		definedFlag = &s_flags->flags[name];
	}

	return definedFlag->flags.load_from_xml(_node);
}

void DefinedFlags::initialise_static()
{
	an_assert(s_flags == nullptr);
	s_flags = new DefinedFlags();
	Flags::s_none.flags = 0;
	Flags::s_all.flags = 0xffffffff;
}

void DefinedFlags::close_static()
{
	an_assert(s_flags != nullptr);
	delete_and_clear(s_flags);
}

Name DefinedFlags::get_flag_name(int const & _flags)
{
	an_assert(s_flags != nullptr);
	Name bestFit;
	for_every(flags, s_flags->flags)
	{
		if (flags->flags.flags == _flags &&
			! flags->isMask)
		{
			return flags->name;
		}
		if (flags->flags.flags & _flags)
		{
			bestFit = flags->name;
		}
	}
	return bestFit;
}

Name const & DefinedFlags::get_mask_name(int const & _flags)
{
	an_assert(s_flags != nullptr);
	for_every(flags, s_flags->flags)
	{
		if (flags->isMask &&
			flags->flags.flags == _flags)
		{
			return flags->name;
		}
	}
	return Name::invalid();
}

bool DefinedFlags::has(Name const & _name)
{
	an_assert(s_flags != nullptr);
	return s_flags->flags.has_key(_name);
}

#define HAS_MASK(_name) \
	if (! has(Name(String(_name)))) \
	{ \
		error(TXT("there is no collision mask \"%S\" defined in settings!"), _name); \
		result = false; \
	}

bool DefinedFlags::check_and_cache()
{
	bool result = true;

	cache_and_update_built_ins();

	return result;
}

#define UPDATE_DEFINED_FLAG(mask) \
	DefinedFlag mask##Flags; \
	mask##Flags.isMask = true; \
	mask##Flags.flags = Flags::s_##mask; \
	mask##Flags.name = NAME(mask); \
	s_flags->flags[NAME(mask)] = mask##Flags;

void DefinedFlags::cache_and_update_built_ins()
{
	Flags::s_default = Flags::s_all;

	UPDATE_DEFINED_FLAG(default);
	UPDATE_DEFINED_FLAG(none);
	UPDATE_DEFINED_FLAG(all);
}
