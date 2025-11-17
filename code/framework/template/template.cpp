#include "template.h"

#include "..\library\library.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(Framework::Template);
LIBRARY_STORED_DEFINE_TYPE(Template, template);

Template::Template(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

Template::~Template()
{
}

bool Template::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(objectsNode, _node->children_named(TXT("objects")))
	{
		for_every(node, objectsNode->all_children())
		{
			RefCountObjectPtr<LibraryStored> object;
			bool res = Library::create_standalone_object_from_xml(node, _lc, OUT_ object);
			if (object.is_set())
			{
				if (res)
				{
					if (fast_cast<Template>(object.get()) ||
						fast_cast<CreateFromTemplate>(object.get()))
					{
						// if this is to change, remember that newly created template and create from template may be not prepared as we first go through library stores and execute jobs
						error_loading_xml(node, TXT("can't load \"%S\" into template"), node->get_name().to_char());
					}
					else
					{
						objects.push_back(object);
					}
				}
				else
				{
					error_loading_xml(node, TXT("error loading node \"%S\" (part of a template)"), node->get_name().to_char());
					result = false;
				}
			}
			else
			{
				error_loading_xml(node, TXT("node \"%S\" not recognised as an object of a template"), node->get_name().to_char());
				result = false;
			}
		}
	}

	return result;
}

bool Template::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	// no preparation here!
	// if we want to create something with a template, we should use CreateFromTemplate
	return base::prepare_for_game(_library, _pfgContext);
}

void Template::prepare_to_unload()
{
	base::prepare_to_unload();

	for_every(object, objects)
	{
		object->get()->prepare_to_unload();
	}
}
