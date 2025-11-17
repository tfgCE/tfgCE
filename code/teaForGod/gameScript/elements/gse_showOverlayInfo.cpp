#include "gse_showOverlayInfo.h"

#include "..\..\game\game.h"

#include "..\..\overlayInfo\overlayInfoSystem.h"
#include "..\..\overlayInfo\elements\oie_text.h"

#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// what to show
DEFINE_STATIC_NAME(credits);
DEFINE_STATIC_NAME_STR(theEnd, TXT("the end"));

// poi
DEFINE_STATIC_NAME_STR(vrAnchor, TXT("vr anchor"));

// localised strings
DEFINE_STATIC_NAME_STR(lsCredits, TXT("credits; info"));
DEFINE_STATIC_NAME_STR(lsCreditsMyParents, TXT("credits; my parents"));
DEFINE_STATIC_NAME_STR(lsCreditsSpecialThanks, TXT("credits; special thanks"));
DEFINE_STATIC_NAME_STR(lsTheEnd, TXT("game; the end"));

//

bool Elements::ShowOverlayInfo::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	show.load_from_xml(_node, TXT("show"));

	roomVar.load_from_xml(_node, TXT("roomVar"));
	atPOI.load_from_xml(_node, TXT("atPOI"));
	atVRAnchor.load_from_xml(_node, TXT("atVRAnchor"));

	scrollInitialWait.load_from_xml(_node, TXT("scrollInitialWait"));
	scrollPostWait.load_from_xml(_node, TXT("scrollPostWait"));
	scrollRange.load_from_xml(_node, TXT("scrollRange"));
	scrollSpeed.load_from_xml(_node, TXT("scrollSpeed"));
	scale.load_from_xml(_node, TXT("scale"));
	font.load_from_xml(_node, TXT("font"), _lc);
	forceBlend.load_from_xml(_node, TXT("forceBlend"));

	return result;
}

bool Elements::ShowOverlayInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= font.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

void Elements::ShowOverlayInfo::load_on_demand_if_required()
{
	base::load_on_demand_if_required();
	if (auto* f = font.get())
	{
		f->load_on_demand_if_required();
	}
}

Framework::GameScript::ScriptExecutionResult::Type Elements::ShowOverlayInfo::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* ois = TeaForGodEmperor::OverlayInfo::System::get())
	{
		String text;
		if (show.is_set())
		{
			if (show.get() == NAME(credits))
			{
				text = LOC_STR(NAME(lsCredits));
				String specialThanks;
				specialThanks += LOC_STR(NAME(lsCreditsMyParents));
				specialThanks += LOC_STR(NAME(lsCreditsSpecialThanks));
				text = text.replace(String(TXT("%specialThanks%")), specialThanks);
			}
			if (show.get() == NAME(theEnd))
			{
				text = LOC_STR(NAME(lsTheEnd));
			}
		}

		if (!text.is_empty())
		{
			OverlayInfo::Elements::Text* t = new OverlayInfo::Elements::Text();

			t->with_text(text);

			if (auto* f = font.get())
			{
				t->with_font(f);
			}
			if (scrollRange.is_set())
			{
				t->with_vertical_scroll_down(scrollRange.get(), scrollSpeed.get(1.0f), scrollInitialWait.get(0.0f), scrollPostWait.get(0.0f));
			}
			if (scale.is_set())
			{
				t->with_scale(scale.get());
			}
			if (forceBlend.is_set())
			{
				t->with_force_blend(forceBlend.get());
			}

			Optional<Transform> vrPlacement;

			if (roomVar.is_set())
			{
				Framework::Room* inRoom = nullptr;
				if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(roomVar.get()))
				{
					if (auto* r = exPtr->get())
					{
						inRoom = r;
					}
				}
				if (inRoom && atPOI.is_set())
				{
					Framework::PointOfInterestInstance* foundPOI;
					if (inRoom->find_any_point_of_interest(atPOI.get(), foundPOI))
					{
						Transform atPOIAt = foundPOI->calculate_placement();

						if (auto* game = Game::get_as<Game>())
						{
							if (auto* pa = game->access_player().get_actor())
							{
								Transform vrAnchor = pa->get_presence()->get_vr_anchor();
								vrPlacement = vrAnchor.to_local(atPOIAt);
							}
						}

						if (!vrPlacement.is_set())
						{
							Framework::PointOfInterestInstance* vrAnchorPOI = nullptr;
							if (inRoom->find_any_point_of_interest(atVRAnchor.get(NAME(vrAnchor)), vrAnchorPOI))
							{
								vrPlacement = vrAnchorPOI->calculate_placement().to_local(atPOIAt);
							}
						}
					}
				}
			}

			if (vrPlacement.is_set())
			{
				t->with_vr_placement(vrPlacement.get());
			}

			ois->add(t);
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
