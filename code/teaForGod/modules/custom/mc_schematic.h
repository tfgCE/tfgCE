#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\core\types\colour.h"

#include "..\..\schematics\schematic.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class Schematic
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(Schematic);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			Schematic(Framework::IModulesOwner* _owner);
			virtual ~Schematic();

			TeaForGodEmperor::Schematic const * get_schematic() const { return schematic.get(); }

		public:	// Module
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

		protected:
			RefCountObjectPtr<TeaForGodEmperor::Schematic> schematic;
		};

		class SchematicData
		: public Framework::ModuleData
		{
			FAST_CAST_DECLARE(SchematicData);
			FAST_CAST_BASE(Framework::ModuleData);
			FAST_CAST_END();

			typedef Framework::ModuleData base;
		public:
			SchematicData(Framework::LibraryStored* _inLibraryStored);
			virtual ~SchematicData();

			TeaForGodEmperor::Schematic const* get_schematic() const { return schematic.get(); }

		public: // Framework::ModuleData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			override_ void prepare_to_unload();

		private:
			RefCountObjectPtr<TeaForGodEmperor::Schematic> schematic;
		};
	};
};

