#pragma once

namespace Framework
{
	namespace MeshGeneration
	{
		namespace CustomData
		{
			/**
			 *	Class to hide *Ref objects behind names (ids).
			 */
			template<typename ClassRef>
			struct RefLookup
			{
			public:
				bool load_from_xml(IO::XML::Node const * _node, tchar const * const _childrenName, tchar const * _useObjAttr, tchar const * _ownObjChild, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

				ClassRef const * get(Name const & _id) const;

			private:
				struct Element
				{
					Name id;
					ClassRef ref;
				};
				Array<Element> refs;
			};

			#include "meshGenRefLookup.inl"
		};
	};
};
