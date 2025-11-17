#pragma once

#include "..\game\energy.h"

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\text\localisedString.h"

namespace TeaForGodEmperor
{
	class LineModel;

	class ExtraUpgradeOption
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(ExtraUpgradeOption);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		ExtraUpgradeOption(Framework::Library * _library, Framework::LibraryName const & _name);

		static Array<ExtraUpgradeOption*> get_all(TagCondition const& _tagCondition);

		String const& get_ui_desc() const { return uiDesc.get(); }
		Name const& get_ui_desc_id() const { return uiDesc.get_id(); }
		
		bool should_apply_to_both_hands() const { return applyToBothHands; }

		Energy const & get_health_amount() const { return healthAmount; }
		Energy const & get_ammo_amount() const { return ammoAmount; }
		EnergyCoef const & get_health_missing() const { return healthMissing; }
		EnergyCoef const & get_ammo_missing() const { return ammoMissing; }

	public:
		LineModel* get_line_model() const { return lineModel.get(); }
		LineModel* get_line_model_for_display() const { return lineModelForDisplay.get()? lineModelForDisplay.get() : lineModel.get(); }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~ExtraUpgradeOption();

	private:
		bool applyToBothHands = false; // makes sense for ammo/energy
		
		Energy healthAmount = Energy::zero();
		Energy ammoAmount = Energy::zero();
		EnergyCoef healthMissing = EnergyCoef::zero();
		EnergyCoef ammoMissing = EnergyCoef::zero();

		Framework::UsedLibraryStored<LineModel> lineModel;
		Framework::UsedLibraryStored<LineModel> lineModelForDisplay;
		Framework::LocalisedString uiDesc;
	};
};
