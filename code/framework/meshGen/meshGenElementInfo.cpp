#include "meshGenElementInfo.h"

#include "meshGenGenerationContext.h"
#include "meshGenerator.h"

using namespace Framework;
using namespace MeshGeneration;

//

DEFINE_STATIC_NAME(start);
DEFINE_STATIC_NAME(end);

//

REGISTER_FOR_FAST_CAST(ElementInfo);

ElementInfo::~ElementInfo()
{
}

bool ElementInfo::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	from.load_from_xml_child_node(_node, TXT("from"));
	to.load_from_xml_child_node(_node, TXT("to"));

	result &= fromWMP.load_from_xml(_node->first_child_named(TXT("fromWMP")));
	result &= toWMP.load_from_xml(_node->first_child_named(TXT("toWMP")));

	info = _node->get_string_attribute_or_from_child(TXT("info"), info);
	infoValueParam = _node->get_name_attribute_or_from_child(TXT("infoValueParam"), infoValueParam);

	colour.load_from_xml(_node, TXT("colour"));
	
	return result;
}

bool ElementInfo::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool ElementInfo::process(GenerationContext & _context, ElementInstance & _instance) const
{
	bool result = true;

#ifndef AN_DEVELOPMENT
	return result;
#else
	if (!MeshGenerator::does_gather_generation_info())
	{
		return result;
	}

	Vector3 useFrom = Vector3::zero;
	Vector3 useTo = Vector3::zero;

	if (!fromWMP.is_empty())
	{
		WheresMyPoint::Value tempValue;
		WheresMyPoint::Context context(&_instance);
		context.set_random_generator(_instance.access_context().access_random_generator());
		if (!fromWMP.update(tempValue, context))
		{
			error_generating_mesh(_instance, TXT("error processing fromWMP"));
			return false;
		}
		if (!tempValue.is_set())
		{
			error_generating_mesh(_instance, TXT("fromWMP result invalid"));
			return false;
		}
		if (tempValue.get_type() != type_id<Vector3>())
		{
			error_generating_mesh(_instance, TXT("fromWMP result invalid (not Vector3)"));
			return false;
		}
		useFrom = tempValue.get_as<Vector3>();
	}
	else if (from.is_set())
	{
		useFrom = from.get(_context);
	}

	if (!toWMP.is_empty())
	{
		WheresMyPoint::Value tempValue;
		WheresMyPoint::Context context(&_instance);
		context.set_random_generator(_instance.access_context().access_random_generator());
		if (!toWMP.update(tempValue, context))
		{
			error_generating_mesh(_instance, TXT("error processing toWMP"));
			return false;
		}
		if (!tempValue.is_set())
		{
			error_generating_mesh(_instance, TXT("toWMP result invalid"));
			return false;
		}
		if (tempValue.get_type() != type_id<Vector3>())
		{
			error_generating_mesh(_instance, TXT("toWMP result invalid (not Vector3)"));
			return false;
		}
		useTo = tempValue.get_as<Vector3>();
	}
	else if (to.is_set())
	{
		useTo = to.get(_context);
	}

	an_assert(_context.generationInfo);

	String text = info;
	if (infoValueParam.is_valid())
	{
		WheresMyPoint::Value tempValue;
		if (_context.restore_value_for_wheres_my_point(infoValueParam, tempValue))
		{
			if (! tempValue.is_set())
			{
				error_generating_mesh(_instance, TXT("infoValueParam - no valid value"));
				return false;
			}
			text += TXT(" = ");
			if (tempValue.get_type() == type_id<float>())
			{
				text += String::printf(TXT("%.3f"), tempValue.get_as<float>());
			}
			else
			{
				text += TXT("??");
				todo_important(TXT("implement"));
			}
		}
		else
		{
			error_generating_mesh(_instance, TXT("infoValueParam - no value"));
			return false;
		}
	}
	_context.generationInfo->add_info(useFrom, useTo, info, colour);

	return result;
#endif
}
