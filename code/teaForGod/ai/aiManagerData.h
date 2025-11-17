#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\other\factoryOfNamed.h"
#include "..\..\core\types\name.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	class Library;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		class ManagerData
		: public RefCountObject
		{
			FAST_CAST_DECLARE(ManagerData);
			FAST_CAST_END();
		public:
			ManagerData();
			virtual ~ManagerData();

			Name const & get_name() const { return name; }

			virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
			virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			virtual void log(LogInfoContext& _log) const;

		protected:
			Name name;
		};

		class ManagerDatas
		: private FactoryOfNamed<ManagerData, Name>
		{
			typedef FactoryOfNamed<ManagerData, Name> base;
		public:
			static void initialise_static();
			static void close_static();

			static void register_data(Name const & _name, Create _create) { base::add(_name, _create); }
			static ManagerData* create_data(Name const & _name) { return base::create(_name); }
		};
	};
};
