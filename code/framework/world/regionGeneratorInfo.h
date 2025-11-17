#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\other\simpleVariableStorage.h"

#include "..\library\usedLibraryStored.h"

#include <functional>

struct String;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	class Region;
	struct LibraryStoredRenamer;

	class RegionGeneratorInfo
	: public RefCountObject
	{
		FAST_CAST_DECLARE(RegionGeneratorInfo);
		FAST_CAST_END();

	public:
		typedef std::function< RegionGeneratorInfo* ()> Construct;

		static void register_region_generator_info(String const & _name, RegionGeneratorInfo::Construct _func);
		static void initialise_static();
		static void close_static();
		static RegionGeneratorInfo* create(String const & _name);

	public:
		RegionGeneratorInfo() {}
		virtual ~RegionGeneratorInfo() {}

		virtual bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc) = 0;
		virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext) = 0;

		virtual bool post_generate(Region* _region) const = 0;

	private:
	};
};

