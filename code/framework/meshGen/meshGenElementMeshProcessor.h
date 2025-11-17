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

namespace Framework
{
	namespace MeshGeneration
	{
		/**
		 *	MeshProcessor is similar to mesh modifier but focuses solely on meshes
		 *	Allows for more complex operations and also to act only on specific parts
		 */

		class ElementMeshProcessorOperator
		: public RefCountObject
		{
			FAST_CAST_DECLARE(ElementMeshProcessorOperator);
			FAST_CAST_END();
		public:
			virtual ~ElementMeshProcessorOperator() {}

		public:
			virtual bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc) { return true; }
			virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext) { return true; }
			virtual bool process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int> & _newMeshPartIndices) const { return true; }
		};

		struct ElementMeshProcessorOperatorSet
		{
		public:
			void clear() { operators.clear(); }
			bool load_from_xml_child_node(IO::XML::Node const* _node, LibraryLoadingContext& _lc, tchar const* _childNodeName);
			bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
			bool process(GenerationContext& _context, ElementInstance& _instance, Array<int> const& _actOnMeshPartIndices, OUT_ Array<int>& _newMeshPartIndices) const;

		private:
			Array<RefCountObjectPtr<ElementMeshProcessorOperator>> operators;
		};

		class ElementMeshProcessor
		: public Element
		{
			FAST_CAST_DECLARE(ElementMeshProcessor);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementMeshProcessor();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			ElementMeshProcessorOperatorSet operators;

			int againstCheckpointsUp = 0;
		};

		class RegisteredElementMeshProcessorOperators
		: public FactoryOfNamed<ElementMeshProcessorOperator, String>
		{
		public:
			typedef FactoryOfNamed<ElementMeshProcessorOperator, String> base;
		public:
			static void initialise_static();
			static void close_static();
		};
	};
};
