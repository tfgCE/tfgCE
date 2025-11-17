#pragma once

#include "..\loaderHubDraggableData.h"

#include "..\..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"

namespace Framework
{
	class TexturePart;
};

namespace TeaForGodEmperor
{
	class WeaponPart;
};

namespace Loader
{
	namespace HubDraggables
	{
		class ObjectItem
		: public Loader::IHubDraggableData
		{
			FAST_CAST_DECLARE(ObjectItem);
			FAST_CAST_BASE(Loader::IHubDraggableData);
			FAST_CAST_END();
		public:
			ObjectItem(Framework::IModulesOwner* _object);

			Framework::IModulesOwner* get_object() const { return object.get(); }

			// first add those on top 
			void add_icon(Framework::TexturePart* _icon);
			Array<Framework::UsedLibraryStored<Framework::TexturePart>> const& get_icons() const { return icons; }

		private:
			SafePtr<Framework::IModulesOwner> object;

			Array<Framework::UsedLibraryStored<Framework::TexturePart>> icons;
		};
	};
};

