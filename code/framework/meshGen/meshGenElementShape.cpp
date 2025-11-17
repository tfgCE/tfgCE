#include "meshGenElementShape.h"

#include "..\..\core\io\xml.h"

using namespace Framework;
using namespace MeshGeneration;

namespace Framework
{
	namespace MeshGeneration
	{
		namespace BasicShapes
		{
			void add_shapes();
		};

		namespace AdvancedShapes
		{
			void add_shapes();
		};
	};
};

//

REGISTER_FOR_FAST_CAST(ElementShapeData);

ElementShapeData::~ElementShapeData()
{
}

bool ElementShapeData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	return true;
}

bool ElementShapeData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	return true;
}

//

REGISTER_FOR_FAST_CAST(ElementShape);

ElementShape::~ElementShape()
{
	delete_and_clear(data);
}

bool ElementShape::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	shape = _node->get_name_attribute(TXT("type"), shape);
	if (RegisteredElementShape const * registeredElementShape = RegisteredElementShapes::find(shape))
	{
		generate_shape = registeredElementShape->generate_shape;
		if (registeredElementShape->create_data)
		{
			an_assert(data == nullptr);
			data = registeredElementShape->create_data();
			data->set_element(this);
			result &= data->load_from_xml(_node, _lc);
		}
	}
	else
	{
		error_loading_xml(_node, TXT("could not recognise shape \"%S\""), shape.to_char());
	}

	return result;
}

bool ElementShape::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	if (data)
	{
		result &= data->prepare_for_game(_library, _pfgContext);
	}
	
	return result;
}

bool ElementShape::process(GenerationContext & _context, ElementInstance & _instance) const
{
	if (generate_shape)
	{
		return generate_shape(_context, _instance, data);
	}
	else
	{
		error_generating_mesh(_instance, TXT("could not generate shape, no function available"));
		return false;
	}
}

#ifdef AN_DEVELOPMENT
void ElementShape::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	if (data)
	{
		data->for_included_mesh_generators(_do);
	}
}
#endif

//

RegisteredElementShapes* RegisteredElementShapes::s_shapes = nullptr;

void RegisteredElementShapes::initialise_static()
{
	an_assert(s_shapes == nullptr);
	s_shapes = new RegisteredElementShapes();

	// add default shapes
	BasicShapes::add_shapes();
	AdvancedShapes::add_shapes();
}

void RegisteredElementShapes::close_static()
{
	an_assert(s_shapes != nullptr);
	delete_and_clear(s_shapes);
}

RegisteredElementShape const * RegisteredElementShapes::find(Name const & _shape)
{
	an_assert(s_shapes);
	for_every_const(shape, s_shapes->shapes)
	{
		if (shape->name == _shape)
		{
			return shape;
		}
	}
	error(TXT("could not find shape named \"%S\""), _shape.to_char());
	return nullptr;
}

void RegisteredElementShapes::add(RegisteredElementShape & _shape)
{
	an_assert(s_shapes);
	s_shapes->shapes.push_back(_shape);
}
