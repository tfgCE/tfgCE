#include "gse_showText.h"

#include "..\..\overlayInfo\overlayInfoSystem.h"
#include "..\..\overlayInfo\elements\oie_text.h"
#include "..\..\tutorials\tutorialSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool ShowText::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	id = _node->get_name_attribute(TXT("id"), id);
	colour.load_from_xml_child_or_attr(_node, TXT("colour"));
	verticalAlign = _node->get_float_attribute(TXT("verticalAlign"), verticalAlign);

	text.load_from_xml(_node, _lc, id.to_char(), false); // id is something else here

	offset.load_from_xml_child_node(_node, TXT("offset"));
	offset.load_from_xml(_node, TXT("offsetPitch"), TXT("offsetYaw"), TXT("offsetRoll"));

	pulse = _node->get_bool_attribute_or_from_child_presence(TXT("pulse"), pulse);
	relToCamera = _node->get_bool_attribute_or_from_child_presence(TXT("relToCamera"), relToCamera);

	return result;
}

bool ShowText::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type ShowText::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* ois = OverlayInfo::System::get())
	{
		auto* t = new OverlayInfo::Elements::Text(text.get());
		
		t->with_id(id);

		t->with_colour(colour);
		t->with_vertical_align(verticalAlign);

		if (TutorialSystem::check_active())
		{
			TutorialSystem::configure_oie_element(t, offset);
		}
		else
		{
			t->with_location(OverlayInfo::Element::Relativity::RelativeToAnchor);
			t->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchor, offset);
			t->with_distance(10.0f);
		}
		if (relToCamera)
		{
			t->with_location(OverlayInfo::Element::Relativity::RelativeToCamera);
			t->with_rotation(OverlayInfo::Element::Relativity::RelativeToCamera, offset);
		}

		if (pulse)
		{
			t->with_pulse();
		}

		ois->add(t);
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
