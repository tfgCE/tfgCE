#include "worldAddress.h"

#include "region.h"
#include "room.h"

#include "..\..\core\other\parserUtils.h"

//

using namespace Framework;

//

void WorldAddress::build_for(Room const* _room)
{
	address.clear();
	address.push_back(_room->get_world_address_id());
	auto* r = _room->get_in_region();
	while (r)
	{
		address.push_back(r->get_world_address_id());
		r = r->get_in_region();
	}
}

void WorldAddress::build_for(Region const* _region)
{
	address.clear();
	auto* r = _region;
	while (r)
	{
		address.push_back(r->get_world_address_id());
		r = r->get_in_region();
	}
}

bool WorldAddress::parse(String const& _string)
{
	address.clear();
	List<String> tokens;
	_string.split(String(TXT(":")), tokens);
	for_every(t, tokens)
	{
		if (!address.has_place_left())
		{
			error(TXT("too many elements to world address"));
			return false;
		}
		address.push_back(ParserUtils::hex_to_int(*t));
	}

	return true;
}

String WorldAddress::to_string() const
{
	String o;
	for_every(a, address)
	{
		if (!o.is_empty())
		{
			o += TXT(":");
		}
		o += ParserUtils::int_to_hex(*a);
	}
	return o;
}
