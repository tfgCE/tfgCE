#include "createFromTemplate.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

using namespace Framework;

REGISTER_FOR_FAST_CAST(Framework::CreateFromTemplate);
LIBRARY_STORED_DEFINE_TYPE(CreateFromTemplate, createFromTemplate);

CreateFromTemplate::CreateFromTemplate(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

CreateFromTemplate::~CreateFromTemplate()
{
}

bool CreateFromTemplate::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= fromTemplate.load_from_xml(_node, TXT("template"), _lc);

	result &= renamer.load_from_xml_child_node(_node, _lc);

	result &= meshGeneratorParameters.load_from_xml_child_node(_node, TXT("meshGeneratorParameters"));
	_lc.load_group_into(meshGeneratorParameters);

	return result;
}

bool CreateFromTemplate::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::CreateFromTemplates)
	{
		if (!fromTemplate.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::CreateFromTemplates))
		{
			error(TXT("could not find template \"%S\""), fromTemplate.get_name().to_string().to_char());
			result = false;
		}
		else
		{
			if (!created)
			{
				for_every(object, fromTemplate->objects)
				{
					object->get()->create_from_template(_library, *this);
				}
				created = true;
			}
		}
	}
	return result;
}
