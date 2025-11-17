#include "moduleCollisionData.h"

#include "..\library\usedLibraryStored.inl"

#include "..\library\library.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(ModuleCollisionData);

ModuleCollisionData::ModuleCollisionData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleCollisionData::read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc)
{
	if (_attr->get_name() == TXT("model") ||
		_attr->get_name() == TXT("movementModel"))
	{
		return movementCollisionModel.load_from_xml(_attr, _lc);
	}
	if (_attr->get_name() == TXT("preciseModel"))
	{
		return preciseCollisionModel.load_from_xml(_attr, _lc);
	}
	if (_attr->get_name() == TXT("material") ||
		_attr->get_name() == TXT("movementMaterial"))
	{
		return movementCollisionMaterial.load_from_xml(_attr, _lc);
	}
	if (_attr->get_name() == TXT("preciseMaterial"))
	{
		return preciseCollisionMaterial.load_from_xml(_attr, _lc);
	}
	return base::read_parameter_from(_attr, _lc);
}

bool ModuleCollisionData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("collision") ||
		_node->get_name() == TXT("movementCollision"))
	{
		return movementCollisionModel.load_from_xml(_node, TXT("model"), _lc);
	}
	if (_node->get_name() == TXT("preciseCollision"))
	{
		return preciseCollisionModel.load_from_xml(_node, TXT("model"), _lc);
	}
	if (_node->get_name() == TXT("material") ||
		_node->get_name() == TXT("movementMaterial"))
	{
		return movementCollisionMaterial.load_from_xml(_node, TXT("material"), _lc);
	}
	if (_node->get_name() == TXT("preciseMaterial"))
	{
		return preciseCollisionMaterial.load_from_xml(_node, TXT("material"), _lc);
	}
	return base::read_parameter_from(_node, _lc);
}

bool ModuleCollisionData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result = movementCollisionModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result = preciseCollisionModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result = movementCollisionMaterial.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result = preciseCollisionMaterial.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	if (movementCollisionModel.is_set() && movementCollisionMaterial.is_set())
	{
		warn(TXT("use either movement collision model %S or movement collision material %S"), movementCollisionModel->get_name().to_string().to_char(), movementCollisionMaterial->get_name().to_string().to_char());
	}
	if (preciseCollisionModel.is_set() && preciseCollisionMaterial.is_set())
	{
		warn(TXT("use either precise collision model %S or precise collision material %S"), preciseCollisionModel->get_name().to_string().to_char(), preciseCollisionMaterial->get_name().to_string().to_char());
	}

	return result;
}
