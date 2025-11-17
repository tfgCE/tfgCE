#pragma once

#include "meshGenElement.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Meshes
{
	class Mesh3D;
	struct Mesh3DPart;
};

#define ELEMENT_SHAPE_GENERATE_FN(_var) bool (*_var)(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
#define ELEMENT_SHAPE_CREATE_DATA_FN(_var) ::Framework::MeshGeneration::ElementShapeData* (*_var)()

#define REGISTER_MESH_GEN_SHAPE(name, generate_shape, create_data) \
	::Framework::MeshGeneration::RegisteredElementShapes::add(::Framework::MeshGeneration::RegisteredElementShape(Name(name), generate_shape, create_data).temp_ref());

namespace Framework
{
	namespace MeshGeneration
	{
		class ElementShapeData
		{
			FAST_CAST_DECLARE(ElementShapeData);
			FAST_CAST_END();
		public:
			virtual ~ElementShapeData();

			void set_element(Element* _element) { element = _element; }
			Element* get_element() const { return element; }

			virtual bool is_stateless() const { return true; }

			virtual bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
#ifdef AN_DEVELOPMENT
			virtual void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const {}
#endif

		private:
			Element* element = nullptr;
		};

		class ElementShape
		: public Element
		{
			FAST_CAST_DECLARE(ElementShape);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementShape();

			ElementShapeData const * get_data() const { return data; }

		private:
			override_ bool is_stateless() const { return data ? data->is_stateless() : true; }

			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
#ifdef AN_DEVELOPMENT
			override_ void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const;
#endif

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			Name shape;
			ELEMENT_SHAPE_GENERATE_FN(generate_shape) = nullptr;
			ElementShapeData* data = nullptr;
		};

		struct RegisteredElementShape
		{
			Name name;
			ELEMENT_SHAPE_GENERATE_FN(generate_shape) = nullptr;
			ELEMENT_SHAPE_CREATE_DATA_FN(create_data) = nullptr;

			RegisteredElementShape(Name const & _name, ELEMENT_SHAPE_GENERATE_FN(_generate_shape), ELEMENT_SHAPE_CREATE_DATA_FN(_create_data) = nullptr) : name(_name), generate_shape(_generate_shape), create_data(_create_data) {}
			RegisteredElementShape& temp_ref() { return *this; }
		};

		class RegisteredElementShapes
		{
		public:
			static void initialise_static();
			static void close_static();

			static RegisteredElementShape const * find(Name const & _shape);

			static void add(RegisteredElementShape & _shape);

		private:
			static RegisteredElementShapes* s_shapes;

			Array<RegisteredElementShape> shapes;
		};
	};
};
