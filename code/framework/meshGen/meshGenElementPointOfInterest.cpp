#include "meshGenElementPointOfInterest.h"

#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"

#include "..\appearance\appearanceControllerData.h"

#include "..\world\pointOfInterest.h"

#include "..\..\core\io\xml.h"

using namespace Framework;
using namespace MeshGeneration;

//

DEFINE_STATIC_NAME(throughParam);

//

REGISTER_FOR_FAST_CAST(ElementPointOfInterest);

ElementPointOfInterest::ElementPointOfInterest()
{
}

ElementPointOfInterest::~ElementPointOfInterest()
{
}

bool ElementPointOfInterest::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	if (!poi.is_set())
	{
		poi = new PointOfInterest();
	}

	poiName.load_from_xml(_node, TXT("name"));
	if (poiName.is_set())
	{
		poi->name = NAME(throughParam);
	}

	tagsParam = _node->get_name_attribute_or_from_child(TXT("tagsParam"), tagsParam);

	result &= poi->load_from_xml(_node, _lc, TXT("poiParameters"), TXT("poiParametersFromOwner"));

	return result;
}

bool ElementPointOfInterest::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool ElementPointOfInterest::process(GenerationContext & _context, ElementInstance & _instance) const
{
	if (poi.is_set())
	{
		PointOfInterestPtr newPoi(poi->create_copy());
		if (poiName.is_set())
		{
			newPoi->name = poiName.get(_context);
		}
		if (tagsParam.is_valid())
		{
			if (auto* addTags = _context.get_parameter<Tags>(tagsParam))
			{
				newPoi->tags.set_tags_from(*addTags);
			}
		}
		// copy parameters from context
		newPoi->parameters.do_for_every([newPoi, &_context](SimpleVariableInfo& _info)
		{
			if (_info.was_read_empty())
			{
				if (_context.has_parameter(_info.get_name(), _info.type_id()))
				{
					_context.get_parameter(_info.get_name(), _info.type_id(), _info.access_raw());
					newPoi->parameters.mark_read_empty(_info.get_name(), _info.type_id(), false);
				}
				else
				{
					newPoi->parameters.invalidate(_info.get_name(), _info.type_id());
				}
			}
		});
		_context.access_pois().push_back(newPoi);
	}
	return true;
}
