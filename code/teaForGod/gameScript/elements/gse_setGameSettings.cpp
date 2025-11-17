#include "gse_setGameSettings.h"

#include "..\..\teaEnums.h"

#include "..\..\game\gameSettings.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

template <typename EnumStruct, typename EnumStructType>
static EnumStructType parse_using_enum_MAX(String const& _value, EnumStructType _default)
{
	for_count(int, i, EnumStructType::MAX)
	{
		if (_value == EnumStruct::to_char((EnumStructType)i))
		{
			return (EnumStructType)i;
		}
	}
	return _default;
}

#define LOAD_ENUM(type, name, defaultValue) \
	if (_node->get_attribute(TXT(#name))) \
	{ \
		name = parse_using_enum_MAX<type, type::Type>(_node->get_string_attribute(TXT(#name)), defaultValue); \
	}

//

bool SetGameSettings::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	playerImmortal.load_from_xml(_node, TXT("playerImmortal"));
	weaponInfinite.load_from_xml(_node, TXT("weaponInfinite"));

	return result;
}

#define APPLY_DIFFICULTY(what) if (what.is_set()) gs.difficulty.what = what.get();

Framework::GameScript::ScriptExecutionResult::Type SetGameSettings::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	auto& gs = GameSettings::get();

	APPLY_DIFFICULTY(playerImmortal);
	APPLY_DIFFICULTY(weaponInfinite);

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
