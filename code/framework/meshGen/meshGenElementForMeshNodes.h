#pragma once

#include "meshGenElement.h"
#include "meshGenValueDef.h"

#include "..\..\core\tags\tagCondition.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	namespace MeshGeneration
	{
		/*
		 *	Gathers all "forEach" to create sets and for each set process elements
		 *	It may either process all or just look for best
		 *
		 *		for each
		 *			mesh node plug (process all)
		 *				+
		 *			mesh node socket (process one/best)
		 *		
		 *	will scan all plugs and for each plug it will go through all sockets but element will be processed once
		 *	it's up to user to store best socket info (or even if it was actually found!, note to clear this info when processing each plug)
		 *
		 *	should have parameters defined
		 */
		class ElementForMeshNodes
		: public Element
		{
			FAST_CAST_DECLARE(ElementForMeshNodes);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementForMeshNodes();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
#ifdef AN_DEVELOPMENT
			override_ void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const;
#endif

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			struct ForEach;
			struct ForEachMeshNode;
			typedef RefCountObjectPtr<ForEach> ForEachPtr;
			typedef RefCountObjectPtr<ForEachMeshNode> ForEachMeshNodePtr;

			int againstCheckpointsUp = NONE; // to get extra checkpoints (if we know how much there are), if NONE, there is no limit

			struct ForEachMeshNode
			: public RefCountObject
			{
				ValueDef<Name> name;
				TagCondition tagged;
				WheresMyPoint::ToolSet toolSet; // store values and return true or false
				Array<ForEachMeshNodePtr> drop;
				bool processAll = false; // if all, will accept each, if not all, will scan through all and choose best
				bool randomOrder = false; // will check in random order

				bool load_from_xml(IO::XML::Node const * _node, bool _asToDrop = false);
			};

			struct ForEach
			: public RefCountObject
			{
				Array<ForEachMeshNodePtr> meshNodes;

				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			};
			Array<ForEachPtr> forEach;

			RefCountObjectPtr<Element> element;
		};
	};
};
