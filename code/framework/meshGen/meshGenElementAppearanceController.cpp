#include "meshGenElementAppearanceController.h"

#include "meshGenGenerationContext.h"

#include "meshGenUtils.h"

#include "..\appearance\appearanceController.h"
#include "..\appearance\appearanceControllerData.h"
#include "..\appearance\appearanceControllers.h"

#include "..\..\core\io\xml.h"
#include "..\..\core\mesh\socketDefinitionSet.h"

using namespace Framework;
using namespace MeshGeneration;

//

REGISTER_FOR_FAST_CAST(ElementAppearanceController);

ElementAppearanceController::ElementAppearanceController()
{
}

ElementAppearanceController::~ElementAppearanceController()
{
	delete_and_clear(controller);
}

bool ElementAppearanceController::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("controller")))
	{
		if (!node->has_children())
		{
			warn_loading_xml(node, TXT("no controller provided"));
		}
		for_every(child, node->all_children())
		{
			if (controller)
			{
				error_loading_xml(child, TXT("appearance controller already loaded"));
				result = false;
				break;
			}
			if (AppearanceControllerData* newController = AppearanceControllers::create_data(Name(child->get_name())))
			{
				if (newController->load_from_xml(child, _lc))
				{
					controller = newController;
				}
				else
				{
					result = false;
					break;
				}
			}
			else
			{
				error_loading_xml(node, TXT("appearance controller \"%S\" not recognised"), child->get_name().to_char());
				result = false;
			}
		}
	}

	return result;
}

bool ElementAppearanceController::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	if (controller)
	{
		result &= controller->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool ElementAppearanceController::process(GenerationContext & _context, ElementInstance & _instance) const
{
	if (controller)
	{
		if (_context.does_require_appearance_controllers())
		{
			AppearanceControllerData* ac = controller->create_copy();
			_context.access_appearance_controllers().push_back(AppearanceControllerDataPtr(ac));
			ac->apply_mesh_gen_params(_context);
		}
		return true;
	}
	else
	{
		error(TXT("no appearance controller!"));
		return false;
	}
}
