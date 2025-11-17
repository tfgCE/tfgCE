#include "extraUpgradeOption.h"

#include "library.h"

#include "..\game\game.h"

#ifdef AN_CLANG
#include "..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(ExtraUpgradeOption);
LIBRARY_STORED_DEFINE_TYPE(ExtraUpgradeOption, extraUpgradeOption);

ExtraUpgradeOption::ExtraUpgradeOption(Framework::Library * _library, Framework::LibraryName const & _name)
: base(_library, _name)
{
}

ExtraUpgradeOption::~ExtraUpgradeOption()
{
}

bool ExtraUpgradeOption::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	applyToBothHands = _node->get_bool_attribute_or_from_child_presence(TXT("applyToBothHands"), applyToBothHands);

	healthAmount = Energy::load_from_attribute_or_from_child(_node, TXT("healthAmount"), healthAmount);
	ammoAmount = Energy::load_from_attribute_or_from_child(_node, TXT("ammoAmount"), ammoAmount);
	healthAmount = Energy::load_from_attribute_or_from_child(_node, TXT("health"), healthAmount);
	ammoAmount = Energy::load_from_attribute_or_from_child(_node, TXT("ammo"), ammoAmount);
	healthMissing = EnergyCoef::load_from_attribute_or_from_child(_node, TXT("healthMissing"), healthMissing);
	ammoMissing = EnergyCoef::load_from_attribute_or_from_child(_node, TXT("ammoMissing"), ammoMissing);

	uiDesc.load_from_xml_child(_node, TXT("uiDesc"), _lc, TXT("uiDesc"));

	result &= lineModel.load_from_xml(_node, TXT("lineModel"), _lc);
	result &= lineModelForDisplay.load_from_xml(_node, TXT("lineModelForDisplay"), _lc);

	return result;
}

bool ExtraUpgradeOption::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);
	
	result &= lineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= lineModelForDisplay.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

Array<ExtraUpgradeOption*> ExtraUpgradeOption::get_all(TagCondition const& _tagCondition)
{
	Array<ExtraUpgradeOption*> euos;
	Library::get_current_as<Library>()->get_extra_upgrade_options().do_for_every(
		[&euos, &_tagCondition](Framework::LibraryStored* _libraryStored)
		{
			if (auto * euo = fast_cast<ExtraUpgradeOption>(_libraryStored))
			{
				if (_tagCondition.is_empty() || _tagCondition.check(_libraryStored->get_tags()))
				{
					euos.push_back(euo);
				}
			}
		});
	return euos;
}

