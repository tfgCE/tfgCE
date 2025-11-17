#include "meshGenElementModifier.h"

#include "meshGenGenerationContext.h"

#include "..\..\core\io\xml.h"

using namespace Framework;
using namespace MeshGeneration;

namespace Framework
{
	namespace MeshGeneration
	{
		namespace Modifiers
		{
			void add_modifiers();
		};
	};
};

//

REGISTER_FOR_FAST_CAST(ElementModifierData);

//

REGISTER_FOR_FAST_CAST(ElementModifier);

ElementModifier::~ElementModifier()
{
	delete_and_clear(data);
}

bool ElementModifier::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	modifier = _node->get_name_attribute(TXT("type"), modifier);
	if (RegisteredElementModifier const * registeredElementModifier = RegisteredElementModifiers::find(modifier))
	{
		updateWMPPhase = registeredElementModifier->updateWMPPhase;
		process_modifier = registeredElementModifier->process_modifier;
		if (registeredElementModifier->create_data)
		{
			an_assert(data == nullptr);
			data = registeredElementModifier->create_data();
			result &= data->load_from_xml(_node, _lc);
		}
	}
	else
	{
		error_loading_xml(_node, TXT("could not recognise modifier \"%S\""), modifier.to_char());
	}

	if (_node->first_child_named(TXT("element")))
	{
		result &= MeshGeneration::Element::load_single_element_from_xml(_node, _lc, TXT("element"), element);
	}

	if (_node->first_child_named(TXT("elementOnResult")))
	{
		result &= MeshGeneration::Element::load_single_element_from_xml(_node, _lc, TXT("elementOnResult"), elementOnResult);
		if (elementOnResult.is_set())
		{
			elementOnResult->skip_using_stack();
			elementOnResult->skip_using_checkpoint();
		}
	}

	if (!element.is_set())
	{
		// keep stuff at the same level
		useStack = false;
	}

	return result;
}

bool ElementModifier::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	if (element.is_set())
	{
		result &= element->prepare_for_game(_library, _pfgContext);
	}

	if (elementOnResult.is_set())
	{
		result &= elementOnResult->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool ElementModifier::process(GenerationContext & _context, ElementInstance & _instance) const
{
	if (process_modifier)
	{
		Checkpoint checkpoint(_context);
		bool result = process_modifier(_context, _instance, element.get(), data);
		if (elementOnResult.is_set())
		{
			_context.push_stack();
			_context.push_checkpoint(_context);
			result &= _context.process(elementOnResult.get(), &_instance);
			_context.pop_checkpoint();
			_context.pop_stack();
		}
		return result;
	}
	else
	{
		error_generating_mesh(_instance, TXT("could not process modifier or no element associated, no function available"));
		return false;
	}
}

#ifdef AN_DEVELOPMENT
void ElementModifier::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	if (element.is_set())
	{
		element->for_included_mesh_generators(_do);
	}
	if (elementOnResult.is_set())
	{
		elementOnResult->for_included_mesh_generators(_do);
	}
}
#endif

//

RegisteredElementModifiers* RegisteredElementModifiers::s_modifiers = nullptr;

void RegisteredElementModifiers::initialise_static()
{
	an_assert(s_modifiers == nullptr);
	s_modifiers = new RegisteredElementModifiers();

	Modifiers::add_modifiers();
}

void RegisteredElementModifiers::close_static()
{
	an_assert(s_modifiers != nullptr);
	delete_and_clear(s_modifiers);
}

RegisteredElementModifier const * RegisteredElementModifiers::find(Name const & _modifier)
{
	an_assert(s_modifiers);
	for_every_const(modifier, s_modifiers->modifiers)
	{
		if (modifier->name == _modifier)
		{
			return modifier;
		}
	}
	error(TXT("could not find modifier named \"%S\""), _modifier.to_char());
	return nullptr;
}

void RegisteredElementModifiers::add(RegisteredElementModifier & _modifier)
{
	an_assert(s_modifiers);
	s_modifiers->modifiers.push_back(_modifier);
}
