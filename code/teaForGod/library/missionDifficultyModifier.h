#pragma once

#include "..\game\energy.h"
#include "..\game\gameDirectorDefinition.h"

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\library\usedLibraryStored.h"
#include "..\..\framework\text\localisedString.h"

//

namespace TeaForGodEmperor
{
	class MissionDifficultyModifier
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(MissionDifficultyModifier);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		struct ForPool
		{
			Name pool; // empty - default (if can't find anything else)
			Optional<float> concurrentLimitCoef;
			Optional<float> amountCoef;
			Optional<Energy> amountAdd;
		};

		// accumulate and then are applied
		struct BonusCoef
		{
			float experienceCoef = 0.0f;
			float meritPointsCoef = 0.0f;

			bool is_empty() const { return experienceCoef == 0.0f && meritPointsCoef == 0.0f; }

			void reset() { experienceCoef = 0.0f; meritPointsCoef = 0.0f; }

			BonusCoef & operator += (BonusCoef const& _o)
			{
				experienceCoef += _o.experienceCoef;
				meritPointsCoef += _o.meritPointsCoef;
				return *this;
			}

			BonusCoef operator + (BonusCoef const& _o) const
			{
				BonusCoef r;
				r.experienceCoef = this->experienceCoef + _o.experienceCoef;
				r.meritPointsCoef = this->meritPointsCoef + _o.meritPointsCoef;
				return *this;
			}

			bool load_from_xml(IO::XML::Node const* _node);
			bool save_to_xml(IO::XML::Node * _node) const;

			String as_string() const;
			String as_applied_bonus_string() const;
		};

	public:
		MissionDifficultyModifier(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~MissionDifficultyModifier();

	public:
		String get_type_desc() const;
		String get_description() const;
		String get_bonus_info(bool _successPossible) const;

	public:
		BonusCoef const& get_on_end_bonus() const { return onEndBonus; }
		BonusCoef const& get_on_success_bonus() const { return onSuccessBonus; }
		BonusCoef const& get_on_come_back_bonus() const { return onComeBackBonus; }

		ForPool const* get_for_pool(Name const& _pool) const;

		TagCondition const& get_cell_region_tag_condition() const { return cellRegionTagCondition; }

		GameDirectorDefinition::AutoHostiles const& get_auto_hostiles() const { return autoHostiles; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		Framework::LocalisedString typeDesc; // used with mission name?
		Framework::LocalisedString description;

		BonusCoef onEndBonus;
		BonusCoef onSuccessBonus;
		BonusCoef onComeBackBonus;

		Array<ForPool> forPool;

		TagCondition cellRegionTagCondition; // if gives empty result, is ignored, used at all levels (top, sub regions etc)

		GameDirectorDefinition::AutoHostiles autoHostiles; // overrides
	};
};
