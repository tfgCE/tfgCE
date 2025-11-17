#include "gsc_compareCellCoord.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\..\core\io\xml.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool CompareCellCoord::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	varA = _node->get_name_attribute(TXT("a"), varA);
	varB = _node->get_name_attribute(TXT("b"), varB);

	error_loading_xml_on_assert(varA.is_valid(), result, _node, TXT("no \"a\" provided"));
	error_loading_xml_on_assert(varB.is_valid(), result, _node, TXT("no \"b\" provided"));

	aOffset.load_from_xml_child_node(_node, TXT("aOffset"));
	bOffset.load_from_xml_child_node(_node, TXT("bOffset"));

	{
		String cf = _node->get_string_attribute(TXT("compare"));
		if (cf.is_empty()) compareFunc = CompareFunc::Equal; else
		if (cf == TXT("equal")) compareFunc = CompareFunc::Equal; else
		if (cf == TXT("a.y greater than b.y")) compareFunc = CompareFunc::AYgreaterBY; else
		if (cf == TXT("a.y less than b.y")) compareFunc = CompareFunc::AYlessBY; else
		{
			error_loading_xml(_node, TXT("compare function \"%S\" not recognised"), cf.to_char());
			result = false;
		}
	}

	return result;
}

bool CompareCellCoord::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool result = false;

	auto* viA = _execution.get_variables().find_existing<VectorInt2>(varA);
	auto* viB = _execution.get_variables().find_existing<VectorInt2>(varB);

	if (viA && viB)
	{
		VectorInt2 a = viA->get<VectorInt2>();
		VectorInt2 b = viB->get<VectorInt2>();
		a += aOffset;
		b += bOffset;
		if (compareFunc == CompareFunc::Equal) result = a == b;
		if (compareFunc == CompareFunc::AYgreaterBY) result = a.y > b.y;
		if (compareFunc == CompareFunc::AYlessBY) result = a.y < b.y;
	}

	return result;
}
