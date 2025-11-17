#pragma once

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\module\moduleCustom.h"

namespace Framework
{
	class TexturePart;
};

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class HubIconsData;

		class HubIcons
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(HubIcons);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			HubIcons(Framework::IModulesOwner* _owner);
			virtual ~HubIcons();

		public:
			Framework::TexturePart* get_icon() const;

		protected: // Module
			override_ void setup_with(Framework::ModuleData const * _moduleData);

		protected:
			HubIconsData const * hubIconsData = nullptr;
		};

		class HubIconsData
		: public Framework::ModuleData
		{
			FAST_CAST_DECLARE(HubIconsData);
			FAST_CAST_BASE(Framework::ModuleData);
			FAST_CAST_END();

			typedef Framework::ModuleData base;
		public:
			static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored);
				
			HubIconsData(Framework::LibraryStored* _inLibraryStored);
			virtual ~HubIconsData();

		protected: // Framework::ModuleData
			override_ bool read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

		private:
			::Framework::UsedLibraryStored<::Framework::TexturePart> icon;

			friend class HubIcons;
		};
	};
};

