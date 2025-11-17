#include "missionSelectionTier.h"

#include "..\game\persistence.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(MissionSelectionTier);
LIBRARY_STORED_DEFINE_TYPE(MissionSelectionTier, missionSelectionTier);

MissionSelectionTier::MissionSelectionTier(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
{
}

MissionSelectionTier::~MissionSelectionTier()
{
}

bool MissionSelectionTier::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// reset
	//
	tier = 0;
	forTierDiffs.clear();


	// load
	//
	tier = _node->get_int_attribute_or_from_child(TXT("tier"), tier);
	for_every(n, _node->children_named(TXT("for")))
	{
		forTierDiffs.push_back();
		auto& f = forTierDiffs.get_last();
		f.diff.load_from_xml(n, TXT("tierDiff"));
		f.limit = n->get_int_attribute_or_from_child(TXT("limit"), f.limit);
	}

	return result;
}

bool MissionSelectionTier::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

int MissionSelectionTier::get_limit_for_tier_diff(int _tierDiff) const
{
	for_every(ftd, forTierDiffs)
	{
		if (ftd->diff.is_empty() || ftd->diff.does_contain(_tierDiff))
		{
			return ftd->limit;
		}
	}
	return 0;
}
