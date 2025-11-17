#include "moduleAppearanceData.h"

#include "..\appearance\appearanceControllerData.h"
#include "..\appearance\appearanceControllers.h"

#include "..\library\usedLibraryStored.inl"
#include "..\library\library.h"

#include "..\..\core\system\core.h"
#include "..\..\core\utils\utils_loadingForSystemShader.h"

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(ModuleAppearanceData);

ModuleAppearanceData::ModuleAppearanceData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

ModuleAppearanceData::~ModuleAppearanceData()
{
}

bool ModuleAppearanceData::read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc)
{
	if (_attr->get_name() == TXT("tags"))
	{
		tags.load_from_string(_attr->get_as_string());
		return true;
	}
	else if (_attr->get_name() == TXT("name"))
	{
		name = _attr->get_as_name();
		return true;
	}
	else if (_attr->get_name() == TXT("syncTo"))
	{
		syncTo = _attr->get_as_name();
		useControllers = false;
		return true;
	}
	else if (_attr->get_name() == TXT("useControllers"))
	{
		useControllers = _attr->get_as_bool();
		return true;
	}
	else
	{
		return base::read_parameter_from(_attr, _lc);
	}
}

bool ModuleAppearanceData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("useControllers"))
	{
		useControllers = true;
		return true;
	}
	else if (_node->get_name() == TXT("dontUseControllers"))
	{
		useControllers = false;
		return true;
	}
	else if (_node->get_name() == TXT("copyMaterialParameters"))
	{
		bool result = true;
		copyMaterialParametersOnce = _node->get_bool_attribute_or_from_child_presence(TXT("once"), copyMaterialParametersOnce);
		for_every(node, _node->children_named(TXT("copy")))
		{
			Name var = node->get_name_attribute(TXT("var"));
			if (var.is_valid())
			{
				copyMaterialParameters.push_back(var);
			}
			else
			{
				error_loading_xml(_node, TXT("no \"var\" attribute"));
				result = false;
			}
		}
		return result;
	}
	else if (_node->get_name() == TXT("animationSet"))
	{
		bool result = true;
		UsedLibraryStored<AnimationSet> animationSet;
		if (animationSet.load_from_xml(_node, TXT("name"), _lc))
		{
			animationSets.push_back(animationSet);
		}
		else
		{
			result = false;
		}
		error_loading_xml_on_assert(_node->has_attribute(TXT("name")), result, _node, TXT("requires \"name\" for animation set"));
		return result;
	}
	else if (_node->get_name() == TXT("mesh"))
	{
		bool result = true;
		UsedLibraryStored<Mesh> mesh;
		if (mesh.load_from_xml(_node, TXT("name"), _lc))
		{
			meshes.push_back(mesh);
		}
		else
		{
			result = false;
		}
		error_loading_xml_on_assert(_node->has_attribute(TXT("name")) || _node->has_attribute(TXT("generator")) || _node->has_attribute(TXT("depot")), result, _node, TXT("requires \"name\", \"generator\" or \"depot\" for mesh"));
		result &= meshGenerator.load_from_xml(_node, TXT("generator"), _lc);
		result &= meshDepot.load_from_xml(_node, TXT("depot"), _lc);
		return result;
	}
	else if (_node->get_name() == TXT("materials"))
	{
		bool result = true;
		materialSetups.clear();
		for_every(materialNode, _node->children_named(TXT("material")))
		{
			MaterialSetup ms;
			result &= ms.load_from_xml(materialNode, _lc);
			materialSetups.push_back(ms);
		}
		return result;
	}
	else if (_node->get_name() == TXT("skeleton"))
	{
		bool result = true;
		result &= skeleton.load_from_xml(_node, TXT("name"), _lc);
		result &= skelGenerator.load_from_xml(_node, TXT("generator"), _lc);
		return result;
	}
	else if (_node->get_name() == TXT("generatorVariables"))
	{
		bool result = true;
		result &= generatorVariables.load_from_xml(_node);
		return result;
	}
	else if (_node->get_name() == TXT("controllers"))
	{
		for_every(childNode, _node->all_children())
		{
			if (AppearanceControllerData* newController = AppearanceControllers::create_data(Name(childNode->get_name())))
			{
				bool result = false;
				if (newController->load_from_xml(childNode, _lc))
				{
					result = true;
					for_every(forNode, childNode->children_named(TXT("for")))
					{
						if (CoreUtils::Loading::should_load_for_system_or_shader_option(forNode))
						{
							result &= newController->load_from_xml(forNode, _lc);
						}
					}
					if (result)
					{
						controllers.push_back(AppearanceControllerDataPtr(newController));
					}
				}
				if (!result)
				{
					return result;
				}
			}
			else
			{
				error_loading_xml(childNode, TXT("appearance controller data \"%S\" not recognised"), childNode->get_name().to_char());
				return false;
			}
		}
		return true;
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

bool ModuleAppearanceData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(animationSet, animationSets)
	{
		result &= animationSet->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	for_every(mesh, meshes)
	{
		result &= mesh->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	if (meshGenerator.is_name_valid())
	{
		result &= meshGenerator.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	if (meshDepot.is_name_valid())
	{
		result &= meshDepot.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	if (skeleton.is_name_valid())
	{
		result &= skeleton.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	if (skelGenerator.is_name_valid())
	{
		result &= skelGenerator.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	for_every(materialSetup, materialSetups)
	{
		result &= materialSetup->prepare_for_game(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	for_every_ref(controller, controllers)
	{
		result &= controller->prepare_for_game(_library, _pfgContext);
	}

	return result;
}
