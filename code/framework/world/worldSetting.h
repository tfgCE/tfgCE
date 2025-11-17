#pragma once

#include "..\library\usedLibraryStored.h"

#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\types\name.h"
#include "..\..\core\tags\tagCondition.h"

namespace Framework
{
	class LibraryStored;
	class RegionType;
	class DoorType;
	class RoomType;
	class WorldSetting;

	struct CopyWorldSettings;
	struct WorldSettingConditionContext;

	typedef RefCountObjectPtr<WorldSetting> WorldSettingPtr;

	/*
	 *	World setting class provides way to include specific kind of objects directly or through tag conditions.
	 *	It can be part of a room type or a region type.
	 *	Chain of overruling is : room -> room origin -> region -> region
	 *						or : room type -> region type -> region type
	 *	These are advices, as there should be a chain provided through WorldSettingConditionContext
	 *	Child overrides parent ALTHOUGH it can get parent's results or obey parent's rules
	 *
	 *	Main setting (without type) is one that is always checked (by default) and should be least restrict (with at least fallback using universal approach).
	 *
	 *	In general, we try to override_ specific stuff with generation parameters (changing shape, size, cross-section of corridor) but when we want to include
	 *	very different things, we should resolve in usage of WorldSetting.
	 */
	class WorldSetting
	: public RefCountObject
	{
	public:
		WorldSetting();
		virtual ~WorldSetting();

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	private:
		struct Type
		{
			Name type;
			String optionId; // required if forGameTags is set
			TagCondition forGameTags;

			enum Other
			{
				Ignored,
				Allowed,
				Required,
			};
			Other checkParent = Other::Ignored;
			Other checkMain = Other::Required; // if main type is required, allowed or ignored

			static Other parse_other(String const & _v)
			{
				if (_v == TXT("allowed")) return Allowed;
				if (_v == TXT("required")) return Required;
				if (_v == TXT("ignored") || _v.is_empty()) return Ignored;
				error(TXT("world setting check \"%S\" not recognised"), _v.to_char());
				return Ignored;
			}

			Array<RefCountObjectPtr<TagCondition>> tagged;
			Array<RefCountObjectPtr<TagCondition>> taggedFallback; // if fails, we can try using fallback
			Array<UsedLibraryStored<DoorType>> doorTypes;
			Array<UsedLibraryStored<RoomType>> roomTypes;
			Array<UsedLibraryStored<RegionType>> regionTypes;

			bool is_ok(LibraryStored const * _stored, bool _fallback) const; // tags and types only!

			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		};

		Type const * get_type(Name const & _type) const;

		List<Type> types;

		friend struct WorldSettingConditionContext;
		friend struct CopyWorldSettings;
	};

	struct CopyWorldSettings
	{
		struct Source
		{
			OPTIONAL_ Name fromType;
			LibraryName from;
		};
		Array<Source> from;
		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

		bool apply_to(WorldSettingPtr & _ws) const;
	};

};
